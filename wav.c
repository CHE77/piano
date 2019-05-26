/*
 *  wav.c  WAV file module of Piano.
 *
 *  Copyright (C) 2008 Tigran Aivazian <tigran@bibles.org.uk>
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#pragma pack (1)
struct wav_file_head_t
{
   unsigned char id[4]; /* could be {'R', 'I', 'F', 'F'} or {'F', 'O', 'R', 'M'} */
   unsigned int	length; /* Length of subsequent file (including remainder of header).
               This is in Intel reverse byte order if RIFF, Motorola format if FORM. */
   unsigned char type[4]; /* {'W', 'A', 'V', 'E'} or {'A', 'I', 'F', 'F'} */
} wav_file_head;

struct wav_chunk_head_t
{
   unsigned char id[4];	/* 4 ascii chars that is the chunk ID */
   unsigned int length;	/* Length of subsequent data within this chunk.
         This is in Intel reverse byte order if RIFF, Motorola format if FORM.
    NOTE: this doesn't include any extra byte needed to pad the chunk out to an even size. */
} wav_chunk_head;

struct wav_format_t {
   short tag;
   unsigned short channels;
   unsigned int sample_rate;
   unsigned int avg_bytes_per_sec;
   unsigned short block_align;
   unsigned short sample_bits;
   /* Note: there may be additional fields here, depending upon 'tag' */
} wav_format;
#pragma pack()

static struct sample_t {
   unsigned char *wave_data;
   unsigned int wave_size;
} sample[NKEYS]; /* one buffer per key */

/* fills in sample[key] structure */
static void load_sample(int key)
{
   int fd;
   unsigned char *ptr = NULL;
   char *filename = "sin440.wav";

   fd = open(filename, O_RDONLY); /* will depend on 'key' eventually */
   if (fd == -1) {
      fprintf(stderr, "%s: open(), %s\n", __func__, strerror(errno));
      exit(1);
   }

   if (read(fd, &wav_file_head, sizeof(wav_file_head)) != sizeof(wav_file_head)) {
      fprintf(stderr, "%s: read(wav_file_head): %s\n", __func__, strerror(errno));
      exit(1);
   }

   if (memcmp("RIFF", wav_file_head.id, 4) && memcmp("WAVE", wav_file_head.id, 4)) {
      fprintf(stderr, "%s: \"%s\" is not a WAV file\n", __func__, filename);
      exit(1);
   }

   while (read(fd, &wav_chunk_head, sizeof(wav_chunk_head)) == sizeof(wav_chunk_head)) {
      if (!memcmp("fmt ", wav_chunk_head.id, 4)) {
         if (read(fd, &wav_format, sizeof(wav_format)) != sizeof(wav_format)) {
            fprintf(stderr, "%s: read(wav_format): %s\n", __func__, strerror(errno));
            exit(1);
         } 

         if (wav_format.tag != 1) {
            fprintf(stderr, "%s: can't handle compressed WAV files\n", __func__);
            exit(1);
         }
/*
         *sample_bits = wav_format.sample_bits;
         *sample_rate = (unsigned short)wav_format.sample_rate;
         *channels = wav_format.channels;
*/
      } else if (!memcmp("data", wav_chunk_head.id, 4)) {
         ptr = (unsigned char *)malloc(wav_chunk_head.length);
         if (!ptr) {
            fprintf(stderr, "%s: malloc(%d) failed\n", __func__, wav_chunk_head.length);
            exit(1);
         }

         if (read(fd, ptr, wav_chunk_head.length) != wav_chunk_head.length) {
            fprintf(stderr, "%s: read() %d bytes of data): %s\n", __func__, wav_chunk_head.length, strerror(errno));
            exit(1);
         } 

         sample[key].wave_size = (wav_chunk_head.length * 8) / ((unsigned int)wav_format.sample_bits * (unsigned int)wav_format.channels);
         sample[key].wave_data = ptr;
         break;
      } else {
         if (wav_chunk_head.length & 1) ++wav_chunk_head.length;  // If odd, round it up to account for pad byte
         lseek(fd, wav_chunk_head.length, SEEK_CUR);
      }
   }

   return;
}

static void play_samples(void)
{
   snd_pcm_sframes_t count = 0, frames;
   unsigned char *wave_data = sample[0].wave_data;
   snd_pcm_uframes_t wave_size = sample[0].wave_size;
   
   do {
      snd_pcm_prepare(pcm);
      frames = snd_pcm_writei(pcm, wave_data + count, wave_size - count);
      fprintf(stderr, "%s: written %ld frames\n", __func__, frames);

      if (frames < 0) {
         frames = snd_pcm_recover(pcm, frames, 0);
         fprintf(stderr, "%s: Recovered from error, frames=%ld\n",
            __func__, frames);
      }
      if (frames < 0) {
         fprintf(stderr, "%s: Error (frames=%ld): %s\n",
            __func__, frames, snd_strerror(frames));
         break;
      }

      count += frames;
   } while (count < wave_size);
}
