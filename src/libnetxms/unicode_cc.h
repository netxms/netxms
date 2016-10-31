#ifndef _unicode_cc_h_
#define _unicode_cc_h_

#ifndef _WIN32

#if HAVE_ICONV_H
#include <iconv.h>
#endif

/**
 * UNICODE character set
 */
#ifndef __DISABLE_ICONV

// iconv cache functions
#if WITH_ICONV_CACHE
iconv_t IconvOpen(const char *to, const char *from);
void IconvClose(iconv_t cd);
#else
#define IconvOpen iconv_open
#define IconvClose iconv_close
#endif

// configure first test for libiconv, then for iconv
// if libiconv was found, HAVE_ICONV will not be set correctly
#if HAVE_LIBICONV
#undef HAVE_ICONV
#define HAVE_ICONV 1
#endif

#if HAVE_ICONV_UCS_2_INTERNAL
#define UCS2_CODEPAGE_NAME "UCS-2-INTERNAL"
#elif HAVE_ICONV_UCS_2BE && WORDS_BIGENDIAN
#define UCS2_CODEPAGE_NAME "UCS-2BE"
#elif HAVE_ICONV_UCS_2LE && !WORDS_BIGENDIAN
#define UCS2_CODEPAGE_NAME "UCS-2LE"
#elif HAVE_ICONV_UCS_2
#define UCS2_CODEPAGE_NAME "UCS-2"
#elif HAVE_ICONV_UCS2
#define UCS2_CODEPAGE_NAME "UCS2"
#elif HAVE_ICONV_UTF_16
#define UCS2_CODEPAGE_NAME "UTF-16"
#else
#ifdef UNICODE
#error Cannot determine valid UCS-2 codepage name
#else
#warning Cannot determine valid UCS-2 codepage name
#undef HAVE_ICONV
#endif
#endif

#if HAVE_ICONV_UCS_4_INTERNAL
#define UCS4_CODEPAGE_NAME "UCS-4-INTERNAL"
#elif HAVE_ICONV_UCS_4BE && WORDS_BIGENDIAN
#define UCS4_CODEPAGE_NAME "UCS-4BE"
#elif HAVE_ICONV_UCS_4LE && !WORDS_BIGENDIAN
#define UCS4_CODEPAGE_NAME "UCS-4LE"
#elif HAVE_ICONV_UCS_4
#define UCS4_CODEPAGE_NAME "UCS-4"
#elif HAVE_ICONV_UCS4
#define UCS4_CODEPAGE_NAME "UCS4"
#elif HAVE_ICONV_UTF_32
#define UCS4_CODEPAGE_NAME "UTF-32"
#else
#if defined(UNICODE) && defined(UNICODE_UCS4)
#error Cannot determine valid UCS-4 codepage name
#else
#warning Cannot determine valid UCS-4 codepage name
#undef HAVE_ICONV
#endif
#endif

#ifdef UNICODE_UCS4
#define UNICODE_CODEPAGE_NAME UCS4_CODEPAGE_NAME
#else /* assume UCS-2 */
#define UNICODE_CODEPAGE_NAME UCS2_CODEPAGE_NAME
#endif

#endif   /* __DISABLE_ICONV */

#endif   /* _WIN32 */

extern char g_cpDefault[];

#endif
