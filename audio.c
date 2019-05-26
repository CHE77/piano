/*
 *  audio.c  Audio output module of Piano.
 *
 *  Copyright (C) 2008 Tigran Aivazian <tigran@bibles.org.uk>
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include "piano.h"

static snd_pcm_t *pcm;
static snd_output_t *output;
static pthread_t audio_thrid;
static void *audio_thread(void *arg);

static char *devname = "hw:0,0";		/* playback device name */
static unsigned int rate = 44100;		/* stream rate in Hz */
static unsigned int channels = 2;		/* number of channels */
static unsigned int buffer_time = 50000;	/* ring buffer length in microseconds */
static unsigned int period_time = 10000;	/* period time in microseconds */
static int resample = 0;			/* enable alsa-lib resampling */
static double freq;   /* sinusoidal wave frequency in Hz */
static snd_pcm_sframes_t buffer_size;
static snd_pcm_sframes_t period_size;
static snd_pcm_channel_area_t *areas;

const double max_phase = 2.0 * M_PI;
static double phase_step;
int max_signal_value = 0x7FFF; /* maximum value of a 16-bit signal */
int signal_value = 0; /* current signal value */

static void generate_sine(const snd_pcm_channel_area_t *areas, int count, double *_phase)
{
   double phase = *_phase;
   double res;
   unsigned char *samples[channels], *tmp;
   int steps[channels];
   unsigned int chn, byte;
   int ires;
	
   /* prepare the contents of areas */
   for (chn = 0; chn < channels; chn++) {
      samples[chn] = (((unsigned char *)areas[chn].addr) + (areas[chn].first / 8));
      steps[chn] = areas[chn].step / 8;
   }
   /* fill the channel areas */
   while (count-- > 0) {
      res = signal_value * sin(phase);
      ires = res;
      tmp = (unsigned char *)(&ires);
      for (chn = 0; chn < channels; chn++) {
         for (byte = 0; byte < 2; byte++)
            *(samples[chn] + byte) = tmp[byte];
         samples[chn] += steps[chn];
      }
      phase += phase_step;
      if (phase >= max_phase)
         phase -= max_phase;
   }
   *_phase = phase;
}

static int set_hwparams(snd_pcm_t *handle, snd_pcm_hw_params_t *params)
{
	unsigned int rrate;
	snd_pcm_uframes_t size;
	int err, dir;

	/* choose all parameters */
	err = snd_pcm_hw_params_any(handle, params);
	if (err < 0) {
		printf("Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
		return err;
	}
	/* set hardware resampling */
	err = snd_pcm_hw_params_set_rate_resample(handle, params, resample);
	if (err < 0) {
		printf("Resampling setup failed for playback: %s\n", snd_strerror(err));
		return err;
	}
	/* set the interleaved read/write format */
	err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) {
		printf("Access type not available for playback: %s\n", snd_strerror(err));
		return err;
	}
	/* set the sample format */
	err = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16);
	if (err < 0) {
		printf("Sample format not available for playback: %s\n", snd_strerror(err));
		return err;
	}
	/* set the count of channels */
	err = snd_pcm_hw_params_set_channels(handle, params, channels);
	if (err < 0) {
		printf("Channels count (%i) not available for playbacks: %s\n", channels, snd_strerror(err));
		return err;
	}
	/* set the stream rate */
	rrate = rate;
	err = snd_pcm_hw_params_set_rate_near(handle, params, &rrate, 0);
	if (err < 0) {
		printf("Rate %iHz not available for playback: %s\n", rate, snd_strerror(err));
		return err;
	}
	if (rrate != rate) {
		printf("Rate doesn't match (requested %iHz, get %iHz)\n", rate, err);
		return -EINVAL;
	}
	/* set the buffer time */
	err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &buffer_time, &dir);
	if (err < 0) {
		printf("Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(err));
		return err;
	}
	err = snd_pcm_hw_params_get_buffer_size(params, &size);
	if (err < 0) {
		printf("Unable to get buffer size for playback: %s\n", snd_strerror(err));
		return err;
	}
        buffer_size = size;
	/* set the period time */
	err = snd_pcm_hw_params_set_period_time_near(handle, params, &period_time, &dir);
	if (err < 0) {
		printf("Unable to set period time %i for playback: %s\n", period_time, snd_strerror(err));
		return err;
	}
	err = snd_pcm_hw_params_get_period_size(params, &size, &dir);
	if (err < 0) {
		printf("Unable to get period size for playback: %s\n", snd_strerror(err));
		return err;
	}
        period_size = size;
	/* write the parameters to device */
	err = snd_pcm_hw_params(handle, params);
	if (err < 0) {
		printf("Unable to set hw params for playback: %s\n", snd_strerror(err));
		return err;
	}
	return 0;
}

static int set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams)
{
   int err;

   /* get the current swparams */
   err = snd_pcm_sw_params_current(handle, swparams);
   if (err < 0) {
      fprintf(stderr, "Unable to determine current swparams for playback: %s\n", snd_strerror(err));
      return err;
   }
   /* start the transfer when the buffer is almost full: */
   /* (buffer_size / avail_min) * avail_min */
   err = snd_pcm_sw_params_set_start_threshold(handle, swparams, (buffer_size / period_size) * period_size);
   if (err < 0) {
      fprintf(stderr, "Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
      return err;
   }
   /* allow the transfer when at least period_size samples can be processed */
   err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_size);
   if (err < 0) {
      fprintf(stderr, "Unable to set avail min for playback: %s\n", snd_strerror(err));
      return err;
   }
   /* write the parameters to the playback device */
   err = snd_pcm_sw_params(handle, swparams);
   if (err < 0) {
      fprintf(stderr, "Unable to set sw params for playback: %s\n", snd_strerror(err));
      return err;
   }
   return 0;
}

