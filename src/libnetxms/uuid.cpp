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
#include <uuid.h>

#if _WITH_ENCRYPTION
#include <openssl/rand.h>
#endif

/**
 * NULL GUID
 */
const uuid __EXPORT uuid::NULL_UUID = uuid();

/*
 * Offset between 15-Oct-1582 and 1-Jan-70
 */
#define TIME_OFFSET_HIGH 0x01B21DD2
#define TIME_OFFSET_LOW  0x13814000

/**
 * Unpacked UUID structure
 */
struct __uuid
{
	uint32_t	time_low;
	uint16_t time_mid;
	uint16_t time_hi_and_version;
	uint16_t clock_seq;
	uint8_t node[6];
};

/**
 * Internal routine for packing UUID's
 */
static void uuid_pack(struct __uuid *uu, uuid_t ptr)
{
	uint32_t tmp;
	uint8_t *out = ptr;

	tmp = uu->time_low;
	out[3] = (uint8_t)tmp;
	tmp >>= 8;
	out[2] = (uint8_t)tmp;
	tmp >>= 8;
	out[1] = (uint8_t)tmp;
	tmp >>= 8;
	out[0] = (uint8_t)tmp;
	
	tmp = uu->time_mid;
	out[5] = (uint8_t)tmp;
	tmp >>= 8;
	out[4] = (uint8_t)tmp;

	tmp = uu->time_hi_and_version;
	out[7] = (uint8_t)tmp;
	tmp >>= 8;
	out[6] = (uint8_t)tmp;

	tmp = uu->clock_seq;
	out[9] = (uint8_t)tmp;
	tmp >>= 8;
	out[8] = (uint8_t)tmp;

	memcpy(out + 10, uu->node, 6);
}

/**
 * Internal routine for unpacking UUID
 */
