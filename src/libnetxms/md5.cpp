/*
  Copyright (C) 1999, 2000, 2002 Aladdin Enterprises.  All rights reserved.

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  L. Peter Deutsch
  ghost@aladdin.com

 */
/* $Id$ */
/*
  Independent implementation of MD5 (RFC 1321).

  This code implements the MD5 Algorithm defined in RFC 1321, whose
  text is available at
	http://www.ietf.org/rfc/rfc1321.txt
  The code is derived from the text of the RFC, including the test suite
  (section A.5) but excluding the rest of Appendix A.  It does not include
  any code or documentation that is identified in the RFC as being
  copyrighted.

  The original and principal author of md5.c is L. Peter Deutsch
  <ghost@aladdin.com>.  Other authors are noted in the change history
  that follows (in reverse chronological order):

  2026-11-17 Raden Solutions: block transform rewritten for shorter dependency
	chain (message word and constant added before round function,
	cheaper F and G forms), multi-block processing with state kept in
	registers, unaligned-safe word loads; runtime byte order detection
	removed.
  2002-04-13 lpd Clarified derivation from RFC 1321; now handles byte order
	either statically or dynamically; added missing #include <string.h>
	in library.
  2002-03-11 lpd Corrected argument list for main(), and added int return
	type, in test program and T value program.
  2002-02-21 lpd Added missing #include <stdio.h> in test program.
  2000-07-03 lpd Patched to eliminate warnings about "constant is
	unsigned in ANSI C, signed in traditional"; made test program
	self-checking.
  1999-11-04 lpd Edited comments slightly for automatic TOC extraction.
  1999-10-18 lpd Fixed typo in header comment (ansi2knr rather than md5).
  1999-05-03 lpd Original version.
 */

#ifndef _WIN32
#include <config.h>
#endif

#include "md5.h"
#include <string.h>
#include <stdint.h>

/**
 * Rotate 32 bit value left
 */
static inline uint32_t RotateLeft(uint32_t x, int n)
{
   return (x << n) | (x >> (32 - n));
}

/**
 * Load 32 bit little endian value from arbitrary aligned memory location
 */
static inline uint32_t Load32LE(const md5_byte_t *p)
{
#ifdef WORDS_BIGENDIAN
   return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
#else
   uint32_t v;
   memcpy(&v, p, 4);
   return v;
#endif
}

/**
 * MD5 steps. Message word and constant are added first because they do not depend on previous
 * step, then the round function, so the critical dependency chain through "b" stays as short
 * as possible. F(x,y,z) is computed as z ^ (x & (y ^ z)), which needs one operation less than
 * RFC form. G(x,y,z) is computed as (x & z) + (y & ~z) (operands have no common bits), so that
 * the part not depending on "b" can be added before "b" is known.
 */
#define STEP_F(a, b, c, d, k, s, t) \
   a += X(k) + (t); a += (d) ^ ((b) & ((c) ^ (d))); a = RotateLeft(a, s) + (b)
#define STEP_G(a, b, c, d, k, s, t) \
   a += X(k) + (t); a += (c) & ~(d); a += (b) & (d); a = RotateLeft(a, s) + (b)
#define STEP_H(a, b, c, d, k, s, t) \
   a += X(k) + (t); a += (b) ^ ((c) ^ (d)); a = RotateLeft(a, s) + (b)
#define STEP_I(a, b, c, d, k, s, t) \
   a += X(k) + (t); a += (c) ^ ((b) | ~(d)); a = RotateLeft(a, s) + (b)

/**
 * Process given number of 64 byte blocks
 */
