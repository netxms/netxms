/* 
** NetXMS - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** $module: nms_agent.h
**
**/

#ifndef _nms_agent_h_
#define _nms_agent_h_

#include <stdio.h>
#include <string.h>
#include <nms_common.h>


//
// Prefix for exportable functions
//

#ifdef _WIN32
#ifdef LIBNXAGENT_EXPORTS
#define LIBNXAGENT_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXAGENT_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXAGENT_EXPORTABLE
#endif


//
// Some constants
//

#define AGENT_LISTEN_PORT        4700
#define AGENT_PROTOCOL_VERSION   1
#define MAX_PARAM_NAME           256
#define MAX_RESULT_LENGTH        256


//
// Error codes
//

#define ERR_SUCCESS                 0
#define ERR_UNKNOWN_COMMAND         400
#define ERR_AUTH_REQUIRED           401
#define ERR_UNKNOWN_PARAMETER       404
#define ERR_REQUEST_TIMEOUT         408
#define ERR_AUTH_FAILED             440
#define ERR_ALREADY_AUTHENTICATED   441
#define ERR_AUTH_NOT_REQUIRED       442
#define ERR_INTERNAL_ERROR          500
#define ERR_NOT_IMPLEMENTED         501
#define ERR_OUT_OF_RESOURCES        503
#define ERR_NOT_CONNECTED           900
#define ERR_CONNECTION_BROKEN       901
#define ERR_BAD_RESPONCE            902


//
// Parameter handler return codes
//

#define SYSINFO_RC_SUCCESS       0
#define SYSINFO_RC_UNSUPPORTED   1
#define SYSINFO_RC_ERROR         2


//
// Subagent's parameter information
//

typedef struct
{
   char szName[MAX_PARAM_NAME];
   LONG (* fpHandler)(char *,char *,char *);
   char *pArg;
} NETXMS_SUBAGENT_PARAM;


//
// Subagent initialization structure
//

typedef struct
{
   DWORD dwVersion;
   DWORD dwNumParameters;
   NETXMS_SUBAGENT_PARAM *pParamList;
} NETXMS_SUBAGENT_INFO;


//
// Inline functions for returning parameters
//

#ifdef __cplusplus
#ifndef LIBNXAGENT_INLINE

inline void ret_string(char *rbuf, char *value)
{
   memset(rbuf, 0, MAX_RESULT_LENGTH);
   strncpy(rbuf, value, MAX_RESULT_LENGTH - 3);
   strcat(rbuf, "\r\n");
}

inline void ret_int(char *rbuf, long value)
{
   sprintf(rbuf, "%ld\r\n", value);
}

inline void ret_uint(char *rbuf, unsigned long value)
{
   sprintf(rbuf, "%lu\r\n", value);
}

inline void ret_double(char *rbuf, double value)
{
   sprintf(rbuf, "%f\r\n", value);
}

inline void ret_int64(char *rbuf, INT64 value)
{
#ifdef _WIN32
   sprintf(rbuf, "%I64d\r\n", value);
#else    /* _WIN32 */
   sprintf(rbuf, "%lld\r\n", value);
#endif   /* _WIN32 */
}

inline void ret_uint64(char *rbuf, QWORD value)
{
#ifdef _WIN32
   sprintf(rbuf, "%I64u\r\n", value);
#else    /* _WIN32 */
   sprintf(rbuf, "%llu\r\n", value);
#endif   /* _WIN32 */
}

#endif   /* LIBNXAGENT_INLINE */
#else    /* __cplusplus */

void LIBNXAGENT_EXPORTABLE ret_string(char *rbuf, char *value)
void LIBNXAGENT_EXPORTABLE ret_int(char *rbuf, long value)
void LIBNXAGENT_EXPORTABLE ret_uint(char *rbuf, unsigned long value)
void LIBNXAGENT_EXPORTABLE ret_double(char *rbuf, double value)
void LIBNXAGENT_EXPORTABLE ret_int64(char *rbuf, INT64 value)
void LIBNXAGENT_EXPORTABLE ret_uint64(char *rbuf, QWORD value)

#endif   /* __cplusplus */


//
// Functions from libnxagent
//

#ifdef __cplusplus
extern "C" {
#endif

void LIBNXAGENT_EXPORTABLE NxStrStrip(char *pszStr);
BOOL LIBNXAGENT_EXPORTABLE NxGetParameterArg(char *param, int index, char *arg, int maxSize);

#ifdef __cplusplus
}
#endif

#endif   /* _nms_agent_h_ */
