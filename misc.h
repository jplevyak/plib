/* -*-Mode: c++;-*-
   Copyright (c) 2003-2008 John Plevyak, All Rights Reserved
*/
#ifndef _misc_H
#define _misc_H

#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include "vec.h"

int buf_read(cchar *pathname, char **buf, int *len);
void fail(cchar *str, ...);
void error(cchar *fmt, ...);
char *trim(char *s, char *e = 0);
bool str2bool(char *s);
int str2strVec(char *s, Vec<char *> &v, char *delimitor = ",");
int xmkdir(cchar *p, mode_t mode);
int expand_filename(char *p, int len);

static inline char *xitoa(int i, char *c) { // c points to END of the string
  int neg = 0;
  if (i < 0) { neg = 1; i = -i; }
  do {
    *c-- = (char)(i%10+48);
  } while ((i/=10) > 0);
  if (neg) *c-- = '-';
  return c+1;
}

static inline char *xlltoa(int64 i, char *c) { // c points to END of the string
  int neg = 0;
  if (i < 0) { neg = 1; i = -i; }
  do {
    *c-- = (char)(i%10+48);
  } while ((i/=10) > 0);
  if (neg) *c-- = '-';
  return c+1;
}

static inline char *xutoa(uint32 i, char *e) {
  do {
    *--e = (char)(i%10+48);
  } while ((i/=10) > 0);
  return e;
}

static inline char *xutoa(uint64 i, char *e) {
  do {
    *--e = (char)(i%10+48);
  } while ((i/=10) > 0);
  return e;
}

static inline uint64 xatoull(char *s, char *e) {
  uint64 n = 0;
  if (isdigit(*s)) {
    n = *s - '0';
    s++;
    if (s >= e) return n;
  }
  while (isdigit(*s)) {
    n *= 10;
    n += *s - '0';
    s++;
  }
  return n;
}

static inline void *_xmemdup(void *p, int len) {
  if (len <= 0)
    return 0;
  void *pp = MALLOC(len);
  memcpy(pp, p, len);
  return pp;
}

static inline char*
dupstr(cchar *s, cchar *e = 0) {
  int l = e ? e-s : strlen(s);
  char *ss = (char*)MALLOC(l+1);
  memcpy(ss, s, l);
  ss[l] = 0;
  return ss;
}

static inline char *memdup(void *p, int len) {
  char *pp = (char*)MALLOC(len);
  memcpy(pp, p, len);
  return pp;
}

static inline char *skip_token(char *s) {
  while (isspace(*s)) s++;
  while (*s && !isspace(*s)) s++;
  while (isspace(*s)) s++;
  return s;
}

static inline char *skip_eol(char *p) { 
  while (*p && *p != '\n') p++;
  return p;
}

int appendfile2fp(FILE *fp, char *filename);
// filename = p1 + p2 + p3 + 4
bool is_directory(cchar *p1, cchar *p2 = 0, cchar *p3 = 0, cchar *p4 = 0);
bool is_regular_file(cchar *p1, cchar *p2 = 0, cchar *p3 = 0, cchar *p4 = 0);
bool file_exists(cchar *p1, cchar *p2 = 0, cchar *p3 = 0, cchar *p4 = 0);
char *dupstrs(cchar *p1, cchar *p2 = 0, cchar *p3 = 0, cchar *p4 = 0);

#define htonll(_x) (((uint64)htonl((uint32)_x)) | (((uint64)(htonl((uint32)((_x)>>32))))<<32))
#define ntohll(_x) (((uint64)ntohl((uint32)_x)) | (((uint64)(ntohl((uint32)((_x)>>32))))<<32))

char *quote_string(cchar *s);
char *escape_string(cchar *s);

#endif
