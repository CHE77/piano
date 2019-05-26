/*
 *  piano.h  main header of Piano.
 *
 *  Copyright (C) 2008 Tigran Aivazian <tigran@bibles.org.uk>
 */

#ifndef ATTRIBUTE_UNUSED
/* do not print warning (gcc) when function parameter is not used */
#define ATTRIBUTE_UNUSED __attribute__ ((__unused__))
#endif

#define POLYPHONY 128 /* maximum number of concurrently sounding notes */
#define NKEYS 88    /* number of keys on the piano's keyboard */

/* range for the audio stream rate */
#define MINRATE  4000
#define MAXRATE  96000

/* range for the number of audio channels (2=stereo) */
#define MINCHANNELS 1
#define MAXCHANNELS 24

/* range for the audio buffer time (in microseconds) */
#define MINBUFFERTIME  1000
#define MAXBUFFERTIME  1000000

/* range for the audio period time (in microseconds) */
#define MINPERIODTIME  1000
#define MAXPERIODTIME  1000000

/* init.c */
extern int verbose;
extern void init(int argc, char *argv[]);

/* signal.c */
extern int stop_pending(void);

/* scales.c */
#define MINMIDINOTE  21    /* A0 */
#define MAXMIDINOTE 108    /* C8 */
struct scale {
   char *name;  /* note name like "A0", "A0#" etc */
   double freq; /* fundamental frequency in Hz */
};
extern struct scale *scale;
extern void scales_init(void);

/* midi.c */
extern void set_midichan(unsigned char chan);

/* audio.c */
extern void set_rate(unsigned int rate);
extern void set_pcm_devname(char *name);
extern void set_channels(unsigned int c);
extern void set_buffer_time(unsigned int b);
extern void set_period_time(unsigned int p);
extern void set_resample(void);
extern void set_freq(double f);
extern int max_signal_value; /* maximum value of a 16-bit signal */
extern int signal_value; /* current signal value */
