#
# Makefile for the Piano
#
# Copyright (C) 2008 Tigran Aivazian <tigran@bibles.org.uk>.
#

CC = gcc
#CFLAGS = -g -Wall -O3 -D_GNU_SOURCE -D_REENTRANT -pthread
LDFLAGS = -lm -lasound -pthread -ltermcap -lreadline

# uncommenting the next line will disable assert()
#CFLAGS += -DNDEBUG

SRC = main.c init.c midi.c signal.c audio.c shell.c scales.c

OBJ = $(SRC:.c=.o)

piano:	$(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY:		clean
clean:
	@rm -f piano *.o core.*
