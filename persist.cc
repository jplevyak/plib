/* -*-Mode: c++;-*-
   Copyright (c) 2003-2009 John Plevyak, All Rights Reserved
*/
#include "plib.h"

#ifdef linux
#define USE_MREMAP 1
#endif

#ifndef HAS_32BIT
#define PERSISTENT_MEMORY ((void *)(intptr_t)(1ULL << 42))
#else
#define PERSISTENT_MEMORY ((void *)(intptr_t)(0x5C000000))
#endif
#define PERSISTENT_MEMORY_SIZE (1LL << 17)  // 128k to start
#define PERSISTENT_MEMORY_HEADER (round2(sizeof(PersistentMemory), 16))

#define PERSISTENT_MMFLAGS (MAP_SHARED | MAP_NORESERVE | MAP_FIXED)
#define TRANSIENT_MMFLAGS (MAP_PRIVATE | MAP_FIXED)
#define MMFLAGS (persistent_memory_persistent ? PERSISTENT_MMFLAGS : TRANSIENT_MMFLAGS)

mspace persistent_mspace = 0;
PersistentMemory *persistent_memory = 0;
char persistent_memory_filename[512] = "persistent.memory";
int persistent_memory_persistent = 0;

static int persistent_fd = 0;

uint64 persistent_memory_len() {
  void *p = mspace_get_morecore_ptr(persistent_mspace);
  return ((char *)p) - ((char *)PERSISTENT_MEMORY);
}

void *persistent_memory_morecore(intptr_t increment, mspace data) {
  void *p = mspace_get_morecore_ptr(data);
  intptr_t l = ((char *)p) - ((char *)PERSISTENT_MEMORY);
  intptr_t ll = l + increment;
  void *pp = (void *)(((char *)p) + increment);
  if (increment > 0) {
    assert(!truncate(persistent_memory_filename, ll));
#ifndef USE_MREMAP
    munmap(PERSISTENT_MEMORY, l);
    void *r = mmap(PERSISTENT_MEMORY, l, PROT_READ | PROT_WRITE, MMFLAGS, persistent_fd, 0);
    assert(r == PERSISTENT_MEMORY);
#else
    void *r = mremap(PERSISTENT_MEMORY, l, ll, 0);
    assert(r == PERSISTENT_MEMORY);
#endif
  } else {
    if (!increment) return p;
#ifndef USE_MREMAP
    munmap(pp, -increment);
#else
    void *r = mremap(PERSISTENT_MEMORY, l, ll, 0);
    assert(r == PERSISTENT_MEMORY);
#endif
    assert(!truncate(persistent_memory_filename, ll));
  }
  mspace_set_morecore_ptr(data, pp);
  return p;
}

int init_persistent_memory() {
  persistent_fd = open(persistent_memory_filename, O_RDWR | O_CREAT | O_NOATIME, 00660);
  if (persistent_fd < 0) return -1;
  assert(!truncate(persistent_memory_filename, PERSISTENT_MEMORY_SIZE));
  void *r = mmap(PERSISTENT_MEMORY, PERSISTENT_MEMORY_SIZE, PROT_READ | PROT_WRITE, MMFLAGS, persistent_fd, 0);
  assert(r == PERSISTENT_MEMORY);
  mspace m = create_mspace_with_base((void *)(((char *)r) + PERSISTENT_MEMORY_HEADER),
                                     PERSISTENT_MEMORY_SIZE - PERSISTENT_MEMORY_HEADER, 0);
  mspace_set_morecore(m, persistent_memory_morecore, (void *)(((char *)r) + PERSISTENT_MEMORY_SIZE));
  persistent_mspace = m;
  persistent_memory = (PersistentMemory *)PERSISTENT_MEMORY;
  memset(persistent_memory, 0, sizeof(*persistent_memory));
  save_persistent_memory();
  return 0;
}

int open_persistent_memory() {
#ifndef MALLOC_MEMORY
  persistent_fd = open(persistent_memory_filename, O_RDWR | O_NOATIME, 00770);
  if (persistent_fd < 0) return -1;
  void *r = mmap(PERSISTENT_MEMORY, PERSISTENT_MEMORY_SIZE, PROT_READ | PROT_WRITE, MMFLAGS, persistent_fd, 0);
  assert(r == PERSISTENT_MEMORY);
  persistent_mspace = mspace_from_base((void *)(((char *)PERSISTENT_MEMORY) + PERSISTENT_MEMORY_HEADER));
  uint64 l = persistent_memory_len();
  if (l > PERSISTENT_MEMORY_SIZE) {
    assert(!truncate(persistent_memory_filename, l));
    munmap(PERSISTENT_MEMORY, PERSISTENT_MEMORY_SIZE);
    r = mmap(PERSISTENT_MEMORY, l, PROT_READ | PROT_WRITE, MMFLAGS, persistent_fd, 0);
    assert(r == PERSISTENT_MEMORY);
  }
  persistent_memory = (PersistentMemory *)PERSISTENT_MEMORY;
  mspace_set_morecore_pfn(persistent_mspace, persistent_memory_morecore);
#endif
  return 0;
}

void close_persistent_memory() {
#ifndef MALLOC_MEMORY
  munmap(PERSISTENT_MEMORY, persistent_memory_len());
  close(persistent_fd);
  persistent_fd = 0;
  persistent_memory = 0;
#endif
}

void save_persistent_memory() {
  size_t s = persistent_memory_len();
  size_t x = ::write(persistent_fd, PERSISTENT_MEMORY, s);
  assert(x == s);
}

void read_persistent_memory() {
  struct stat sb;
  fstat(persistent_fd, &sb);
  size_t s = sb.st_size;
  size_t x = ::read(persistent_fd, PERSISTENT_MEMORY, s);
  assert(x == s);
}