/* recover from underrun and suspend */
static int xrun_recovery(int err)
{
   if (err == -EPIPE) { /* under-run */
      err = snd_pcm_prepare(pcm);
      if (err < 0)
         fprintf(stderr, "Can't recover from underrun: %s\n", snd_strerror(err));
         return 0;
      } else if (err == -ESTRPIPE) {
         while ((err = snd_pcm_resume(pcm)) == -EAGAIN)
            sleep(1); /* wait until the suspend flag is released */
         if (err < 0) {
            err = snd_pcm_prepare(pcm);
            if (err < 0)
               fprintf(stderr, "Can't recover from suspend: %s\n", snd_strerror(err));
         }
         return 0;
      }
   return err;
}

static void stream_audio(snd_pcm_channel_area_t *areas)
{
   signed short *ptr;
   int err, cptr;
   double current_phase = 0;

   if (!signal_value) return;

   while (1) {
      generate_sine(areas, period_size, &current_phase);
      ptr = areas->addr;
      cptr = period_size;
      while (cptr > 0) {
         err = snd_pcm_writei(pcm, ptr, cptr);
         if (err == -EAGAIN) continue;
         if (err < 0) {
            if (xrun_recovery(err) < 0) {
               fprintf(stderr, "%s: Write error: %s\n", __func__, snd_strerror(err));
               exit(1);
            }
            break; /* skip one period */
         }
         ptr += err * channels;
         cptr -= err;
      }
   }
}

void set_rate(unsigned int r)
{
   if (r < MINRATE || r > MAXRATE) {
      fprintf(stderr, "piano: invalid rate = %u, must be within [%u...%u]\n", r, MINRATE, MAXRATE);
      exit(1);
   } else
      rate = r;
}

void set_channels(unsigned int c)
{
   if (c < MINCHANNELS || c > MAXCHANNELS) {
      fprintf(stderr, "piano: invalid number of channels = %u, must be within [%u...%u]\n", c, MINCHANNELS, MAXCHANNELS);
      exit(1);
   } else
      channels = c;
}

void set_buffer_time(unsigned int b)
{
   if (b < MINBUFFERTIME || b > MAXBUFFERTIME) {
      fprintf(stderr, "piano: invalid buffer time = %u, must be within [%u...%u]\n", b, MINBUFFERTIME, MAXBUFFERTIME);
      exit(1);
   } else
      buffer_time = b;
}

void set_period_time(unsigned int p)
{
   if (p < MINPERIODTIME || p > MAXPERIODTIME) {
      fprintf(stderr, "piano: invalid period time = %u, must be within [%u...%u]\n", p, MINPERIODTIME, MAXPERIODTIME);
      exit(1);
   } else
      period_time = p;
}

void set_pcm_devname(char *name)
{
   devname = strdup(name);
}

void set_resample(void)
{
   resample = 1;
}

static void *audio_thread(void *arg ATTRIBUTE_UNUSED)
{
   while (!stop_pending())
      stream_audio(areas);
   return 0;
}

void set_freq(double f)
{
   freq = f;
   phase_step = max_phase*freq/(double)rate;
}

void audio_init(void)
{
   int err;
   unsigned int chn;
   signed short *samples;
   snd_pcm_hw_params_t *hwparams;
   snd_pcm_sw_params_t *swparams;

   snd_pcm_hw_params_alloca(&hwparams);
   snd_pcm_sw_params_alloca(&swparams);

   err = snd_output_stdio_attach(&output, stderr, 0);
   if (err < 0) {
      fprintf(stderr, "%s: attaching output failed: %s\n", __func__, snd_strerror(err));
      exit(1);
   }

   err = snd_pcm_open(&pcm, devname, SND_PCM_STREAM_PLAYBACK, 0);
   if (err < 0) {
      fprintf(stderr, "%s: Error opening PCM device %s: %s\n",
         __func__, devname, snd_strerror(err));
      exit(1);
   }

   if ((err = set_hwparams(pcm, hwparams)) < 0) {
      fprintf(stderr, "%s: Can't set hwparams: %s\n", __func__, snd_strerror(err));
      exit(1);
   }

   if ((err = set_swparams(pcm, swparams)) < 0) {
      fprintf(stderr, "%s: Can't set swparams: %s\n", __func__, snd_strerror(err));
      exit(1);
   }

   if (verbose) {
      fprintf(stderr, "PCM device: %s\n", devname);
      snd_pcm_dump(pcm, output);
   }

   samples = malloc(period_size * channels * 2);
   if (!samples) {
      fprintf(stderr, "%s: Can't malloc memory for samples\n", __func__);
      exit(1);
   }

   areas = calloc(channels, sizeof(snd_pcm_channel_area_t));
   if (!areas) {
      fprintf(stderr, "%s: Can't calloc memory for areas\n", __func__);
      exit(1);
   }

   set_freq(440.0); /* A4 */

   for (chn = 0; chn < channels; chn++) {
      areas[chn].addr = samples;
      areas[chn].first = chn * 16;
      areas[chn].step = channels * 16;
   }

   err = pthread_create(&audio_thrid, NULL, audio_thread, NULL);
   if (err < 0) {
      fprintf(stderr, "%s: Error creating thread: %s\n", __func__, strerror(errno));
      exit(1);
   }
}

void audio_cleanup(void)
{
   free(areas[0].addr);
   free(areas);
   snd_pcm_close(pcm);
}
