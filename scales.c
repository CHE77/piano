/*
 *  scales.c  definitions of temperaments for Piano.
 *
 *  Copyright (C) 2008 Tigran Aivazian <tigran@bibles.org.uk>
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "piano.h"

/* the entries 0..MINMIDINOTE are wasted */
static struct scale et[MAXMIDINOTE+1];
struct scale *scale;

char *note_names[] = {
    "N0", "N1",   "N2",  "N3",  "N4",  "N5",  "N6",  "N7",  "N8",  "N9",  "N10", "N11",
    "N12","N13",  "N14", "N15", "N16", "N17", "N18", "N19", "N20", "A0",  "A0#", "B0",
    "C1", "C1#",  "D1",  "D1#", "E1",  "F1",  "F1#", "G1",  "G1#", "A1",  "A1#", "B1",
    "C2", "C2#",  "D2",  "D2#", "E2",  "F2",  "F2#", "G2",  "G2#", "A2",  "A2#", "B2",
    "C3", "C3#",  "D3",  "D3#", "E3",  "F3",  "F3#", "G3",  "G3#", "A3",  "A3#", "B3",
    "C4", "C4#",  "D4",  "D4#", "E4",  "F4",  "F4#", "G4",  "G4#", "A4",  "A4#", "B4",
    "C5", "C5#",  "D5",  "D5#", "E5",  "F5",  "F5#", "G5",  "G5#", "A5",  "A5#", "B5",
    "C6", "C6#",  "D6",  "D6#", "E6",  "F6",  "F6#", "G6",  "G6#", "A6",  "A6#", "B6",
    "C7", "C7#",  "D7",  "D7#", "E7",  "F7",  "F7#", "G7",  "G7#", "A7",  "A7#", "B7",
    "C8", "N109", "N110","N111","N112","N113","N114","N115","N116","N117","N118","N119",
    "N120","N121","N122","N123","N124","N125","N126","N127","N128",
};


/* Equal Temperament */
static void et_init(void)
{
   int i;
   double etfactor = pow(2.0, 1.0/12.0);

   memset(et+MINMIDINOTE, 0, (MAXMIDINOTE-MINMIDINOTE+1)*sizeof(struct scale));

   for (i=MINMIDINOTE; i<MAXMIDINOTE+1; i++)
      et[i].name = note_names[i];

   et[MINMIDINOTE].freq = 27.5;
   for (i=MINMIDINOTE+1; i<MAXMIDINOTE+1; i++) {
      et[i].freq = et[i-1].freq * etfactor;
      /* if (verbose) fprintf(stderr, "%-4s %.3f\n", et[i].name, et[i].freq); */
   }
}

void scales_init(void)
{
   et_init();
   scale = et; /* make equal temperament active */
}
