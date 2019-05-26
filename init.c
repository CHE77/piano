/*
 *  init.c  initialisation and cleanup functions of Piano.
 *
 *  Copyright (C) 2008 Tigran Aivazian <tigran@bibles.org.uk>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <readline/readline.h>
#include "piano.h"

int verbose = 0;
static int noshell = 0;

static void usage(void)
{
   fprintf(stderr,
"Usage: piano [options]...\n"
"-h,--help       help\n"
"-d,--device     audio playback device\n"
"-r,--rate       stream rate in Hz (%i...%i)\n"
"-c,--channels   number of audio channels in stream (%i...%i)\n"
"-b,--buffer     ring buffer time in microseconds (%i...%i)\n"
"-p,--period     period time in microseconds (%i...%i)\n"
"-R,--resample   enable software resampling\n"
"-m,--midichan   restrict MIDI input to a channel (1...16)\n"
"-N,--noshell    disable piano shell\n"
"-v,--verbose    be verbose\n"
"\n", MINRATE, MAXRATE, MINCHANNELS, MAXCHANNELS,
MINBUFFERTIME, MAXBUFFERTIME, MINPERIODTIME, MAXPERIODTIME);

   exit(1);
}

static void cleanup(void)
{
   extern void audio_cleanup(void);
   extern void midi_cleanup(void);

   audio_cleanup();
   midi_cleanup();

   /* readline changes the terminal state so we need
      to restore it before exiting the program */
   rl_cleanup_after_signal();
}

static void parse_cmdline(int argc, char *argv[])
{
   struct option long_option[] =
   {
      {"help", 0, NULL, 'h'},
      {"device", 1, NULL, 'd'},
      {"rate", 1, NULL, 'r'},
      {"channels", 1, NULL, 'c'},
      {"buffer", 1, NULL, 'b'},
      {"period", 1, NULL, 'p'},
      {"format", 1, NULL, 'f'},
      {"verbose", 1, NULL, 'v'},
      {"resample", 1, NULL, 'R'},
      {"noshell", 1, NULL, 'N'},
      {NULL, 0, NULL, 0},
   };

   while (1) {
      int c;
      if ((c = getopt_long(argc, argv, "hd:r:c:b:p:vRm:N", long_option, NULL)) < 0) break;
      switch (c) {
         case 'h':
            usage();
         case 'd':
            set_pcm_devname(optarg);
            break;
         case 'r':
            set_rate(atoi(optarg));
            break;
         case 'c':
            set_channels(atoi(optarg));
            break;
         case 'b':
            set_buffer_time(atoi(optarg));
            break;
         case 'p':
            set_period_time(atoi(optarg));
            break;
         case 'v':
            verbose = 1;
            break;
         case 'R':
            set_resample();
            break;
         case 'm':
            set_midichan(atoi(optarg));
            break;
         case 'N':
            noshell = 1;
            break;
         default:
            usage();
      }
   }
}

void init(int argc, char *argv[])
{
   extern void signals_init(void);
   extern void audio_init(void);
   extern void midi_init(void);
   extern void shell_init(void);

   parse_cmdline(argc, argv); 
   signals_init();
   scales_init();
   audio_init();
   midi_init();
   if (!noshell)
      shell_init();
   atexit(cleanup);
}
