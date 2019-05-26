/*
 *  midi.c  midi module of Piano.
 *
 *  Copyright (C) 2008 Tigran Aivazian <tigran@bibles.org.uk>
 */

#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include "piano.h"

static unsigned char midi_channel = 0;
static int port;
static snd_seq_t *seq; /* initialised by snd_seq_open() in midi_init() */
static char *seqdevname = "default";
static pthread_t midi_thrid;

void set_midichan(unsigned char chan)
{
   if (chan < 1 || chan > 16) {
      fprintf(stderr, "%s: Invalid MIDI channel %d, must be within (1...16)\n",
         __func__, chan);
      exit(1);
   }
   midi_channel = chan - 1;
}

static void *midi_thread(void *arg);

void midi_init(void)
{
   int err;

   err = snd_seq_open(&seq, seqdevname, SND_SEQ_OPEN_INPUT, 0);
   if (err) {
      fprintf(stderr, "%s: Error opening devic %s: %s\n",
         __func__, seqdevname, snd_strerror(err));
      exit(1);
   }

   err = snd_seq_set_client_name(seq, "piano");
   if (err) {
      fprintf(stderr, "%s: Error setting client name: %s\n",
         __func__, snd_strerror(err));
      exit(1);
   }

   err = snd_seq_create_simple_port(seq, "piano",
             SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
             SND_SEQ_PORT_TYPE_APPLICATION);
   if (err < 0) {
      fprintf(stderr, "%s: Error creating MIDI port: %s\n",
         __func__, snd_strerror(err));
      exit(1);
   } else
      port = err;

   err = pthread_create(&midi_thrid, NULL, midi_thread, NULL);
   if (err < 0) {
      fprintf(stderr, "%s: Error creating MIDI thread: %s\n",
         __func__, strerror(errno));
      exit(1);
   }

   system("aconnect 14:0 128:0"); /* XXX do this properly! */
   return;
}

void midi_cleanup(void)
{
   snd_seq_delete_port(seq, port);
   snd_seq_close(seq);
}

static void *midi_thread(void *arg ATTRIBUTE_UNUSED)
{
   while (1) {
      int err;
      snd_seq_event_t *event;
      double freq;

      err = snd_seq_event_input(seq, &event);
      if (err < 0) {
         fprintf(stderr, "%s: %s\n", __func__, snd_strerror(err));
         continue;
      }

      assert(err == 1); /* normally only one message at a time */
      assert(event);

      /* some keyboards (e.g. Casio CTK-900) don't send NoteOFF event but instead
         send a NoteON with velocity==0. This is allowed by the MIDI Spec.
         We convert these to NoteOFF. */
      if ((event->type == SND_SEQ_EVENT_NOTEON) && (event->data.note.velocity == 0))
         event->type = SND_SEQ_EVENT_NOTEOFF;

      switch (event->type) {
         case SND_SEQ_EVENT_NOTEON:
            if (midi_channel != event->data.note.channel)
               break;
            freq = scale[event->data.note.note].freq;
            set_freq(freq);
            signal_value = max_signal_value;
#if 0
            if (verbose) printf("%s: NoteON: chan=%d, note=%d, vel=%d (freq=%.3f)\n", __func__, 
               event->data.note.channel, event->data.note.note, event->data.note.velocity, freq);
#endif
            break;
         case SND_SEQ_EVENT_NOTEOFF:
            if (midi_channel != event->data.note.channel)
               break;
            signal_value = 0;
#if 0
            if (verbose) printf("%s: NoteOFF: chan=%d, note=%d, vel=%d\n", __func__,
               event->data.note.channel, event->data.note.note, event->data.note.velocity);
#endif
            break;
      }

      snd_seq_free_event(event);
   } /* while (1) */

   return 0;
}
