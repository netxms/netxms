/*
$$LIC$$
 */
/*
** mpoll.c - poll funcs
**
** Copyright Fraunhofer FOKUS
**
** $Date: 2008-10-14 12:40:12 +0200 (Di, 14. Okt 2008) $
**
** $Revision: 1.6 $
**
** Remarks: timer funcs original version from linuxatm
**          todo: replace timer funcs
*/
#include "libipfix.h"

/*------ defines ---------------------------------------------------------*/

#define alloc_t(t) ((t *) malloc(sizeof(t)))

#define Q_INSERT_HEAD(r,i) { (i)->next = r; (i)->prev = NULL; \
  if (r) (r)->prev = i; r = i; }
#define Q_INSERT_AFTER(r,i,a) { if (a) { (i)->next = (a)->next; \
  (i)->prev = a; if ((a)->next) (a)->next->prev = i; (a)->next = i; } \
  else { (i)->next = r; (i)->prev = NULL; if (r) (r)->prev = i; r = i; } }
#define Q_INSERT_BEFORE(r,i,b) { if (b) { (i)->next = b; \
  (i)->prev = (b)->prev; if ((b)->prev) (b)->prev->next = i; else r = i; \
  (b)->prev = i; } else { (i)->next = r; (i)->prev = NULL; \
  if (r) (r)->prev = i; r = i; } }
#define Q_REMOVE(r,i) { if ((i)->next) (i)->next->prev = (i)->prev; \
	if ((i)->prev) (i)->prev->next = (i)->next; else r = (i)->next; }

/*------ stuctures -------------------------------------------------------*/

typedef struct _timer 
{
    struct timeval expiration;
    void           (*callback)(void *user);
    void           *user;
    struct _timer  *prev,*next;
} mtimer_t;

typedef struct poll_node
{
    struct poll_node   *next;
    int                reuse;     /* flag */

    SOCKET           fd;
    int              mask;
    pcallback_f      func;
    void             *arg;
} poll_node_t;

/*------ globals ---------------------------------------------------------*/

static struct timeval g_tnow;  /* need to control timers */
static mtimer_t       *timers = NULL;

static poll_node_t  static_pnodes[16];
static size_t       nstatic_pnodes=sizeof(static_pnodes)/sizeof(poll_node_t);
static poll_node_t  *g_pnode_freelist=NULL;
static poll_node_t  *g_pnodes=NULL;
static int          usedpnodes=0;

static int          g_fdsetupdate =0;
static int          g_breakflag   =0;    /* flag, 1 => break mpoll_loop() */

/*------ revision id -----------------------------------------------------*/

static char const cvsid[]="$Id: mpoll.c 956 2008-10-14 10:40:12Z hir $";

/*------ export funcs ----------------------------------------------------*/

mtimer_t *_mtimer_ustart( int32_t usec, 
                          void    (*callback)(void *user),
                          void    *user)
{
    mtimer_t *n,*walk,*last;

    n = alloc_t(mtimer_t);
    n->expiration.tv_usec = g_tnow.tv_usec+usec;
    n->expiration.tv_sec = g_tnow.tv_sec;
    n->callback = callback;
    n->user = user;
    while (n->expiration.tv_usec > 1000000) {
        n->expiration.tv_usec -= 1000000;
        n->expiration.tv_sec++;
    }
    last = NULL;
    for (walk = timers; walk; walk = walk->next)
        if (walk->expiration.tv_sec > n->expiration.tv_sec ||
          (walk->expiration.tv_sec == n->expiration.tv_sec &&
          walk->expiration.tv_usec > n->expiration.tv_usec)) break;
        else last = walk;
    if (walk) 
	 {
        Q_INSERT_BEFORE(timers,n,walk)
	 }
    else 
	 {
        Q_INSERT_AFTER(timers,n,last)
	 }
    return n;
}

