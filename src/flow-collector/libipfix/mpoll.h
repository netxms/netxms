/*
** mpoll.h - export declarations for poll funcs
**
** Copyright Fraunhofer FOKUS
**
** $Id$
**
** Remarks: This is experimental code!
**
*/
#ifndef _MPOLL_H
#define _MPOLL_H

#define MPOLL_IN        1
#define MPOLL_OUT       2
#define MPOLL_EXCEPT    4


typedef struct _timer 
{
    struct timeval expiration;
    void           (*callback)(void *user);
    void           *user;
    struct _timer  *prev,*next;
} mtimer_t;

typedef mtimer_t* mptimer_t;
typedef void (*pcallback_f)(int fd, int mask, void *arg);
typedef void (*tcallback_f)(void *arg);

int       mpoll_fdadd ( SOCKET fd, int mask, pcallback_f callback, void *arg );
void      mpoll_fdrm  ( SOCKET fd );
mptimer_t mpoll_timeradd ( int32_t usec, tcallback_f callback, void *arg );
void      mpoll_timerrm  ( mptimer_t timer );
int       LIBIPFIX_EXPORTABLE mpoll_loop  ( int timeout );
void      mpoll_break    ( void );
void      mpoll_cleanup  ( void );

#endif 

