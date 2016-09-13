/* -*-Mode: c++;-*-
  Copyright (c) 2006-2008 John Plevyak, All Rights Reserved
*/
#include "plib.h"

#define PASSES 30

static uint32 modular_exponent(uint32 base, uint32 power, uint32 modulus) {
  uint64 result = 1;
  for (int i = 31; i >= 0; i--) {
    result = (result * result) % modulus;
    if (power & (1 << i)) result = (result * base) % modulus;
  }
  return (uint32)result;
}

// true: possibly prime
static bool miller_rabin_pass(uint32 a, uint32 n) {
  uint32 s = 0;
  uint32 d = n - 1;
  while ((d % 2) == 0) {
    d /= 2;
    s++;
  }
  uint32 a_to_power = modular_exponent(a, d, n);
  if (a_to_power == 1) return true;
  for (uint32 i = 0; i < s - 1; i++) {
    if (a_to_power == n - 1) return true;
    a_to_power = modular_exponent(a_to_power, 2, n);
  }
  if (a_to_power == n - 1) return true;
  return false;
}

bool miller_rabin(uint32 n) {
  uint32 a = 0;
  for (int t = 0; t < PASSES; t++) {
    do {
      a = RND64() % n;
    } while (!a);
    if (!miller_rabin_pass(a, n)) return false;
  }
  return true;
}

uint32 next_higher_prime(uint32 n) {
  if (!(n & 1)) n++;
  while (1) {
    if (miller_rabin(n)) return n;
    n += 2;
  }
}

uint32 next_lower_prime(uint32 n) {
  if (!(n & 1)) n--;
  while (1) {
    if (miller_rabin(n)) return n;
    n -= 2;
  }
}