static void uuid_unpack(const uuid_t in, struct __uuid *uu)
{
	const uint8_t *ptr = in;
	uint32_t tmp;

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

/**
 * Clear a UUID
 */
void LIBNETXMS_EXPORTABLE _uuid_clear(uuid_t uu)
{
	memset(uu, 0, 16);
}

#define UUCMP(u1,u2) if (u1 != u2) return((u1 < u2) ? -1 : 1);

/**
 * compare whether or not two UUID's are the same
 *
 * Returns 1/-1 if the two UUID's are different, and 0 if they are the same.
 */
int LIBNETXMS_EXPORTABLE _uuid_compare(const uuid_t uu1, const uuid_t uu2)
{
	struct __uuid	uuid1, uuid2;

	uuid_unpack(uu1, &uuid1);
	uuid_unpack(uu2, &uuid2);

	UUCMP(uuid1.time_low, uuid2.time_low);
	UUCMP(uuid1.time_mid, uuid2.time_mid);
	UUCMP(uuid1.time_hi_and_version, uuid2.time_hi_and_version);
	UUCMP(uuid1.clock_seq, uuid2.clock_seq);
	return memcmp(uuid1.node, uuid2.node, 6);
}

/**
 * isnull.c --- Check whether or not the UUID is null
 * Returns true if the uuid is the NULL uuid
 */
bool LIBNETXMS_EXPORTABLE _uuid_is_null(const uuid_t uu)
{
	const uint8_t *cp = uu;
	for (int i = 0; i < 16; i++)
		if (*cp++)
			return false;
	return true;
}

/**
 * Parse UUID
 */
int LIBNETXMS_EXPORTABLE _uuid_parse(const TCHAR *in, uuid_t uu)
{
	struct __uuid uuid;
	int i;
	const TCHAR *cp;
	TCHAR buf[3];

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
	uuid.time_low = _tcstoul(in, nullptr, 16);
	uuid.time_mid = (uint16_t)_tcstoul(in + 9, nullptr, 16);
	uuid.time_hi_and_version = (uint16_t)_tcstoul(in + 14, nullptr, 16);
	uuid.clock_seq = (uint16_t)_tcstoul(in + 19, nullptr, 16);
	cp = in + 24;
	buf[2] = 0;
	for(i = 0; i < 6; i++)
   {
		buf[0] = *cp++;
		buf[1] = *cp++;
		uuid.node[i] = (BYTE)_tcstoul(buf, nullptr, 16);
	}
	
	uuid_pack(&uuid, uu);
	return 0;
}

/**
 * Convert packed UUID to string
 */
TCHAR LIBNETXMS_EXPORTABLE *_uuid_to_string(const uuid_t uu, TCHAR *out)
{
	struct __uuid uuid;
	uuid_unpack(uu, &uuid);
	_sntprintf(out, 37,
		_T("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"),
		uuid.time_low, uuid.time_mid, uuid.time_hi_and_version,
		uuid.clock_seq >> 8, uuid.clock_seq & 0xFF,
		uuid.node[0], uuid.node[1], uuid.node[2],
		uuid.node[3], uuid.node[4], uuid.node[5]);
   return out;
}

#ifdef UNICODE

/**
 * Parse UUID
 */
int LIBNETXMS_EXPORTABLE _uuid_parseA(const char *in, uuid_t uu)
{
   struct __uuid uuid;
   int i;
   const char *cp;
   char buf[3];

   if (strlen(in) != 36)
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
   uuid.time_low = strtoul(in, nullptr, 16);
   uuid.time_mid = (uint16_t)strtoul(in + 9, nullptr, 16);
   uuid.time_hi_and_version = (uint16_t)strtoul(in + 14, nullptr, 16);
   uuid.clock_seq = (uint16_t)strtoul(in + 19, nullptr, 16);
   cp = in + 24;
   buf[2] = 0;
   for(i = 0; i < 6; i++)
   {
      buf[0] = *cp++;
      buf[1] = *cp++;
      uuid.node[i] = (BYTE)strtoul(buf, nullptr, 16);
   }

   uuid_pack(&uuid, uu);
   return 0;
}

/**
 * Convert packed UUID to string (non-UNICODE version)
 */
char LIBNETXMS_EXPORTABLE *_uuid_to_stringA(const uuid_t uu, char *out)
{
   struct __uuid uuid;
   uuid_unpack(uu, &uuid);
   snprintf(out, 37,
      "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      uuid.time_low, uuid.time_mid, uuid.time_hi_and_version,
      uuid.clock_seq >> 8, uuid.clock_seq & 0xFF,
      uuid.node[0], uuid.node[1], uuid.node[2],
      uuid.node[3], uuid.node[4], uuid.node[5]);
   return out;
}

#endif

#ifndef _WIN32

static int get_random_fd()
{
   int fd = open("/dev/urandom", O_RDONLY);
   if (fd == -1)
      fd = open("/dev/random", O_RDONLY | O_NONBLOCK);
	return fd;
}

/*
 * Generate a series of random bytes.  Use /dev/urandom if possible,
 * and if not, use srandom/random.
 */
static void get_random_bytes(void *buf, int nbytes)
{
#if _WITH_ENCRYPTION
   if (RAND_bytes(static_cast<unsigned char*>(buf), nbytes))
      return;
#endif

	char *cp = (char *)buf;
   int fd = get_random_fd();
	if (fd >= 0)
   {
	   int lose_counter = 0;

		while (nbytes > 0)
      {
			int i = read(fd, cp, nbytes);
			if ((i < 0) &&
			    ((errno == EINTR) || (errno == EAGAIN)))
				continue;
			if (i <= 0)
			{
				if (lose_counter++ == 8)
					break;
				continue;
			}
			nbytes -= i;
			cp += i;
			lose_counter = 0;
		}
		close(fd);
	}
	if (nbytes == 0)
		return;

	// Use libc rand() as last resort
	for(int i = 0; i < nbytes; i++)
		*cp++ = rand() & 0xFF;
}

static void uuid_generate_random(uuid_t out)
{
	uuid_t	buf;
	struct __uuid uu;

	get_random_bytes(buf, sizeof(buf));
	uuid_unpack(buf, &uu);

	uu.clock_seq = (uu.clock_seq & 0x3FFF) | 0x8000;
	uu.time_hi_and_version = (uu.time_hi_and_version & 0x0FFF) | 0x4000;
	uuid_pack(&uu, out);
}

#endif  /* _WIN32 */

/*
 * This is the generic front-end to uuid_generate_random and
 * uuid_generate_time.  It uses uuid_generate_random only if
 * /dev/urandom is available, since otherwise we won't have
 * high-quality randomness.
 */
void LIBNETXMS_EXPORTABLE _uuid_generate(uuid_t out)
{
#ifdef _WIN32
	UUID uuid;
	UuidCreate(&uuid);
	memcpy(out, &uuid, UUID_LENGTH);
#else
   uuid_generate_random(out);
#endif
}
