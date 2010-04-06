/* -*-Mode: c++;-*-
   Copyright (c) 1994-2009 John Plevyak, All Rights Reserved
*/
#ifndef _arg_H
#define _arg_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __alpha
#define atoll atol
#endif

#define DBL_ARG_UNSET (DBL_MAX)

/* Argument Handling
*/
struct ArgumentState;

typedef void ArgumentFunction(struct ArgumentState *arg_state, char *arg);

typedef struct {
  cchar *name;
  char key;
  cchar *description;
  cchar *type;
  const void *location;
  cchar *env;
  ArgumentFunction *pfn;
} ArgumentDescription;

typedef struct ArgumentState {
  cchar **file_argument;
  int nfile_arguments;
  cchar *program_name;
  ArgumentDescription *desc;
#if defined __cplusplus
  ArgumentState(cchar *name, ArgumentDescription *adesc) 
    : file_argument(0), nfile_arguments(0), program_name(name), 
      desc(adesc) {}
#endif
} ArgumentState;

void usage(ArgumentState *arg_state, char *exit_if_null);
void process_args(ArgumentState *arg_state, int argc, char **argv);
void reprocess_config_args(ArgumentState *arg_state, int argc, char **argv);
void free_args(ArgumentState *arg_state);

#endif
