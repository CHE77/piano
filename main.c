/*
 *  main.c  main module of Piano.
 *
 *  Copyright (C) 2008 Tigran Aivazian <tigran@bibles.org.uk>
 */

#include <unistd.h>
#include "piano.h"

int main(int argc, char *argv[])
{
   init(argc, argv);
   while(!stop_pending()) { sleep(10); }
   return 0;
}
