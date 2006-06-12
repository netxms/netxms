/*
 ** Copyright (c) 1990- 1993, 1996 Open Software Foundation, Inc.
 ** Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, Ca. &
 ** Digital Equipment Corporation, Maynard, Mass.
 ** Copyright (c) 1998 Microsoft.
 ** To anyone who acknowledges that this file is provided "AS IS"
 ** without any express or implied warranty: permission to use, copy,
 ** modify, and distribute this file for any purpose is hereby
 ** granted without fee, provided that the above copyright notices and
 ** this notice appears in all source code copies, and that none of
 ** the names of Open Software Foundation, Inc., Hewlett-Packard
 ** Company, or Digital Equipment Corporation be used in advertising
 ** or publicity pertaining to distribution of the software without
 ** specific, written prior permission.  Neither Open Software
 ** Foundation, Inc., Hewlett-Packard Company, Microsoft, nor Digital Equipment
 ** Corporation makes any representations about the suitability of
 ** this software for any purpose.
 */

#include "libnetxms.h"
#include <uuid.h>

#ifndef _WIN32
#ifdef HAVE_SYS_SYSINFO_H
#  include <sys/sysinfo.h>
#else
#  include <unistd.h>
#endif
#endif

/* MD5 */
#include "md5.h"
#define MD5_CTX md5_state_t

/* set the following to the number of 100ns ticks of the actual
resolution of
your system's clock */
#define UUIDS_PER_TICK 1024

/* Set the following to a call to acquire a system wide global lock
*/
#define LOCK
#define UNLOCK

typedef QWORD uuid_time_t;
typedef struct {
  char nodeID[6];
} uuid_node_t;

/* various forward declarations */
static int read_state(WORD *clockseq, uuid_time_t *timestamp,
uuid_node_t * node);
static void write_state(WORD clockseq, uuid_time_t timestamp,
uuid_node_t node);
static void format_uuid_v1(uuid_t * uuid, WORD clockseq,
uuid_time_t timestamp, uuid_node_t node);
static void format_uuid_v3(uuid_t * uuid, unsigned char hash[16]);
static void get_current_time(uuid_time_t * timestamp);
static WORD true_random(void);


#ifdef _WIN32

static void get_system_time(uuid_time_t *uuid_time)
{
  ULARGE_INTEGER time;

  GetSystemTimeAsFileTime((FILETIME *)&time);

    /* NT keeps time in FILETIME format which is 100ns ticks since
     Jan 1, 1601.  UUIDs use time in 100ns ticks since Oct 15, 1582.
     The difference is 17 Days in Oct + 30 (Nov) + 31 (Dec)
     + 18 years and 5 leap days.
  */

    time.QuadPart +=
          (unsigned __int64) (1000*1000*10)       // seconds
        * (unsigned __int64) (60 * 60 * 24)       // days
        * (unsigned __int64) (17+30+31+365*18+5); // # of days

  *uuid_time = time.QuadPart;

};

static void get_random_info(char seed[16])
{
  MD5_CTX c;
  typedef struct {
      MEMORYSTATUS m;
      SYSTEM_INFO s;
      FILETIME t;
      LARGE_INTEGER pc;
      DWORD tc;
      DWORD l;
      char hostname[MAX_COMPUTERNAME_LENGTH + 1];
  } randomness;
  randomness r;

  I_md5_init(&c);
  /* memory usage stats */
  GlobalMemoryStatus(&r.m);
  /* random system stats */
  GetSystemInfo(&r.s);
  /* 100ns resolution (nominally) time of day */
  GetSystemTimeAsFileTime(&r.t);
  /* high resolution performance counter */
  QueryPerformanceCounter(&r.pc);
  /* milliseconds since last boot */
  r.tc = GetTickCount();
  r.l = MAX_COMPUTERNAME_LENGTH + 1;

  GetComputerName(r.hostname, &r.l );
  I_md5_append(&c, (md5_byte_t *)&r, sizeof(randomness));
  I_md5_finish(&c, seed);
};

#else

static void get_system_time(uuid_time_t *uuid_time)
{
    struct timeval tp;

    gettimeofday(&tp, (struct timezone *)0);

    /* Offset between UUID formatted times and Unix formatted times.
       UUID UTC base time is October 15, 1582.
       Unix base time is January 1, 1970.
    */
    *uuid_time = (tp.tv_sec * 10000000) + (tp.tv_usec * 10) +
      I64(0x01B21DD213814000);
};

