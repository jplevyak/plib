/* -*-Mode: c++;-*-
   Copyright (c) 2003-2006 John Plevyak, All Rights Reserved
*/
#include "plib.h"

static void *thread_pool_start(void *data) {
  ThreadPool *pool = (ThreadPool *)data;
  while (1) {
    void *(*start)(void *);
    void *data;
    ThreadPoolJob *job = 0;
    if (pool->get_job(&start, &data, &job)) return 0;
    if (job)
      job->main();
    else
      start(data);
  }
}

pthread_t ThreadPool::thread_create(void *(*start_routine)(void *), void *arg, int stacksize) {
  pthread_t t;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  if (stacksize) pthread_attr_setstacksize(&attr, stacksize);
  pthread_create(&t, &attr, start_routine, arg);
  pthread_attr_destroy(&attr);
  return t;
}

static void start_thread(ThreadPool *pool) {
  pool->nthreads++;
  ThreadPool::thread_create(thread_pool_start, (void *)pool, pool->stacksize);
}

void ThreadPool::add_job(void *(*start)(void *), void *data) {
  pthread_mutex_lock(&mutex);
  ThreadPoolJob *job = job_freelist.alloc();
  job->start = start;
  job->data = data;
  jobs.enqueue(job);
  if (nthreadswaiting) {
    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&mutex);
  } else if (nthreads < maxthreads) {
    pthread_mutex_unlock(&mutex);
    start_thread(this);
  }
}

void ThreadPool::add_job(ThreadPoolJob *ajob) {
  pthread_mutex_lock(&mutex);
  ajob->thread_pool_integral = 1;
  jobs.enqueue(ajob);
  if (nthreadswaiting)
    pthread_cond_signal(&condition);
  else if (nthreads < maxthreads)
    start_thread(this);
  pthread_mutex_unlock(&mutex);
}

int ThreadPool::get_job(void *(**start)(void *), void **data, ThreadPoolJob **ajob) {
  pthread_mutex_lock(&mutex);
  while (1) {
    ThreadPoolJob *job = jobs.dequeue();
    if (job) {
      if (job->thread_pool_integral)
        *ajob = job;
      else {
        *start = job->start;
        *data = job->data;
        job_freelist.free(job);
      }
      pthread_mutex_unlock(&mutex);
      return 0;
    }
    if (nthreadswaiting + 1 > maxthreads) break;
    nthreadswaiting++;
    pthread_cond_wait(&condition, &mutex);
    nthreadswaiting--;
    if (nthreadswaiting + 1 > maxthreads) break;
  }
  nthreads--;
  if (nthreadswaiting)
    pthread_cond_signal(&condition);
  else
    pthread_cond_signal(&shutdown_condition);
  pthread_mutex_unlock(&mutex);
  return 1;
}

ThreadPool::ThreadPool(int astacksize, int amaxthreads) {
  maxthreads = amaxthreads;
  stacksize = astacksize;
  nthreadswaiting = nthreads = 0;
  pthread_mutexattr_t mattr;
  pthread_mutexattr_init(&mattr);
  pthread_mutex_init(&mutex, &mattr);
  pthread_condattr_t cattr;
  pthread_condattr_init(&cattr);
  pthread_cond_init(&condition, &cattr);
  pthread_cond_init(&shutdown_condition, &cattr);
}

void ThreadPool::shutdown() {
  pthread_mutex_lock(&mutex);
  int saved_maxthreads = maxthreads;
  maxthreads = 0;
  pthread_cond_signal(&condition);
  while (nthreads) pthread_cond_wait(&shutdown_condition, &mutex);
  maxthreads = saved_maxthreads;
  pthread_mutex_unlock(&mutex);
}

ThreadPool::~ThreadPool() {
  shutdown();
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&condition);
  pthread_cond_destroy(&shutdown_condition);
}
