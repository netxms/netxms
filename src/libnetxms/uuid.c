/* 
** libuuid integrated into NetXMS project
** Copyright (C) 1996, 1997 Theodore Ts'o.
** Integrated into NetXMS by Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: uuid.cpp
**
**/

#include "libnetxms.h"
#include "uuidP.h"


//
// Clear a UUID
//

void LIBNETXMS_EXPORTABLE uuid_clear(uuid_t uu)
{
	memset(uu, 0, 16);
}

#define UUCMP(u1,u2) if (u1 != u2) return((u1 < u2) ? -1 : 1);


//
// compare whether or not two UUID's are the same
//
// Returns 0 if the two UUID's are different, and 1 if they are the same.
//

int LIBNETXMS_EXPORTABLE uuid_compare(uuid_t uu1, uuid_t uu2)
{
	struct uuid	uuid1, uuid2;

	uuid_unpack(uu1, &uuid1);
	uuid_unpack(uu2, &uuid2);

	UUCMP(uuid1.time_low, uuid2.time_low);
	UUCMP(uuid1.time_mid, uuid2.time_mid);
	UUCMP(uuid1.time_hi_and_version, uuid2.time_hi_and_version);
	UUCMP(uuid1.clock_seq, uuid2.clock_seq);
	return memcmp(uuid1.node, uuid2.node, 6);
}


//
// isnull.c --- Check whether or not the UUID is null
// Returns 1 if the uuid is the NULL uuid
//

int LIBNETXMS_EXPORTABLE uuid_is_null(uuid_t uu)
{
	unsigned char 	*cp;
	int i;

	for (i=0, cp = uu; i < 16; i++)
		if (*cp++)
			return 0;
	return 1;
}


//
// Internal routine for packing UUID's
//

void uuid_pack(struct uuid *uu, uuid_t ptr)
{
	unsigned int	tmp;
	unsigned char	*out = ptr;

	tmp = uu->time_low;
	out[3] = (unsigned char) tmp;
	tmp >>= 8;
	out[2] = (unsigned char) tmp;
	tmp >>= 8;
	out[1] = (unsigned char) tmp;
	tmp >>= 8;
	out[0] = (unsigned char) tmp;
	
	tmp = uu->time_mid;
	out[5] = (unsigned char) tmp;
	tmp >>= 8;
	out[4] = (unsigned char) tmp;

	tmp = uu->time_hi_and_version;
	out[7] = (unsigned char) tmp;
	tmp >>= 8;
	out[6] = (unsigned char) tmp;

	tmp = uu->clock_seq;
	out[9] = (unsigned char) tmp;
	tmp >>= 8;
	out[8] = (unsigned char) tmp;

	memcpy(out+10, uu->node, 6);
}


//
// Internal routine for unpacking UUID
//

void uuid_unpack(uuid_t in, struct uuid *uu)
{
	unsigned char	*ptr = in;
	unsigned int	tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	tmp = (tmp << 8) | *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->time_low = tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->time_mid = tmp;
	
	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->time_hi_and_version = tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->clock_seq = tmp;

	memcpy(uu->node, ptr, 6);
}


//
// Parse UUID
//

int LIBNETXMS_EXPORTABLE uuid_parse(TCHAR *in, uuid_t uu)
{
	struct uuid uuid;
	int i;
	TCHAR *cp, buf[3];

	if (_tcslen(in) != 36)
		return -1;
	for (i=0, cp = in; i <= 36; i++,cp++) {
		if ((i == 8) || (i == 13) || (i == 18) ||
		    (i == 23))
			if (*cp == _T('-'))
				continue;
		if (i == 36)
			if (*cp == 0)
				continue;
		if (!_istxdigit(*cp))
			return -1;
	}
	uuid.time_low = _tcstoul(in, NULL, 16);
	uuid.time_mid = (WORD)_tcstoul(in + 9, NULL, 16);
	uuid.time_hi_and_version = (WORD)_tcstoul(in + 14, NULL, 16);
	uuid.clock_seq = (WORD)_tcstoul(in + 19, NULL, 16);
	cp = in + 24;
	buf[2] = 0;
	for(i = 0; i < 6; i++)
   {
		buf[0] = *cp++;
		buf[1] = *cp++;
		uuid.node[i] = (BYTE)_tcstoul(buf, NULL, 16);
	}
	
	uuid_pack(&uuid, uu);
	return 0;
}


//
// Convert packed UUID to string
//

TCHAR LIBNETXMS_EXPORTABLE *uuid_to_string(uuid_t uu, TCHAR *out)
{
	struct uuid uuid;

	uuid_unpack(uu, &uuid);
	_sntprintf(out, 64,
		_T("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"),
		uuid.time_low, uuid.time_mid, uuid.time_hi_and_version,
		uuid.clock_seq >> 8, uuid.clock_seq & 0xFF,
		uuid.node[0], uuid.node[1], uuid.node[2],
		uuid.node[3], uuid.node[4], uuid.node[5]);
   return out;
}
