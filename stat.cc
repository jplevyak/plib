/* -*-Mode: c++; -*-
   Copyright (c) 2010 John Plevyak, All Rights Reserved
*/
#include "plib.h"

struct stat_thread {
  pthread_t id;
  struct stat_thread *next;
};

DEF_TLS(Stat *, thread_stats);
Stat *global_stats = 0;
static int nstats = 0;
static StringChainHash<> stat_strings;
static pthread_mutex_t stat_mutex;
typedef HashMap<cchar *, StringHashFns, int> MapCCharInt;
static MapCCharInt stats;
static pthread_cond_t stat_condition, stat_done_condition;
static stat_thread *stat_threads;
static int nstat_waiting = 0;
static uint64_t snap_serial = 0;
DEF_TLS(uint64_t, my_snap_serial);

void init_stat() {
  init_recursive_mutex(&stat_mutex);
  pthread_cond_init(&stat_condition, NULL);
  pthread_cond_init(&stat_done_condition, NULL);
}

void init_stat_thread() {
  INIT_TLS(thread_stats);
  INIT_TLS(my_snap_serial);
  TLS(thread_stats) = 0;
  stat_thread *t = new stat_thread;
  t->id = pthread_self();
  pthread_mutex_lock(&stat_mutex);
  t->next = stat_threads;
  stat_threads = t;
  pthread_mutex_unlock(&stat_mutex);
}

static void process_stat_list(Stat *list, Stat *snap) {
  while (list) {
    snap[list->id].sum += list->sum;
    snap[list->id].count += list->count;
    list = list->next;
  }
}

int process_stat_snap_internal() {
  int r = 0;
  pthread_mutex_lock(&stat_mutex);
  if (TLS(my_snap_serial) != snap_serial) {
    process_stat_list(TLS(thread_stats), stat_snap_requested);
    TLS(my_snap_serial) = snap_serial;
    nstat_waiting--;
    pthread_cond_signal(&stat_condition);
    r = 1;
  }
  pthread_mutex_unlock(&stat_mutex);
  return r;
}

void snap_stats(Stat **pstat, int *plen) {
  pthread_mutex_lock(&stat_mutex);
  while (stat_snap_requested) {
    process_stat_snap();
    pthread_cond_wait(&stat_done_condition, &stat_mutex);
  }
  int l = sizeof(Stat) * nstats;
  Stat *s = (Stat *)MALLOC(l);
  memset(s, 0, l);
  form_Map(MapCCharInt::ME, x, stats) s[x->value - 1].name = x->key;
  for (int i = 0; i < nstats; i++) {
    s[i].id = i;
    assert(s[i].name);
  }
  process_stat_list(global_stats, s);
  pthread_t me = pthread_self();
  stat_thread *t = stat_threads;
  nstat_waiting = 0;
  while (t) {
    if (me != t->id)
      nstat_waiting++;
    else
      process_stat_list(TLS(thread_stats), s);
    t = t->next;
  }
  if (nstat_waiting > 0) {
    snap_serial++;
    TLS(my_snap_serial) = snap_serial;
    stat_snap_requested = s;
    while (nstat_waiting > 0) pthread_cond_wait(&stat_condition, &stat_mutex);
    stat_snap_requested = 0;
    pthread_cond_signal(&stat_done_condition);
  }
  *pstat = s;
  *plen = nstats;
  pthread_mutex_unlock(&stat_mutex);
}

static void get_stat_id(cchar *name, Stat &s) {
  cchar *cname = stat_strings.canonicalize(name);
  int id = stats.get(cname);
  if (!id) {
    id = nstats++;
    stats.put(cname, id + 1);
  } else
    id = id - 1;
  s.id = id;
}

void register_global_stat(cchar *name, Stat &s) {
  pthread_mutex_lock(&stat_mutex);
  get_stat_id(name, s);
  s.next = global_stats;
  global_stats = &s;
  pthread_mutex_unlock(&stat_mutex);
}

void register_stat(cchar *name, Stat &s) {
  pthread_mutex_lock(&stat_mutex);
  get_stat_id(name, s);
  pthread_mutex_unlock(&stat_mutex);
  s.next = TLS(thread_stats);
  assert((Stat *)&TLS(thread_stats) != &s);
  TLS(thread_stats) = &s;
}

GSTAT(test_gstat_stat);
STAT(test_stat_stat);

void *stat_pthread(void *) {
  init_stat_thread();
  INIT_TLS(test_stat_stat);
  register_stat("test_stat", TLS(test_stat_stat));
  while (1) {
    wait_for(HRTIME_MSEC * 20);
    if (process_stat_snap()) break;
  }
  STAT_INC(test_stat_stat);
  GSTAT_INC(test_gstat_stat);
  Stat *allstats = 0;
  int nallstats = 0;
  snap_stats(&allstats, &nallstats);
  assert(allstats[TLS(test_stat_stat).id].sum == 5);
  assert(allstats[TLS(test_stat_stat).id].count == 9);
  assert(allstats[test_gstat_stat.id].sum == 5);
  assert(allstats[test_gstat_stat.id].count == 9);
  return 0;
}

#ifdef TEST_LIB
void test_stat() {
  init_stat();
  init_stat_thread();
  INIT_TLS(test_stat_stat);
  register_stat("test_stat", TLS(test_stat_stat));
  register_global_stat("test_gstat", test_gstat_stat);
  create_thread(stat_pthread, 0, 0);
  wait_for(HRTIME_MSEC * 20);

  STAT_SUM(test_stat_stat, 5);
  STAT_ADD(test_stat_stat, 7);
  STAT_INC(test_stat_stat);
  STAT_DEC(test_stat_stat);

  GSTAT_SUM(test_gstat_stat, 5);
  GSTAT_ADD(test_gstat_stat, 7);
  GSTAT_INC(test_gstat_stat);
  GSTAT_DEC(test_gstat_stat);

  Stat *allstats = 0;
  int nallstats = 0;
  snap_stats(&allstats, &nallstats);
  while (1) {
    wait_for(HRTIME_MSEC * 20);
    if (process_stat_snap()) break;
  }
  assert(nallstats == 2);
  assert(allstats[TLS(test_stat_stat).id].sum == 5);
  assert(allstats[TLS(test_stat_stat).id].count == 8);
  assert(allstats[test_gstat_stat.id].sum == 5);
  assert(allstats[test_gstat_stat.id].count == 8);
  printf("stat test\tPASSED\n");
}
#endif
