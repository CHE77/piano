/*
 *  signal.c  signal handlers of Piano.
 *
 *  Copyright (C) 2008 Tigran Aivazian <tigran@bibles.org.uk>
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

volatile sig_atomic_t stop = 0;

int stop_pending(void)
{
   return stop;
}

static void handler(int sig)
{
   stop = 1;
   exit(1);
}

void signals_init(void)
{
   signal(SIGINT, handler);
   signal(SIGTERM, handler);
}
