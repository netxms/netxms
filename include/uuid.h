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

#undef uuid_t
typedef unsigned char uuid_t[16];

/* UUID Variant definitions */
#define UUID_VARIANT_NCS 	0
#define UUID_VARIANT_DCE 	1
#define UUID_VARIANT_MICROSOFT	2
#define UUID_VARIANT_OTHER	3

#ifdef __cplusplus
extern "C" {
#endif

void LIBNETXMS_EXPORTABLE uuid_clear(uuid_t uu);
int LIBNETXMS_EXPORTABLE uuid_compare(uuid_t uu1, uuid_t uu2);
void LIBNETXMS_EXPORTABLE uuid_copy(uuid_t uu1, uuid_t uu2);
void LIBNETXMS_EXPORTABLE uuid_generate(uuid_t out);
int LIBNETXMS_EXPORTABLE uuid_is_null(uuid_t uu);

#ifdef __cplusplus
}
#endif

/*
int uuid_parse(char *in, uuid_t uu);
void uuid_unparse(uuid_t uu, char *out);
*/

#endif
