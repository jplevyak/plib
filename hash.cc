/* -*-Mode: c++; -*-
   lookup3.c, by Bob Jenkins, May 2006, Public Domain 
   Changes Copyright (c) 2010 John Plevyak, All Rights Reserved
*/
#include "plib.h"

#include <sys/param.h>  /* attempt to define endianness */
#ifdef linux
# include <endian.h>    /* attempt to define endianness */
#endif

/*
 *  * My best guess at if you are big-endian or little-endian.  This may
 *   * need adjustment.
 *    */
#if (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
     __BYTE_ORDER == __LITTLE_ENDIAN) || \
    (defined(i386) || defined(__i386__) || defined(__i486__) || \
     defined(__i586__) || defined(__i686__) || defined(vax) || defined(MIPSEL))
# define HASH_LITTLE_ENDIAN 1
# define HASH_BIG_ENDIAN 0
#elif (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && \
       __BYTE_ORDER == __BIG_ENDIAN) || \
      (defined(sparc) || defined(POWERPC) || defined(mc68000) || defined(sel))
# define HASH_LITTLE_ENDIAN 0
# define HASH_BIG_ENDIAN 1
#else
# define HASH_LITTLE_ENDIAN 0
# define HASH_BIG_ENDIAN 0
#endif

#define hashsize(n) ((uint32_t)1<<(n))
#define hashmask(n) (hashsize(n)-1)
#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

/*
 * -------------------------------------------------------------------------------
 *  mix -- mix 3 32-bit values reversibly.
 *
 *  This is reversible, so any information in (a,b,c) before mix() is
 *  still in (a,b,c) after mix().
 *
 *  If four pairs of (a,b,c) inputs are run through mix(), or through
 *  mix() in reverse, there are at least 32 bits of the output that
 *  are sometimes the same for one pair and different for another pair.
 *  This was tested for:
 *  * pairs that differed by one bit, by two bits, in any combination
 *    of top bits of (a,b,c), or in any combination of bottom bits of
 *      (a,b,c).
 *      * "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
 *        the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
 *          is commonly produced by subtraction) look like a single 1-bit
 *            difference.
 *            * the base values were pseudorandom, all zero but one bit set, or 
 *              all zero plus a counter that starts at zero.
 *
 *              Some k values for my "a-=c; a^=rot(c,k); c+=b;" arrangement that
 *              satisfy this are
 *                  4  6  8 16 19  4
 *                      9 15  3 18 27 15
 *                         14  9  3  7 17  3
 *                         Well, "9 15 3 18 27 15" didn't quite get 32 bits diffing
 *                         for "differ" defined as + with a one-bit base and a two-bit delta.  I
 *                         used http://burtleburtle.net/bob/hash/avalanche.html to choose 
 *                         the operations, constants, and arrangements of the variables.
 *
 *                         This does not achieve avalanche.  There are input bits of (a,b,c)
 *                         that fail to affect some output bits of (a,b,c), especially of a.  The
 *                         most thoroughly mixed value is c, but it doesn't really even achieve
 *                         avalanche in c.
 *
 *                         This allows some parallelism.  Read-after-writes are good at doubling
 *                         the number of bits affected, so the goal of mixing pulls in the opposite
 *                         direction as the goal of parallelism.  I did what I could.  Rotates
 *                         seem to cost as much as shifts on every machine I could lay my hands
 *                         on, and rotates are much kinder to the top and bottom bits, so I used
 *                         rotates.
 *                         -------------------------------------------------------------------------------
 *                         */
#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

/*
 * -------------------------------------------------------------------------------
 *  final -- final mixing of 3 32-bit values (a,b,c) into c
 *
 *  Pairs of (a,b,c) values differing in only a few bits will usually
 *  produce values of c that look totally different.  This was tested for
 *  * pairs that differed by one bit, by two bits, in any combination
 *    of top bits of (a,b,c), or in any combination of bottom bits of
 *      (a,b,c).
 *      * "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
 *        the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
 *          is commonly produced by subtraction) look like a single 1-bit
 *            difference.
 *            * the base values were pseudorandom, all zero but one bit set, or 
 *              all zero plus a counter that starts at zero.
 *
 *              These constants passed:
 *               14 11 25 16 4 14 24
 *                12 14 25 16 4 14 24
 *                and these came close:
 *                  4  8 15 26 3 22 24
 *                   10  8 15 26 3 22 24
 *                    11  8 15 26 3 22 24
 *                    -------------------------------------------------------------------------------
 *                    */
#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

/*
 * --------------------------------------------------------------------
 *   This works on all machines.  To be useful, it requires
 *    -- that the key be an array of uint32_t's, and
 *     -- that the length be the number of uint32_t's in the key
 *
 *      The function hashword() is identical to hashlittle() on little-endian
 *       machines, and identical to hashbig() on big-endian machines,
 *        except that the length has to be measured in uint32_ts rather than in
 *         bytes.  hashlittle() is more complicated than hashword() only because
 *          hashlittle() has to dance around fitting the key bytes into registers.
 *          --------------------------------------------------------------------
 *          */
