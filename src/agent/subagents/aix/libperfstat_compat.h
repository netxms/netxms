/* This is older version of libperfstat.h to keep binary compatibility with older AIX 6.1 */

/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* bos61J src/bos/usr/ccs/lib/libperfstat/libperfstat.h 1.4.4.21          */
/*                                                                        */
/* Licensed Materials - Property of IBM                                   */
/*                                                                        */
/* Restricted Materials of IBM                                            */
/*                                                                        */
/* COPYRIGHT International Business Machines Corp. 2000,2009              */
/* All Rights Reserved                                                    */
/*                                                                        */
/* US Government Users Restricted Rights - Use, duplication or            */
/* disclosure restricted by GSA ADP Schedule Contract with IBM Corp.      */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */
/*
 * @(#)91       1.4.4.21     src/bos/usr/ccs/lib/libperfstat/libperfstat.h, libperfstat, bos61J, 0931A_61J 7/9/09 01:45:56
 * LEVEL 1,  5 Years Bull Confidential Information
 *
 */
#ifndef LIBPERFSTAT_H
#define LIBPERFSTAT_H

/* This file describes the structures and constants used by the libperfstat API */

#include <sys/types.h>
#include <sys/corral.h>
#include <sys/corralid.h>
#include <sys/rset.h>
#ifdef __cplusplus
extern "C" {
#endif

#define IDENTIFIER_LENGTH 64    /* length of strings included in the structures */
#define PERFSTAT_ENABLE 0x80000000
#define PERFSTAT_DISABLE 0x40000000	
#define PERFSTAT_RESET 0x20000000
#define PERFSTAT_QUERY 0x10000000
#define PERFSTAT_IOSTAT 0x00000001
#define PERFSTAT_LV 0x00000002
#define PERFSTAT_VG 0x00000003
#define PERFSTAT_DICTIONARY 0x00000004

/*definitions moved  from libperfstat_lv.h*/
#define FIRST_LOGICALVOLUME  ""            /* pseudo-name for the first logical volume */
#define FIRST_VOLUMEGROUP    ""            /* pseudo-name for the first volume group */



#define FIRST_CPU  ""           /* pseudo-name for the first logical cpu */
#define FIRST_DISK  ""          /* pseudo-name for the first disk */
#define FIRST_DISKPATH ""       /* pseudo-name for the first disk path */
#define FIRST_DISKADAPTER  ""   /* pseudo-name for the first disk adapter */
#define FIRST_NETINTERFACE  ""  /* pseudo-name for the first network interface */
#define FIRST_PAGINGSPACE  ""   /* pseudo-name for the first paging space */
#define FIRST_PROTOCOL  ""      /* pseudo-name for the first protocol */
#define FIRST_NETBUFFER  ""     /* pseudo-name for the first network buffer size */
#define FIRST_PSIZE    (0ull)   /* Pseudo-name for the first memory page-size */

#define FIRST_WPARNAME ""       /* pseudo-name for the first WPAR */
#define FIRST_WPARID -1	        /* pseudo-id for the first WPAR */

#define DEFAULT_DEF  "not available"
#define XINTFRAC     ((double)(_system_configuration.Xint)/(double)(_system_configuration.Xfrac))
#define HTIC2NANOSEC(x)  (((double)(x) * XINTFRAC))
typedef struct { /* structure element identifier */
	char name[IDENTIFIER_LENGTH]; /* name of the identifier */
} perfstat_id_t;

typedef enum { WPARNAME, WPARID, RSETHANDLE } wparid_specifier;

typedef struct { /* WPAR identifier */
	wparid_specifier spec; /* Specifier to choose wpar id or name */
	union  {
		cid_t wpar_id; /* WPAR ID */
		rsethandle_t rset; /* Rset Handle */
		char wparname[MAXCORRALNAMELEN+1]; /* WPAR NAME */
	} u;
	char name[IDENTIFIER_LENGTH]; /* name of the structure element identifier */
} perfstat_id_wpar_t;
	

typedef struct { /* cpu information */
	char name[IDENTIFIER_LENGTH]; /* logical processor name (cpu0, cpu1, ..) */
	u_longlong_t user;     /* raw number of clock ticks spent in user mode */
	u_longlong_t sys;      /* raw number of clock ticks spent in system mode */
	u_longlong_t idle;     /* raw number of clock ticks spent idle */
	u_longlong_t wait;     /* raw number of clock ticks spent waiting for I/O */
	u_longlong_t pswitch;  /* number of context switches (changes of currently running process) */
	u_longlong_t syscall;  /* number of system calls executed */
	u_longlong_t sysread;  /* number of read system calls executed */
	u_longlong_t syswrite; /* number of write system calls executed */
	u_longlong_t sysfork;  /* number of fork system call executed */
	u_longlong_t sysexec;  /* number of exec system call executed */
	u_longlong_t readch;   /* number of characters tranferred with read system call */
	u_longlong_t writech;  /* number of characters tranferred with write system call */
	u_longlong_t bread;    /* number of block reads */
	u_longlong_t bwrite;   /* number of block writes */
	u_longlong_t lread;    /* number of logical read requests */
	u_longlong_t lwrite;   /* number of logical write requests */
	u_longlong_t phread;   /* number of physical reads (reads on raw device) */
	u_longlong_t phwrite;  /* number of physical writes (writes on raw device) */
	u_longlong_t iget;     /* number of inode lookups */
	u_longlong_t namei;    /* number of vnode lookup from a path name */
	u_longlong_t dirblk;   /* number of 512-byte block reads by the directory search routine to locate an entry for a file */
	u_longlong_t msg;      /* number of IPC message operations */
	u_longlong_t sema;     /* number of IPC semaphore operations  */
	u_longlong_t minfaults;		/* number of page faults with no I/O */
	u_longlong_t majfaults;		/* number of page faults with disk I/O */
	u_longlong_t puser;		/* raw number of physical processor tics in user mode */
	u_longlong_t psys;		/* raw number of physical processor tics in system mode */
	u_longlong_t pidle;		/* raw number of physical processor tics idle */
	u_longlong_t pwait;		/* raw number of physical processor tics waiting for I/O */
	u_longlong_t redisp_sd0;	/* number of thread redispatches within the scheduler affinity domain 0 */
	u_longlong_t redisp_sd1;	/* number of thread redispatches within the scheduler affinity domain 1 */
	u_longlong_t redisp_sd2;	/* number of thread redispatches within the scheduler affinity domain 2 */
	u_longlong_t redisp_sd3;	/* number of thread redispatches within the scheduler affinity domain 3 */
	u_longlong_t redisp_sd4;	/* number of thread redispatches within the scheduler affinity domain 4 */
	u_longlong_t redisp_sd5;	/* number of thread redispatches within the scheduler affinity domain 5 */
	u_longlong_t migration_push;	/* number of thread migrations from the local runque to another queue due to starvation load balancing */
	u_longlong_t migration_S3grq;	/* number of  thread migrations from the global runque to the local runque resulting in a move accross scheduling domain 3 */
	u_longlong_t migration_S3pul;	/* number of  thread migrations from another processor's runque resulting in a move accross scheduling domain 3 */
	u_longlong_t invol_cswitch;	/* number of  involuntary thread context switches */
	u_longlong_t vol_cswitch;	/* number of  voluntary thread context switches */
	u_longlong_t runque;		/* number of  threads on the runque */
	u_longlong_t bound;		/* number of  bound threads */
	u_longlong_t decrintrs;		/* number of decrementer tics interrupts */
	u_longlong_t mpcrintrs;		/* number of mpc's received interrupts */
	u_longlong_t mpcsintrs;		/* number of mpc's sent interrupts */
	u_longlong_t devintrs;		/* number of device interrupts */
	u_longlong_t softintrs;		/* number of offlevel handlers called */
	u_longlong_t phantintrs;	/* number of phantom interrupts */
	u_longlong_t idle_donated_purr; /* number of idle cycles donated by a dedicated partition enabled for donation */  
	u_longlong_t idle_donated_spurr;/* number of idle spurr cycles donated by a dedicated partition enabled for donation */
	u_longlong_t busy_donated_purr; /* number of busy cycles donated by a dedicated partition enabled for donation */
	u_longlong_t busy_donated_spurr;/* number of busy spurr cycles donated by a dedicated partition enabled for donation */
	u_longlong_t idle_stolen_purr;  /* number of idle cycles stolen by the hypervisor from a dedicated partition */
	u_longlong_t idle_stolen_spurr; /* number of idle spurr cycles stolen by the hypervisor from a dedicated partition */
	u_longlong_t busy_stolen_purr;  /* number of busy cycles stolen by the hypervisor from a dedicated partition */
	u_longlong_t busy_stolen_spurr; /* number of busy spurr cycles stolen by the hypervisor from a dedicated partition */
	u_longlong_t hpi; /* number of hypervisor page-ins */
	u_longlong_t hpit; /* Time spent in hypervisor page-ins (in nanoseconds)*/
        u_longlong_t puser_spurr;        /* number of spurr cycles spent in user mode */
        u_longlong_t psys_spurr;        /* number of spurr cycles spent in kernel mode */
        u_longlong_t pidle_spurr;        /* number of spurr cycles spent in idle mode */
        u_longlong_t pwait_spurr;        /* number of spurr cycles spent in wait mode */
        int spurrflag;                  /* set if running in spurr mode */

	/* home SRAD redispatch statistics */
	u_longlong_t localdispatch;		/* number of local thread dispatches on this logical CPU */
	u_longlong_t neardispatch;		/* number of near thread dispatches on this logical CPU */
	u_longlong_t fardispatch;		/* number of far thread dispatches on this logical CPU */
} perfstat_cpu_t;

typedef struct { /* global cpu information */
	int ncpus;                /* number of active logical processors */
	int ncpus_cfg;	           /* number of configured processors */
	char description[IDENTIFIER_LENGTH]; /* processor description (type/official name) */
	u_longlong_t processorHZ; /* processor speed in Hz */
	u_longlong_t user;        /*  raw total number of clock ticks spent in user mode */
	u_longlong_t sys;         /* raw total number of clock ticks spent in system mode */
	u_longlong_t idle;        /* raw total number of clock ticks spent idle */
	u_longlong_t wait;        /* raw total number of clock ticks spent waiting for I/O */
	u_longlong_t pswitch;     /* number of process switches (change in currently running process) */
	u_longlong_t syscall;     /* number of system calls executed */
	u_longlong_t sysread;     /* number of read system calls executed */
	u_longlong_t syswrite;    /* number of write system calls executed */
	u_longlong_t sysfork;     /* number of forks system calls executed */
	u_longlong_t sysexec;     /* number of execs system calls executed */
	u_longlong_t readch;      /* number of characters tranferred with read system call */
	u_longlong_t writech;     /* number of characters tranferred with write system call */
	u_longlong_t devintrs;    /* number of device interrupts */
	u_longlong_t softintrs;   /* number of software interrupts */
	time_t lbolt;	           /* number of ticks since last reboot */
	u_longlong_t loadavg[3];  /* (1<<SBITS) times the average number of runnables processes during the last 1, 5 and 15 minutes.    */
	/* To calculate the load average, divide the numbers by (1<<SBITS). SBITS is defined in <sys/proc.h>. */
	u_longlong_t runque;      /* length of the run queue (processes ready) */
	u_longlong_t swpque;      /* ength of the swap queue (processes waiting to be paged in) */
	u_longlong_t bread;       /* number of blocks read */
	u_longlong_t bwrite;      /* number of blocks written */
	u_longlong_t lread;       /* number of logical read requests */
	u_longlong_t lwrite;      /* number of logical write requests */
	u_longlong_t phread;      /* number of physical reads (reads on raw devices) */
	u_longlong_t phwrite;     /* number of physical writes (writes on raw devices) */
	u_longlong_t runocc;      /* updated whenever runque is updated, i.e. the runqueue is occupied.
							   * This can be used to compute the simple average of ready processes  */
	u_longlong_t swpocc;      /* updated whenever swpque is updated. i.e. the swpqueue is occupied.
							   * This can be used to compute the simple average processes waiting to be paged in */
	u_longlong_t iget;        /* number of inode lookups */
	u_longlong_t namei;       /* number of vnode lookup from a path name */
	u_longlong_t dirblk;      /* number of 512-byte block reads by the directory search routine to locate an entry for a file */
	u_longlong_t msg;         /* number of IPC message operations */
	u_longlong_t sema;        /* number of IPC semaphore operations */
	u_longlong_t rcvint;      /* number of tty receive interrupts */
	u_longlong_t xmtint;      /* number of tyy transmit interrupts */
	u_longlong_t mdmint;      /* number of modem interrupts */
	u_longlong_t tty_rawinch; /* number of raw input characters  */
	u_longlong_t tty_caninch; /* number of canonical input characters (always zero) */
	u_longlong_t tty_rawoutch;/* number of raw output characters */
	u_longlong_t ksched;      /* number of kernel processes created */
	u_longlong_t koverf;      /* kernel process creation attempts where:
							   * -the user has forked to their maximum limit
							   * -the configuration limit of processes has been reached */
	u_longlong_t kexit;       /* number of kernel processes that became zombies */
	u_longlong_t rbread;      /* number of remote read requests */
	u_longlong_t rcread;      /* number of cached remote reads */
	u_longlong_t rbwrt;       /* number of remote writes */
	u_longlong_t rcwrt;       /* number of cached remote writes */
	u_longlong_t traps;       /* number of traps */
	int ncpus_high;           /* index of highest processor online */
	u_longlong_t puser;       /* raw number of physical processor tics in user mode */
	u_longlong_t psys;        /* raw number of physical processor tics in system mode */
	u_longlong_t pidle;       /* raw number of physical processor tics idle */
	u_longlong_t pwait;       /* raw number of physical processor tics waiting for I/O */
	u_longlong_t decrintrs;   /* number of decrementer tics interrupts */
	u_longlong_t mpcrintrs;   /* number of mpc's received interrupts */
	u_longlong_t mpcsintrs;   /* number of mpc's sent interrupts */
	u_longlong_t phantintrs;  /* number of phantom interrupts */
	u_longlong_t idle_donated_purr; /* number of idle cycles donated by a dedicated partition enabled for donation */ 
	u_longlong_t idle_donated_spurr;/* number of idle spurr cycles donated by a dedicated partition enabled for donation */
	u_longlong_t busy_donated_purr; /* number of busy cycles donated by a dedicated partition enabled for donation */
	u_longlong_t busy_donated_spurr;/* number of busy spurr cycles donated by a dedicated partition enabled for donation */
	u_longlong_t idle_stolen_purr;  /* number of idle cycles stolen by the hypervisor from a dedicated partition */
	u_longlong_t idle_stolen_spurr; /* number of idle spurr cycles stolen by the hypervisor from a dedicated partition */
	u_longlong_t busy_stolen_purr;  /* number of busy cycles stolen by the hypervisor from a dedicated partition */ 
	u_longlong_t busy_stolen_spurr; /* number of busy spurr cycles stolen by the hypervisor from a dedicated partition */
	short iowait;  /* number of processes that are asleep waiting for buffered I/O */
	short physio;  /* number of processes waiting for raw I/O */
	longlong_t twait;     /* number of threads that are waiting for filesystem direct(cio) */
	u_longlong_t hpi; /* number of hypervisor page-ins */
	u_longlong_t hpit; /* Time spent in hypervisor page-ins (in nanoseconds) */
        u_longlong_t puser_spurr;        /* number of spurr cycles spent in user mode */
        u_longlong_t psys_spurr;        /* number of spurr cycles spent in kernel mode */
        u_longlong_t pidle_spurr;        /* number of spurr cycles spent in idle mode */
        u_longlong_t pwait_spurr;        /* number of spurr cycles spent in wait mode */
        int spurrflag;                  /* set if running in spurr mode */

} perfstat_cpu_total_t;

typedef struct { /* WPAR cpu total information */
        int ncpus;                /* number of active logical processors in Global */
        char description[IDENTIFIER_LENGTH]; /* processor description (type/official name) */
        u_longlong_t processorHZ; /* processor speed in Hz */
        u_longlong_t pswitch;     /* number of process switches (change in currently running process) */
	u_longlong_t sysfork;     /* number of forks system calls executed */
        u_longlong_t loadavg[3];  /* (1<<SBITS) times the average number of runnables processes during the last 1, 5 and 15 minutes.    */
                                  /* To calculate the load average, divide the numbers by (1<<SBITS). SBITS is defined in <sys/proc.h>. */
	u_longlong_t runque;      /* length of the run queue (processes ready) */
        u_longlong_t swpque;      /* ength of the swap queue (processes waiting to be paged in) */
	u_longlong_t runocc;      /* updated whenever runque is updated, i.e. the runqueue is occupied.
                                   * This can be used to compute the simple average of ready processes  */
        u_longlong_t swpocc;      /* updated whenever swpque is updated. i.e. the swpqueue is occupied.
				   * This can be used to compute the simple average processes waiting to be paged in */
        u_longlong_t puser;       /* raw number of physical processor tics in user mode */
        u_longlong_t psys;        /* raw number of physical processor tics in system mode */
        u_longlong_t pidle;       /* raw number of physical processor tics idle */
        u_longlong_t pwait;       /* raw number of physical processor tics waiting for I/O */
	int ncpus_cfg;             /* number of configured processors in the system*/
	u_longlong_t syscall;     /* number of system calls executed */
        u_longlong_t sysread;     /* number of read system calls executed */
        u_longlong_t syswrite;    /* number of write system calls executed */
        u_longlong_t sysexec;     /* number of execs system calls executed */
        u_longlong_t readch;      /* number of characters tranferred with read system call */
        u_longlong_t writech;     /* number of characters tranferred with write system call */
        u_longlong_t devintrs;    /* number of device interrupts */
        u_longlong_t softintrs;   /* number of software interrupts */
	u_longlong_t bread;       /* number of blocks read */
        u_longlong_t bwrite;      /* number of blocks written */
        u_longlong_t lread;       /* number of logical read requests */
        u_longlong_t lwrite;      /* number of logical write requests */
        u_longlong_t phread;      /* number of physical reads (reads on raw devices) */
        u_longlong_t phwrite;     /* number of physical writes (writes on raw devices) */
        u_longlong_t iget;        /* number of inode lookups */
        u_longlong_t namei;       /* number of vnode lookup from a path name */
        u_longlong_t dirblk;      /* number of 512-byte block reads by the directory search routine to locate an entry for a file */
        u_longlong_t msg;         /* number of IPC message operations */
        u_longlong_t sema;        /* number of IPC semaphore operations */
	u_longlong_t ksched;      /* number of kernel processes created */
        u_longlong_t koverf;      /* kernel process creation attempts where:
                                   * -the user has forked to their maximum limit
                                   * -the configuration limit of processes has been reached */
        u_longlong_t kexit;       /* number of kernel processes that became zombies */
} perfstat_cpu_total_wpar_t;

typedef struct { /* disk information */
	char name[IDENTIFIER_LENGTH];        /* name of the disk */
	char description[IDENTIFIER_LENGTH]; /* disk description (from ODM) */
	char vgname[IDENTIFIER_LENGTH];      /* volume group name (from ODM) */
	u_longlong_t size;              /* size of the disk (in MB) */
	u_longlong_t free;              /* free portion of the disk (in MB) */
	u_longlong_t bsize;             /* disk block size (in bytes) */
	u_longlong_t xrate;             /* OBSOLETE: xrate capability */
#define __rxfers xrate                  /* number of transfers from disk */
	u_longlong_t xfers;             /* number of transfers to/from disk */
	u_longlong_t wblks;             /* number of blocks written to disk */
	u_longlong_t rblks;             /* number of blocks read from disk */
	u_longlong_t qdepth;            /* instantaneous "service" queue depth
	              (number of requests sent to disk and not completed yet) */
	u_longlong_t time;              /* amount of time disk is active */
	char adapter[IDENTIFIER_LENGTH];/* disk adapter name */
	uint paths_count;               /* number of paths to this disk */
	u_longlong_t  q_full;	        /* "service" queue full occurrence count
	         (number of times the disk is not accepting any more request) */
	u_longlong_t  rserv;	        /* read or receive service time */
	u_longlong_t  rtimeout;         /* number of read request timeouts */
	u_longlong_t  rfailed;          /* number of failed read requests */
	u_longlong_t  min_rserv;        /* min read or receive service time */
	u_longlong_t  max_rserv;        /* max read or receive service time */
	u_longlong_t  wserv;	        /* write or send service time */
	u_longlong_t  wtimeout;         /* number of write request timeouts */
	u_longlong_t  wfailed;          /* number of failed write requests */
	u_longlong_t  min_wserv;        /* min write or send service time */
	u_longlong_t  max_wserv;        /* max write or send service time */
	u_longlong_t  wq_depth;         /* instantaneous wait queue depth
	                      (number of requests waiting to be sent to disk) */
	u_longlong_t  wq_sampled;       /* accumulated sampled dk_wq_depth */
	u_longlong_t  wq_time;          /* accumulated wait queueing time */
	u_longlong_t  wq_min_time;      /* min wait queueing time */
	u_longlong_t  wq_max_time;      /* max wait queueing time */
	u_longlong_t  q_sampled;        /* accumulated sampled dk_q_depth */
    cid_t         wpar_id;          /* WPAR identifier.*/ 
                                /* Pad of 3 short is available here*/
} perfstat_disk_t;

typedef struct { /* global disk information */
	int number;          /* total number of disks */
	u_longlong_t size;   /* total size of all disks (in MB) */
	u_longlong_t free;   /* free portion of all disks (in MB) */
	u_longlong_t xrate;  /* __rxfers: total number of transfers from disk */
	u_longlong_t xfers;  /* total number of transfers to/from disk */
	u_longlong_t wblks;  /* 512 bytes blocks written to all disks */
	u_longlong_t rblks;  /* 512 bytes blocks read from all disks */
	u_longlong_t time;   /* amount of time disks are active */
    cid_t   wpar_id;     /* WPAR identifier.*/ 
                    /* Pad of 3 short is available here*/
} perfstat_disk_total_t;

typedef struct { /* Disk adapter information */
	char name[IDENTIFIER_LENGTH];        /* name of the adapter (from ODM)*/
	char description[IDENTIFIER_LENGTH]; /* adapter description (from ODM)*/
	int number;          /* number of disks connected to adapter */
	u_longlong_t size;   /* total size of all disks (in MB)  */
	u_longlong_t free;   /* free portion of all disks (in MB)  */
	u_longlong_t xrate;  /* __rxfers: total number of reads via adapter */
	u_longlong_t xfers;  /* total number of transfers via adapter */
	u_longlong_t rblks;  /* 512 bytes blocks written via adapter */
	u_longlong_t wblks;  /* 512 bytes blocks read via adapter  */
	u_longlong_t time;   /* amount of time disks are active */
} perfstat_diskadapter_t;

typedef struct { /* mpio information */
	char name[IDENTIFIER_LENGTH];        /* name of the path */
	u_longlong_t xrate;  /* __rxfers: number of reads via the path */
	u_longlong_t xfers;  /* number of transfers via the path */
	u_longlong_t rblks;  /* 512 bytes blocks written via the path */
	u_longlong_t wblks;  /* 512 bytes blocks read via the path  */
	u_longlong_t time;   /* amount of time disks are active */
	char adapter[IDENTIFIER_LENGTH]; /* disk adapter name (from ODM) */
	u_longlong_t  q_full;	        /* "service" queue full occurrence count
                (number of times the disk is not accepting any more request) */
	u_longlong_t  rserv;	        /* read or receive service time */
	u_longlong_t  rtimeout;         /* number of read request timeouts */
	u_longlong_t  rfailed;          /* number of failed read requests */
	u_longlong_t  min_rserv;        /* min read or receive service time */
	u_longlong_t  max_rserv;        /* max read or receive service time */
	u_longlong_t  wserv;	        /* write or send service time */
	u_longlong_t  wtimeout;         /* number of write request timeouts */
	u_longlong_t  wfailed;          /* number of failed write requests */
	u_longlong_t  min_wserv;        /* min write or send service time */
	u_longlong_t  max_wserv;        /* max write or send service time */
	u_longlong_t  wq_depth;         /* instantaneous wait queue depth
                             (number of requests waiting to be sent to disk) */
	u_longlong_t  wq_sampled;       /* accumulated sampled dk_wq_depth */
	u_longlong_t  wq_time;          /* accumulated wait queueing time */
	u_longlong_t  wq_min_time;      /* min wait queueing time */
	u_longlong_t  wq_max_time;      /* max wait queueing time */
	u_longlong_t  q_sampled;        /* accumulated sampled dk_q_depth */
} perfstat_diskpath_t;
typedef struct {                         /* Page size identifier */
#define PAGE_4K   (4*1024ull)            /*  4K page-size        */
#define PAGE_64K  (64*1024ull)           /* 64K page-size        */
#define PAGE_16M  (16*1024*1024ull)      /* 16M page-size        */
#define PAGE_16G  (16*1024*1024*1024ull) /* 16G page-size        */

    psize_t psize;                       /* Page size in bytes   */
} perfstat_psize_t;

typedef struct {                /* Per page size memory statistics                */
    psize_t      psize;         /* page size in bytes                             */
    u_longlong_t real_total;    /* number of real memory frames of this page size */
    u_longlong_t real_free;     /* number of pages on free list                   */
    u_longlong_t real_pinned;   /* number of pages pinned                         */
    u_longlong_t real_inuse;    /* number of pages in use                         */
    u_longlong_t pgexct;        /* number of page faults                          */
    u_longlong_t pgins;         /* number of pages paged in                       */
    u_longlong_t pgouts;        /* number of pages paged out                      */
    u_longlong_t pgspins;       /* number of page ins from paging space           */
    u_longlong_t pgspouts;      /* number of page outs from paging space          */
    u_longlong_t scans;         /* number of page scans by clock                  */
    u_longlong_t cycles;        /* number of page replacement cycles              */
    u_longlong_t pgsteals;      /* number of page steals                          */
    u_longlong_t numperm;       /* number of frames used for files                */
    u_longlong_t numpgsp;       /* number of pages with allocated paging space    */
    u_longlong_t real_system;   /* number of pages used by system segments.                                   *
                                 * This is the sum of all the used pages in segment marked for system usage.  *
                                 * Since segment classifications are not always guaranteed to be accurate,    *
                                 * This number is only an approximation.                                      */
    u_longlong_t real_user;     /* number of pages used by non-system segments.                               *
                                 * This is the sum of all pages used in segments not marked for system usage. *
                                 * Since segment classifications are not always guaranteed to be accurate,    *
                                 * This number is only an approximation.                                      */
    u_longlong_t real_process;  /* number of pages used by process segments.                                  *
                                 * This is real_total - real_free - numperm - real_system.                    *
                                 * Since real_system is an approximation, this number is too.                 */
    u_longlong_t virt_active;   /* Active virtual pages.                                                      *
                                 * Virtual pages are considered active if they have been accessed             */
} perfstat_memory_page_t;

typedef struct {                /* page-size statistics instrumented for wpars    */
    psize_t      psize;         /* page size in bytes                             */
    u_longlong_t real_total;    /* number of real memory frames of this page size */
    u_longlong_t real_pinned;   /* number of pages pinned                         */
    u_longlong_t real_inuse;    /* number of pages in use                         */
    u_longlong_t pgexct;        /* number of page faults                          */
    u_longlong_t pgins;         /* number of pages paged in                       */
    u_longlong_t pgouts;        /* number of pages paged out                      */
    u_longlong_t pgspins;       /* number of page ins from paging space           */
    u_longlong_t pgspouts;      /* number of page outs from paging space          */
    u_longlong_t scans;         /* number of page scans by clock                  */
    u_longlong_t pgsteals;      /* number of page steals                          */
 
} perfstat_memory_page_wpar_t;

typedef struct {/* tape information */
	char name[IDENTIFIER_LENGTH];            /* name of the tape */
	char description[IDENTIFIER_LENGTH];    /* tape description (from ODM) */
	 u_longlong_t size;                                          /* size of the tape (in MB) */
	 u_longlong_t free;                                         /* free portion of the tape (in MB) */
	 u_longlong_t bsize;                                        /* tape block size (in bytes) */
	u_longlong_t xfers;                                        /* number of transfers to/from tape */
	u_longlong_t rxfers;                                        /* number of read transfers to/from tape */
	u_longlong_t wblks;                                       /* number of blocks written to tape */
	u_longlong_t rblks;                                        /* number of blocks read from tape */
	u_longlong_t time;                                         /* amount of time tape is active */
	char adapter[IDENTIFIER_LENGTH];         /* tape adapter name */
	uint paths_count;                                           /* number of paths to this tape */
	u_longlong_t  rserv;                                       /* read or receive service time */
	u_longlong_t  rtimeout;                                  /* number of read request timeouts */
	u_longlong_t  rfailed;                                     /* number of failed read requests */
	u_longlong_t  min_rserv;                                /* min read or receive service time */
	u_longlong_t  max_rserv;                               /* max read or receive service time */
	u_longlong_t  wserv;                                     /* write or send service time */
	u_longlong_t  wtimeout;                                 /* number of write request timeouts */
	u_longlong_t  wfailed;                                    /* number of failed write requests */
	u_longlong_t  min_wserv;                              /* min write or send service time */
	u_longlong_t  max_wserv;                             /* max write or send service time */
	} perfstat_tape_t;

typedef struct { /* global tape information */
	int number;                                                    /* total number of tapes */
	u_longlong_t size;                                          /* total size of all tapes (in MB) */
	u_longlong_t free;                                         /* free portion of all tapes(in MB) */
	u_longlong_t rxfers;                                        /* number of read transfers to/from tape */
	u_longlong_t xfers;                                        /* total number of transfers to/from tape */
	u_longlong_t wblks;                                       /* 512 bytes blocks written to all tapes */
	u_longlong_t rblks;                                        /* 512 bytes blocks read from all tapes */
   	u_longlong_t time;                                         /* amount of time tapes are active */
	}perfstat_tape_total_t;

typedef struct { /* Virtual memory utilization */
	u_longlong_t virt_total;    /* total virtual memory (in 4KB pages) */
	u_longlong_t real_total;    /* total real memory (in 4KB pages) */
	u_longlong_t real_free;     /* free real memory (in 4KB pages) */
	u_longlong_t real_pinned;   /* real memory which is pinned (in 4KB pages) */
	u_longlong_t real_inuse;    /* real memory which is in use (in 4KB pages) */
	u_longlong_t pgbad;         /* number of bad pages */
	u_longlong_t pgexct;        /* number of page faults */
	u_longlong_t pgins;         /* number of pages paged in */
	u_longlong_t pgouts;        /* number of pages paged out */
	u_longlong_t pgspins;       /* number of page ins from paging space */
	u_longlong_t pgspouts;      /* number of page outs from paging space */
	u_longlong_t scans;         /* number of page scans by clock */
	u_longlong_t cycles;        /* number of page replacement cycles */
	u_longlong_t pgsteals;      /* number of page steals */
	u_longlong_t numperm;       /* number of frames used for files (in 4KB pages) */
	u_longlong_t pgsp_total;    /* total paging space (in 4KB pages) */
	u_longlong_t pgsp_free;     /* free paging space (in 4KB pages) */
	u_longlong_t pgsp_rsvd;     /* reserved paging space (in 4KB pages) */
	u_longlong_t real_system;   /* real memory used by system segments (in 4KB pages). This is the sum of all the used pages in segment marked for system usage.
	                             * Since segment classifications are not always guaranteed to be accurate, this number is only an approximation. */
	u_longlong_t real_user;     /* real memory used by non-system segments (in 4KB pages). This is the sum of all pages used in segments not marked for system usage.
								 * Since segment classifications are not always guaranteed to be accurate, this number is only an approximation. */
	u_longlong_t real_process;  /* real memory used by process segments (in 4KB pages). This is real_total-real_free-numperm-real_system. Since real_system is an
								 * approximation, this number is too. */
	u_longlong_t virt_active;   /* Active virtual pages. Virtual pages are considered active if they have been accessed */
	u_longlong_t iome; /* I/O memory entitlement of the partition in bytes*/
	u_longlong_t iomu; /* I/O memory entitlement of the partition in use in bytes*/
	u_longlong_t iohwm; /* High water mark of I/O memory entitlement used in bytes*/
	u_longlong_t pmem; /* Amount of physical mmeory currently backing partition's logical memory in bytes*/
} perfstat_memory_total_t;

typedef struct { /* WPAR Virtual memory utilization */
        u_longlong_t real_total;    /* Global total real memory (in 4KB pages) */
        u_longlong_t real_free;     /* Global free real memory (in 4KB pages) */
        u_longlong_t real_pinned;   /* real memory which is pinned (in 4KB pages) */
        u_longlong_t real_inuse;    /* real memory which is in use (in 4KB pages) */
        u_longlong_t pgexct;        /* number of page faults */
        u_longlong_t pgins;         /* number of pages paged in */
        u_longlong_t pgouts;        /* number of pages paged out */
        u_longlong_t pgspins;       /* number of page ins from paging space */
        u_longlong_t pgspouts;      /* number of page outs from paging space */
        u_longlong_t scans;         /* number of page scans by clock */
        u_longlong_t pgsteals;      /* number of page steals */
        u_longlong_t numperm;       /* number of frames used for files (in 4KB pages) */
        u_longlong_t virt_active;   /* Active virtual pages. Virtual pages are considered active if they have been accessed */

} perfstat_memory_total_wpar_t;


typedef struct { /* Description of the network interface */
	char name[IDENTIFIER_LENGTH];   /* name of the interface */
	char description[IDENTIFIER_LENGTH]; /* interface description (from ODM, similar to lscfg output) */
	uchar type;               /* ethernet, tokenring, etc. interpretation can be done using /usr/include/net/if_types.h */
	u_longlong_t mtu;         /* network frame size */
	u_longlong_t ipackets;    /* number of packets received on interface */
	u_longlong_t ibytes;      /* number of bytes received on interface */
	u_longlong_t ierrors;     /* number of input errors on interface */
	u_longlong_t opackets;    /* number of packets sent on interface */
	u_longlong_t obytes;      /* number of bytes sent on interface */
	u_longlong_t oerrors;     /* number of output errors on interface */
	u_longlong_t collisions;  /* number of collisions on csma interface */
	u_longlong_t bitrate;     /* adapter rating in bit per second */
} perfstat_netinterface_t;

typedef struct { /* Description of the network interfaces */
	int number;               /* number of network interfaces  */
	u_longlong_t ipackets;    /* number of packets received on interface */
	u_longlong_t ibytes;      /* number of bytes received on interface */
	u_longlong_t ierrors;     /* number of input errors on interface */
	u_longlong_t opackets;    /* number of packets sent on interface */
	u_longlong_t obytes;      /* number of bytes sent on interface */
	u_longlong_t oerrors;     /* number of output errors on interface */
	u_longlong_t collisions;  /* number of collisions on csma interface */
} perfstat_netinterface_total_t;

enum {
	LV_PAGING=1,
	NFS_PAGING,
	UNKNOWN_PAGING
};

typedef struct { /* Paging space data for a specific logical volume */
	char name[IDENTIFIER_LENGTH];    /* Paging space name */
	char type; /* type of paging device (LV_PAGING or NFS_PAGING) *
		    * Possible values are:                            *
		    *     LV_PAGING      logical volume               *
		    *     NFS_PAGING     NFS file                     */
	union{
	       struct{
		      char hostname[IDENTIFIER_LENGTH]; /* host name of paging server */
		      char filename[IDENTIFIER_LENGTH]; /* swap file name on server  */
	       } nfs_paging;
	       struct{
		      char vgname[IDENTIFIER_LENGTH];/*  volume group name  */
	       } lv_paging;
	} u;
	longlong_t lp_size;    /* size in number of logical partitions  */
	longlong_t mb_size;    /* size in megabytes  */
	longlong_t mb_used;    /* portion used in megabytes  */
	longlong_t io_pending; /* number of pending I/O */
	char active;           /* indicates if active (1 if so, 0 if not) */
	char automatic;        /* indicates if automatic (1 if so, 0 if not) */
} perfstat_pagingspace_t;


typedef struct {  /* network buffers  */
	char name[IDENTIFIER_LENGTH]; /* size in ascii, always power of 2 (ex: "32", "64", "128") */
	u_longlong_t inuse;           /* number of buffer currently allocated */
	u_longlong_t calls;           /* number of buffer allocations since last reset */
	u_longlong_t delayed;         /* number of delayed allocations */
	u_longlong_t free;            /* number of free calls */
	u_longlong_t failed;          /* number of failed allocations */
	u_longlong_t highwatermark;   /* high threshold for number of buffer allocated */
	u_longlong_t freed;           /* number of buffers freed */
} perfstat_netbuffer_t;

typedef struct { /* utilization of protocols */
	char name[IDENTIFIER_LENGTH]; /* ip, ipv6, icmp, icmpv6, udp, tcp, rpc, nfs, nfsv2, nfsv3*/
	union{
		struct{
			u_longlong_t ipackets;       /* number of input packets */
			u_longlong_t ierrors;        /* number of input errors */
			u_longlong_t iqueueoverflow; /* number of input queue overflows */
			u_longlong_t opackets;       /* number of output packets */
			u_longlong_t oerrors;        /* number of output errors */
		} ip;
		struct{
			u_longlong_t ipackets;       /* number of input packets */	   
			u_longlong_t ierrors;        /* number of input errors */	   
			u_longlong_t iqueueoverflow; /* number of input queue overflows */
			u_longlong_t opackets;       /* number of output packets */
			u_longlong_t oerrors;        /* number of output errors */
		} ipv6;
		struct{
			u_longlong_t received; /* number of packets received */
			u_longlong_t sent;     /* number of packets sent */
			u_longlong_t errors;   /* number of errors */
		} icmp;
		struct{
			u_longlong_t received; /* number of packets received */
			u_longlong_t sent;     /* number of packets sent */
			u_longlong_t errors;   /* number of errors */
		} icmpv6;
		struct{
			u_longlong_t ipackets;  /* number of input packets */
			u_longlong_t ierrors;   /* number of input errors */
			u_longlong_t opackets;  /* number of output packets */
			u_longlong_t no_socket; /* number of packets dropped due to no socket */
		} udp;
		struct{
			u_longlong_t ipackets;    /* number of input packets */
			u_longlong_t ierrors;     /* number of input errors */
			u_longlong_t opackets;    /* number of output packets */
			u_longlong_t initiated;   /* number of connections initiated */
			u_longlong_t accepted;    /* number of connections accepted */
			u_longlong_t established; /* number of connections established */
			u_longlong_t dropped;     /* number of connections dropped */
		} tcp;
		struct{
			struct{
				struct{
					u_longlong_t calls;     /* total NFS client RPC connection-oriented calls */
					u_longlong_t badcalls;  /* rejected NFS client RPC calls */
					u_longlong_t badxids;   /* bad NFS client RPC call responses */
					u_longlong_t timeouts;  /* timed out NFS client RPC calls with no reply */
					u_longlong_t newcreds;  /* total NFS client RPC authentication refreshes */
					u_longlong_t badverfs;  /* total NFS client RPC bad verifier in response */
					u_longlong_t timers;    /* NFS client RPC timout greater than timeout value */
					u_longlong_t nomem;     /* NFS client RPC calls memory allocation failure */
					u_longlong_t cantconn;  /* failed NFS client RPC calls */
					u_longlong_t interrupts;/* NFS client RPC calls fail due to interrupt */
				} stream; /* connection oriented rpc client */
				struct{
					u_longlong_t calls;    /* total NFS client RPC connectionless calls */
					u_longlong_t badcalls; /* rejected NFS client RPC calls */
					u_longlong_t retrans;  /* retransmitted NFS client RPC calls */
					u_longlong_t badxids;  /* bad NFS client RPC call responses */
					u_longlong_t timeouts; /* timed out NFS client RPC calls with no reply */
					u_longlong_t newcreds; /* total NFS client RPC authentication refreshes */
					u_longlong_t badverfs; /* total NFS client RPC bad verifier in response */
					u_longlong_t timers;   /* NFS client RPC timout greater than timeout value */
					u_longlong_t nomem;    /* NFS client RPC calls memory allocation failure */
					u_longlong_t cantsend; /* NFS client RPC calls not sent */
				} dgram; /* connection less rpc client */
			} client; /* rpc client */
			struct{
				struct{
					u_longlong_t calls;    /* total NFS server RPC connection-oriented requests */
					u_longlong_t badcalls; /* rejected NFS server RPC requests */
					u_longlong_t nullrecv; /* NFS server RPC calls failed due to unavailable packet */
					u_longlong_t badlen;   /* NFS server RPC requests failed due to bad length */
					u_longlong_t xdrcall;  /* NFS server RPC requests failed due to bad header */
					u_longlong_t dupchecks;/* NFS server RPC calls found in request cache */
					u_longlong_t dupreqs;  /* total NFS server RPC call duplicates */
				} stream; /* connection oriented rpc server */
				struct{
					u_longlong_t calls;    /* total NFS server RPC connectionless requests */
					u_longlong_t badcalls; /* rejected NFS server RPC requests */
					u_longlong_t nullrecv; /* NFS server RPC calls failed due to unavailable packet */
					u_longlong_t badlen;   /* NFS server RPC requests failed due to bad length */
					u_longlong_t xdrcall;  /* NFS server RPC requests failed due to bad header */
					u_longlong_t dupchecks;/* NFS server RPC calls found in request cache */
					u_longlong_t dupreqs;  /* total NFS server RPC call duplicates */
				} dgram; /* connection less rpc server */
			} server; /* rpc server*/
		} rpc;
	      struct{
		      struct{
			      u_longlong_t calls;    /* total NFS client requests */
			      u_longlong_t badcalls; /* total NFS client failed calls */
			      u_longlong_t clgets;   /* total number of client nfs clgets */
			      u_longlong_t cltoomany;/* total number of client nfs cltoomany */

		      } client; /* nfs client */
		      struct{
			      u_longlong_t calls;     /* total NFS server requests */
			      u_longlong_t badcalls;  /* total NFS server failed calls */
			      u_longlong_t public_v2; /* total number of nfs version 2 server calls */
			      u_longlong_t public_v3; /* total number of nfs version 3 server calls */
		      } server; /* nfs server */
	      } nfs;
	      struct{
		      struct{
			      u_longlong_t calls;     /* NFS V2 client requests  */
			      u_longlong_t null;      /* NFS V2 client null requests */
			      u_longlong_t getattr;   /* NFS V2 client getattr requests */
			      u_longlong_t setattr;   /* NFS V2 client setattr requests */
			      u_longlong_t root;      /* NFS V2 client root requests */
			      u_longlong_t lookup;    /* NFS V2 client file name lookup requests */
			      u_longlong_t readlink;  /* NFS V2 client readlink requests */
			      u_longlong_t read;      /* NFS V2 client read requests */
			      u_longlong_t writecache;/* NFS V2 client write cache requests */
			      u_longlong_t write;     /* NFS V2 client write requests */
			      u_longlong_t create;    /* NFS V2 client file creation requests */
			      u_longlong_t remove;    /* NFS V2 client file removal requests */
			      u_longlong_t rename;    /* NFS V2 client file rename requests */
			      u_longlong_t link;      /* NFS V2 client link creation requests */
			      u_longlong_t symlink;   /* NFS V2 client symbolic link creation requests */
			      u_longlong_t mkdir;     /* NFS V2 client directory creation requests */
			      u_longlong_t rmdir;     /* NFS V2 client directory removal requests */
			      u_longlong_t readdir;   /* NFS V2 client read-directory requests */
			      u_longlong_t statfs;    /* NFS V2 client file stat requests */
		      } client; /* nfs2 client */
		      struct{
			      u_longlong_t calls;     /* NFS V2 server requests */
			      u_longlong_t null;      /* NFS V2 server null requests */
			      u_longlong_t getattr;   /* NFS V2 server getattr requests */
			      u_longlong_t setattr;   /* NFS V2 server setattr requests */
			      u_longlong_t root;      /* NFS V2 server root requests */
			      u_longlong_t lookup;    /* NFS V2 server file name lookup requests */
			      u_longlong_t readlink;  /* NFS V2 server readlink requests */
			      u_longlong_t read;      /* NFS V2 server read requests */
			      u_longlong_t writecache;/* NFS V2 server cache requests */
			      u_longlong_t write;     /* NFS V2 server write requests */
			      u_longlong_t create;    /* NFS V2 server file creation requests */
			      u_longlong_t remove;    /* NFS V2 server file removal requests */
			      u_longlong_t rename;    /* NFS V2 server file rename requests */
			      u_longlong_t link;      /* NFS V2 server link creation requests */
			      u_longlong_t symlink;   /* NFS V2 server symbolic link creation requests */
			      u_longlong_t mkdir;     /* NFS V2 server directory creation requests */
			      u_longlong_t rmdir;     /* NFS V2 server directory removal requests */
			      u_longlong_t readdir;   /* NFS V2 server read-directory requests */
			      u_longlong_t statfs;    /* NFS V2 server file stat requests */
		      } server; /* nfsv2 server */
	      } nfsv2;
	      struct{
		      struct{
			      u_longlong_t calls;       /* NFS V3 client calls */
			      u_longlong_t null;        /* NFS V3 client null requests */
			      u_longlong_t getattr;     /* NFS V3 client getattr requests */
			      u_longlong_t setattr;     /* NFS V3 client setattr requests */
			      u_longlong_t lookup;      /* NFS V3 client file name lookup requests */
			      u_longlong_t access;      /* NFS V3 client access requests */
			      u_longlong_t readlink;    /* NFS V3 client readlink requests */
			      u_longlong_t read;        /* NFS V3 client read requests */
			      u_longlong_t write;       /* NFS V3 client write requests */
			      u_longlong_t create;      /* NFS V3 client file creation requests */
			      u_longlong_t mkdir;       /* NFS V3 client directory creation requests */
			      u_longlong_t symlink;     /* NFS V3 client symbolic link creation requests */
			      u_longlong_t mknod;       /* NFS V3 client mknod creation requests */
			      u_longlong_t remove;      /* NFS V3 client file removal requests */
			      u_longlong_t rmdir;       /* NFS V3 client directory removal requests */
			      u_longlong_t rename;      /* NFS V3 client file rename requests */
			      u_longlong_t link;        /* NFS V3 client link creation requests */
			      u_longlong_t readdir;     /* NFS V3 client read-directory requests */
			      u_longlong_t readdirplus; /* NFS V3 client read-directory plus requests */
			      u_longlong_t fsstat;      /* NFS V3 client file stat requests */
			      u_longlong_t fsinfo;      /* NFS V3 client file info requests */
			      u_longlong_t pathconf;    /* NFS V3 client path configure requests */
			      u_longlong_t commit;      /* NFS V3 client commit requests */
		      } client; /* nfsv3 client */
		      struct{
			      u_longlong_t calls;       /* NFS V3 server requests */
			      u_longlong_t null;        /* NFS V3 server null requests */
			      u_longlong_t getattr;     /* NFS V3 server getattr requests */
			      u_longlong_t setattr;     /* NFS V3 server setattr requests */
			      u_longlong_t lookup;      /* NFS V3 server file name lookup requests */
			      u_longlong_t access;      /* NFS V3 server file access requests */
			      u_longlong_t readlink;    /* NFS V3 server readlink requests */
			      u_longlong_t read;        /* NFS V3 server read requests */
			      u_longlong_t write;       /* NFS V3 server write requests */
			      u_longlong_t create;      /* NFS V3 server file creation requests */
			      u_longlong_t mkdir;       /* NFS V3 server director6 creation requests */
			      u_longlong_t symlink;     /* NFS V3 server symbolic link creation requests */
			      u_longlong_t mknod;       /* NFS V3 server mknode creation requests */
			      u_longlong_t remove;      /* NFS V3 server file removal requests */
			      u_longlong_t rmdir;       /* NFS V3 server directory removal requests */
			      u_longlong_t rename;      /* NFS V3 server file rename requests */
			      u_longlong_t link;        /* NFS V3 server link creation requests */
			      u_longlong_t readdir;     /* NFS V3 server read-directory requests */
			      u_longlong_t readdirplus; /* NFS V3 server read-directory plus requests */
			      u_longlong_t fsstat;      /* NFS V3 server file stat requests */
			      u_longlong_t fsinfo;      /* NFS V3 server file info requests */
			      u_longlong_t pathconf;	  /* NFS V3 server path configure requests */
			      u_longlong_t commit;      /* NFS V3 server commit requests */
		      } server; /* nfsv3 server */
		} nfsv3;
      struct {
         struct {
               u_longlong_t operations;   /* NFS V4 client operations */
               u_longlong_t null;         /* NFS V4 client null operations */
               u_longlong_t getattr;      /* NFS V4 client getattr operations */
               u_longlong_t setattr;      /* NFS V4 client setattr operations */
               u_longlong_t lookup;       /* NFS V4 client lookup operations */   
               u_longlong_t access;       /* NFS V4 client access operations */
               u_longlong_t readlink;     /* NFS V4 client read link operations */
               u_longlong_t read;         /* NFS V4 client read operations */
               u_longlong_t write;        /* NFS V4 client write operations */
               u_longlong_t create;       /* NFS V4 client create operations */
               u_longlong_t mkdir;        /* NFS V4 client mkdir operations */
               u_longlong_t symlink;      /* NFS V4 client symlink operations */   
               u_longlong_t mknod;        /* NFS V4 client mknod operations */
               u_longlong_t remove;       /* NFS V4 client remove operations */
               u_longlong_t rmdir;        /* NFS V4 client rmdir operations */
               u_longlong_t rename;       /* NFS V4 client rename operations */
               u_longlong_t link;         /* NFS V4 client link operations */
               u_longlong_t readdir;      /* NFS V4 client readdir operations */
               u_longlong_t statfs;       /* NFS V4 client statfs operations */
               u_longlong_t finfo;        /* NFS V4 client file info operations */
               u_longlong_t commit;       /* NFS V4 client commit operations */
               u_longlong_t open;         /* NFS V4 client open operations */
               u_longlong_t open_confirm; /* NFS V4 client open confirm operations */
               u_longlong_t open_downgrade;    /* NFS V4 client open downgrade operations */
               u_longlong_t close;        /* NFS V4 client close operations */
               u_longlong_t lock;         /* NFS V4 client lock operations */ 
               u_longlong_t unlock;       /* NFS V4 client unlock operations */ 
               u_longlong_t lock_test;    /* NFS V4 client lock test operations */
               u_longlong_t set_clientid; /* NFS V4 client set client id operations */
               u_longlong_t renew;        /* NFS V4 client renew operations */ 
               u_longlong_t client_confirm;/* NFS V4 client confirm operations */
               u_longlong_t secinfo;      /* NFS V4 client secinfo operations */
               u_longlong_t release_lock;   /* NFS V4 client release lock operations */   
               u_longlong_t replicate;    /* NFS V4 client replicate operations */
               u_longlong_t pcl_stat;     /* NFS V4 client pcl_stat operations */
               u_longlong_t acl_stat_l;   /* NFS V4 client acl_stat long operations */
               u_longlong_t pcl_stat_l;   /* NFS V4 client pcl_stat long operations */
               u_longlong_t acl_read;     /* NFS V4 client acl_read operations */
               u_longlong_t pcl_read;     /* NFS V4 client pcl_read operations */
               u_longlong_t acl_write;    /* NFS V4 client acl_write operations */
               u_longlong_t pcl_write;    /* NFS V4 client pcl_write operations */
               u_longlong_t delegreturn;  /* NFS V4 client delegreturn operations */
         } client; /* nfsv4 client*/
         struct {
               u_longlong_t null;         /* NFS V4 server null calls */
               u_longlong_t compound;     /* NFS V4 server compound calls */
               u_longlong_t operations;   /* NFS V4 server operations */
               u_longlong_t access;       /* NFS V4 server access operations */
               u_longlong_t close;        /* NFS V4 server close operations */
               u_longlong_t commit;       /* NFS V4 server commit operations */
               u_longlong_t create;       /* NFS V4 server create operations */
               u_longlong_t delegpurge;    /* NFS V4 server del_purge operations */
               u_longlong_t delegreturn;      /* NFS V4 server del_ret operations */
               u_longlong_t getattr;      /* NFS V4 server getattr operations */
               u_longlong_t getfh;        /* NFS V4 server getfh operations */
               u_longlong_t link;         /* NFS V4 server link operations */
               u_longlong_t lock;         /* NFS V4 server lock operations */
               u_longlong_t lockt;        /* NFS V4 server lockt operations */
               u_longlong_t locku;        /* NFS V4 server locku operations */
               u_longlong_t lookup;       /* NFS V4 server lookup operations */
               u_longlong_t lookupp;      /* NFS V4 server lookupp operations */
               u_longlong_t nverify;      /* NFS V4 server nverify operations */
               u_longlong_t open;         /* NFS V4 server open operations */
               u_longlong_t openattr;     /* NFS V4 server openattr operations */
               u_longlong_t open_confirm;      /* NFS V4 server confirm operations */   
               u_longlong_t open_downgrade;    /* NFS V4 server downgrade operations */
               u_longlong_t putfh;        /* NFS V4 server putfh operations */
               u_longlong_t putpubfh;     /* NFS V4 server putpubfh operations */
               u_longlong_t putrootfh;    /* NFS V4 server putrotfh operations */
               u_longlong_t read;         /* NFS V4 server read operations */
               u_longlong_t readdir;      /* NFS V4 server readdir operations */   
               u_longlong_t readlink;     /* NFS V4 server readlink operations */
               u_longlong_t remove;       /* NFS V4 server remove operations */
               u_longlong_t rename;       /* NFS V4 server rename operations */
               u_longlong_t renew;        /* NFS V4 server renew operations */
               u_longlong_t restorefh;    /* NFS V4 server restorefh operations */
               u_longlong_t savefh;       /* NFS V4 server savefh operations */
               u_longlong_t secinfo;      /* NFS V4 server secinfo operations */
               u_longlong_t setattr;      /* NFS V4 server setattr operations */ 
               u_longlong_t set_clientid;      /* NFS V4 server setclid operations */
               u_longlong_t clientid_confirm;     /* NFS V4 server clid_cfm operations */
               u_longlong_t verify;       /* NFS V4 server verify operations */
               u_longlong_t write;        /* NFS V4 server write operations */
               u_longlong_t release_lock;   /* NFS V4 server release_lo operations */
         } server; /* nfsv4 server*/
      } nfsv4;
	} u;
} perfstat_protocol_t;

typedef union {
	uint	w;
	struct {
		unsigned smt_capable :1;	/* OS supports SMT mode */
		unsigned smt_enabled :1;	/* SMT mode is on */
		unsigned lpar_capable :1;	/* OS supports logical partitioning */
		unsigned lpar_enabled :1;	/* logical partitioning is on */
		unsigned shared_capable :1;	/* OS supports shared processor LPAR */
		unsigned shared_enabled :1;	/* partition runs in shared mode */
		unsigned dlpar_capable :1;	/* OS supports dynamic LPAR */
		unsigned capped :1;		/* partition is capped */
		unsigned kernel_is_64 :1;	/* kernel is 64 bit */
		unsigned pool_util_authority :1;/* pool utilization available */
		unsigned donate_capable :1; 	/* capable of donating cycles */
		unsigned donate_enabled :1;	/* enabled for donating cycles */
		unsigned ams_capable:1; /* 1 = AMS(Active Memory Sharing) capable, 0 = Not AMS capable */ 
		unsigned ams_enabled:1; /* 1 = AMS(Active Memory Sharing) enabled, 0 = Not AMS enabled */
		unsigned spare :18; 		/* reserved for future usage */
	} b;
} perfstat_partition_type_t;

typedef struct { /* partition total information */
	char name[IDENTIFIER_LENGTH]; /* name of the logical partition */
	perfstat_partition_type_t type; /* set of bits describing the partition */
	int lpar_id;		/* logical partition identifier */
	int group_id;		/* identifier of the LPAR group this partition is a member of */
	int pool_id;		/* identifier of the shared pool of physical processors this partition is a member of */
	int online_cpus;	/* number of virtual CPUs currently online on the partition */
	int max_cpus;		/* maximum number of virtual CPUs this parition can ever have */
	int min_cpus;		/* minimum number of virtual CPUs this partition must have */
	u_longlong_t online_memory; /* amount of memory currently online */
	u_longlong_t max_memory;    /* maximum amount of memory this partition can ever have */
	u_longlong_t min_memory;    /* minimum amount of memory this partition must have */
	int entitled_proc_capacity; /* number of processor units this partition is entitled to receive */
	int max_proc_capacity;	    /* maximum number of processor units this partition can ever have */
	int min_proc_capacity;      /* minimum number of processor units this partition must have */
	int proc_capacity_increment;   /* increment value to the entitled capacity */
	int unalloc_proc_capacity;     /* number of processor units currently unallocated in the shared processor pool this partition belongs to */
	int var_proc_capacity_weight;  /* partition priority weight to receive extra capacity */
	int unalloc_var_proc_capacity_weight; /* number of variable processor capacity weight units currently unallocated  in the shared processor pool this partition belongs to */
	int online_phys_cpus_sys;   /* number of physical CPUs currently active in the system containing this partition */
	int max_phys_cpus_sys;  /* maximum possible number of physical CPUs in the system containing this partition */
	int phys_cpus_pool;     /* number of the physical CPUs currently in the shared processor pool this partition belong to */
	u_longlong_t puser;	/* raw number of physical processor tics in user mode */
	u_longlong_t psys;	/* raw number of physical processor tics in system mode */
	u_longlong_t pidle;	/* raw number of physical processor tics idle */
	u_longlong_t pwait;	/* raw number of physical processor tics waiting for I/O */
	u_longlong_t pool_idle_time;    /* number of clock tics a processor in the shared pool was idle */
	u_longlong_t phantintrs;	/* number of phantom interrupts received by the partition */
	u_longlong_t invol_virt_cswitch;  /* number involuntary virtual CPU context switches */
	u_longlong_t vol_virt_cswitch;  /* number voluntary virtual CPU context switches */
	u_longlong_t timebase_last;	/* most recently cpu time base */
	u_longlong_t reserved_pages;     /* Currenlty number of 16GB pages. Cannot participate in DR operations */
	u_longlong_t reserved_pagesize;    /*Currently 16GB pagesize Cannot participate in DR operations */
	u_longlong_t idle_donated_purr; /* number of idle cycles donated by a dedicated partition enabled for donation */
	u_longlong_t idle_donated_spurr;/* number of idle spurr cycles donated by a dedicated partition enabled for donation */
	u_longlong_t busy_donated_purr; /* number of busy cycles donated by a dedicated partition enabled for donation */
	u_longlong_t busy_donated_spurr;/* number of busy spurr cycles donated by a dedicated partition enabled for donation */
	u_longlong_t idle_stolen_purr;  /* number of idle cycles stolen by the hypervisor from a dedicated partition */
	u_longlong_t idle_stolen_spurr; /* number of idle spurr cycles stolen by the hypervisor from a dedicated partition */
	u_longlong_t busy_stolen_purr;  /* number of busy cycles stolen by the hypervisor from a dedicated partition */
	u_longlong_t busy_stolen_spurr; /* number of busy spurr cycles stolen by the hypervisor from a dedicated partition */ 

	/* structure members added through Line item 0SK/KER  feature 576885*/
	u_longlong_t shcpus_in_sys; /* Number of physical processors allocated for shared processor use */
	u_longlong_t max_pool_capacity; /* Maximum processor capacity of partition’s pool */
	u_longlong_t entitled_pool_capacity; /* Entitled processor capacity of partition’s pool */
	u_longlong_t pool_max_time; /* Summation of maximum time that could be consumed by the pool (nano seconds) */
	u_longlong_t pool_busy_time; /* Summation of busy (non-idle) time accumulated across all partitions in the pool (nano seconds) */
	u_longlong_t pool_scaled_busy_time; /* Scaled summation of busy (non-idle) time accumulated across all partitions in the pool (nano seconds) */
	u_longlong_t shcpu_tot_time; /* Summation of total time across all physical processors allocated for shared processor use (nano seconds) */
	u_longlong_t shcpu_busy_time; /* Summation of busy (non-idle) time accumulated across all shared processor partitions (nano seconds) */
	u_longlong_t shcpu_scaled_busy_time; /* Scaled summation of busy time accumulated across all shared processor partitions (nano seconds) */
	int ams_pool_id; /* AMS pool id of the pool the LPAR belongs to */
	int var_mem_weight; /* variable memory capacity weight */
	u_longlong_t iome; /* I/O memory entitlement of the partition in bytes*/
	u_longlong_t pmem; /* Physical memory currently backing the partition's logical memory in bytes*/
	u_longlong_t hpi; /* number of hypervisor page-ins */
	u_longlong_t hpit; /* Time spent in hypervisor page-ins (in nanoseconds)*/       
        u_longlong_t hypv_pagesize; /* Hypervisor page size in KB*/
        uint online_lcpus; /*Number of online logical cpus */
        uint smt_thrds;    /* Number of SMT threads */
        u_longlong_t puser_spurr;        /* number of spurr cycles spent in user mode */
        u_longlong_t psys_spurr;        /* number of spurr cycles spent in kernel mode */
        u_longlong_t pidle_spurr;        /* number of spurr cycles spent in idle mode */
        u_longlong_t pwait_spurr;        /* number of spurr cycles spent in wait mode */
        int spurrflag;                  /* set if running in spurr mode */

} perfstat_partition_total_t;

typedef union { /* WPAR Type & Flags */
        uint    w;
        struct {
                unsigned app_wpar :1;        /* Application WPAR */
                unsigned cpu_rset :1;        /* WPAR restricted to CPU resource set */ 
		unsigned cpu_xrset:1;        /* WPAR restricted to CPU Exclusive resource set */
		unsigned cpu_limits :1;      /* CPU resource limits enforced */
		unsigned mem_limits :1;      /* Memory resource limits enforced */	
                unsigned spare :27;             /* reserved for future usage */
        } b;
} perfstat_wpar_type_t;

typedef struct { /* Workload partition Information */
       char name[MAXCORRALNAMELEN+1]; /* name of the Workload Partition */
       perfstat_wpar_type_t type; /* set of bits describing the wpar */
       cid_t wpar_id;            /* workload partition identifier */
       uint  online_cpus; /* Number of Virtual CPUs in partition rset or  number of virtual CPUs currently online on the Global partition*/
       int   cpu_limit; /* CPU limit in 100ths of % - 1..10000 */
       int   mem_limit; /* Memory limit in 100ths of % - 1..10000 */
       u_longlong_t online_memory; /* amount of memory currently online in Global Partition */
       int entitled_proc_capacity; /* number of processor units this partition is entitled to receive */
} perfstat_wpar_total_t;


typedef struct { /* logical volume data */
        char name[IDENTIFIER_LENGTH];       /* logical volume name */
        char vgname[IDENTIFIER_LENGTH];     /* volume group name  */
        u_longlong_t open_close;              /* LVM_QLVOPEN, etc. (see lvm.h) */
        u_longlong_t state;                   /* LVM_UNDEF, etc. (see lvm.h) */
        u_longlong_t mirror_policy;           /* LVM_PARALLEL, etc. (see lvm.h) */
        u_longlong_t mirror_write_consistency;/* LVM_CONSIST, etc. (see lvm.h) */
        u_longlong_t write_verify;            /* LVM_VERIFY, etc. (see lvm.h) */
        u_longlong_t ppsize;                  /* physical partition size in MB */
        u_longlong_t logical_partitions;      /* total number of logical paritions
                                               configured for this logical
                                               volume */
        ushort mirrors;                     /* number of physical mirrors for
                                               each logical partition */
		u_longlong_t iocnt;		/* Number of read and write requests */
		u_longlong_t kbreads;		/*Number of Kilobytes read*/
		u_longlong_t kbwrites;		/*Number of Kilobytes written*/
} perfstat_logicalvolume_t;

typedef struct { /* volume group data */
        char name[IDENTIFIER_LENGTH];    /* volume group name */
        u_longlong_t total_disks;           /* number of physical volumes
                                               in the volume group */
        u_longlong_t active_disks;          /* number of active physical volumes
                                               in the volume group */
        u_longlong_t total_logical_volumes; /* number of logical volumes
                                               in the volume group */
        u_longlong_t opened_logical_volumes;/* number of logical volumes opened
                                               in the volume group */
		u_longlong_t iocnt;		/*Number of read and write requests*/
		
		u_longlong_t kbreads;		/*Number of Kilobytes read*/
		
		u_longlong_t kbwrites;		/*Number of Kilobytes written*/
} perfstat_volumegroup_t;

extern int perfstat_logicalvolume(perfstat_id_t *name,
                           perfstat_logicalvolume_t* userbuff,
                           int sizeof_userbuff,
                           int desired_number);
extern int perfstat_volumegroup(perfstat_id_t *name,
                         perfstat_volumegroup_t* userbuff,
                         int sizeof_userbuff,
                         int desired_number);


extern int perfstat_cpu_total(perfstat_id_t *name,
                              perfstat_cpu_total_t* userbuff,
                              int sizeof_userbuff,
                              int desired_number);
extern int perfstat_cpu_total_wpar(perfstat_id_wpar_t *name,
				   perfstat_cpu_total_wpar_t* userbuff,
				   int sizeof_userbuff,
				   int desired_number);
extern int perfstat_cpu_total_rset(perfstat_id_wpar_t *name,
				   perfstat_cpu_total_t* userbuff,
				   int sizeof_userbuff,
				   int desired_number);
extern int perfstat_cpu(perfstat_id_t *name,
                        perfstat_cpu_t* userbuff,
                        int sizeof_userbuff,
                        int desired_number);
extern int perfstat_cpu_rset(perfstat_id_wpar_t *name,
                        perfstat_cpu_t* userbuff,
                        int sizeof_userbuff,
                        int desired_number);
extern int perfstat_disk_total(perfstat_id_t *name,
                               perfstat_disk_total_t* userbuff,
                               int sizeof_userbuff,
                               int desired_number);
extern int perfstat_disk_total_wpar(perfstat_id_wpar_t *name,
                               perfstat_disk_total_t* userbuff,
                               int sizeof_userbuff,
                               int desired_number);
extern int perfstat_disk(perfstat_id_t *name,
                         perfstat_disk_t* userbuff,
                         int sizeof_userbuff,
                         int desired_number);
extern int perfstat_tape(perfstat_id_t *name,
	  					 perfstat_tape_t* userbuff,
                         int sizeof_userbuff,
						 int desired_number);
extern int perfstat_tape_total(perfstat_id_t *name,
                               perfstat_tape_total_t* userbuff,
                               int sizeof_userbuff,
                               int desired_number);

extern int perfstat_diskadapter(perfstat_id_t *name,
                                perfstat_diskadapter_t* userbuff,      
                                int sizeof_userbuff,		       
                                int desired_number);		       
extern int perfstat_diskpath(perfstat_id_t *name,
                             perfstat_diskpath_t* userbuff,      
                             int sizeof_userbuff,		       
                             int desired_number);		       
extern int perfstat_memory_page(perfstat_psize_t *psize,
                                perfstat_memory_page_t* userbuff,
                                int sizeof_userbuff,
                                int desired_number);
extern int perfstat_memory_page_wpar(perfstat_id_wpar_t *name,
                                     perfstat_psize_t *psize,
                                     perfstat_memory_page_wpar_t* userbuff,
                             int sizeof_userbuff,		       
                             int desired_number);		       
extern int perfstat_memory_total(perfstat_id_t *name,
                                 perfstat_memory_total_t* userbuff,
                                 int sizeof_userbuff,
                                 int desired_number);
extern int perfstat_memory_total_wpar(perfstat_id_wpar_t *name,
                                      perfstat_memory_total_wpar_t* userbuff,
                                      int sizeof_userbuff,
                                      int desired_number);
extern int perfstat_netinterface_total(perfstat_id_t *name,
                                       perfstat_netinterface_total_t* userbuff,
                                       int sizeof_userbuff,
                                       int desired_number);
extern int perfstat_netinterface(perfstat_id_t *name,
                                 perfstat_netinterface_t* userbuff,
                                 int sizeof_userbuff,
                                 int desired_number);
extern int perfstat_pagingspace(perfstat_id_t *name,
                                perfstat_pagingspace_t* userbuff,	   
                                int sizeof_userbuff,			   
                                int desired_number);			   
extern int perfstat_netbuffer(perfstat_id_t *name,
                              perfstat_netbuffer_t* userbuff,
                              int sizeof_userbuff,
                              int desired_number);
extern int perfstat_protocol(perfstat_id_t *name,
                             perfstat_protocol_t* userbuff,
                             int sizeof_userbuff,
                             int desired_number);
extern int perfstat_partition_total(perfstat_id_t *name,
                              perfstat_partition_total_t* userbuff,
                              int sizeof_userbuff,
                              int desired_number);
extern int perfstat_wpar_total(perfstat_id_wpar_t *name,
                              perfstat_wpar_total_t* userbuff,
                              int sizeof_userbuff,
                              int desired_number);

extern void perfstat_reset(void);

#define FLUSH_CPUTOTAL		1LL
#define FLUSH_DISK		2LL
#define RESET_DISK_MINMAX	4LL
#define FLUSH_DISKADAPTER	8LL
#define FLUSH_DISKPATH		16LL
#define FLUSH_PAGINGSPACE	32LL
#define FLUSH_NETINTERFACE	64LL
#define RESET_DISK_ALL          128LL
#define FLUSH_LOGICALVOLUME             128LL
#define FLUSH_VOLUMEGROUP               256LL

extern int perfstat_partial_reset(char * name,u_longlong_t defmask);
extern int perfstat_config(uint command, void* arg);

#ifdef __cplusplus
}
#endif

#endif /*unnef LIBPERFSTAT_H*/

