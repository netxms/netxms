/*
** NetXMS Application Connector Library
** Copyright (C) 2015 Raden Solutions
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files
** (the "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to permit
** persons to whom the Software is furnished to do so, subject to the 
** following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
** OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
** OTHER DEALINGS IN THE SOFTWARE.
**/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32

#include <winsock2.h>
#include <windows.h>

#define SELECT_NFDS(f) (0)

#else /* _WIN32 */

#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

#define SOCKET int
#define closesocket(x) close(x)
#define WSAGetLastError() (errno)
#define WSAEWOULDBLOCK EWOULDBLOCK
#define WSAEINPROGRESS EINPROGRESS

#define SELECT_NFDS(f) ((f) + 1)

#ifndef SUN_LEN
#define SUN_LEN(su) (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif

#endif /* _WIN32 */

#include "nxappc.h"
#include <nxappc_internal.h>

static char s_name[128] = "";
static SOCKET s_socket = -1;
static int s_timeout = 200;   // default timeout 200 ms

#define CHECK_CONNECTION do { if ((s_socket == -1) && (s_name[0] != 0)) { if (nxappc_reconnect() == NXAPPC_FAIL) return NXAPPC_FAIL; } } while(0)

/**
 * Set timeout
 */
void LIBNXAPPC_EXPORTABLE nxappc_set_timeout(int t)
{
   if (t > 0)
      s_timeout = t;
}

/**
 * Time difference in ms between two timeval structures
 */
static int time_diff(struct timeval *t1, struct timeval *t2)
{
   long long usec1 = (long long)t1->tv_sec * 1000000L + (long long)t1->tv_usec;
   long long usec2 = (long long)t2->tv_sec * 1000000L + (long long)t2->tv_usec;
   return (int)((usec1 - usec2) / 1000);
} 

/**
 * Connect with timeout
 */
static int connect_ex(SOCKET s, struct sockaddr *addr, int len)
{
	int rc = connect(s, addr, len);
	if (rc == -1)
	{
		if ((WSAGetLastError() == WSAEWOULDBLOCK) || (WSAGetLastError() == WSAEINPROGRESS))
		{
			struct timeval tv;
			fd_set wrfs, exfs;

			FD_ZERO(&wrfs);
			FD_SET(s, &wrfs);

			FD_ZERO(&exfs);
			FD_SET(s, &exfs);

#ifdef _WIN32
			tv.tv_sec = s_timeout / 1000;
			tv.tv_usec = (s_timeout % 1000) * 1000;
			rc = select(0, NULL, &wrfs, &exfs, &tv);
#else
         int timeout = s_timeout;
			do
			{
            struct timeval start, stop;
            
				tv.tv_sec = s_timeout / 1000;
				tv.tv_usec = (s_timeout % 1000) * 1000;
            
            gettimeofday(&start, NULL);
				rc = select(s + 1, NULL, &wrfs, &exfs, &tv);
				if ((rc != -1) || (errno != EINTR))
					break;
            gettimeofday(&stop, NULL);
            
            timeout -= time_diff(&stop, &start);
			} while(timeout > 0);
#endif
			if (rc > 0)
			{
				if (FD_ISSET(s, &exfs))
				{
#ifdef _WIN32
					int err, len = sizeof(int);
					if (getsockopt(s, SOL_SOCKET, SO_ERROR, (char *)&err, &len) == 0)
						WSASetLastError(err);
#endif
					rc = -1;
				}
				else
				{
					rc = 0;
				}
			}
			else if (rc == 0)	// timeout, return error
			{
				rc = -1;
#ifdef _WIN32
				WSASetLastError(WSAETIMEDOUT);
#endif
			}
		}
	}
	return rc;
}

/**
 * Extended send() - send all data even if single call to send()
 * cannot handle them all
 */
static int send_ex(void *data, int len)
{
	int nLeft = (int)len;
	int nRet;

   if (s_socket == -1)
      return -1;

	do
	{
retry:
#ifdef MSG_NOSIGNAL
		nRet = send(s_socket, ((char *)data) + (len - nLeft), nLeft, MSG_NOSIGNAL);
#else
		nRet = send(s_socket, ((char *)data) + (len - nLeft), nLeft, 0);
#endif
		if (nRet <= 0)
		{
			if ((WSAGetLastError() == WSAEWOULDBLOCK)
#ifndef _WIN32
			    || (errno == EAGAIN)
#endif
			   )
			{
				// Wait until socket becomes available for writing
				struct timeval tv;
				fd_set wfds;

				tv.tv_sec = s_timeout / 1000;
				tv.tv_usec = (s_timeout % 1000) * 1000;
				FD_ZERO(&wfds);
				FD_SET(s_socket, &wfds);
				nRet = select(SELECT_NFDS(s_socket + 1), NULL, &wfds, NULL, &tv);
				if ((nRet > 0) || ((nRet == -1) && (errno == EINTR)))
					goto retry;
			}
			break;
		}
		nLeft -= nRet;
	} while (nLeft > 0);

   if (nRet <= 0)
   {
      closesocket(s_socket);
      s_socket = -1;
   }

	return nLeft == 0 ? (int)len : nRet;
}

