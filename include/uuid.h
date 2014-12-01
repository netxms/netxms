/*
 * Public include file for the UUID library
 * 
 * Copyright (C) 1996, 1997, 1998 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU 
 * Library General Public License.
 * %End-Header%
 */

#ifndef _UUID_H_
#define _UUID_H_

#define UUID_LENGTH     16

#if !HAVE_UUID_T
#undef uuid_t
typedef unsigned char uuid_t[16];
#endif

/* UUID Variant definitions */
#define UUID_VARIANT_NCS         0
#define UUID_VARIANT_DCE         1
#define UUID_VARIANT_MICROSOFT   2
#define UUID_VARIANT_OTHER       3

#ifdef __cplusplus
extern "C" {
#endif

void LIBNETXMS_EXPORTABLE uuid_clear(uuid_t uu);
int LIBNETXMS_EXPORTABLE uuid_compare(uuid_t uu1, uuid_t uu2);
void LIBNETXMS_EXPORTABLE uuid_generate(uuid_t out);
int LIBNETXMS_EXPORTABLE uuid_is_null(uuid_t uu);
int LIBNETXMS_EXPORTABLE uuid_parse(const TCHAR *in, uuid_t uu);
TCHAR LIBNETXMS_EXPORTABLE *uuid_to_string(uuid_t uu, TCHAR *out);

#ifdef __cplusplus
}
#endif

#endif
