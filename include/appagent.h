/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: appagent.h
**
**/

#ifndef _appagent_h_
#define _appagent_h_

#ifdef _WIN32
#ifdef APPAGENT_EXPORTS
#define APPAGENT_EXPORTABLE __declspec(dllexport)
#else
#define APPAGENT_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define APPAGENT_EXPORTABLE
#endif

#include <nms_common.h>
#include <nms_util.h>

#ifdef _WIN32
#include <aclapi.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#endif

#ifndef SUN_LEN
#define SUN_LEN(su) (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif

#ifdef _WIN32
#define HPIPE HANDLE
#define INVALID_PIPE_HANDLE INVALID_HANDLE_VALUE
#else
#define HPIPE int
#define INVALID_PIPE_HANDLE (-1)
#endif

/**
 * Message constants
 */
#define APPAGENT_MSG_START_INDICATOR     "APPAGENT"
#define APPAGENT_MSG_START_INDICATOR_LEN 8
#define APPAGENT_MSG_SIZE_FIELD_LEN      2
#define APPAGENT_MSG_HEADER_LEN          16

/**
 * Command codes
 */
#define APPAGENT_CMD_GET_METRIC          0x0001
#define APPAGENT_CMD_LIST_METRICS        0x0002
#define APPAGENT_CMD_REQUEST_COMPLETED   0x0003

/**
 * Request completion codes
 */
#define APPAGENT_RCC_SUCCESS             0
#define APPAGENT_RCC_NO_SUCH_METRIC      1
#define APPAGENT_RCC_NO_SUCH_INSTANCE    2
#define APPAGENT_RCC_METRIC_READ_ERROR   3
#define APPAGENT_RCC_COMM_FAILURE        4
#define APPAGENT_RCC_BAD_REQUEST         5

/**
 * Communication message structure
 */

#ifdef __HP_aCC
#pragma pack 1
#else
#pragma pack(1)
#endif

typedef struct __apagent_msg
{
	char prefix[APPAGENT_MSG_START_INDICATOR_LEN];
	WORD length;		// message length including prefix and length field
	WORD command;		// command code
	WORD rcc;			// request completion code
	BYTE padding[2];
	BYTE payload[1];	// actual payload size determined by message length
} APPAGENT_MSG;

#ifdef __HP_aCC
#pragma pack
#else
#pragma pack()
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Application agent metric definition
 */
typedef struct __appagent_metric
{
	const TCHAR *name;
	const TCHAR *userArg;
	int (*handler)(const TCHAR *, const TCHAR *, TCHAR *);
	int type;
	const TCHAR *description;
} APPAGENT_METRIC;

/**
 * Application agent list definition
 */
typedef struct __appagent_list
{
	const TCHAR *name;
	const TCHAR *userArg;
	int (*handler)(const TCHAR *, const TCHAR *, StringList *);
	const TCHAR *description;
} APPAGENT_LIST;

/**
 * Application agent table definition
 */
typedef struct __appagent_table
{
	const TCHAR *name;
	const TCHAR *userArg;
	int (*handler)(const TCHAR *, const TCHAR *, Table *);
   const TCHAR *instanceColumns;
	const TCHAR *description;
} APPAGENT_TABLE;

/**
 * Agent initialization structure
 */
typedef struct __appagent_init
{
	const TCHAR *name;
	const TCHAR *userId;
	void (*messageDispatcher)(APPAGENT_MSG *);
	void (*logger)(int, const TCHAR *, va_list);
	int numMetrics;
	APPAGENT_METRIC *metrics;
	int numLists;
	APPAGENT_LIST *lists;
	int numTables;
	APPAGENT_LIST *tables;
} APPAGENT_INIT;

/**
 * Application-side API
 */
bool APPAGENT_EXPORTABLE AppAgentInit(APPAGENT_INIT *initData);
void APPAGENT_EXPORTABLE AppAgentStart();
void APPAGENT_EXPORTABLE AppAgentStop();
void APPAGENT_EXPORTABLE AppAgentPostEvent(int code, const TCHAR *name, const char *format, ...);

/**
 * Client-side API
 */
bool APPAGENT_EXPORTABLE AppAgentConnect(const TCHAR *name, HPIPE *hPipe);
void APPAGENT_EXPORTABLE AppAgentDisconnect(HPIPE hPipe);
int APPAGENT_EXPORTABLE AppAgentGetMetric(HPIPE hPipe, const TCHAR *name, TCHAR *value, int bufferSize);
int APPAGENT_EXPORTABLE AppAgentListMetrics(HPIPE hPipe, APPAGENT_METRIC **metrics, UINT32 *size);

#ifdef __cplusplus
}
#endif

#endif
