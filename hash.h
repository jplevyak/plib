/* -*-Mode: c++;-*-
   Copyright (c) 2010 John Plevyak, All Rights Reserved
*/
#ifndef _hash_H
#define _hash_H

#include <emmintrin.h>

/*
 * These are noncryptographic hash and pseudo random number genreators from:
 * http://burtleburtle.net/bob
 */

struct rand64state_t;
struct hash128state_t;

// fast for all size keys
uint64 hash64(const void *key, size_t len);

void rand64init(rand64state_t *state, uint64 seed);
uint64 rand64(rand64state_t *state);

// fast for very large data
uint128 hash128(const void *data, size_t len);
// hash128(..).u64[0] for uint64, hash128(..).u32[0] for uint32
// incremental interface
void hash128init(hash128state_t *state);
void hash128update(hash128state_t *state, const void *data, size_t len);
uint128 hash128final(hash128state_t *state, const void *data = 0, size_t len = 0);

int32 jump_consistent_hash(uint64 key, int32 num_buckets);

#endif
