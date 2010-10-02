/* -*-Mode: c++;-*-
   Copyright (c) 2008 John Plevyak, All Rights Reserved
*/

#ifndef _plib_H_
#define _plib_H_

#ifdef LEAK_DETECT
#define GC_DEBUG
#include "gc.h"
#define MEM_INIT() GC_INIT()
#define MALLOC(_n) GC_MALLOC(_n)
#define REALLOC(_p,_n) GC_REALLOC((_p),(_n))
#define MEMALIGN(_p,_n,_a) _p = GC_MALLOC(_n)
#define CHECK_LEAKS() GC_gcollect()
#define FREE(_p) GC_FREE(_p)
#define DELETE(_x)
#else
#ifdef USE_GC
#include "gc_cpp.h"
#define MEM_INIT() GC_INIT()
#define MALLOC(_n) GC_MALLOC(_n)
#define REALLOC(_p,_n) GC_REALLOC((_p),(_n))
#define MEMALIGN(_p,_n,_a) _p = GC_MALLOC(_n)
#define FREE(_x) (void)(_x)
#define DELETE(_x) (void)(_x)
#define malloc dont_use_malloc
#define realloc dont_use_realloc
#define free dont_use_free
//#define delete dont_use_delete
#else
#define MEM_INIT() 
#define MALLOC ::malloc
#define REALLOC ::realloc
#define MEMALIGN(_p,_a,_n) ::posix_memalign((void**)&(_p),(_a),(_n))
#define FREE ::free
#define DELETE(_x) delete _x
class gc {};
#endif
#endif

#define round2(_x,_n) ((_x + ((_n)-1)) & ~((_n)-1))
#define tohex1(_x) \
((((_x)&15) > 9) ? (((_x)&15) - 10 + 'A') : (((_x)&15) + '0'))
#define tohex2(_x) \
((((_x)>>4) > 9) ? (((_x)>>4) - 10 + 'A') : (((_x)>>4) + '0'))
#define numberof(_x) ((sizeof(_x))/(sizeof((_x)[0])))
#define max(a,b) ((a)>(b)?(a):(b))

typedef char int8;
typedef unsigned char uint8;
typedef uint8 byte;
typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;
typedef short int16;
typedef unsigned short uint16;
/* typedef uint32 uint; * already part of most systems */

typedef const char cchar;
typedef char *charptr_t;
typedef const char *ccharptr_t;

#ifdef EXTERN
#define EXTERN_INIT(_x) = _x
#define EXTERN_ARGS(_x) _x
#else
#define EXTERN_INIT(_x)
#define EXTERN_ARGS(_x)
#define EXTERN extern
#endif

#define PERROR(_s) do { perror(_s); exit(1); } while(0)
#define RETURN(_x) do { result = _x; goto Lreturn; } while (0)

#define INIT_RAND64(_seed) init_genrand64(_seed)
#define RND64() genrand64_int64()
#define RNDD() genrand64_real1()

#define __STDC_LIMIT_MACROS 1

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <float.h>

#include "tls.h"
#include "arg.h"
#include "barrier.h"
#include "config.h"
#include "stat.h"
#include "dlmalloc.h"
#include "freelist.h"
#include "defalloc.h"
#include "list.h"
#include "log.h"
#include "vec.h"
#include "map.h"
#include "threadpool.h"
#include "misc.h"
#include "util.h"
#include "conn.h"
#include "md5.h"
#include "mt64.h"
#include "hash.h"
#include "persist.h"
#include "prime.h"
#include "service.h"
#include "timer.h"
#include "unit.h"

#endif
