/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: nxsocket.h
**
**/

#ifndef _nxsocket_h_
#define _nxsocket_h_

#ifdef _WIN32

// Socket compatibility
#define SHUT_RD      0
#define SHUT_WR      1
#define SHUT_RDWR    2

#define SELECT_NFDS(x)  ((int)(x))

#ifdef __cplusplus

static inline SOCKET CreateSocket(int af, int ptype, int protocol)
{
   SOCKET s = socket(af, ptype, protocol);
   if (s != INVALID_SOCKET)
   {
      DWORD flags = 0;
      GetHandleInformation((HANDLE)s, &flags);
      if (flags & HANDLE_FLAG_INHERIT)
         SetHandleInformation((HANDLE)s, HANDLE_FLAG_INHERIT, 0);
   }
   return s;
}

static inline void SetSocketReuseFlag(SOCKET s)
{
	BOOL val = TRUE;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(BOOL));
}

static inline void SetSocketExclusiveAddrUse(SOCKET s)
{
	BOOL val = TRUE;
	setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char *)&val, sizeof(BOOL));
}

static inline void SetSocketNonBlocking(SOCKET s)
{
	u_long one = 1;
	ioctlsocket(s, FIONBIO, &one);
}

static inline void SetSocketBlocking(SOCKET s)
{
	u_long zero = 0;
	ioctlsocket(s, FIONBIO, &zero);
}

static inline void SetSocketNoDelay(SOCKET s)
{
	BOOL val = TRUE;
	setsockopt(s,  IPPROTO_TCP, TCP_NODELAY, (char *)&val, sizeof(BOOL));
}

static inline void SetSocketBroadcast(SOCKET s)
{
	BOOL val = TRUE;
	setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char *)&val, sizeof(BOOL));
}

static inline bool IsValidSocket(SOCKET s)
{
   int val;
   int len = sizeof(int);
   if (getsockopt(s, SOL_SOCKET, SO_BROADCAST, (char *)&val, &len) == 0)
      return true;
   return (WSAGetLastError() != WSAENOTSOCK);
}

#endif /* __cplusplus */

#else /* _WIN32 */

// Socket compatibility
typedef int SOCKET;

#define closesocket(x)     _close(x)
#define WSAGetLastError()  (errno)

#define WSAECONNREFUSED    ECONNREFUSED
#define WSAECONNRESET      ECONNRESET
#define WSAEINPROGRESS     EINPROGRESS
#define WSAEINTR           EINTR
#define WSAESHUTDOWN       ESHUTDOWN
#define WSAEWOULDBLOCK     EWOULDBLOCK
#define WSAEPROTONOSUPPORT EPROTONOSUPPORT
#define WSAEOPNOTSUPP      EOPNOTSUPP
#define WSAENOTSOCK        ENOTSOCK
#define INVALID_SOCKET     (-1)
#define SELECT_NFDS(x)     (x)

#if !(HAVE_SOCKLEN_T) && !defined(_USE_GNU_PTH)
typedef unsigned int socklen_t;
#endif

#ifndef SUN_LEN
#define SUN_LEN(su) (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif

#ifdef __cplusplus

static inline SOCKET CreateSocket(int af, int ptype, int protocol)
{
   return socket(af, ptype, protocol);
}

static inline void SetSocketReuseFlag(SOCKET sd)
{
	int nVal = 1;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const void *)&nVal, (socklen_t)sizeof(nVal));
}

static inline void SetSocketExclusiveAddrUse(SOCKET s)
{
}

static inline void SetSocketNonBlocking(SOCKET s)
{
   int f = fcntl(s, F_GETFL);
   if (f != -1) 
      fcntl(s, F_SETFL, f | O_NONBLOCK);
}

static inline void SetSocketBlocking(SOCKET s)
{
   int f = fcntl(s, F_GETFL);
   if (f != -1) 
      fcntl(s, F_SETFL, f & ~O_NONBLOCK);
}

static inline void SetSocketNoDelay(SOCKET s)
{
	int val = 1;
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const void *)&val, sizeof(int));
}

static inline void SetSocketBroadcast(SOCKET s)
{
	int val = 1;
	setsockopt(s, SOL_SOCKET, SO_BROADCAST, (const void *)&val, sizeof(int));
}

static inline bool IsValidSocket(SOCKET s)
{
   int val;
   socklen_t len = sizeof(int);
   if (getsockopt(s, SOL_SOCKET, SO_BROADCAST, &val, &len) == 0)
      return true;
   return (errno != EBADF) && (errno != ENOTSOCK);
}

#endif /* __cplusplus */

#endif /* _WIN32 */

#endif
