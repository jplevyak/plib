/* -*-Mode: c++;-*-
   Copyright (c) 2003-2008 John Plevyak, All Rights Reserved
*/
#ifndef _freelist_h
#define _freelist_h

#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#define _XOPEN_SOURCE 600
#else
#include <malloc.h>
#endif

// alignment must be a power of 2 greater than 8

class FreeList { public:
  int size, count, alignment;
  int active, allocated;
  void *head;
  void *block_head;

  void *alloc();
  void free(void *ptr);
  void xpand();
  void init(int asize, int acount = 64, int aalignment = 16);
  void x();

  FreeList(int asize = 0, int acount = 64, int aalignment = 16) { init(asize,acount,aalignment); }
  ~FreeList();
};

template<class C> class ClassFreeList : public FreeList { public:
  C protoObject;

  C *alloc();

  ClassFreeList(int acount = 64, int aalignment = 16)
    : FreeList(sizeof(C), acount, aalignment) {}
};

inline void
FreeList::init(int asize, int acount, int aalignment) {
  size = asize;
  count = acount;
  alignment = aalignment;
  active = allocated = 0;
  head = 0;
  block_head = 0;
  size = (size + alignment - 1) & ~(alignment-1);
}

inline void
FreeList::xpand() {
  void *last = head;
#if _XOPEN_SOURCE == 600
  if (posix_memalign(&head, alignment, (size*count) + sizeof(void*)))
    head = 0;
#else
  head = (void*)::memalign(alignment, (size*count) + sizeof(void*));
#endif
  *(void**)(((char*)head) + (size*count)) = block_head;
  block_head = head;
  void **p = (void**)head;
  for (int i = 0; i < count-1; i++) {
    void **n = (void**)(((char*)p) + size);
    *p = n;
    p = n;
  }
  *p = last;
  allocated += (size*count) + sizeof(void*);
}

inline void *
FreeList::alloc() {
#ifndef VALGRIND_TEST
  if (!head)
    xpand();
  void *v = head;
  head = *(void**)head;
  active++;
  return v;
#else
  return MALLOC(size);
#endif
}

inline void
FreeList::free(void *ptr) {
#ifndef VALGRIND_TEST
  void *v = head;
  head = ptr;
  (*(void**)head) = v;
  active--;
#else
  FREE(ptr);
#endif
}

inline
FreeList::~FreeList() {
  while (block_head) {
    void *bh = *(void**)(((char*)block_head) + (count*size));
    free(block_head);
    block_head = bh;
  }
}

template<class C> inline C *
ClassFreeList<C>::alloc() {
  C* o = (C*)FreeList::alloc();
  memcpy((void*)o, (void*)&protoObject, sizeof(protoObject));
  return o;
}

#endif
