/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "libnetxms.h"

#if !(HAVE_WCSTOULL)

#ifndef UNDER_CE
#include <sys/types.h>
#include <errno.h>
#endif

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>

#if defined(__FreeBSD__) && __FreeBSD__ < 5
#define iswalpha(c) isalpha(c)
#define iswdigit(c) isdigit(c)
#define iswspace(c) isspace(c)
#define iswupper(c) isupper(c)
#endif

#if !HAVE_WINT_T && !defined(_WIN32)  
typedef int wint_t;
#endif


/*
 * Convert a string to an 64 bit unsigned integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */

UINT64 LIBNETXMS_EXPORTABLE wcstoull(const WCHAR *nptr, WCHAR **endptr, int base)
{
	const WCHAR *s;
	UINT64 acc, cutoff;
   wint_t c;
	int neg, any, cutlim;

	/*
	 * See strtoq for comments as to the logic used.
	 */
	s = nptr;
	do {
		c = *s++;
	} while (iswspace(c));
	if (c == L'-') {
		neg = 1;
		c = *s++;
	} else { 
		neg = 0;
		if (c == L'+')
			c = *s++;
	}
	if ((base == 0 || base == 16) &&
	    c == L'0' && (*s == L'x' || *s == L'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == L'0' ? 8 : 10;

	cutoff = ULLONG_MAX / (UINT64)base;
	cutlim = (int)(ULLONG_MAX % (UINT64)base);
	for (acc = 0, any = 0;; c = *s++) {
		if (iswdigit(c))
			c -= L'0';
		else if (iswalpha(c))
			c -= iswupper(c) ? L'A' - 10 : L'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0)
			continue;
		if (acc > cutoff || (acc == cutoff && c > cutlim)) {
			any = -1;
			acc = ULLONG_MAX;
#ifndef UNDER_CE
			errno = ERANGE;
#endif
		} else {
			any = 1;
			acc *= (UINT64)base;
			acc += c;
		}
	}
	if (neg && any > 0)
		acc = ~acc + 1;
	if (endptr != 0)
		*endptr = (WCHAR *) (any ? s - 1 : nptr);
	return (acc);
}

#endif   /* !(HAVE_STRTOULL) */
