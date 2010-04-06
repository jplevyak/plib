/* -*-Mode: c++;-*-
   Copyright (c) 2003-2008 John Plevyak, All Rights Reserved
*/

#ifndef _defalloc_H_
#define  _defalloc_H_

class DefaultAlloc { public:
  static void *alloc(int s) { return MALLOC(s); }
  static void free(void *p) { FREE(p); }
};

#endif

