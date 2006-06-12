/*
 * uuid.h -- private header file for uuids
 * 
 * Copyright (C) 1996, 1997 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU 
 * Library General Public License.
 * %End-Header%
 */

#ifndef _uuidP_h_
#define _uuidP_h_

#include <sys/types.h>

#include <uuid.h>

/*
 * Offset between 15-Oct-1582 and 1-Jan-70
 */
#define TIME_OFFSET_HIGH 0x01B21DD2
#define TIME_OFFSET_LOW  0x13814000

struct uuid
{
	DWORD	time_low;
	WORD	time_mid;
	WORD	time_hi_and_version;
	WORD	clock_seq;
	BYTE	node[6];
};


/*
 * prototypes
 */
void uuid_pack(struct uuid *uu, uuid_t ptr);
void uuid_unpack(uuid_t in, struct uuid *uu);

#endif