uint32_t hashword(
const uint32_t *k,                   /* the key, an array of uint32_t values */
size_t          length,               /* the length of the key, in uint32_ts */
uint32_t        initval)         /* the previous hash, or an arbitrary value */
{
  uint32_t a,b,c;

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + (((uint32_t)length)<<2) + initval;

  /*------------------------------------------------- handle most of the key */
  while (length > 3)
  {
    a += k[0];
    b += k[1];
    c += k[2];
    mix(a,b,c);
    length -= 3;
    k += 3;
  }

  /*------------------------------------------- handle the last 3 uint32_t's */
  switch(length)                     /* all the case statements fall through */
  { 
  case 3 : c+=k[2];
  case 2 : b+=k[1];
  case 1 : a+=k[0];
    final(a,b,c);
  case 0:     /* case 0: nothing left to add */
    break;
  }
  /*------------------------------------------------------ report the result */
  return c;
}


/*
 * --------------------------------------------------------------------
 *  hashword2() -- same as hashword(), but take two seeds and return two
 *  32-bit values.  pc and pb must both be nonnull, and *pc and *pb must
 *  both be initialized with seeds.  If you pass in (*pb)==0, the output 
 *  (*pc) will be the same as the return value from hashword().
 *  --------------------------------------------------------------------
 *  */
void hashword2 (
const uint32_t *k,                   /* the key, an array of uint32_t values */
size_t          length,               /* the length of the key, in uint32_ts */
uint32_t       *pc,                      /* IN: seed OUT: primary hash value */
uint32_t       *pb)               /* IN: more seed OUT: secondary hash value */
{
  uint32_t a,b,c;

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)(length<<2)) + *pc;
  c += *pb;

  /*------------------------------------------------- handle most of the key */
  while (length > 3)
  {
    a += k[0];
    b += k[1];
    c += k[2];
    mix(a,b,c);
    length -= 3;
    k += 3;
  }

  /*------------------------------------------- handle the last 3 uint32_t's */
  switch(length)                     /* all the case statements fall through */
  { 
  case 3 : c+=k[2];
  case 2 : b+=k[1];
  case 1 : a+=k[0];
    final(a,b,c);
  case 0:     /* case 0: nothing left to add */
    break;
  }
  /*------------------------------------------------------ report the result */
  *pc=c; *pb=b;
}


/*
 * -------------------------------------------------------------------------------
 *  hashlittle() -- hash a variable-length key into a 32-bit value
 *    k       : the key (the unaligned variable-length array of bytes)
 *      length  : the length of the key, counting by bytes
 *        initval : can be any 4-byte value
 *        Returns a 32-bit value.  Every bit of the key affects every bit of
 *        the return value.  Two keys differing by one or two bits will have
 *        totally different hash values.
 *
 *        The best hash table sizes are powers of 2.  There is no need to do
 *        mod a prime (mod is sooo slow!).  If you need less than 32 bits,
 *        use a bitmask.  For example, if you need only 10 bits, do
 *          h = (h & hashmask(10));
 *          In which case, the hash table should have hashsize(10) elements.
 *
 *          If you are hashing n strings (uint8_t **)k, do it like this:
 *            for (i=0, h=0; i<n; ++i) h = hashlittle( k[i], len[i], h);
 *
 *            By Bob Jenkins, 2006.  bob_jenkins@burtleburtle.net.  You may use this
 *            code any way you wish, private, educational, or commercial.  It's free.
 *
 *            Use for hash table lookup, or anything where one collision in 2^^32 is
 *            acceptable.  Do NOT use for cryptographic purposes.
 *            -------------------------------------------------------------------------------
 *            */

uint32_t hashlittle( const void *key, size_t length, uint32_t initval)
{
  uint32_t a,b,c;                                          /* internal state */
  union { const void *ptr; size_t i; } u;     /* needed for Mac Powerbook G4 */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + initval;

  u.ptr = key;
  if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0)) {
    const uint32_t *k = (const uint32_t *)key;         /* read 32-bit chunks */

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a,b,c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    /* 
 *      * "k[2]&0xffffff" actually reads beyond the end of the string, but
 *           * then masks off the part it's not allowed to read.  Because the
 *                * string is aligned, the masked-off tail is in the same word as the
 *                     * rest of the string.  Every machine with memory protection I've seen
 *                          * does it on word boundaries, so is OK with this.  But VALGRIND will
 *                               * still catch it and complain.  The masking trick does make the hash
 *                                    * noticably faster for short strings (like English words).
 *                                         */
#ifndef VALGRIND

    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff; a+=k[0]; break;
    case 5 : b+=k[1]&0xff; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff; break;
    case 2 : a+=k[0]&0xffff; break;
    case 1 : a+=k[0]&0xff; break;
    case 0 : return c;              /* zero length strings require no mixing */
    }