static void get_random_info(char seed[16])
{
  MD5_CTX c;
  typedef struct {
#ifdef HAVE_SYS_SYSINFO_H
      struct sysinfo s;
#else
      long clock_tick;
      long open_max;
      long stream_max;
      long expr_nest_max;
      long bc_scale_max;
      long bc_string_max;
#endif
      struct timeval t;
      char hostname[257];
  } randomness;
  randomness r;

  I_md5_init(&c);

#ifdef HAVE_SYS_SYSINFO_H
  sysinfo(&r.s);
#else
  r.clock_tick    = sysconf(_SC_CLK_TCK);
  r.open_max      = sysconf(_SC_OPEN_MAX);
  r.stream_max    = sysconf(_SC_STREAM_MAX);
  r.expr_nest_max = sysconf(_SC_EXPR_NEST_MAX);
  r.bc_scale_max  = sysconf(_SC_BC_SCALE_MAX);
  r.bc_string_max = sysconf(_SC_BC_STRING_MAX);
#endif

  gettimeofday(&r.t, (struct timezone *)0);
  gethostname(r.hostname, 256);
  I_md5_append(&c, (md5_byte_t *)&r, sizeof(randomness));
  I_md5_finish(&c, seed);
};

#endif


static void get_node_identifier(uuid_node_t *node)
{
  char seed[16];
  FILE * fd;
  static inited = 0;
  static uuid_node_t saved_node;

  if (!inited)
  {
      fd = fopen("nodeid", "rb");
      if (fd)
      {
           fread(&saved_node, sizeof(uuid_node_t), 1, fd);
           fclose(fd);
      }
      else
      {
           get_random_info(seed);
           seed[0] |= 0x80;
           memcpy(&saved_node, seed, sizeof(uuid_node_t));
           fd = fopen("nodeid", "wb");
           if (fd) {
                  fwrite(&saved_node, sizeof(uuid_node_t), 1, fd);
                  fclose(fd);
           };
      };
      inited = 1;
  };

  *node = saved_node;
};


/* uuid_create -- generator a UUID */
int LIBNETXMS_EXPORTABLE uuid_create(uuid_t * uuid)
{
  uuid_time_t timestamp, last_time;
  WORD clockseq;
  uuid_node_t node;
  uuid_node_t last_node;
  int f;

  /* acquire system wide lock so we're alone */
  LOCK;

  /* get current time */
  get_current_time(&timestamp);

  /* get node ID */
  get_node_identifier(&node);

  /* get saved state from NV storage */
  f = read_state(&clockseq, &last_time, &last_node);

  /* if no NV state, or if clock went backwards, or node ID changed
     (e.g., net card swap) change clockseq */
  if (!f || memcmp(&node, &last_node, sizeof(uuid_node_t)))
      clockseq = true_random();
  else if (timestamp < last_time)
      clockseq++;

  /* stuff fields into the UUID */
  format_uuid_v1(uuid, clockseq, timestamp, node);

  /* save the state for next time */
  write_state(clockseq, timestamp, node);

  UNLOCK;
  return(1);
};

/* format_uuid_v1 -- make a UUID from the timestamp, clockseq,
                     and node ID */
static void format_uuid_v1(uuid_t * uuid, WORD clock_seq, uuid_time_t
timestamp, uuid_node_t node) {
    /* Construct a version 1 uuid with the information we've gathered
     * plus a few constants. */
  uuid->time_low = (DWORD)(timestamp & 0xFFFFFFFF);
    uuid->time_mid = (WORD)((timestamp >> 32) & 0xFFFF);
    uuid->time_hi_and_version = (WORD)((timestamp >> 48) &
       0x0FFF);
    uuid->time_hi_and_version |= (1 << 12);
    uuid->clock_seq_low = clock_seq & 0xFF;
    uuid->clock_seq_hi_and_reserved = (clock_seq & 0x3F00) >> 8;
    uuid->clock_seq_hi_and_reserved |= 0x80;
    memcpy(&uuid->node, &node, sizeof uuid->node);
};

/* data type for UUID generator persistent state */
typedef struct {
  uuid_time_t ts;       /* saved timestamp */
  uuid_node_t node;     /* saved node ID */
  WORD cs;        /* saved clock sequence */
  } uuid_state;

static uuid_state st;

/* read_state -- read UUID generator state from non-volatile store */
static int read_state(WORD *clockseq, uuid_time_t *timestamp,
                      uuid_node_t *node) 
{
   FILE * fd;
   static int inited = 0;

   /* only need to read state once per boot */
   if (!inited)
   {
      fd = fopen("state", "rb");
      if (!fd)
         return 0;
      fread(&st, sizeof(uuid_state), 1, fd);
      fclose(fd);
      inited = 1;
   }
   *clockseq = st.cs;
   *timestamp = st.ts;
   *node = st.node;
   return 1;
};

