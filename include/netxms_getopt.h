/*
 * Copyright (c) 2014 Raden Solutions
 *
 * Custom header file for getopt implementation derived from NetBSD implementation
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * Copyright (c) 2002 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Sponsored in part by the Defense Advanced Research Projects
 * Agency (DARPA) and Air Force Research Laboratory, Air Force
 * Materiel Command, USAF, under agreement number F39502-99-1-0512.
 */
/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Dieter Baron and Thomas Klausner.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _NETXMS_GETOPT_H
#define _NETXMS_GETOPT_H

#include <nms_util.h>

#if USE_BUNDLED_GETOPT || !HAVE_DECL_GETOPT_LONG

#define REPLACE_GETOPT 1

#undef HAVE_GETOPT_LONG
#define HAVE_GETOPT_LONG 1

#undef HAVE_DECL_GETOPT_LONG
#define HAVE_DECL_GETOPT_LONG 1

#else

#undef HAVE_GETOPT_LONG
#define HAVE_GETOPT_LONG 1

#endif

#if !REPLACE_GETOPT && HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef REPLACE_GETOPT

extern LIBNETXMS_EXPORTABLE char *optargA;
extern LIBNETXMS_EXPORTABLE int optindA;
extern LIBNETXMS_EXPORTABLE int opterrA;
extern LIBNETXMS_EXPORTABLE int optoptA;

#define optarg optargA
#define optind optindA
#define opterr opterrA
#define optopt optoptA

#endif   /* REPLACE_GETOPT */

extern LIBNETXMS_EXPORTABLE const WCHAR *optargW;
extern LIBNETXMS_EXPORTABLE int optindW;
extern LIBNETXMS_EXPORTABLE int opterrW;
extern LIBNETXMS_EXPORTABLE int optoptW;

#ifdef UNICODE
#define _toptarg optargW
#define _toptind optindW
#define _topterr opterrW
#define _toptopt optoptW
#else
#define _toptarg optarg
#define _toptind optind
#define _topterr opterr
#define _toptopt optopt
#endif

#ifdef REPLACE_GETOPT

struct option
{
#if __STDC__ || defined(_WIN32)
  const char *name;
#else
  char *name;
#endif
  int has_arg;
  int *flag;
  int val;
};

#define no_argument        0
#define required_argument  1
#define optional_argument  2

int LIBNETXMS_EXPORTABLE getoptA(int nargc, char * const *nargv, const char *options);
int LIBNETXMS_EXPORTABLE getopt_longA(int nargc, char * const *nargv, const char *options, const struct option *long_options, int *idx);
int LIBNETXMS_EXPORTABLE getopt_long_onlyA(int nargc, char * const *nargv, const char *options, const struct option *long_options, int *idx);

#define getopt getoptA
#define getopt_long getopt_longA
#define getopt_long_only getopt_long_onlyA

#endif   /* REPLACE_GETOPT */

int LIBNETXMS_EXPORTABLE getoptW(int nargc, WCHAR * const *nargv, const char *options);
int LIBNETXMS_EXPORTABLE getopt_longW(int nargc, WCHAR * const *nargv, const char *options, const struct option *long_options, int *idx);
int LIBNETXMS_EXPORTABLE getopt_long_onlyW(int nargc, WCHAR * const *nargv, const char *options, const struct option *long_options, int *idx);

#ifdef UNICODE
#define _tgetopt getoptW
#define _tgetopt_long getopt_longW
#define _tgetopt_long_only getopt_long_onlyW
#else
#define _tgetopt getopt
#define _tgetopt_long getopt_long
#define _tgetopt_long_only getopt_long_only
#endif

#ifdef	__cplusplus
}
#endif

#endif /* _NETXMS_GETOPT_H */