#else /* make valgrind happy */

    const uint8_t  *k8;
    k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=((uint32_t)k8[10])<<16;  /* fall through */
    case 10: c+=((uint32_t)k8[9])<<8;    /* fall through */
    case 9 : c+=k8[8];                   /* fall through */
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=((uint32_t)k8[6])<<16;   /* fall through */
    case 6 : b+=((uint32_t)k8[5])<<8;    /* fall through */
    case 5 : b+=k8[4];                   /* fall through */
    case 4 : a+=k[0]; break;
    case 3 : a+=((uint32_t)k8[2])<<16;   /* fall through */
    case 2 : a+=((uint32_t)k8[1])<<8;    /* fall through */
    case 1 : a+=k8[0]; break;
    case 0 : return c;
    }

#endif /* !valgrind */

  } else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0)) {
    const uint16_t *k = (const uint16_t *)key;         /* read 16-bit chunks */
    const uint8_t  *k8;

    /*--------------- all but last block: aligned reads and different mixing */
    while (length > 12)
    {
      a += k[0] + (((uint32_t)k[1])<<16);
      b += k[2] + (((uint32_t)k[3])<<16);
      c += k[4] + (((uint32_t)k[5])<<16);
      mix(a,b,c);
      length -= 12;
      k += 6;
    }

    /*----------------------------- handle the last (probably partial) block */
    k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[4]+(((uint32_t)k[5])<<16);
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 11: c+=((uint32_t)k8[10])<<16;     /* fall through */
    case 10: c+=k[4];
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 9 : c+=k8[8];                      /* fall through */
    case 8 : b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 7 : b+=((uint32_t)k8[6])<<16;      /* fall through */
    case 6 : b+=k[2];
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 5 : b+=k8[4];                      /* fall through */
    case 4 : a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 3 : a+=((uint32_t)k8[2])<<16;      /* fall through */
    case 2 : a+=k[0];
             break;
    case 1 : a+=k8[0];
             break;
    case 0 : return c;                     /* zero length requires no mixing */
    }

  } else {                        /* need to read the key one byte at a time */
    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      a += ((uint32_t)k[1])<<8;
      a += ((uint32_t)k[2])<<16;
      a += ((uint32_t)k[3])<<24;
      b += k[4];
      b += ((uint32_t)k[5])<<8;
      b += ((uint32_t)k[6])<<16;
      b += ((uint32_t)k[7])<<24;
      c += k[8];
      c += ((uint32_t)k[9])<<8;
      c += ((uint32_t)k[10])<<16;
      c += ((uint32_t)k[11])<<24;
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=((uint32_t)k[11])<<24;
    case 11: c+=((uint32_t)k[10])<<16;
    case 10: c+=((uint32_t)k[9])<<8;
    case 9 : c+=k[8];
    case 8 : b+=((uint32_t)k[7])<<24;
    case 7 : b+=((uint32_t)k[6])<<16;
    case 6 : b+=((uint32_t)k[5])<<8;
    case 5 : b+=k[4];
    case 4 : a+=((uint32_t)k[3])<<24;
    case 3 : a+=((uint32_t)k[2])<<16;
    case 2 : a+=((uint32_t)k[1])<<8;
    case 1 : a+=k[0];
             break;
    case 0 : return c;
    }
  }

  final(a,b,c);
  return c;
}


/*
 *  * hashlittle2: return 2 32-bit hash values
 *   *
 *    * This is identical to hashlittle(), except it returns two 32-bit hash
 *     * values instead of just one.  This is good enough for hash table
 *      * lookup with 2^^64 buckets, or if you want a second hash if you're not
 *       * happy with the first, or if you want a probably-unique 64-bit ID for
 *        * the key.  *pc is better mixed than *pb, so use *pc first.  If you want
 *         * a 64-bit value do something like "*pc + (((uint64_t)*pb)<<32)".
 *          */
