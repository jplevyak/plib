/* -*-Mode: c++; -*-
   Copyright (c) 2003-2008 John Plevyak, All Rights Reserved
*/
#ifndef _config_H
#define _config_H

#define GET_CONFIG              0
#define DYNAMIC_CONFIG          1
#define SET_CONFIG              2

EXTERN char config_filenames[1024] EXTERN_INIT("~/.plibrc,plib.init");

// -1 for not found
int int64_config(int dyn, int64 *pint, int64 def, cchar *n1, cchar *n2 = 0, cchar *n3 = 0);
static inline int int_config(int dyn, int *pint, int def, cchar *n1, cchar *n2 = 0, cchar *n3 = 0) {
  int64 v = 0;
  int64_config(dyn, &v, (int)def, n1, n2, n3);
  return (int)v;
}
static inline int get_int_config(cchar *n1, cchar *n2 = 0, cchar *n3 = 0) {
  int64 v = 0;
  int64_config(GET_CONFIG, &v, INT_MIN, n1, n2, n3);
  return (int)v;
}
int string_config(int dyn, cchar **pstring, cchar *def, cchar *n1, cchar *n2 = 0, cchar *n3 = 0);
static inline cchar *get_string_config(cchar *n1, cchar *n2 = 0, cchar *n3 = 0) {
  cchar *v = 0;
  string_config(GET_CONFIG, &v, 0, n1, n2, n3);
  return v;
}

void init_config();
void reinit_config();
typedef void (*config_callback_pfn)(void *);
void config_callback(config_callback_pfn pfn, void *);

#endif