static void md5_process_blocks(md5_word_t *abcd, const md5_byte_t *data, size_t blocks)
{
   uint32_t a = abcd[0], b = abcd[1], c = abcd[2], d = abcd[3];
   for(; blocks > 0; blocks--, data += 64)
   {
#ifdef WORDS_BIGENDIAN
      uint32_t words[16];
      for(int i = 0; i < 16; i++)
         words[i] = Load32LE(data + i * 4);
#define X(k) words[k]
#else
#define X(k) Load32LE(data + (k) * 4)
#endif

      uint32_t sa = a, sb = b, sc = c, sd = d;
      STEP_F(a, b, c, d,  0,  7, 0xd76aa478);
      STEP_F(d, a, b, c,  1, 12, 0xe8c7b756);
      STEP_F(c, d, a, b,  2, 17, 0x242070db);
      STEP_F(b, c, d, a,  3, 22, 0xc1bdceee);
      STEP_F(a, b, c, d,  4,  7, 0xf57c0faf);
      STEP_F(d, a, b, c,  5, 12, 0x4787c62a);
      STEP_F(c, d, a, b,  6, 17, 0xa8304613);
      STEP_F(b, c, d, a,  7, 22, 0xfd469501);
      STEP_F(a, b, c, d,  8,  7, 0x698098d8);
      STEP_F(d, a, b, c,  9, 12, 0x8b44f7af);
      STEP_F(c, d, a, b, 10, 17, 0xffff5bb1);
      STEP_F(b, c, d, a, 11, 22, 0x895cd7be);
      STEP_F(a, b, c, d, 12,  7, 0x6b901122);
      STEP_F(d, a, b, c, 13, 12, 0xfd987193);
      STEP_F(c, d, a, b, 14, 17, 0xa679438e);
      STEP_F(b, c, d, a, 15, 22, 0x49b40821);

      STEP_G(a, b, c, d,  1,  5, 0xf61e2562);
      STEP_G(d, a, b, c,  6,  9, 0xc040b340);
      STEP_G(c, d, a, b, 11, 14, 0x265e5a51);
      STEP_G(b, c, d, a,  0, 20, 0xe9b6c7aa);
      STEP_G(a, b, c, d,  5,  5, 0xd62f105d);
      STEP_G(d, a, b, c, 10,  9, 0x02441453);
      STEP_G(c, d, a, b, 15, 14, 0xd8a1e681);
      STEP_G(b, c, d, a,  4, 20, 0xe7d3fbc8);
      STEP_G(a, b, c, d,  9,  5, 0x21e1cde6);
      STEP_G(d, a, b, c, 14,  9, 0xc33707d6);
      STEP_G(c, d, a, b,  3, 14, 0xf4d50d87);
      STEP_G(b, c, d, a,  8, 20, 0x455a14ed);
      STEP_G(a, b, c, d, 13,  5, 0xa9e3e905);
      STEP_G(d, a, b, c,  2,  9, 0xfcefa3f8);
      STEP_G(c, d, a, b,  7, 14, 0x676f02d9);
      STEP_G(b, c, d, a, 12, 20, 0x8d2a4c8a);

      STEP_H(a, b, c, d,  5,  4, 0xfffa3942);
      STEP_H(d, a, b, c,  8, 11, 0x8771f681);
      STEP_H(c, d, a, b, 11, 16, 0x6d9d6122);
      STEP_H(b, c, d, a, 14, 23, 0xfde5380c);
      STEP_H(a, b, c, d,  1,  4, 0xa4beea44);
      STEP_H(d, a, b, c,  4, 11, 0x4bdecfa9);
      STEP_H(c, d, a, b,  7, 16, 0xf6bb4b60);
      STEP_H(b, c, d, a, 10, 23, 0xbebfbc70);
      STEP_H(a, b, c, d, 13,  4, 0x289b7ec6);
      STEP_H(d, a, b, c,  0, 11, 0xeaa127fa);
      STEP_H(c, d, a, b,  3, 16, 0xd4ef3085);
      STEP_H(b, c, d, a,  6, 23, 0x04881d05);
      STEP_H(a, b, c, d,  9,  4, 0xd9d4d039);
      STEP_H(d, a, b, c, 12, 11, 0xe6db99e5);
      STEP_H(c, d, a, b, 15, 16, 0x1fa27cf8);
      STEP_H(b, c, d, a,  2, 23, 0xc4ac5665);

      STEP_I(a, b, c, d,  0,  6, 0xf4292244);
      STEP_I(d, a, b, c,  7, 10, 0x432aff97);
      STEP_I(c, d, a, b, 14, 15, 0xab9423a7);
      STEP_I(b, c, d, a,  5, 21, 0xfc93a039);
      STEP_I(a, b, c, d, 12,  6, 0x655b59c3);
      STEP_I(d, a, b, c,  3, 10, 0x8f0ccc92);
      STEP_I(c, d, a, b, 10, 15, 0xffeff47d);
      STEP_I(b, c, d, a,  1, 21, 0x85845dd1);
      STEP_I(a, b, c, d,  8,  6, 0x6fa87e4f);
      STEP_I(d, a, b, c, 15, 10, 0xfe2ce6e0);
      STEP_I(c, d, a, b,  6, 15, 0xa3014314);
      STEP_I(b, c, d, a, 13, 21, 0x4e0811a1);
      STEP_I(a, b, c, d,  4,  6, 0xf7537e82);
      STEP_I(d, a, b, c, 11, 10, 0xbd3af235);
      STEP_I(c, d, a, b,  2, 15, 0x2ad7d2bb);
      STEP_I(b, c, d, a,  9, 21, 0xeb86d391);

#undef X

      a += sa;
      b += sb;
      c += sc;
      d += sd;
   }
   abcd[0] = a;
   abcd[1] = b;
   abcd[2] = c;
   abcd[3] = d;
}

