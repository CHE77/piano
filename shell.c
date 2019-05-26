/*
 *  shell.c  user interaction shell module of Piano.
 *
 *  Copyright (C) 2008 Tigran Aivazian <tigran@bibles.org.uk>
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pthread.h>
#include "piano.h"

static pthread_t shell_thrid;

static void *shell_thread(void *arg);

void shell_init(void)
{
   int err;

   rl_vi_editing_mode(0, 0);
   err = pthread_create(&shell_thrid, NULL, shell_thread, NULL);
   if (err < 0) {
      fprintf(stderr, "%s: Erro creating shell thread: %s\n",
         __func__, strerror(errno));
      exit(1);
   }
}

static void *shell_thread(void *arg ATTRIBUTE_UNUSED)
{
   printf("Welcome to piano v0.1, type \"help\" if you wish.\n");
   while (1) {
      char *line = readline("> ");
      if (line && *line) {
         add_history(line);
         if (!strcmp("q", line) || !strcmp("quit", line) ||
             !strcmp("exit", line) || !strcmp("bye", line) ||
             !strcmp("good-bye", line)) {
            exit(1);
         } else if (!strcmp("help", line) || !strcmp("?", line)) {
            printf("Available commands:\n"
                   "q - quit the piano program.\n"
                   "help - list available commands.\n");
         } else
            printf("Invalid command \"%s\".\n", line);
         free(line);
      } else if (!line)
          printf("To quit press \"q\", not Ctrl-D.\n");
   }
   return 0;
}