/* write_state -- save UUID generator state back to non-volatile storage */
static void write_state(WORD clockseq, uuid_time_t timestamp,
                        uuid_node_t node)
{
  FILE * fd;
  static int inited = 0;
  static uuid_time_t next_save;

  if (!inited)
  {
      next_save = timestamp;
      inited = 1;
  };
  /* always save state to volatile shared state */
  st.cs = clockseq;
  st.ts = timestamp;
  st.node = node;
  if (timestamp >= next_save)
  {
      fd = fopen("state", "wb");
      fwrite(&st, sizeof(uuid_state), 1, fd);
      fclose(fd);
      /* schedule next save for 10 seconds from now */
      next_save = timestamp + (10 * 10 * 1000 * 1000);
  };
};

/* get-current_time -- get time as 60 bit 100ns ticks since whenever.
  Compensate for the fact that real clock resolution is
  less than 100ns. */
static void get_current_time(uuid_time_t * timestamp)
{
    uuid_time_t                time_now;
    static uuid_time_t  time_last;
    static WORD   uuids_this_tick;
  static int                   inited = 0;

  if (!inited) 
  {
      get_system_time(&time_now);
      uuids_this_tick = UUIDS_PER_TICK;
      inited = 1;
  };

    while (1) {
        get_system_time(&time_now);

      /* if clock reading changed since last UUID generated... */
        if (time_last != time_now) {
           /* reset count of uuids gen'd with this clock reading */
            uuids_this_tick = 0;
           break;
      };
        if (uuids_this_tick < UUIDS_PER_TICK) {
           uuids_this_tick++;
           break;
      };
      /* going too fast for our clock; spin */
    };
  /* add the count of uuids to low order bits of the clock reading */
  *timestamp = time_now + uuids_this_tick;
};

/* true_random -- generate a crypto-quality random number.
   This sample doesn't do that. */
static WORD true_random(void)
{
  static int inited = 0;
  uuid_time_t time_now;

  if (!inited) {
      get_system_time(&time_now);
      time_now = time_now/UUIDS_PER_TICK;
      srand((unsigned int)(((time_now >> 32) ^ time_now)&0xffffffff));
      inited = 1;
  };

    return (rand());
}

/* uuid_create_from_name -- create a UUID using a "name" from a "name
space" */
void LIBNETXMS_EXPORTABLE uuid_create_from_name(
  uuid_t * uuid,        /* resulting UUID */
  uuid_t nsid,          /* UUID to serve as context, so identical
                           names from different name spaces generate
                           different UUIDs */
  void * name,          /* the name from which to generate a UUID */
  int namelen           /* the length of the name */
) {
  MD5_CTX c;
  unsigned char hash[16];
  uuid_t net_nsid;      /* context UUID in network byte order */

  /* put name space ID in network byte order so it hashes the same
      no matter what endian machine we're on */
  net_nsid = nsid;
  htonl(net_nsid.time_low);
  htons(net_nsid.time_mid);
  htons(net_nsid.time_hi_and_version);

  I_md5_init(&c);
  I_md5_append(&c, (md5_byte_t *)&net_nsid, sizeof(uuid_t));
  I_md5_append(&c, name, namelen);
  I_md5_finish(&c, hash);

  /* the hash is in network byte order at this point */
  format_uuid_v3(uuid, hash);
};

/* format_uuid_v3 -- make a UUID from a (pseudo)random 128 bit number
*/
static void format_uuid_v3(uuid_t * uuid, unsigned char hash[16]) {
    /* Construct a version 3 uuid with the (pseudo-)random number
     * plus a few constants. */

    memcpy(uuid, hash, sizeof(uuid_t));

  /* convert UUID to local byte order */
  ntohl(uuid->time_low);
  ntohs(uuid->time_mid);
  ntohs(uuid->time_hi_and_version);

  /* put in the variant and version bits */
    uuid->time_hi_and_version &= 0x0FFF;
    uuid->time_hi_and_version |= (3 << 12);
    uuid->clock_seq_hi_and_reserved &= 0x3F;
    uuid->clock_seq_hi_and_reserved |= 0x80;
};

/* uuid_compare --  Compare two UUID's "lexically" and return
       -1   u1 is lexically before u2
        0   u1 is equal to u2
        1   u1 is lexically after u2

    Note:   lexical ordering is not temporal ordering!
*/
int LIBNETXMS_EXPORTABLE uuid_compare(const uuid_t *u1, const uuid_t *u2)
{
  int i;

#define CHECK(f1, f2) if (f1 != f2) return f1 < f2 ? -1 : 1;
  CHECK(u1->time_low, u2->time_low);
  CHECK(u1->time_mid, u2->time_mid);
  CHECK(u1->time_hi_and_version, u2->time_hi_and_version);
  CHECK(u1->clock_seq_hi_and_reserved, u2->clock_seq_hi_and_reserved);
  CHECK(u1->clock_seq_low, u2->clock_seq_low)
  for (i = 0; i < 6; i++) {
      if (u1->node[i] < u2->node[i])
           return -1;
      if (u1->node[i] > u2->node[i])
      return 1;
    }
  return 0;
};
