/* -*-Mode: c++; -*-
   Copyright (c) 2010 John Plevyak, All Rights Reserved
*/

#ifndef _tls_H
#define _tls_H

#ifndef __APPLE__
// as of OSX 10.4
#define HAVE_TLS 1
#endif

#ifdef HAVE_TLS
#define DEF_TLS(_t, _s) __thread _t _s
#define INIT_TLS(_s)
#define TLS(_s) _s
#else
inline void *tls_mallocz(size_t s) { void *x = MALLOC(s); memset(x, 0, s); return x; }
inline pthread_key_t tls_init(pthread_key_t *k) { pthread_key_create(k, NULL); return *k; }
#define DEF_TLS(_type, _s) pthread_key_t _s##_key = tls_init(&_s##_key); struct _s##_t { _type _s; }
#define INIT_TLS(_s) pthread_setspecific(_s##_key, tls_mallocz(sizeof(_s##_t)))
#define TLS(_s) (((_s##_t*)pthread_getspecific(_s##_key))->_s)
#endif

#endif
