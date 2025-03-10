/* -*-Mode: c++;-*-
   Copyright (c) 2003-2013 John Plevyak, All Rights Reserved
*/

#ifndef _vec_H_
#define _vec_H_

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "defalloc.h"

// Simple Vector class, also supports open hashed sets

#define VEC_INTEGRAL_SHIFT_DEFAULT 2 /* power of 2 (1 << VEC_INTEGRAL_SHIFT)*/
#define VEC_INTEGRAL_SIZE (1 << (S))
#define VEC_INITIAL_SHIFT ((S) + 1)
#define VEC_INITIAL_SIZE (1 << VEC_INITIAL_SHIFT)

#define SET_LINEAR_SIZE 4 /* must be <= than VEC_INTEGRAL_SIZE */
#define SET_INITIAL_INDEX 2

template <class C, class A = DefaultAlloc, int S = VEC_INTEGRAL_SHIFT_DEFAULT>  // S must be a power of 2
class Vec : public gc {
 public:
  enum InitType { SET, MOVE };
  int n;
  int i;  // size index for sets, reserve for vectors
  C *v;
  C e[VEC_INTEGRAL_SIZE];

  Vec();
  Vec<C, A, S>(const Vec<C, A, S> &vv);
  Vec<C, A, S>(Vec<C, A, S> &vv, InitType init_type);
  Vec<C, A, S>(const C c);
  ~Vec();

  C &operator[](int i) const { return v[i]; }

  C get(int i);
  void add(C a);
  int add_exclusive(C a);
  C &add();
  C pop();
  void reset();
  void clear();
  void free();
  void free_and_clear();
  void delete_and_clear();
  void set_clear();
  C *set_add(C a);
  void set_remove(C a);  // expensive, use BlockHash for cheaper remove
  C *set_add_internal(C a);
  int set_union(Vec<C, A, S> &v);
  int set_intersection(Vec<C, A, S> &v);
  int some_intersection(Vec<C, A, S> &v);
  int some_disjunction(Vec<C, A, S> &v);
  int some_difference(Vec<C, A, S> &v);
  void set_intersection(Vec<C, A, S> &v, Vec<C, A, S> &result);
  void set_disjunction(Vec<C, A, S> &v, Vec<C, A, S> &result);
  void set_difference(Vec<C, A, S> &v, Vec<C, A, S> &result);
  int set_count();
  int count();
  C *in(C a);
  C *set_in(C a);
  C first_in_set();
  C *set_in_internal(C a);
  void set_expand();
  int index(C a);
  void set_to_vec();
  void vec_to_set();
  void move(Vec<C, A, S> &v);
  void copy(const Vec<C, A, S> &v);
  void fill(int n);
  void append(const Vec<C> &v);
  void prepend(const Vec<C> &v);
  void remove_index(int index);
  void remove(C a) {
    int i = index(a);
    if (i >= 0) remove_index(i);
  }
  C &insert(int index);
  void insert(int index, Vec<C> &vv);
  void insert(int index, C a);
  void push(C a) { insert(0, a); }
  void reverse();
  void reserve(int n);
  C *end() const { return v + n; }
  C &first() const { return v[0]; }
  C &last() const { return v[n - 1]; }
  Vec<C, A, S> &operator=(Vec<C, A, S> &v) {
    this->copy(v);
    return *this;
  }
  bool is_vec() const { return !i || i >= n; }
  bool is_set() const { return !is_vec(); }
  int length() const { return n; }
  int write(int fd);
  int read(int fd);
  void qsort(bool (*lt)(C, C));

  // private:
  void move_internal(Vec<C, A, S> &v);
  void copy_internal(const Vec<C, A, S> &v);
  void add_internal(C a);
  C &add_internal();
  void addx();
};

