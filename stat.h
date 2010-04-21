/* -*-Mode: c++; -*-
   Copyright (c) 2010 John Plevyak, All Rights Reserved
*/
#ifndef _stat_H
#define _stat_H

struct Stat {
  int id;
  cchar *name;
  int64_t sum;
  int64_t count;
  Stat *next; // next registered stat
};

void init_stat();
void init_stat_thread();
void register_stat(cchar *name, Stat &s); // per thread
void register_global_stat(cchar *name, Stat &s); // per process
int process_stat_snap_internal();
EXTERN Stat* stat_snap_requested EXTERN_INIT(0);
static inline int process_stat_snap() { // call in event loop
  if (stat_snap_requested) 
    return process_stat_snap_internal();
  return 0;
}
void snap_stats(Stat **, int *);

#define STAT(_s) __thread Stat _s
#define STAT_SUM(_s, _v) do  { _s.sum += _v; _s.count++; } while (0)
#define STAT_ADD(_s, _v) do  { _s.count += _v; } while (0)
#define STAT_INC(_s) STAT_ADD(_s, 1)
#define STAT_DEC(_s) STAT_ADD(_s, -1)

#define GSTAT(_s) Stat _s
#define GSTAT_SUM(_s, _v) do  { __sync_fetch_and_add(&_s.sum, _v); __sync_fetch_and_add(&_s.count, 1); } while (0)
#define GSTAT_ADD(_s, _v) do  { __sync_fetch_and_add(&_s.count, _v); } while (0)
#define GSTAT_INC(_s) GSTAT_ADD(_s, 1)
#define GSTAT_DEC(_s) GSTAT_ADD(_s, -1)

void test_stat();

#endif