void hashlittle2( 
  const void *key,       /* the key to hash */
  size_t      length,    /* length of the key */
  uint32_t   *pc,        /* IN: primary initval, OUT: primary hash */
  uint32_t   *pb)        /* IN: secondary initval, OUT: secondary hash */
{
  uint32_t a,b,c;                                          /* internal state */
  union { const void *ptr; size_t i; } u;     /* needed for Mac Powerbook G4 */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + *pc;
  c += *pb;

  u.ptr = key;
  if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0)) {
    const uint32_t *k = (const uint32_t *)key;         /* read 32-bit chunks */

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a,b,c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    /* 
 *      * "k[2]&0xffffff" actually reads beyond the end of the string, but
 *           * then masks off the part it's not allowed to read.  Because the
 *                * string is aligned, the masked-off tail is in the same word as the
 *                     * rest of the string.  Every machine with memory protection I've seen
 *                          * does it on word boundaries, so is OK with this.  But VALGRIND will
 *                               * still catch it and complain.  The masking trick does make the hash
 *                                    * noticably faster for short strings (like English words).
 *                                         */
#ifndef VALGRIND

    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff; a+=k[0]; break;
    case 5 : b+=k[1]&0xff; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff; break;
    case 2 : a+=k[0]&0xffff; break;
    case 1 : a+=k[0]&0xff; break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

#else /* make valgrind happy */

    const uint8_t  *k8;
    k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=((uint32_t)k8[10])<<16;  /* fall through */
    case 10: c+=((uint32_t)k8[9])<<8;    /* fall through */
    case 9 : c+=k8[8];                   /* fall through */
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=((uint32_t)k8[6])<<16;   /* fall through */
    case 6 : b+=((uint32_t)k8[5])<<8;    /* fall through */
    case 5 : b+=k8[4];                   /* fall through */
    case 4 : a+=k[0]; break;
    case 3 : a+=((uint32_t)k8[2])<<16;   /* fall through */
    case 2 : a+=((uint32_t)k8[1])<<8;    /* fall through */
    case 1 : a+=k8[0]; break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

#endif /* !valgrind */

  } else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0)) {
    const uint16_t *k = (const uint16_t *)key;         /* read 16-bit chunks */
    const uint8_t  *k8;

    /*--------------- all but last block: aligned reads and different mixing */
    while (length > 12)
    {
      a += k[0] + (((uint32_t)k[1])<<16);
      b += k[2] + (((uint32_t)k[3])<<16);
      c += k[4] + (((uint32_t)k[5])<<16);
      mix(a,b,c);
      length -= 12;
      k += 6;
    }

    /*----------------------------- handle the last (probably partial) block */
    k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[4]+(((uint32_t)k[5])<<16);
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 11: c+=((uint32_t)k8[10])<<16;     /* fall through */
    case 10: c+=k[4];
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 9 : c+=k8[8];                      /* fall through */
    case 8 : b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 7 : b+=((uint32_t)k8[6])<<16;      /* fall through */
    case 6 : b+=k[2];
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 5 : b+=k8[4];                      /* fall through */
    case 4 : a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 3 : a+=((uint32_t)k8[2])<<16;      /* fall through */
    case 2 : a+=k[0];
             break;
    case 1 : a+=k8[0];
             break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

  } else {                        /* need to read the key one byte at a time */
    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      a += ((uint32_t)k[1])<<8;
      a += ((uint32_t)k[2])<<16;
      a += ((uint32_t)k[3])<<24;
      b += k[4];
      b += ((uint32_t)k[5])<<8;
      b += ((uint32_t)k[6])<<16;
      b += ((uint32_t)k[7])<<24;
      c += k[8];
      c += ((uint32_t)k[9])<<8;
      c += ((uint32_t)k[10])<<16;
      c += ((uint32_t)k[11])<<24;
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=((uint32_t)k[11])<<24;
    case 11: c+=((uint32_t)k[10])<<16;
    case 10: c+=((uint32_t)k[9])<<8;
    case 9 : c+=k[8];
    case 8 : b+=((uint32_t)k[7])<<24;
    case 7 : b+=((uint32_t)k[6])<<16;
    case 6 : b+=((uint32_t)k[5])<<8;
    case 5 : b+=k[4];
    case 4 : a+=((uint32_t)k[3])<<24;
    case 3 : a+=((uint32_t)k[2])<<16;
    case 2 : a+=((uint32_t)k[1])<<8;
    case 1 : a+=k[0];
             break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }
  }

  final(a,b,c);
  *pc=c; *pb=b;
}



/*
 *  * hashbig():
 *   * This is the same as hashword() on big-endian machines.  It is different
 *    * from hashlittle() on all machines.  hashbig() takes advantage of
 *     * big-endian byte ordering. 
 *      */
uint32_t hashbig( const void *key, size_t length, uint32_t initval)
{
  uint32_t a,b,c;
  union { const void *ptr; size_t i; } u; /* to cast key to (size_t) happily */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + initval;

  u.ptr = key;
  if (HASH_BIG_ENDIAN && ((u.i & 0x3) == 0)) {
    const uint32_t *k = (const uint32_t *)key;         /* read 32-bit chunks */

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a,b,c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    /* 
 *      * "k[2]<<8" actually reads beyond the end of the string, but
 *           * then shifts out the part it's not allowed to read.  Because the
 *                * string is aligned, the illegal read is in the same word as the
 *                     * rest of the string.  Every machine with memory protection I've seen
 *                          * does it on word boundaries, so is OK with this.  But VALGRIND will
 *                               * still catch it and complain.  The masking trick does make the hash
 *                                    * noticably faster for short strings (like English words).
 *                                         */
#ifndef VALGRIND

    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff00; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff0000; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff000000; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff00; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff0000; a+=k[0]; break;
    case 5 : b+=k[1]&0xff000000; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff00; break;
    case 2 : a+=k[0]&0xffff0000; break;
    case 1 : a+=k[0]&0xff000000; break;
    case 0 : return c;              /* zero length strings require no mixing */
    }

#else  /* make valgrind happy */

    const uint8_t  *k8;
    k8 = (const uint8_t *)k;
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=((uint32_t)k8[10])<<8;  /* fall through */
    case 10: c+=((uint32_t)k8[9])<<16;  /* fall through */
    case 9 : c+=((uint32_t)k8[8])<<24;  /* fall through */
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=((uint32_t)k8[6])<<8;   /* fall through */
    case 6 : b+=((uint32_t)k8[5])<<16;  /* fall through */
    case 5 : b+=((uint32_t)k8[4])<<24;  /* fall through */
    case 4 : a+=k[0]; break;
    case 3 : a+=((uint32_t)k8[2])<<8;   /* fall through */
    case 2 : a+=((uint32_t)k8[1])<<16;  /* fall through */
    case 1 : a+=((uint32_t)k8[0])<<24; break;
    case 0 : return c;
    }

