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

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#if HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

#if HAVE_NET_IF_H
#include <net/if.h>
#endif

#if HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif

#if HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif

#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#ifdef HAVE_SRANDOM
#define srand(x) srandom(x)
#define rand()   random()
#endif

/**
 * NULL GUID
 */
const uuid uuid::NULL_UUID = uuid();

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
	DWORD	time_low;
	WORD	time_mid;
	WORD	time_hi_and_version;
	WORD	clock_seq;
	BYTE	node[6];
};

/**
 * Internal routine for packing UUID's
 */
static void uuid_pack(struct __uuid *uu, uuid_t ptr)
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

/**
 * Internal routine for unpacking UUID
 */
static void uuid_unpack(const uuid_t in, struct __uuid *uu)
{
	const unsigned char *ptr = in;
	unsigned int tmp;

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
void LIBNETXMS_EXPORTABLE uuid_clear(uuid_t uu)
{
	memset(uu, 0, 16);
}

#define UUCMP(u1,u2) if (u1 != u2) return((u1 < u2) ? -1 : 1);

/**
 * compare whether or not two UUID's are the same
 *
 * Returns 1/-1 if the two UUID's are different, and 0 if they are the same.
 */
int LIBNETXMS_EXPORTABLE uuid_compare(const uuid_t uu1, const uuid_t uu2)
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
bool LIBNETXMS_EXPORTABLE uuid_is_null(const uuid_t uu)
{
	const unsigned char *cp;
	int i;

	for (i=0, cp = uu; i < 16; i++)
		if (*cp++)
			return false;
	return true;
}

/**
 * Parse UUID
 */
int LIBNETXMS_EXPORTABLE uuid_parse(const TCHAR *in, uuid_t uu)
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

/**
 * Convert packed UUID to string
 */
TCHAR LIBNETXMS_EXPORTABLE *uuid_to_string(const uuid_t uu, TCHAR *out)
{
	struct __uuid uuid;

	uuid_unpack(uu, &uuid);
	_sntprintf(out, 64,
		_T("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"),
		uuid.time_low, uuid.time_mid, uuid.time_hi_and_version,
		uuid.clock_seq >> 8, uuid.clock_seq & 0xFF,
		uuid.node[0], uuid.node[1], uuid.node[2],
		uuid.node[3], uuid.node[4], uuid.node[5]);
   return out;
}

#ifndef _WIN32

static int get_random_fd()
{
	int fd = -2;
	int	i;

	if (fd == -2)
   {
		fd = open("/dev/urandom", O_RDONLY);
		if (fd == -1)
			fd = open("/dev/random", O_RDONLY | O_NONBLOCK);
		srand((getpid() << 16) ^ getuid() ^ time(0));
	}
	/* Crank the random number generator a few times */
	for (i = time(0) & 0x1F; i > 0; i--)
		rand();
	return fd;
}

#endif

/*
 * Generate a series of random bytes.  Use /dev/urandom if possible,
 * and if not, use srandom/random.
 */
static void get_random_bytes(void *buf, int nbytes)
{
	int i;
	char *cp = (char *)buf;
#ifndef _WIN32
   int fd = get_random_fd();
	int lose_counter = 0;

	if (fd >= 0)
   {
		while (nbytes > 0)
      {
			i = read(fd, cp, nbytes);
			if ((i < 0) &&
			    ((errno == EINTR) || (errno == EAGAIN)))
				continue;
			if (i <= 0) {
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
#endif
	if (nbytes == 0)
		return;

	/* FIXME: put something better here if no /dev/random! */
   srand((unsigned int)time(NULL) ^ getpid());
	for (i = 0; i < nbytes; i++)
		*cp++ = rand() & 0xFF;
	return;
}

/*
 * Get the ethernet hardware address or other available node ID
 */
static int get_node_id(unsigned char *node_id)
{
#if defined(_AIX)

   struct utsname info;
   if (uname(&info) == -1)
      return -1;
   
   memset(node_id, 0, 6);
   StrToBinA(info.machine, node_id, 6);
   return 0;

#elif defined(HAVE_NET_IF_H) && !defined(sun) && !defined(__sun) && (defined(SIOCGIFHWADDR) || defined(SIOCGENADDR))

	int sd;
	struct ifreq ifr, *ifrp;
	struct ifconf ifc;
	char buf[1024];
	int n, i;
	unsigned char *a;
	
/*
 * BSD 4.4 defines the size of an ifreq to be
 * max(sizeof(ifreq), sizeof(ifreq.ifr_name)+ifreq.ifr_addr.sa_len
 * However, under earlier systems, sa_len isn't present, so the size is 
 * just sizeof(struct ifreq)
 */
#ifdef HAVE_SA_LEN
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#define ifreq_size(i) max(sizeof(struct ifreq),\
     sizeof((i).ifr_name)+(i).ifr_addr.sa_len)
#else
#define ifreq_size(i) sizeof(struct ifreq)
#endif /* HAVE_SA_LEN*/

#ifdef AF_INET6
	sd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_IP);
	if (sd == INVALID_SOCKET)
#endif
	{
		sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
		if (sd == INVALID_SOCKET)
		{
			return -1;
		}
	}
	memset(buf, 0, sizeof(buf));
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if (ioctl(sd, SIOCGIFCONF, (char *)&ifc) < 0)
	{
		close(sd);
		return -1;
	}
	n = ifc.ifc_len;
	for (i = 0; i < n; i+= ifreq_size(*ifr) ) {
		ifrp = (struct ifreq *)((char *) ifc.ifc_buf+i);
		strncpy(ifr.ifr_name, ifrp->ifr_name, IFNAMSIZ);
#if defined(SIOCGIFHWADDR)
		if (ioctl(sd, SIOCGIFHWADDR, &ifr) < 0)
			continue;
		a = (unsigned char *) &ifr.ifr_hwaddr.sa_data;
#elif defined(SIOCGENADDR)
		if (ioctl(sd, SIOCGENADDR, &ifr) < 0)
			continue;
		a = (unsigned char *) ifr.ifr_enaddr;
#else
		 // we don't have a way of getting the hardware address
		a = (unsigned char *)"\x00\x00\x00\x00\x00\x00";
		close(sd);
		return 0;
#endif /* SIOCGENADDR / SIOCGIFHWADDR */
		if (!a[0] && !a[1] && !a[2] && !a[3] && !a[4] && !a[5])
			continue;
		if (node_id)
      {
			memcpy(node_id, a, 6);
			close(sd);
			return 1;
		}
	}
	close(sd);
#endif
	return 0;
}

/* Assume that the gettimeofday() has microsecond granularity */
#define MAX_ADJUSTMENT 10

static int get_clock(DWORD *clock_high, DWORD *clock_low, WORD *ret_clock_seq)
{
	static int adjustment = 0;
	static struct timeval last = {0, 0};
	static unsigned short clock_seq;
	struct timeval tv;
	QWORD clock_reg;
#ifdef _WIN32
   FILETIME ft;
   ULARGE_INTEGER li;
   unsigned __int64 t;
#endif
	
try_again:
#ifdef _WIN32
   GetSystemTimeAsFileTime(&ft);
   li.LowPart  = ft.dwLowDateTime;
   li.HighPart = ft.dwHighDateTime;
   t = li.QuadPart;       // In 100-nanosecond intervals
   t /= 10;    // To microseconds
   tv.tv_sec = (long)(t / 1000000);
   tv.tv_usec = (long)(t % 1000000);
#else
	gettimeofday(&tv, NULL);
#endif
	if ((last.tv_sec == 0) && (last.tv_usec == 0))
   {
		get_random_bytes(&clock_seq, sizeof(clock_seq));
		clock_seq &= 0x1FFF;
		last = tv;
		last.tv_sec--;
	}
	if ((tv.tv_sec < last.tv_sec) ||
	    ((tv.tv_sec == last.tv_sec) &&
	     (tv.tv_usec < last.tv_usec)))
   {
		clock_seq = (clock_seq + 1) & 0x1FFF;
		adjustment = 0;
		last = tv;
	}
   else if ((tv.tv_sec == last.tv_sec) &&
	    (tv.tv_usec == last.tv_usec))
   {
		if (adjustment >= MAX_ADJUSTMENT)
			goto try_again;
		adjustment++;
	}
   else
   {
		adjustment = 0;
		last = tv;
	}
		
	clock_reg = tv.tv_usec * 10 + adjustment;
	clock_reg += ((QWORD)tv.tv_sec) * 10000000;
	clock_reg += (((QWORD)0x01B21DD2) << 32) + 0x13814000;

	*clock_high = (DWORD)(clock_reg >> 32);
	*clock_low = (DWORD)clock_reg;
	*ret_clock_seq = clock_seq;
	return 0;
}

static void uuid_generate_time(uuid_t out)
{
	static unsigned char node_id[6];
	static int has_init = 0;
	struct __uuid uu;
	DWORD clock_mid;

	if (!has_init)
   {
		if (get_node_id(node_id) <= 0)
      {
			get_random_bytes(node_id, 6);
			/*
			 * Set multicast bit, to prevent conflicts
			 * with IEEE 802 addresses obtained from
			 * network cards
			 */
			node_id[0] |= 0x80;
		}
		has_init = 1;
	}
	get_clock(&clock_mid, &uu.time_low, &uu.clock_seq);
	uu.clock_seq |= 0x8000;
	uu.time_mid = (WORD)clock_mid;
	uu.time_hi_and_version = (WORD)((clock_mid >> 16) | 0x1000);
	memcpy(uu.node, node_id, 6);
	uuid_pack(&uu, out);
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

/*
 * This is the generic front-end to uuid_generate_random and
 * uuid_generate_time.  It uses uuid_generate_random only if
 * /dev/urandom is available, since otherwise we won't have
 * high-quality randomness.
 */
void LIBNETXMS_EXPORTABLE uuid_generate(uuid_t out)
{
#ifdef _WIN32
	UUID uuid;

	UuidCreate(&uuid);
	memcpy(out, &uuid, UUID_LENGTH);
#else
	int fd;

	fd = get_random_fd();
	if (fd >= 0)
   {
		close(fd);
		uuid_generate_random(out);
	}
	else
	{
		uuid_generate_time(out);
	}
#endif
}