mtimer_t *_mtimer_start( int32_t  sec,
                         void     (*callback)(void *arg),
                         void     *arg )
{
    mtimer_t *n, *walk, *last;

    n = alloc_t(mtimer_t);
    n->expiration.tv_usec = g_tnow.tv_usec;
    n->expiration.tv_sec  = g_tnow.tv_sec + sec;
    n->callback = callback;
    n->user = arg;

    last = NULL;
    for (walk = timers; walk; walk = walk->next)
        if (walk->expiration.tv_sec > n->expiration.tv_sec ||
            (walk->expiration.tv_sec == n->expiration.tv_sec &&
             walk->expiration.tv_usec > n->expiration.tv_usec)) break;
        else last = walk;
    if (walk) 
	 {
        Q_INSERT_BEFORE(timers,n,walk)
	 }
    else 
	 {
        Q_INSERT_AFTER(timers,n,last)
	 }
    return n;
}

void _mtimer_stop(mtimer_t *timer)
{
    mtimer_t *walk;

    for (walk = timers; walk; walk = walk->next)
        if( walk == timer ) {
            Q_REMOVE(timers,timer);
            free(timer);
            return;
        }
}


struct timeval *_mtimer_getnext(void)
{
    static struct timeval delta;

    if (!timers) 
        return NULL;

    delta.tv_sec  = timers->expiration.tv_sec-g_tnow.tv_sec;
    delta.tv_usec = timers->expiration.tv_usec-g_tnow.tv_usec;
    while (delta.tv_usec < 0) 
    {
        delta.tv_usec += 1000000;
        delta.tv_sec--;
    }
    if (delta.tv_sec < 0) 
        delta.tv_sec = delta.tv_usec = 0;

    return &delta;
}

void _mtimer_pop(mtimer_t *timer)
{
    Q_REMOVE(timers,timer);
    timer->callback(timer->user);
    free(timer);
}

void _mtimer_settime( void )
{
    gettimeofday(&g_tnow, NULL);
}

void _mtimer_expire (void)
{
    while (timers && (timers->expiration.tv_sec < g_tnow.tv_sec ||
                      (timers->expiration.tv_sec==g_tnow.tv_sec 
                       && timers->expiration.tv_usec < g_tnow.tv_usec))) 
        _mtimer_pop(timers);
}

static void _pnode_init ( void )
{
    poll_node_t *node = static_pnodes;
    int         i;

    for( i=0; i<(int)nstatic_pnodes; i++ ) {
        node->next = g_pnode_freelist;
        node->reuse= 1;
        g_pnode_freelist = node; 
        node++;
    }
}

static void _pnode_free ( poll_node_t *node )
{
    if ( node->reuse ) {
        node->next = g_pnode_freelist;
        g_pnode_freelist = node;
    }
    else {
        free(node);
    }
    usedpnodes--;
}

/*
 * name:        mpoll_fdadd()
 * parameters:
 * return:
 */
int mpoll_fdadd ( SOCKET fd, int mask, pcallback_f callback, void *arg )
{
    poll_node_t *node;

    /** module init
     */
    if ( (g_pnode_freelist==NULL) && (usedpnodes==0) )
        _pnode_init();

    if ( g_pnode_freelist ) {
        node = g_pnode_freelist;
        g_pnode_freelist = g_pnode_freelist->next;
    }
    else {
        /** alloc new pnode
         */
        if ( (node=calloc( 1, sizeof(poll_node_t))) ==NULL)
            return -1;
        node->reuse = 0;
    }

    node->fd   = fd;
    node->mask = mask;
    node->func = callback;
    node->arg  = arg;

    node->next = g_pnodes;
    g_pnodes   = node;
    usedpnodes ++;
    g_fdsetupdate ++;

    return 0;
}

/*
 * name:        mpoll_fdrm()
 * parameters:
 * return:
 */
void mpoll_fdrm ( SOCKET fd )
{
    poll_node_t *l, *n = g_pnodes;

    l = n;
    while ( n ) {
        if ( n->fd == fd ) {
            if ( n==g_pnodes ) {
                g_pnodes = g_pnodes->next;
            }
            else {
                l->next = n->next;
            }
            _pnode_free( n );
            break;
        }
        l = n;
        n = n->next;
    }
    g_fdsetupdate ++;
}

/*
 * name:        mpoll_timeradd()
 * parameters:
 * return:
 */
mptimer_t mpoll_timeradd ( int32_t sec, tcallback_f callback, void *arg )
{
    _mtimer_settime();
    return (mptimer_t) _mtimer_start( sec, callback, arg );
}

/*
 * name:        mpoll_timeruadd()
 * parameters:
 * return:
 */