#endif /* !VALGRIND */

  } else {                        /* need to read the key one byte at a time */
    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += ((uint32_t)k[0])<<24;
      a += ((uint32_t)k[1])<<16;
      a += ((uint32_t)k[2])<<8;
      a += ((uint32_t)k[3]);
      b += ((uint32_t)k[4])<<24;
      b += ((uint32_t)k[5])<<16;
      b += ((uint32_t)k[6])<<8;
      b += ((uint32_t)k[7]);
      c += ((uint32_t)k[8])<<24;
      c += ((uint32_t)k[9])<<16;
      c += ((uint32_t)k[10])<<8;
      c += ((uint32_t)k[11]);
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=k[11];
    case 11: c+=((uint32_t)k[10])<<8;
    case 10: c+=((uint32_t)k[9])<<16;
    case 9 : c+=((uint32_t)k[8])<<24;
    case 8 : b+=k[7];
    case 7 : b+=((uint32_t)k[6])<<8;
    case 6 : b+=((uint32_t)k[5])<<16;
    case 5 : b+=((uint32_t)k[4])<<24;
    case 4 : a+=k[3];
    case 3 : a+=((uint32_t)k[2])<<8;
    case 2 : a+=((uint32_t)k[1])<<16;
    case 1 : a+=((uint32_t)k[0])<<24;
             break;
    case 0 : return c;
    }
  }

  final(a,b,c);
  return c;
}

uint64 hash64(const void *key, size_t s) {
  uint32 x, y;
  uint64 r;
#if HASH_LITTLE_ENDIAN
  hashlittle2(key, s, &x, &y);
  r = x;
  r = (r << 32) + y;
  return r;
#else
#error "unimplemented"
#endif
}

#define rot64(x,k) (((x)<<(k))|((x)>>(64-(k))))
uint64 rand64(rand64state_t *x) {
    uint64 e = x->a - rot64(x->b, 7);
    x->a = x->b ^ rot64(x->c, 13);
    x->b = x->c + rot64(x->d, 37);
    x->c = x->d + e;
    x->d = e + x->a;
    return x->d;
}

void rand64init(rand64state_t *x, uint64 seed) {
    uint64 i;
    x->a = 0xf1ea5eed, x->b = x->c = x->d = seed;
    for (i=0; i<20; ++i) {
        (void)rand64(x);
    }
}

typedef uint64 u8;
typedef uint32 u4;
typedef uint16 u2;
typedef uint8 u1;
typedef hash128state_t zorba;
typedef uint128 z128;

#define BYTES_PER_VAL  16
#define BLOCK          48
#define BUFFERED       (BLOCK * BYTES_PER_VAL)

#define LARGE 766

#define xxor(a,b) _mm_xor_si128(a, b)
#define add(a,b) _mm_add_epi64(a, b)
#define shuf(a,b) _mm_shuffle_epi32(a, b)
#define lshift(a,b) _mm_slli_epi64(a, b)
#define rshift(a,b) _mm_srli_epi64(a, b)
#define read(x) _mm_load_si128(x)

/* churn a state */
#define CHURN(s,first,second,third) \
{ \
  second=xxor(first,second);             \
  s = xxor(shuf(s,0x39),xxor(rshift(s,5),first)); \
  s = xxor(add(lshift(s,8),s),third);               \
}

/* churn s, with second and third use of a block */
#define TAIL1(s,third) \
{ \
  s = xxor(shuf(s,0x39), rshift(s,5)); \
  s = add(s, xxor(lshift(s,8), third)); \
}

/* strength: I had to test this without the first or last line */
#define FINAL1(s) \
{ \
  s = xxor(shuf(s,0x1b), rshift(s,1)); \
  s = add(s, lshift(s,17)); \
  s = xxor(shuf(s,0x1b), rshift(s,13)); \
  s = add(s, lshift(s,8)); \
  s = xxor(shuf(s,0x1b), rshift(s,2)); \
  s = add(s, lshift(s,28)); \
  s = xxor(shuf(s,0x1b), rshift(s,16)); \
  s = add(s, lshift(s,4)); \
  s = xxor(shuf(s,0x1b), rshift(s,6)); \
  s = add(s, lshift(s,9)); \
}

#define FINAL4(s) \
{ \
  s1 = xxor(shuf(s1,0x1b), s2); \
  s3 = xxor(shuf(s3,0x1b), s0); \
  s2 = add(shuf(s2,0x39), lshift(s1,6)); \
  s0 = add(shuf(s0,0x39), lshift(s3,6)); \
  s1 = xxor(shuf(s1,0x1b), s0); \
  s2 = xxor(shuf(s2,0x1b), s3); \
  s3 = add(shuf(s3,0x39), lshift(s2,19)); \
  s0 = add(shuf(s0,0x39), lshift(s1,19)); \
  s1 = xxor(shuf(s1,0x1b), s3); \
  s2 = xxor(shuf(s2,0x1b), s0); \
  s3 = add(shuf(s3,0x39), lshift(s1,9)); \
  s0 = add(shuf(s0,0x39), lshift(s2,9)); \
  s1 = xxor(shuf(s1,0x1b), s2); \
  s3 = xxor(shuf(s3,0x1b), s0); \
  s0 = add(shuf(s0,0x39), lshift(s3,5)); \
  s1 = xxor(shuf(s1,0x1b), s0); \
  s0 = add(shuf(s0,0x39), s1); \
}

