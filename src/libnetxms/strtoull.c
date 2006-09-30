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

#if !(HAVE_STRTOULL)

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

/*
 * Convert a string to an 64 bit unsigned integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */

#ifdef UNICODE
QWORD LIBNETXMS_EXPORTABLE wcstoull(const WCHAR *nptr, WCHAR **endptr, int base)
#else
QWORD LIBNETXMS_EXPORTABLE strtoull(const char *nptr, char **endptr, int base)
#endif
{
	const TCHAR *s;
	QWORD acc, cutoff;
#ifdef UNICODE
	_TINT c;
#else
   int c;
#endif
	int neg, any, cutlim;

	/*
	 * See strtoq for comments as to the logic used.
	 */
	s = nptr;
	do {
		c = *s++;
	} while (_istspace(c));
	if (c == _T('-')) {
		neg = 1;
		c = *s++;
	} else { 
		neg = 0;
		if (c == _T('+'))
			c = *s++;
	}
	if ((base == 0 || base == 16) &&
	    c == _T('0') && (*s == _T('x') || *s == _T('X'))) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == _T('0') ? 8 : 10;

	cutoff = ULLONG_MAX / (QWORD)base;
	cutlim = (int)(ULLONG_MAX % (QWORD)base);
	for (acc = 0, any = 0;; c = *s++) {
		if (_istdigit(c))
			c -= _T('0');
		else if (_istalpha(c))
			c -= _istupper(c) ? _T('A') - 10 : _T('a') - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0)
			continue;
		if (acc > cutoff || (acc == cutoff && c > cutlim)) {
			any = -1;
			acc = ULLONG_MAX;
			errno = ERANGE;
		} else {
			any = 1;
			acc *= (QWORD)base;
			acc += c;
		}
	}
	if (neg && any > 0)
		acc = ~acc + 1;
	if (endptr != 0)
		*endptr = (TCHAR *) (any ? s - 1 : nptr);
	return (acc);
}

#endif   /* !(HAVE_STRTOULL) */
