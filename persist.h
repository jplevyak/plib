/* -*-Mode: c++;-*-
   Copyright (c) 2003-2009 John Plevyak, All Rights Reserved
*/
#ifndef _persist_H
#define _persist_H

#ifdef __APPLE__
#define O_NOATIME 0
#endif

// #define MALLOC_MEMORY 1

#ifndef MALLOC_MEMORY
#define PERSISTENT_ALLOC(_s)    mspace_malloc(persistent_mspace, _s)
#define PERSISTENT_FREE(_p)     mspace_free(persistent_mspace, p)
#else
#define PERSISTENT_ALLOC(_s)    ::malloc(_s)
#define PERSISTENT_FREE(_p)     ::free(_p)
#endif

struct PersistentMemory {
  uint64 magic;
  uint32 major_version;
  uint32 minor_version;
  void *randstate;
  void *base;
};

extern mspace persistent_mspace;
extern char persistent_memory_filename[512];
extern PersistentMemory *persistent_memory;
extern int persistent_memory_persistent;

class PersistentAlloc { public:
  static void *alloc(int s) { return PERSISTENT_ALLOC(s); }
  static void free(void *p) { PERSISTENT_FREE(p); }
};

class PersistentObject { public:
  static void *operator new(size_t size) { return PERSISTENT_ALLOC(size); }
  static void operator delete(void *p, size_t size) { PERSISTENT_FREE(p); }
};

int init_persistent_memory();
int open_persistent_memory();
void read_persistent_memory();
void save_persistent_memory();
void close_persistent_memory();
uint64 persistent_memory_len();

static inline char*
pdupstr(cchar *s, cchar *e = 0) {
  int l = e ? e-s : strlen(s);
  char *ss = (char*)PERSISTENT_ALLOC(l+1);
  memcpy(ss, s, l);
  ss[l] = 0;
  return ss;
}

static inline void *map_file_ro(cchar *fn, uint64 n = 0, int *pfd = 0, uint64 *pn = 0) {
  int fd = open(fn, O_RDONLY|O_NOATIME, 00660);
  if (fd < 0) {
    fprintf(stderr, "unable to map ro: %s\n", fn);
    perror("open");
  }
  assert(fd > 0);
  if (!n) {
    n = (uint64)::lseek(fd, 0, SEEK_END);
    ::lseek(fd, 0, SEEK_SET);
  }
  void *m = 0;
  if ((m = mmap(0, n, PROT_READ, MAP_PRIVATE, fd, 0)) == (void*)-1) 
    perror("mmap");
  if (pfd) *pfd = fd;
  if (pn) *pn = n;
  return m;
}

// for some reason read-only files must not be opened O_NOATIME
static inline void *map_file_ro_atime(cchar *fn, uint64 n = 0, int *pfd = 0, uint64 *pn = 0) {
  int fd = open(fn, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "unable to map ro: %s\n", fn);
    perror("open");
  }
  assert(fd > 0);
  if (!n) {
    n = (uint64)::lseek(fd, 0, SEEK_END);
    ::lseek(fd, 0, SEEK_SET);
  }
  void *m = 0;
  if ((m = mmap(0, n, PROT_READ, MAP_PRIVATE, fd, 0)) == (void*)-1) 
    perror("mmap");
  if (pfd) *pfd = fd;
  if (pn) *pn = n;
  return m;
}

static inline void *map_file_ro(int fd, uint64 n = 0) {
  if (!n) {
    n = (uint64)::lseek(fd, 0, SEEK_END);
    ::lseek(fd, 0, SEEK_SET);
  }
  void *m = 0;
  if ((m = mmap(0, n, PROT_READ, MAP_PRIVATE, fd, 0)) == (void*)-1) 
    perror("mmap");
  return m;
}

static inline void *read_file(cchar *fn, uint64 n = 0, int *pfd = 0) {
  int fd = open(fn, O_RDONLY|O_NOATIME, 00660);
  if (fd < 0)
    fprintf(stderr, "unable to open: %s\n", fn);
  assert(fd > 0);
  if (!n) {
    n = (uint64)::lseek(fd, 0, SEEK_END);
    ::lseek(fd, 0, SEEK_SET);
  }
  void *m = MALLOC(n);
  ssize_t nn = ::read(fd, m, n);
  if (nn != (ssize_t)n)
    perror("read");
  if (pfd) *pfd = fd;
  return m;
}

static inline char *read_file_to_string(cchar *fn, uint64 n = 0, int *pfd = 0) {
  int fd = open(fn, O_RDONLY|O_NOATIME, 00660);
  if (fd < 0)
    fprintf(stderr, "unable to open: %s\n", fn);
  assert(fd > 0);
  if (!n) {
    n = (uint64)::lseek(fd, 0, SEEK_END);
    ::lseek(fd, 0, SEEK_SET);
  }
  char *m = (char*)MALLOC(n+1);
  m[n] = 0; 
  ssize_t nn = ::read(fd, m, n);
  if (nn != (ssize_t)n)
    perror("read");
  if (pfd) *pfd = fd;
  return m;
}

static inline void *persist_alloc(cchar *fn, uint64 n, int *pfd = 0) {
  int fd = open(fn, O_RDWR|O_CREAT|O_NOATIME, 00660);
  if (fd < 0)
    fprintf(stderr, "unable to map: %s\n", fn);
  assert(fd > 0);
  assert(!ftruncate(fd, n));
  void *m = 0;
  if ((m = mmap(0, n, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_NORESERVE, fd, 0)) == (void*)-1) 
    perror("mmap");
  if (pfd) *pfd = fd;
  return m;
}

static inline void persist_free(void *p, uint64 n, int fd) {
  if (munmap(p, n)) perror("mumap");
  assert(!close(fd));
}

#endif