/* churn four independent states */
#define CHURN4(data,i,q,s,x,y,z) \
{ \
    x##0 = read(&data[(i)+(q)  ]); \
    x##1 = read(&data[(i)+(q)+1]); \
    x##2 = read(&data[(i)+(q)+2]); \
    x##3 = read(&data[(i)+(q)+3]); \
    CHURN(s##0,x##0,y##0,z##0); \
    CHURN(s##1,x##1,y##1,z##1); \
    CHURN(s##2,x##2,y##2,z##2); \
    CHURN(s##3,x##3,y##3,z##3); \
 }

/* churn one independent state */
#define CHURN1(data,i,q,s,x,y,z) \
{ \
    x = read(&data[(i)+(q)]); \
    CHURN(s,x,y,z); \
}

#define TAIL4(s,third) { \
    TAIL1(s##0,third##0); \
    TAIL1(s##1,third##1); \
    TAIL1(s##2,third##2); \
    TAIL1(s##3,third##3); \
}

#define TO_REG(a,m,i) { \
    a##0 = m[i  ].h; \
    a##1 = m[i+1].h; \
    a##2 = m[i+2].h; \
    a##3 = m[i+3].h; \
}

#define FROM_REG(a,m,i) { \
    m[i  ].h = a##0; \
    m[i+1].h = a##1; \
    m[i+2].h = a##2; \
    m[i+3].h = a##3; \
}

#define REG_REG(a,b) { \
  a##0 = b##0; \
  a##1 = b##1; \
  a##2 = b##2; \
  a##3 = b##3; \
}


/* initialize a zorba state */
void hash128init(zorba *z)
{
  memset(z->s, 0x55, sizeof(z->s));
  memset(z->accum, 0, sizeof(z->accum));
  z->datalen = 0;
  z->messagelen = (u8)0;
}


/* hash a piece of a message */
void hash128update(zorba *z, const void *data, size_t len)
{
  size_t counter;
  int oldlen = z->datalen;
  size_t total = oldlen + len;
  __m128i s0,a0,b0,c0,d0,e0,f0,g0,h0,i0,j0,k0,l0;
  __m128i s1,a1,b1,c1,d1,e1,f1,g1,h1,i1,j1,k1,l1;
  __m128i s2,a2,b2,c2,d2,e2,f2,g2,h2,i2,j2,k2,l2;
  __m128i s3,a3,b3,c3,d3,e3,f3,g3,h3,i3,j3,k3,l3;

  /* exit early if we don't have a complete block */
  z->messagelen += len;
  if (total < BUFFERED) {
    memcpy(((char *)z->data)+oldlen, data, len);
    z->datalen = total;
    return;
  }

  /* load the registers */
  TO_REG(s,z->s,0);
  TO_REG(l,z->accum,0);
  TO_REG(k,z->accum,4);
  TO_REG(j,z->accum,8);
  TO_REG(i,z->accum,12);
  TO_REG(h,z->accum,16);
  TO_REG(g,z->accum,20);
  TO_REG(f,z->accum,24);
  TO_REG(e,z->accum,28);
  TO_REG(d,z->accum,32);
  TO_REG(c,z->accum,36);

  /* use the first cached block, if any */
  if (oldlen) {
    __m128i *cache = (__m128i *)z->data;
    int piece = BUFFERED-oldlen;
    memcpy(((char *)cache)+oldlen, data, piece);
    CHURN4(cache,0, 0,s,b,h,l);
    CHURN4(cache,0, 4,s,a,c,k);
    CHURN4(cache,0, 8,s,l,f,j);
    CHURN4(cache,0,12,s,k,a,i);
    CHURN4(cache,0,16,s,j,d,h);
    CHURN4(cache,0,20,s,i,k,g);
    CHURN4(cache,0,24,s,h,b,f);
    CHURN4(cache,0,28,s,g,i,e);
    CHURN4(cache,0,32,s,f,l,d);
    CHURN4(cache,0,36,s,e,g,c);
    CHURN4(cache,0,40,s,d,j,b);
    CHURN4(cache,0,44,s,c,e,a);
    len -= piece;
  }

  /* use any other complete blocks */
  if ((((size_t)data) & 15) == 0) {
    const __m128i *aligned_data = (const __m128i *)data;
    size_t len2 = len/16;
    for (counter=BLOCK; counter<=len2; counter+=BLOCK) {
      CHURN4(aligned_data,counter,-48,s,b,h,l);
      CHURN4(aligned_data,counter,-44,s,a,i,k);
      CHURN4(aligned_data,counter,-40,s,l,f,j);
      CHURN4(aligned_data,counter,-36,s,k,g,i);
      CHURN4(aligned_data,counter,-32,s,j,d,h);
      CHURN4(aligned_data,counter,-28,s,i,e,g);
      CHURN4(aligned_data,counter,-24,s,h,b,f);
      CHURN4(aligned_data,counter,-20,s,g,c,e);
      CHURN4(aligned_data,counter,-16,s,f,l,d);
      CHURN4(aligned_data,counter,-12,s,e,a,c);
      CHURN4(aligned_data,counter, -8,s,d,j,b);
      CHURN4(aligned_data,counter, -4,s,c,k,a);
    }
    counter *= 16;
  } else {
    for (counter=BUFFERED; counter<=len; counter+=BUFFERED) {
      __m128i *cache = (__m128i *)z->data;
      memcpy(cache, ((const char *)data)+(counter-BUFFERED), BUFFERED);
      CHURN4(cache,0, 0,s,b,h,l);
      CHURN4(cache,0, 4,s,a,i,k);
      CHURN4(cache,0, 8,s,l,f,j);
      CHURN4(cache,0,12,s,k,g,i);
      CHURN4(cache,0,16,s,j,d,h);
      CHURN4(cache,0,20,s,i,e,g);
      CHURN4(cache,0,24,s,h,b,f);
      CHURN4(cache,0,28,s,g,c,e);
      CHURN4(cache,0,32,s,f,l,d);
      CHURN4(cache,0,36,s,e,a,c);
      CHURN4(cache,0,40,s,d,j,b);
      CHURN4(cache,0,44,s,c,k,a);
    }
  }

  /* store the registers */  
  FROM_REG(s,z->s,0);
  FROM_REG(l,z->accum,0);
  FROM_REG(k,z->accum,4);
  FROM_REG(j,z->accum,8);
  FROM_REG(i,z->accum,12);
  FROM_REG(h,z->accum,16);
  FROM_REG(g,z->accum,20);
  FROM_REG(f,z->accum,24);
  FROM_REG(e,z->accum,28);
  FROM_REG(d,z->accum,32);
  FROM_REG(c,z->accum,36);

  /* cache the last partial block, if any */
  len = BUFFERED + len - counter;
  if (len) {
    memcpy(z->data, ((const char *)data)+counter-BUFFERED, len);
    z->datalen = len;
  }

}

/* midhash assumes the message is aligned to a 16-byte boundary */
z128 midhash( const void *message, size_t mlen, const void *key, size_t klen)
{
  __m128i s,a,b,c,d,e,f,g,h,i,j,k,l;
  __m128i *cache = (__m128i *)message;
  z128 val;
  int total = mlen + klen;
  int counter;
  
  if (klen > 16) {
    printf("key length must be 16 bytes or less\n");
    exit(1);
  }

  /* add the key and length; pad to a multiple of 16 bytes */
  memcpy(((char *)cache)+mlen, key, klen);
  ((char *)cache)[total] = klen;
  ((char *)cache)[total+1] = mlen+1;
  total += 2;
  if (total&15) {
    int tail = 16 - (total&15);
    memset(((char *)cache)+total, 0, tail);
    total += tail;
  }
  
  /* hash 32-byte blocks */
  s = _mm_set_epi32(0xdeadbeef, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef);
  c=d=e=f=g=h=i=j=k=l=s; 
  for (counter=2; counter<=total/16; counter+=2) {
    CHURN1(cache,counter,-2,s,b,h,l);
    CHURN1(cache,counter,-1,s,a,i,k);
    l=j; k=i; j=h; i=g; h=f; g=e; f=d; e=c; d=b; c=a;
  }
  
  /* possibly handle trailing 16-byte block, then use up accumulators */
  if (counter != total/16) {
    CHURN1(cache,counter,-2,s,b,h,l);
    TAIL1(s,k);
    TAIL1(s,j);
    TAIL1(s,i);
    TAIL1(s,h);
    TAIL1(s,g);
    TAIL1(s,f);
    TAIL1(s,e);
    TAIL1(s,d);
    TAIL1(s,c);
    TAIL1(s,b);
  } else {
    TAIL1(s,l);
    TAIL1(s,k);
    TAIL1(s,j);
    TAIL1(s,i);
    TAIL1(s,h);
    TAIL1(s,g);
    TAIL1(s,f);
    TAIL1(s,e);
    TAIL1(s,d);
    TAIL1(s,c);
  }
  
  /* final mixing */
  FINAL1(s);
  val.h = s;
  return val;
  
}

/*
 *  * Compute a hash for the total message
 *   * Computing the hash and using the key are done only privately
 *    * This: init() update() final(key1) final(key2) final(key3)
 *     *   is the same as: init() update() final(key3)
 *      */
z128 hash128final(zorba *z, const void *key, size_t klen)
{
  u8 total;
  //unsigned short lengths;
  //unsigned short int field;
  z128 val;

  /* use the key and the lengths */
  total = z->messagelen + klen;
  if (total <= 15) {

    /* one state, just finalization */
    val.h = _mm_set_epi32(0xdeadbeef, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef);
    memcpy(((char *)val.x), (char *)z->data, z->datalen);
    if (klen)
      memcpy(((char *)val.x)+z->datalen, key, klen);
    ((char *)val.x)[15] = 1 + z->datalen + (klen << 4);
    FINAL1(val.h);
    return val;

  } 

  else if (total <= LARGE) {

    return midhash(z->data, z->datalen, key, klen);

  } else {

    /* four states, churn in parallel, finalization mixes those 4 states */
    __m128i s0,a0,b0,c0,d0,e0,f0,g0,h0,i0,j0,k0,l0;
    __m128i s1,a1,b1,c1,d1,e1,f1,g1,h1,i1,j1,k1,l1;
    __m128i s2,a2,b2,c2,d2,e2,f2,g2,h2,i2,j2,k2,l2;
    __m128i s3,a3,b3,c3,d3,e3,f3,g3,h3,i3,j3,k3,l3;
    __m128i *cache = (__m128i *)z->data;
    size_t counter;

    if (klen > 16) {
      printf("key length must be 16 bytes or less\n");
      exit(1);
    }

    total = z->datalen + klen;

    /* load the registers */
    TO_REG(s,z->s,0);
    TO_REG(l,z->accum,0);
    TO_REG(k,z->accum,4);
    TO_REG(j,z->accum,8);
    TO_REG(i,z->accum,12);
    TO_REG(h,z->accum,16);
    TO_REG(g,z->accum,20);
    TO_REG(f,z->accum,24);
    TO_REG(e,z->accum,28);
    TO_REG(d,z->accum,32);
    TO_REG(c,z->accum,36);
    
    /* add the key and length; pad to a multiple of 64 bytes */
    if (klen)
      memcpy(((char *)z->data)+z->datalen, key, klen);
    ((char *)z->data)[total] = klen;
    ((char *)z->data)[total+1] = (z->datalen&63)+1;
    total += 2;
    if (total&63) {
      int tail = 64 - (total&63);
      memset(((char *)z->data)+total, 0, tail);
      total += tail;
    }

    /* consume all remaining 128-byte blocks */

    for (counter=8; counter<=total/16; counter += 8) {
      CHURN4(cache,counter,-8,s,b,h,l);
      CHURN4(cache,counter,-4,s,a,i,k);
      REG_REG(l,j); REG_REG(k,i); REG_REG(j,h); REG_REG(i,g); REG_REG(h,f);
      REG_REG(g,e); REG_REG(f,d); REG_REG(e,c); REG_REG(d,b); REG_REG(c,a);
    }
    
    /* possibly another 64-byte chunk, then consume accumulators */
    if (counter != total/16) {
      CHURN4(cache,counter,-8,s,b,h,l);
      TAIL4(s,k);
      TAIL4(s,j);
      TAIL4(s,i);
      TAIL4(s,h);
      TAIL4(s,g);
      TAIL4(s,f);
      TAIL4(s,e);
      TAIL4(s,d);
      TAIL4(s,c);
      TAIL4(s,b);
    } else {
      TAIL4(s,l);
      TAIL4(s,k);
      TAIL4(s,j);
      TAIL4(s,i);
      TAIL4(s,h);
      TAIL4(s,g);
      TAIL4(s,f);
      TAIL4(s,e);
      TAIL4(s,d);
      TAIL4(s,c);
    }
    
    /* mix s0..s3 to produce the final result */
    FINAL4(s);
    val.h = s0;
    return val;

  }

}



/* hash a message all at once, with a key */
z128 keyhash128( const void *message, size_t mlen, const void *key, size_t klen)
{
  size_t len = mlen + klen;
  if (len <= 15) {

    /* really short message */
    z128 val;
    val.h = _mm_set_epi32(0xdeadbeef, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef);
    memcpy(((char *)val.x), message, mlen);
    memcpy(((char *)val.x)+mlen, key, klen);
    ((char *)val.x)[15] = 1 + mlen + (klen << 4);
    FINAL1(val.h);
    return val;

  } else if (mlen <= LARGE) {
    
    __m128i buf[BLOCK];
    buf[mlen/16] = _mm_setzero_si128();
    memcpy(buf, message, mlen);
    return midhash( buf, mlen, key, klen);

  } else {

    /* long message: do it the normal way */
    zorba z;
    hash128init(&z);
    hash128update(&z, message, mlen);
    return hash128final(&z, key, klen);

  }
}



/* hash a message all at once, without a key */
z128 hash128( const void *message, size_t mlen)
{
  if (mlen <= 15) {

    /* really short message */
    z128 val;
    val.h = _mm_set_epi32(0xdeadbeef, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef);
    memcpy(((char *)val.x), message, mlen);
    ((char *)val.x)[15] = 1 + mlen;
    FINAL1(val.h);
    return val;

  } else if (mlen <= LARGE) {
    
    char key[1];
    __m128i buf[BLOCK];
    buf[mlen/16] = _mm_setzero_si128();
    memcpy(buf, message, mlen);
    return midhash( buf, mlen, key, 0);

  } else {

    /* long message: do it the normal way */
    zorba z;
    char key[1];
    hash128init(&z);
    hash128update(&z, message, mlen);
    return hash128final(&z, key, 0);

  }
}

