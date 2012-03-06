#include <errno.h>
#include "barrier.h"

int 
barrier_init(barrier_t *barrier, int count) {
  int ret = 0;
  barrier->counter = count;
  if ((ret = pthread_mutex_init(&barrier->mutex, 0)) != 0)
    return ret;
  if ((ret = pthread_cond_init (&barrier->cond, 0)) != 0) {
    pthread_mutex_destroy(&barrier->mutex);
    return ret;
  }
  return 0;
}

int 
barrier_destroy(barrier_t *barrier) {
  int ret = 0;
  if ((ret = pthread_mutex_lock(&barrier->mutex)) != 0)
    return ret;
  if (barrier->counter) {
    pthread_mutex_unlock(&barrier->mutex);
    return EBUSY;
  }
  if ((ret = pthread_mutex_unlock(&barrier->mutex)) != 0)
    return ret;
  if ((ret = pthread_mutex_destroy(&barrier->mutex)) != 0) {
    pthread_cond_destroy(&barrier->cond);
    return ret;
  }
  return pthread_cond_destroy(&barrier->cond);
}

int 
barrier_signal(barrier_t *barrier) {
  int ret = 0;
  if ((ret = pthread_mutex_lock(&barrier->mutex)))
    return ret;
  barrier->counter--;
  if (barrier->counter == 0)
    if ((ret = pthread_cond_broadcast(&barrier->cond)) != 0) {
      pthread_mutex_unlock(&barrier->mutex);
      return ret;
    }
  if ((ret = pthread_mutex_unlock(&barrier->mutex)) != 0)
    return ret;
  return ret;
}

int 
barrier_wait(barrier_t *barrier) {
  int ret = 0;
  if ((ret = pthread_mutex_lock(&barrier->mutex)))
    return ret;
  while (barrier->counter)
    if ((ret = pthread_cond_wait(&barrier->cond, &barrier->mutex)) != 0)
      break;
  if ((ret = pthread_mutex_unlock(&barrier->mutex)) != 0)
    return ret;
  return ret;
}

int
barrier_signal_and_wait(barrier_t *barrier) {
  int ret = 0;
  if ((ret = pthread_mutex_lock(&barrier->mutex)))
    return ret;
  barrier->counter--;
  if (barrier->counter == 0)
    if ((ret = pthread_cond_broadcast(&barrier->cond)) != 0) {
      pthread_mutex_unlock(&barrier->mutex);
      return ret;
    }
  while (barrier->counter)
    if ((ret = pthread_cond_wait(&barrier->cond, &barrier->mutex)) != 0)
      break;
  if ((ret = pthread_mutex_unlock(&barrier->mutex)) != 0)
    return ret;
  return ret;
}