// c -- class, p -- pointer to elements of v, v -- vector
#pragma GCC diagnostic ignored "-Wparentheses"
#define forv_Vec(_c, _p, _v)                                                                 \
  if ((_v).n)                                                                                \
    for (_c *qq__##_p = (_c *)0, *_p = (_v).v[0];                                            \
         ((intptr_t)(qq__##_p) < (_v).length()) && ((_p = (_v).v[(intptr_t)qq__##_p]) || 1); \
         qq__##_p = (_c *)(((intptr_t)qq__##_p) + 1))
#define for_Vec(_c, _p, _v)                                                                  \
  if ((_v).n)                                                                                \
    for (_c *qq__##_p = (_c *)0, _p = (_v).v[0];                                             \
         ((intptr_t)(qq__##_p) < (_v).length()) && ((_p = (_v).v[(intptr_t)qq__##_p]) || 1); \
         qq__##_p = (_c *)(((intptr_t)qq__##_p) + 1))
#define forvp_Vec(_c, _p, _v)                                                                 \
  if ((_v).n)                                                                                 \
    for (_c *qq__##_p = (_c *)0, *_p = &(_v).v[0];                                            \
         ((intptr_t)(qq__##_p) < (_v).length()) && ((_p = &(_v).v[(intptr_t)qq__##_p]) || 1); \
         qq__##_p = (_c *)(((intptr_t)qq__##_p) + 1))

template <class C, class A = DefaultAlloc, int S = VEC_INTEGRAL_SHIFT_DEFAULT>
class Accum : public gc {
 public:
  Vec<C, A, S> asset;
  Vec<C, A, S> asvec;
  void add(C c) {
    if (asset.set_add(c)) asvec.add(c);
  }
  void add(Vec<C, A, S> v) {
    for (int i = 0; i < v.n; i++)
      if (v.v[i]) add(v.v[i]);
  }
  void clear() {
    asset.clear();
    asvec.clear();
  }
};

// Intervals store sets in interval format (e.g. [1..10][12..12]).
// Inclusion test is by binary search on intervals.
// Deletion is not supported
class Intervals : public Vec<int> {
 public:
  void insert(int n);
  int in(int n);
};

// UnionFind supports fast unify and finding of
// 'representitive elements'.
// Elements are numbered from 0 to N-1.
class UnionFind : public Vec<int> {
 public:
  // set number of elements, initialized to singletons, may be called repeatedly to increase size
  void size(int n);
  // return representitive element
  int find(int n);
  // unify the sets containing the two elements
  void unify(int n, int m);
};

extern uintptr_t prime2[];
extern uintptr_t open_hash_primes[256];

/* IMPLEMENTATION */

template <class C, class A, int S>
inline Vec<C, A, S>::Vec() : n(0), i(0), v(0) {
  memset((void*)&e[0], 0, sizeof(e));
}

template <class C, class A, int S>
inline Vec<C, A, S>::Vec(const Vec<C, A, S> &vv) : n(0), i(0), v(0) {
  copy(vv);
}

template <class C, class A, int S>
inline Vec<C, A, S>::Vec(Vec<C, A, S> &vv, Vec<C, A, S>::InitType init_type) : n(0), i(0), v(0) {
  if (init_type == SET)
    set_union(vv);
  else
    move(vv);
}

template <class C, class A, int S>
inline Vec<C, A, S>::Vec(C c) : n(1), i(0) {
  v = &e[0];
  e[0] = c;
}

template <class C, class A, int S>
inline C Vec<C, A, S>::get(int i) {
  if (i < n && i >= 0)
    return v[i];
  else
    return 0;
}

template <class C, class A, int S>
inline void Vec<C, A, S>::add(C a) {
  if (n & (VEC_INTEGRAL_SIZE - 1))
    v[n++] = a;
  else if (!v)
    (v = e)[n++] = a;
  else
    add_internal(a);
}

template <class C, class A, int S>
inline C &Vec<C, A, S>::add() {
  C *ret;
  if (n & (VEC_INTEGRAL_SIZE - 1))
    ret = &v[n++];
  else if (!v)
    ret = &(v = e)[n++];
  else
    ret = &add_internal();
  return *ret;
}

template <class C, class A, int S>
inline C Vec<C, A, S>::pop() {
  if (!n) return 0;
  n--;
  C ret = v[n];
  if (!n) clear();
  return ret;
}

template <class C, class A, int S>
inline void Vec<C, A, S>::set_clear() {
  memset((void*)v, 0, n * sizeof(C));
}

template <class C, class A, int S>
inline C *Vec<C, A, S>::set_add(C a) {
  if (n < SET_LINEAR_SIZE) {
    for (C *c = v; c < v + n; c++)
      if (*c == a) return 0;
    add(a);
    return &v[n - 1];
  }
  if (n == SET_LINEAR_SIZE) {
    Vec<C, A, S> vv;
    vv.move(*this);
    for (C *c = vv.v; c < vv.v + vv.n; c++) set_add_internal(*c);
  }
  return set_add_internal(a);
}

template <class C, class A, int S>
void Vec<C, A, S>::set_remove(C a) {
  Vec<C, A, S> tmp;
  tmp.move(*this);
  for (C *c = tmp.v; c < tmp.v + tmp.n; c++)
    if (*c != a) set_add(a);
}

template <class C, class A, int S>
inline int Vec<C, A, S>::count() {
  int x = 0;
  for (C *c = v; c < v + n; c++)
    if (*c) x++;
  return x;
}

template <class C, class A, int S>
inline C *Vec<C, A, S>::in(C a) {
  for (C *c = v; c < v + n; c++)
    if (*c == a) return c;
  return NULL;
}

template <class C, class A, int S>
inline int Vec<C, A, S>::add_exclusive(C a) {
  if (!in(a)) {
    add(a);
    return 1;
  } else
    return 0;
}

template <class C, class A, int S>
inline C *Vec<C, A, S>::set_in(C a) {
  if (n <= SET_LINEAR_SIZE) return in(a);
  return set_in_internal(a);
}

template <class C, class A, int S>
inline C Vec<C, A, S>::first_in_set() {
  for (C *c = v; c < v + n; c++)
    if (*c) return *c;
  return 0;
}

template <class C, class A, int S>
inline int Vec<C, A, S>::index(C a) {
  for (C *c = v; c < v + n; c++)
    if (*c == a) return c - v;
  return -1;
}

template <class C, class A, int S>
inline void Vec<C, A, S>::move_internal(Vec<C, A, S> &vv) {
  n = vv.n;
  i = vv.i;
  if (vv.v == &vv.e[0]) {
    memcpy((void*)e, &vv.e[0], sizeof(e));
    v = e;
  } else
    v = vv.v;
}

template <class C, class A, int S>
inline void Vec<C, A, S>::move(Vec<C, A, S> &vv) {
  move_internal(vv);
  vv.v = 0;
  vv.clear();
}

template <class C, class A, int S>
inline void Vec<C, A, S>::copy(const Vec<C, A, S> &vv) {
  if (vv.v == &vv.e[0]) {
    free();
    n = vv.n;
    i = vv.i;
    v = e;
    memcpy((void*)e, &vv.e[0], sizeof(e));
  } else {
    if (vv.v)
      copy_internal(vv);
    else {
      free();
      n = vv.n;
      i = vv.i;
      v = 0;
    }
  }
}

template <class C, class A, int S>
inline void Vec<C, A, S>::fill(int nn) {
  for (int i = n; i < nn; i++) add() = 0;
}

template <class C, class A, int S>
inline void Vec<C, A, S>::append(const Vec<C> &vv) {
  for (C *c = vv.v; c < vv.v + vv.n; c++)
    if (*c != 0) add(*c);
}

template <class C, class A, int S>
inline void Vec<C, A, S>::prepend(const Vec<C> &vv) {
  if (vv.n) {
    int oldn = n;
    fill(n + vv.n);
    if (oldn) memmove(&v[vv.n], &v[0], oldn * sizeof(v[0]));
    memcpy((void*)&v[0], vv.v, vv.n * sizeof(v[0]));
  }
}

template <class C, class A, int S>
void Vec<C, A, S>::add_internal(C a) {
  addx();
  v[n++] = a;
}

template <class C, class A, int S>
C &Vec<C, A, S>::add_internal() {
  addx();
  return v[n++];
}

template <class C, class A, int S>
C *Vec<C, A, S>::set_add_internal(C c) {
  int j, k;
  if (n) {
    uintptr_t h = (uintptr_t)c;
    h = h % n;
    for (k = h, j = 0; j < i + 3; j++) {
      if (!v[k]) {
        v[k] = c;
        return &v[k];
      } else if (v[k] == c)
        return 0;
      k = (k + open_hash_primes[j]) % n;
    }
  }
  Vec<C, A, S> vv;
  vv.move_internal(*this);
  set_expand();
  if (vv.v) set_union(vv);
  return set_add(c);
}

template <class C, class A, int S>
C *Vec<C, A, S>::set_in_internal(C c) {
  int j, k;
  if (n) {
    uintptr_t h = (uintptr_t)c;
    h = h % n;
    for (k = h, j = 0; j < i + 3; j++) {
      if (!v[k])
        return 0;
      else if (v[k] == c)
        return &v[k];
      k = (k + open_hash_primes[j]) % n;
    }
  }
  return 0;
}

template <class C, class A, int S>
int Vec<C, A, S>::set_union(Vec<C, A, S> &vv) {
  int changed = 0;
  for (int i = 0; i < vv.n; i++)
    if (vv.v[i]) changed = set_add(vv.v[i]) || changed;
  return changed;
}

template <class C, class A, int S>
int Vec<C, A, S>::set_intersection(Vec<C, A, S> &vv) {
  Vec<C, A, S> tv;
  tv.move(*this);
  int changed = 0;
  for (int i = 0; i < tv.n; i++)
    if (tv.v[i]) {
      if (vv.set_in(tv.v[i]))
        set_add(tv.v[i]);
      else
        changed = 1;
    }
  return changed;
}

template <class C, class A, int S>
int Vec<C, A, S>::some_intersection(Vec<C, A, S> &vv) {
  for (int i = 0; i < n; i++)
    if (v[i])
      if (vv.set_in(v[i])) return 1;
  return 0;
}

template <class C, class A, int S>
int Vec<C, A, S>::some_disjunction(Vec<C, A, S> &vv) {
  for (int i = 0; i < n; i++)
    if (v[i])
      if (!vv.set_in(v[i])) return 1;
  for (int i = 0; i < vv.n; i++)
    if (vv.v[i])
      if (!set_in(vv.v[i])) return 1;
  return 0;
}

template <class C, class A, int S>
void Vec<C, A, S>::set_intersection(Vec<C, A, S> &vv, Vec<C, A, S> &result) {
  for (int i = 0; i < n; i++)
    if (v[i])
      if (vv.set_in(v[i])) result.set_add(v[i]);
}

template <class C, class A, int S>
void Vec<C, A, S>::set_disjunction(Vec<C, A, S> &vv, Vec<C, A, S> &result) {
  for (int i = 0; i < n; i++)
    if (v[i])
      if (!vv.set_in(v[i])) result.set_add(v[i]);
  for (int i = 0; i < vv.n; i++)
    if (vv.v[i])
      if (!set_in(vv.v[i])) result.set_add(vv.v[i]);
}

template <class C, class A, int S>
void Vec<C, A, S>::set_difference(Vec<C, A, S> &vv, Vec<C, A, S> &result) {
  for (int i = 0; i < n; i++)
    if (v[i])
      if (!vv.set_in(v[i])) result.set_add(v[i]);
}

template <class C, class A, int S>
int Vec<C, A, S>::some_difference(Vec<C, A, S> &vv) {
  for (int i = 0; i < n; i++)
    if (v[i])
      if (!vv.set_in(v[i])) return 1;
  return 0;
}

template <class C, class A, int S>
int Vec<C, A, S>::set_count() {
  int x = 0;
  for (int i = 0; i < n; i++)
    if (v[i]) x++;
  return x;
}

template <class C, class A, int S>
void Vec<C, A, S>::set_to_vec() {
  C *x = &v[0], *y = x;
  for (; y < v + n; y++) {
    if (*y) {
      if (x != y) *x = *y;
      x++;
    }
  }
  n = x - v;
  if (i) {
    i = 2 << i;  // convert set allocation to reserve
    if (i - n > 0) memset((void*)&v[n], 0, (i - n) * (sizeof(C)));
  } else {
    i = 0;
    if (v == &e[0] && VEC_INTEGRAL_SIZE - n > 0) memset((void*)&v[n], 0, (VEC_INTEGRAL_SIZE - n) * (sizeof(C)));
  }
}

template <class C, class A, int S>
void Vec<C, A, S>::vec_to_set() {
  Vec<C, A, S> vv;
  vv.move(*this);
  for (C *c = vv.v; c < vv.v + vv.n; c++) set_add(*c);
}

template <class C, class A, int S>
void Vec<C, A, S>::remove_index(int index) {
  if (n > 1) memmove(&v[index], &v[index + 1], (n - 1 - index) * sizeof(v[0]));
  n--;
  if (n <= 0) v = e;
}

template <class C, class A, int S>
void Vec<C, A, S>::insert(int index, C a) {
  add();
  memmove(&v[index + 1], &v[index], (n - index - 1) * sizeof(C));
  v[index] = a;
}

template <class C, class A, int S>
void Vec<C, A, S>::insert(int index, Vec<C> &vv) {
  fill(n + vv.n);
  memmove(&v[index + vv.n], &v[index], (n - index - 1) * sizeof(C));
  for (int x = 0; x < vv.n; x++) v[index + x] = vv[x];
}

template <class C, class A, int S>
C &Vec<C, A, S>::insert(int index) {
  add();
  memmove(&v[index + 1], &v[index], (n - index - 1) * sizeof(C));
  memset((void*)&v[index], 0, sizeof(C));
  return v[index];
}

template <class C, class A, int S>
void Vec<C, A, S>::reverse() {
  for (int i = 0; i < n / 2; i++) {
    C *s = &v[i], *e = &v[n - 1 - i];
    C t;
    memcpy((void*)&t, s, sizeof(t));
    memcpy((void*)s, e, sizeof(t));
    memcpy((void*)e, &t, sizeof(t));
  }
}

template <class C, class A, int S>
void Vec<C, A, S>::copy_internal(const Vec<C, A, S> &vv) {
  if (i == vv.i && n == vv.n) {
    memcpy((void*)v, vv.v, n * sizeof(C));
    return;
  }
  free();
  if (vv.is_set()) {
    i = vv.i;
    n = vv.n;
    int nn = 2 << i;
    v = (C *)A::alloc(nn * sizeof(C));
    memcpy((void*)v, vv.v, n * sizeof(C));
    memset((void*)(v + n), 0, (nn - n) * sizeof(C));
    return;
  }
  // is_vec()
  n = vv.n;
  i = 0;  // no reserve
  int l = n, nl = (1 + VEC_INITIAL_SHIFT);
  l = l >> VEC_INITIAL_SHIFT;
  while (l) {
    l = l >> 1;
    nl++;
  }
  nl = 1 << nl;
  v = (C *)A::alloc(nl * sizeof(C));
  memcpy((void*)v, vv.v, n * sizeof(C));
  memset((void*)(v + n), 0, (nl - vv.n) * sizeof(C));
}

template <class C, class A, int S>
void Vec<C, A, S>::set_expand() {
  if (!n)
    i = SET_INITIAL_INDEX;
  else
    i = i + 1;
  n = prime2[i];
  // allocate power of 2 so the fast path in add() will work after set_to_vec()
  v = (C *)A::alloc((2 << i) * sizeof(C));
  memset((void*)v, 0, n * sizeof(C));
}

template <class C, class A, int S>
inline void Vec<C, A, S>::reserve(int x) {
  if (x <= n) return;
  int xx = 1 << VEC_INITIAL_SHIFT;
  while (xx < x) xx *= 2;
  i = xx;
  void *vv = (void *)v;
  v = (C *)A::alloc(i * sizeof(C));
  if (vv && n) memcpy((void*)v, vv, n * sizeof(C));
  memset((void*)&v[n], 0, (i - n) * sizeof(C));
  if (vv && vv != e) A::free(vv);
}

template <class C, class A, int S>
inline void Vec<C, A, S>::addx() {
  if (!v) {
    v = e;
    return;
  }
  if (v == e) {
    v = (C *)A::alloc(VEC_INITIAL_SIZE * sizeof(C));
    memcpy((void*)v, &e[0], n * sizeof(C));
    assert(n < VEC_INITIAL_SIZE);
    memset((void*)&v[n], 0, (VEC_INITIAL_SIZE - n) * sizeof(C));
  } else {
    if ((n & (n - 1)) == 0) {
      int nl = n * 2;
      if (nl <= i)
        return;
      else
        i = 0;
      void *vv = (void *)v;
      v = (C *)A::alloc(nl * sizeof(C));
      memcpy((void*)v, vv, n * sizeof(C));
      memset((void*)&v[n], 0, n * sizeof(C));
      A::free(vv);
    }
  }
}

template <class C, class A, int S>
inline void Vec<C, A, S>::free() {
  if (v && v != e) A::free(v);
}

template <class C, class A, int S>
inline void Vec<C, A, S>::reset() {
  v = NULL;
  n = 0;
  i = 0;
}

template <class C, class A, int S>
inline void Vec<C, A, S>::clear() {
  free();
  reset();
}

template <class C, class A, int S>
inline void Vec<C, A, S>::free_and_clear() {
  for (int x = 0; x < n; x++)
    if (v[x]) A::free(v[x]);
  clear();
}

template <class C, class A, int S>
inline void Vec<C, A, S>::delete_and_clear() {
  for (int x = 0; x < n; x++)
    if (v[x]) delete v[x];
  clear();
}

template <class C, class A, int S>
inline Vec<C, A, S>::~Vec() {
  free();
}

template <class C, class A, int S>
inline int marshal_size(Vec<C, A, S> &v) {
  int l = sizeof(int) * 2;
  for (int x = 0; x < v.n; x++) l += ::marshal_size(v.v[x]);
  return l;
}

template <class C, class A, int S>
inline int marshal(Vec<C, A, S> &v, char *buf) {
  char *x = buf;
  *(int *)x = v.n;
  x += sizeof(int);
  *(int *)x = v.i;
  x += sizeof(int);
  for (int i = 0; i < v.n; i++) x += ::marshal(v.v[i], x);
  return x - buf;
}

template <class C, class A, int S>
inline int unmarshal(Vec<C, A, S> &v, char *buf) {
  char *x = buf;
  v.n = *(int *)x;
  x += sizeof(int);
  v.i = *(int *)x;
  x += sizeof(int);
  if (v.n) {
    v.v = (C *)A::alloc(sizeof(C) * v.n);
    memset((void*)v.v, 0, sizeof(C) * v.n);
  } else
    v.v = v.e;
  for (int i = 0; i < v.n; i++) x += ::unmarshal(v.v[i], x);
  return x - buf;
}

template <class C, class A, int S>
inline int Vec<C, A, S>::write(int fd) {
  int r = 0, t = 0;
  if ((r = ::write(fd, this, sizeof(*this))) < 0) return r;
  t += r;
  if ((r = ::write(fd, v, n * sizeof(C))) < 0) return r;
  t += r;
  return t;
}

template <class C, class A, int S>
inline int Vec<C, A, S>::read(int fd) {
  int r = 0, t = 0;
  if ((r = ::read(fd, this, sizeof(*this))) < 0) return r;
  t += r;
  v = (C *)A::alloc(sizeof(C) * n);
  memset((void*)v, 0, sizeof(C) * n);
  if ((r = ::read(fd, v, n * sizeof(C))) < 0) return r;
  t += r;
  return t;
}

template <class C>
inline void qsort_Vec(C *left, C *right, bool (*lt)(C, C)) {
Lagain:
  if (right - left < 5) {
    for (C *y = right - 1; y > left; y--) {
      for (C *x = left; x < y; x++) {
        if (lt(x[1], x[0])) {
          C t = x[0];
          x[0] = x[1];
          x[1] = t;
        }
      }
    }
  } else {
    C *i = left + 1, *j = right - 1;
    C x = *left;
    for (;;) {
      while (lt(x, *j)) j--;
      while (i < j && lt(*i, x)) i++;
      if (i >= j) break;
      C t = *i;
      *i = *j;
      *j = t;
      i++;
      j--;
    }
    if (j == right - 1) {
      *left = *(right - 1);
      *(right - 1) = x;
      right--;
      goto Lagain;
    }
    if (left < j) qsort_Vec<C>(left, j + 1, lt);
    if (j + 2 < right) qsort_Vec<C>(j + 1, right, lt);
  }
}

template <class C>
inline void qsort_VecRef(C *left, C *right, bool (*lt)(C &, C &)) {
Lagain:
  if (right - left < 5) {
    for (C *y = right - 1; y > left; y--) {
      for (C *x = left; x < y; x++) {
        if (lt(x[1], x[0])) {
          C t = x[0];
          x[0] = x[1];
          x[1] = t;
        }
      }
    }
  } else {
    C *i = left + 1, *j = right - 1;
    C x = *left;
    for (;;) {
      while (lt(x, *j)) j--;
      while (i < j && lt(*i, x)) i++;
      if (i >= j) break;
      C t = *i;
      *i = *j;
      *j = t;
      i++;
      j--;
    }
    if (j == right - 1) {
      *left = *(right - 1);
      *(right - 1) = x;
      right--;
      goto Lagain;
    }
    if (left < j) qsort_VecRef<C>(left, j + 1, lt);
    if (j + 2 < right) qsort_VecRef<C>(j + 1, right, lt);
  }
}

template <class C, class A, int S>
inline void Vec<C, A, S>::qsort(bool (*lt)(C, C)) {
  if (n) qsort_Vec<C>(&v[0], end(), lt);
}

void test_vec();

#endif
