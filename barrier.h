/* -*-Mode: c++;-*-
   Copyright (c) 1994-2008 John Plevyak, All Rights Reserved
*/
#ifndef barrier_H
#define barrier_H

#include <pthread.h>

struct barrier_t {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int counter;
};

#define BARRIER_INITIALIZER \
  { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0 }

extern int barrier_init(barrier_t *barrier, int count);
extern int barrier_destroy(barrier_t *barrier);
extern int barrier_signal(barrier_t *barrier);           // worker signals
extern int barrier_wait(barrier_t *barrier);             // master waits
extern int barrier_signal_and_wait(barrier_t *barrier);  // cooperative barrier

#endif