mptimer_t mpoll_utimeradd ( int32_t usec, tcallback_f callback, void *arg )
{
    _mtimer_settime();
    return (mptimer_t) _mtimer_ustart( usec, callback, arg );
}

/*
 * name:        mpoll_timerrm()
 * parameters:
 * return:
 */
void mpoll_timerrm ( mptimer_t timer )
{
    _mtimer_stop( (mtimer_t*)timer );
}


static void _cb_stoploop( void *arg )
{
    int *flag = (int*)arg;

    *flag = 1;
}

/*
 * name:        mpoll_loop()
 * parameters:  > int timeout  0 => return after one run
 *                            -1 => run infinitely
 *                            otherwise return after timeout seconds
 * return:      0
 */
int mpoll_loop ( int timeout )
{
    volatile int   stopflag;
    mtimer_t       *mtstop;
    fd_set         rset, wset, xset, nrset, nwset, nxset;
    int            mask, ret;
	 SOCKET         fds;

    poll_node_t    *node;
    struct timeval *lt;

    g_fdsetupdate = 1;
    g_breakflag   = 0;
    fds = 0;
    stopflag = 0;
    if ( timeout > 0 ) {
        mtstop = _mtimer_start( timeout, _cb_stoploop, (void*)&stopflag );
    }
    else if ( timeout == 0 ) {
        mtstop = _mtimer_ustart( 1, _cb_stoploop, (void*)&stopflag );
    }

    while ( !stopflag && !g_breakflag ) {
        if ( g_fdsetupdate ) {
            FD_ZERO(&rset);
            FD_ZERO(&wset);
            FD_ZERO(&xset);
            fds = 0;
            for ( node=g_pnodes; node!=NULL; node=node->next ) {
                if ( node->fd >= fds )
                    fds = node->fd +1;
                if ( node->mask & MPOLL_IN ) 
                    FD_SET( node->fd, &rset );
                if ( node->mask & MPOLL_OUT ) 
                    FD_SET( node->fd, &wset );
                if ( node->mask & MPOLL_EXCEPT ) 
                    FD_SET( node->fd, &xset );
            }
            g_fdsetupdate = 0;
        }

        nrset = rset;
        nwset = wset;
        nxset = xset;
        lt=_mtimer_getnext();
        ret = select( SELECT_NFDS(fds), &nrset, &nwset, &nxset, lt );
        if (ret < 0) {
            if (errno==EINTR)
                continue;

            perror("select");
            continue;
        }

        /** check timers
         */
        _mtimer_settime();
        _mtimer_expire ();

        /** check fds
         */
        for ( node=g_pnodes; node!=NULL; node=node->next ) {
            mask = 0;
            if ( FD_ISSET(node->fd, &nrset) )
                mask |= MPOLL_IN;
            if ( FD_ISSET(node->fd, &nwset) )
                mask |= MPOLL_OUT;
            if ( FD_ISSET(node->fd, &nxset) )
                mask |= MPOLL_EXCEPT;

            if ( mask ) {
                if ( node->func ) {
                    node->func( SELECT_NFDS(node->fd), mask, node->arg );
                    if ( g_fdsetupdate ) 
                        break;
                }
                else {
                    mlogf( 0, "INTERNAL ERROR: node %p fd=%d func=%p/%p %d\n", 
                           node, node->fd, node->func, 
                           node->arg, node->reuse );
                }
            }
        }

        /** check timers again
         */
        _mtimer_settime();
        _mtimer_expire ();

        if ( timeout == 0 )
            break;
    }
    return 0;
}

/* name:      mpoll_break()
 * purpose:   this func is used by a callback func
 *            to end mpoll_loop()
 */
void mpoll_break( void )
{
    g_breakflag = 1;
}

/* name:      mpoll_cleanup()
 * purpose:   clean-up this module
 */
void mpoll_cleanup( void )
{
    poll_node_t *n;

    /** cleanup poll nodes
     */
    while( usedpnodes && g_pnodes ) {
        n = g_pnodes;
        g_pnodes = n->next;
        _pnode_free( n );
    }
    g_pnode_freelist=NULL;


    /** cleanup timers
     */
    while( timers ) {
        _mtimer_stop( timers );
    }
}
