/*
   A C-program for MT19937-64 (2004/9/29 version).
   Coded by Takuji Nishimura and Makoto Matsumoto.

   This is a 64-bit version of Mersenne Twister pseudorandom number
   generator.

   Before using, initialize the state by using init_genrand64(seed)
   or init_by_array64(init_key, key_length).

   Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   3. The names of its contributors may not be used to endorse or promote
   products derived from this software without specific prior written
   permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   References:
   T. Nishimura, ``Tables of 64-bit Mersenne Twisters''
   ACM Transactions on Modeling and
   Computer Simulation 10. (2000) 348--357.
   M. Matsumoto and T. Nishimura,
   ``Mersenne Twister: a 623-dimensionally equidistributed
   uniform pseudorandom number generator''
   ACM Transactions on Modeling and
   Computer Simulation 8. (Jan. 1998) 3--30.

   Any feedback is very welcome.
   http://www.math.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove spaces)
*/

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include "mt64.h"

#define MM 156
#define MATRIX_A 0xB5026F5AA96619E9ULL
#define UM 0xFFFFFFFF80000000ULL /* Most significant 33 bits */
#define LM 0x7FFFFFFFULL         /* Least significant 31 bits */

static Rand64State rand64state;

/* initializes mt[RAND64NN] with a seed */
void init_genrand64(Rand64State *state, uint64_t seed) {
  pthread_mutex_init(&state->mutex, 0);
  pthread_mutex_lock(&state->mutex);
  int mti = state->mti = RAND64NN + 1;
  uint64_t *mt = state->mt;
  mt[0] = seed;
  for (mti = 1; mti < RAND64NN; mti++) mt[mti] = (6364136223846793005ULL * (mt[mti - 1] ^ (mt[mti - 1] >> 62)) + mti);
  state->mti = mti;
  pthread_mutex_unlock(&state->mutex);
}
void init_genrand64(uint64_t seed) { init_genrand64(&rand64state, seed); }

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
void init_by_array64(Rand64State *state, uint64_t init_key[], uint64_t key_length) {
  init_genrand64(state, 19650218ULL);
  pthread_mutex_lock(&state->mutex);
  uint64_t i, j, k;
  uint64_t *mt = state->mt;
  i = 1;
  j = 0;
  k = (RAND64NN > key_length ? RAND64NN : key_length);
  for (; k; k--) {
    mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 62)) * 3935559000370003845ULL)) + init_key[j] + j; /* non linear */
    i++;
    j++;
    if (i >= RAND64NN) {
      mt[0] = mt[RAND64NN - 1];
      i = 1;
    }
    if (j >= key_length) j = 0;
  }
  for (k = RAND64NN - 1; k; k--) {
    mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 62)) * 2862933555777941757ULL)) - i; /* non linear */
    i++;
    if (i >= RAND64NN) {
      mt[0] = mt[RAND64NN - 1];
      i = 1;
    }
  }

  mt[0] = 1ULL << 63; /* MSB is 1; assuring non-zero initial array */
  pthread_mutex_unlock(&state->mutex);
}
void init_by_array64(uint64_t init_key[], uint64_t key_length) { init_by_array64(&rand64state, init_key, key_length); }

/* generates a random number on [0, 2^64-1]-interval */
uint64_t genrand64_int64(Rand64State *state) {
  int i;
  uint64_t x;
  static uint64_t mag01[2] = {0ULL, MATRIX_A};

  pthread_mutex_lock(&state->mutex);
  uint64_t *mt = state->mt;
  if (state->mti >= RAND64NN) { /* generate RAND64NN words at one time */

    /* if init_genrand64() has not been called, */
    /* a default initial seed is used     */
    if (state->mti == RAND64NN + 1) init_genrand64(5489ULL);

    for (i = 0; i < RAND64NN - MM; i++) {
      x = (mt[i] & UM) | (mt[i + 1] & LM);
      mt[i] = mt[i + MM] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];
    }
    for (; i < RAND64NN - 1; i++) {
      x = (mt[i] & UM) | (mt[i + 1] & LM);
      mt[i] = mt[i + (MM - RAND64NN)] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];
    }
    x = (mt[RAND64NN - 1] & UM) | (mt[0] & LM);
    mt[RAND64NN - 1] = mt[MM - 1] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];

    state->mti = 0;
  }

  x = mt[state->mti++];
  pthread_mutex_unlock(&state->mutex);

  x ^= (x >> 29) & 0x5555555555555555ULL;
  x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
  x ^= (x << 37) & 0xFFF7EEE000000000ULL;
  x ^= (x >> 43);

  return x;
}
uint64_t genrand64_int64(void) { return genrand64_int64(&rand64state); }

/* generates a random number on [0, 2^63-1]-interval */
int64_t genrand64_int63(Rand64State *state) { return (int64_t)(genrand64_int64(state) >> 1); }
int64_t genrand64_int63(void) { return genrand64_int64(&rand64state); }

/* generates a random number on [0,1]-real-interval */
double genrand64_real1(Rand64State *state) { return (genrand64_int64(state) >> 11) * (1.0 / 9007199254740991.0); }
double genrand64_real1(void) { return genrand64_real1(&rand64state); }

/* generates a random number on [0,1)-real-interval */
double genrand64_real2(Rand64State *state) { return (genrand64_int64(state) >> 11) * (1.0 / 9007199254740992.0); }
double genrand64_real2(void) { return genrand64_real2(&rand64state); }

/* generates a random number on (0,1)-real-interval */
double genrand64_real3(Rand64State *state) {
  return ((genrand64_int64(state) >> 12) + 0.5) * (1.0 / 4503599627370496.0);
}
double genrand64_real3(void) { return genrand64_real3(&rand64state); }
