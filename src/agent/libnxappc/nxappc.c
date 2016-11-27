/*
** NetXMS Application Connector Library
** Copyright (C) 2015-2016 Raden Solutions
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

#if HAVE_CONFIG_H
#include <config.h>
#else
#define HAVE_ALLOCA_H   1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32

#include <winsock2.h>
#include <windows.h>
#include <malloc.h>

#define SELECT_NFDS(f) (0)

#else /* _WIN32 */

#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

#if HAVE_ALLOCA_H
#include <alloca.h>
#endif

#define SOCKET int
#define closesocket(x) close(x)

#define SELECT_NFDS(f) ((f) + 1)

#ifndef SUN_LEN
#define SUN_LEN(su) (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif

#endif /* _WIN32 */

#include "nxappc.h"

static char s_channel[128] = "";
static SOCKET s_socket = -1;

/**
 * Check if channel is ready
 */
#define CHECK_CHANNEL do { if (s_socket == -1) { nxappc_open_channel(); if (s_socket == -1) return NXAPPC_FAIL; } } while(0)

/**
 * Set channel name
 */
int LIBNXAPPC_EXPORTABLE nxappc_set_channel_name(const char *channel)
{
   if (!strcmp(channel, s_channel))
      return NXAPPC_SUCCESS;  // already connected
   strncpy(s_channel, channel, 128);
   if (s_socket != -1)
   {
      closesocket(s_socket);
      s_socket = -1;
   }
   return NXAPPC_SUCCESS;
}

/**
 * Open channel for communication
 */
int LIBNXAPPC_EXPORTABLE nxappc_open_channel(void)
{
#ifdef _WIN32
	struct sockaddr_in addrLocal;
	u_long one = 1;
#else
	struct sockaddr_un addrLocal;
   int f;
#endif

   if (s_socket != -1)
   {
      closesocket(s_socket);
   }

#ifdef _WIN32
   s_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_socket == -1)
		return NXAPPC_FAIL;
	
	addrLocal.sin_family = AF_INET;
   addrLocal.sin_addr.s_addr = inet_addr("127.0.0.1");
   addrLocal.sin_port = htons(atoi(s_channel));
	if (connect(s_socket, (struct sockaddr *)&addrLocal, sizeof(addrLocal)) == -1)
   {
      closesocket(s_socket);
      s_socket = -1;
		return NXAPPC_FAIL;
   }

#else
	s_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (s_socket == -1)
		return NXAPPC_FAIL;
	
	addrLocal.sun_family = AF_UNIX;
   sprintf(addrLocal.sun_path, "/tmp/.nxappc.%s", s_channel);
	if (connect(s_socket, (struct sockaddr *)&addrLocal, SUN_LEN(&addrLocal)) == -1)
   {
      closesocket(s_socket);
      s_socket = -1;
		return NXAPPC_FAIL;
   }

#endif

   return NXAPPC_SUCCESS;
}

/**
 * Reset channel
 */
int LIBNXAPPC_EXPORTABLE nxappc_reset_channel(void)
{
   if (s_socket != -1)
   {
      closesocket(s_socket);
      s_socket = -1;
   }
   return nxappc_open_channel();
}

/**
 * Send with retry
 */
static int send_data(void *data, int size)
{
   int rc = send(s_socket, data, size, 0);
   if (rc >= 0)
      return rc;
   rc = nxappc_reset_channel();
   if (rc < 0)
      return rc;
   return send(s_socket, data, size, 0);
}

/**
 * Send event
 */
int LIBNXAPPC_EXPORTABLE nxappc_send_event(int code, const char *name, const char *format, ...)
{
   CHECK_CHANNEL;
   return NXAPPC_FAIL;
}

/**
 * Send binary data
 */
int LIBNXAPPC_EXPORTABLE nxappc_send_data(void *data, int size)
{
   char *msg;
#ifdef _WIN32
   int rc;
   char buffer[4096];
#endif

   if ((size < 0) || (size > 65532))
      return NXAPPC_FAIL;

   CHECK_CHANNEL;

#ifdef _WIN32
   msg = (size <= 4092) ? buffer : malloc(size + 4);
#else
   msg = alloca(size + 4);
#endif
   msg[0] = NXAPPC_CMD_SEND_DATA;
   msg[1] = 0; // reserved
   *((unsigned short *)&msg[2]) = (unsigned short)size;
   memcpy(&msg[4], data, size);
#ifdef _WIN32
   rc = (send_data(msg, size + 4) == size + 4) ? NXAPPC_SUCCESS : NXAPPC_FAIL;
   if (msg != buffer)
      free(msg);
   return rc;
#else
   return (send_data(msg, size + 4) == size + 4) ? NXAPPC_SUCCESS : NXAPPC_FAIL;
#endif
}

/**
 * Register counter
 */
int LIBNXAPPC_EXPORTABLE nxappc_register_counter(const char *counter, const char *instance)
{
   CHECK_CHANNEL;
   return NXAPPC_FAIL;
}

/**
 * Update integer counter
 */
int LIBNXAPPC_EXPORTABLE nxappc_update_counter_long(const char *counter, const char *instance, long diff)
{
   CHECK_CHANNEL;
   return NXAPPC_FAIL;
}

/**
 * Update floating point counter
 */
int LIBNXAPPC_EXPORTABLE nxappc_update_counter_double(const char *counter, const char *instance, double diff)
{
   CHECK_CHANNEL;
   return NXAPPC_FAIL;
}

/**
 * Reset counter
 */
int LIBNXAPPC_EXPORTABLE nxappc_reset_counter(const char *counter, const char *instance)
{
   CHECK_CHANNEL;
   return NXAPPC_FAIL;
}

/**
 * Update integer counter
 */
int LIBNXAPPC_EXPORTABLE nxappc_set_counter_long(const char *counter, const char *instance, long value)
{
   CHECK_CHANNEL;

   return NXAPPC_FAIL;
}

/**
 * Set floating point counter
 */
int LIBNXAPPC_EXPORTABLE nxappc_set_counter_double(const char *counter, const char *instance, double value)
{
   CHECK_CHANNEL;
   return NXAPPC_FAIL;
}
