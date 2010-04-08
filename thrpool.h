/* -*-Mode: c++;-*-
   Copyright (c) 2003-2008 John Plevyak, All Rights Reserved
*/
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

class ThreadPoolJob { public:
  void *(*start)(void*);
  void *data;

  virtual int main() { assert(!"no main();"); return 0; }

  int thread_pool_integral;
  LINK(ThreadPoolJob, thread_pool_link);

  ThreadPoolJob() : thread_pool_integral(0) { }
};

class ThreadPool { public:
  pthread_mutex_t mutex;
  pthread_cond_t condition, shutdown_condition;
  int nthreads, nthreadswaiting, maxthreads, stacksize;
  CountQue(ThreadPoolJob, thread_pool_link) jobs;
  ClassFreeList<ThreadPoolJob> job_freelist;

  void add_job(void *(*start)(void *), void *data);
  void add_job(ThreadPoolJob *job);
  void shutdown(); // doesn't wait for queued but not running jobs

  ThreadPool(int astacksize = 0, int amaxthreads = INT_MAX);
  ~ThreadPool();
  // public utility function
  static pthread_t thread_create(void *(*start_routine)(void*), void * arg, int stacksize = 0);
  // private
  int get_job(void *(**start)(void *), void **data, ThreadPoolJob **job);
};

#endif