/**
 * Establish connection
 */
int LIBNXAPPC_EXPORTABLE nxappc_connect(void)
{
#ifdef _WIN32
   return nxappc_connect_ex("33717");
#else
   return nxappc_connect_ex("default");
#endif
}

/**
 * Establish connection
 */
int LIBNXAPPC_EXPORTABLE nxappc_connect_ex(const char *name)
{
   if ((s_socket != -1) && !strcmp(name, s_name))
      return NXAPPC_SUCCESS;  // already connected
   strncpy(s_name, name, 128);
   return nxappc_reconnect();
}

/**
 * Re-establish connection
 */
int LIBNXAPPC_EXPORTABLE nxappc_reconnect(void)
{
#ifdef _WIN32
	struct sockaddr_in addrLocal;
#else
	struct sockaddr_un addrLocal;
#endif

   if (s_socket != -1)
   {
      closesocket(s_socket);
   }

#ifdef _WIN32
   s_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (s_socket == -1)
		return NXAPPC_FAIL;
	
   // set socket non-blocking
	u_long one = 1;
	ioctlsocket(s_socket, FIONBIO, &one);
   
	addrLocal.sin_family = AF_INET;
   addrLocal.sin_addr.s_addr = inet_addr("127.0.0.1");
   addrLocal.sin_port = htons(atoi(s_name));
	if (connect_ex(s_socket, (struct sockaddr *)&addrLocal, sizeof(addrLocal)) == -1)
   {
      closesocket(s_socket);
		return NXAPPC_FAIL;
   }

#else
	s_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s_socket == -1)
		return NXAPPC_FAIL;
	
   // set socket non-blocking
   int f = fcntl(s_socket, F_GETFL);
   if (f != -1) 
      fcntl(s_socket, F_SETFL, f | O_NONBLOCK);
   
	addrLocal.sun_family = AF_UNIX;
   sprintf(addrLocal.sun_path, "/tmp/.nxappc.%s", s_name);
	if (connect_ex(s_socket, (struct sockaddr *)&addrLocal, SUN_LEN(&addrLocal)) == -1)
   {
      closesocket(s_socket);
		return NXAPPC_FAIL;
   }

#endif

   return NXAPPC_SUCCESS;
}

/**
 * Disconnect
 */
void LIBNXAPPC_EXPORTABLE nxappc_disconnect(void)
{
   if (s_socket != -1)
   {
      shutdown(s_socket, 2);
#ifdef _WIN32
      closesocket(s_socket);
#else
      close(s_socket);
#endif
   }
}

/**
 * Send message start indicator
 */
static int SendStartIndicator(char command)
{
   char buffer[NXAPPC_MSG_START_INDICATOR_LEN + 1];
   memcpy(buffer, NXAPPC_MSG_START_INDICATOR, NXAPPC_MSG_START_INDICATOR_LEN);
   buffer[NXAPPC_MSG_START_INDICATOR_LEN] = command;
   return send_ex(buffer, NXAPPC_MSG_START_INDICATOR_LEN + 1);
}

/**
 * Send event
 */
int LIBNXAPPC_EXPORTABLE nxappc_send_event(int code, const char *name, const char *format, ...)
{
   CHECK_CONNECTION;

   return NXAPPC_FAIL;
}

/**
 * Send binary data
 */
int LIBNXAPPC_EXPORTABLE nxappc_send_data(void *data, int size)
{
   CHECK_CONNECTION;

   if (SendStartIndicator(NXAPPC_CMD_SEND_DATA) <= 0)
      return NXAPPC_FAIL;

   if (send_ex(&size, sizeof(int)) <= 0)
      return NXAPPC_FAIL;

   return (send_ex(data, size) > 0) ? NXAPPC_SUCCESS : NXAPPC_FAIL;
}

/**
 * Register counter
 */
int LIBNXAPPC_EXPORTABLE nxappc_register_counter(const char *counter, const char *instance)
{
   CHECK_CONNECTION;

   return NXAPPC_FAIL;
}

/**
 * Update integer counter
 */
int LIBNXAPPC_EXPORTABLE nxappc_update_counter_long(const char *counter, const char *instance, long diff)
{
   CHECK_CONNECTION;

   return NXAPPC_FAIL;
}

/**
 * Update floating point counter
 */
int LIBNXAPPC_EXPORTABLE nxappc_update_counter_double(const char *counter, const char *instance, double diff)
{
   CHECK_CONNECTION;

   return NXAPPC_FAIL;
}

/**
 * Reset counter
 */
int LIBNXAPPC_EXPORTABLE nxappc_reset_counter(const char *counter, const char *instance)
{
   CHECK_CONNECTION;

   return NXAPPC_FAIL;
}

/**
 * Update integer counter
 */
int LIBNXAPPC_EXPORTABLE nxappc_set_counter_long(const char *counter, const char *instance, long value)
{
   CHECK_CONNECTION;

   return NXAPPC_FAIL;
}

/**
 * Set floating point counter
 */
int LIBNXAPPC_EXPORTABLE nxappc_set_counter_double(const char *counter, const char *instance, double value)
{
   CHECK_CONNECTION;

   return NXAPPC_FAIL;
}