#undef STEP_F
#undef STEP_G
#undef STEP_H
#undef STEP_I

/**
 * Initialize MD5 state
 */
void I_md5_init(md5_state_t *pms)
{
   pms->count[0] = pms->count[1] = 0;
   pms->abcd[0] = 0x67452301;
   pms->abcd[1] = 0xefcdab89;
   pms->abcd[2] = 0x98badcfe;
   pms->abcd[3] = 0x10325476;
}

/**
 * Append data to MD5 state
 */
void I_md5_append(md5_state_t *pms, const md5_byte_t *data, unsigned int nbytes)
{
   if (nbytes == 0)
      return;

   unsigned int offset = (pms->count[0] >> 3) & 63;

   // Update message length (in bits)
   md5_word_t nbits = static_cast<md5_word_t>(nbytes << 3);
   pms->count[1] += nbytes >> 29;
   pms->count[0] += nbits;
   if (pms->count[0] < nbits)
      pms->count[1]++;

   // Complete partial block accumulated by previous calls
   if (offset != 0)
   {
      unsigned int copy = (offset + nbytes > 64) ? 64 - offset : nbytes;
      memcpy(pms->buf + offset, data, copy);
      if (offset + copy < 64)
         return;
      data += copy;
      nbytes -= copy;
      md5_process_blocks(pms->abcd, pms->buf, 1);
   }

   // Process full blocks directly from input
   if (nbytes >= 64)
   {
      md5_process_blocks(pms->abcd, data, nbytes / 64);
      data += nbytes & ~63u;
      nbytes &= 63;
   }

   // Save remaining partial block
   if (nbytes > 0)
      memcpy(pms->buf, data, nbytes);
}

/**
 * Finish MD5 calculation
 */
void I_md5_finish(md5_state_t *pms, md5_byte_t digest[16])
{
   static const md5_byte_t pad[64] = { 0x80 };
   md5_byte_t length[8];
   for(int i = 0; i < 8; i++)
      length[i] = static_cast<md5_byte_t>(pms->count[i >> 2] >> ((i & 3) << 3));
   I_md5_append(pms, pad, ((55 - (pms->count[0] >> 3)) & 63) + 1);
   I_md5_append(pms, length, 8);
   for(int i = 0; i < 16; i++)
      digest[i] = static_cast<md5_byte_t>(pms->abcd[i >> 2] >> ((i & 3) << 3));
}
