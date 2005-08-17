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

#include <nms_common.h>
#include <nms_util.h>


//
// Some constants
//

#define AGENT_LISTEN_PORT        4700
#define AGENT_PROTOCOL_VERSION   2
#define MAX_RESULT_LENGTH        256
#define MAX_CMD_LEN              256
#define COMMAND_TIMEOUT          60
#define MAX_SUBAGENT_NAME        64


//
// Error codes
//

#define ERR_SUCCESS                 ((DWORD)0)
#define ERR_UNKNOWN_COMMAND         ((DWORD)400)
#define ERR_AUTH_REQUIRED           ((DWORD)401)
#define ERR_ACCESS_DENIED           ((DWORD)403)
#define ERR_UNKNOWN_PARAMETER       ((DWORD)404)
#define ERR_REQUEST_TIMEOUT         ((DWORD)408)
#define ERR_AUTH_FAILED             ((DWORD)440)
#define ERR_ALREADY_AUTHENTICATED   ((DWORD)441)
#define ERR_AUTH_NOT_REQUIRED       ((DWORD)442)
#define ERR_INTERNAL_ERROR          ((DWORD)500)
#define ERR_NOT_IMPLEMENTED         ((DWORD)501)
#define ERR_OUT_OF_RESOURCES        ((DWORD)503)
#define ERR_NOT_CONNECTED           ((DWORD)900)
#define ERR_CONNECTION_BROKEN       ((DWORD)901)
#define ERR_BAD_RESPONCE            ((DWORD)902)
#define ERR_IO_FAILURE              ((DWORD)903)
#define ERR_RESOURCE_BUSY           ((DWORD)904)
#define ERR_EXEC_FAILED             ((DWORD)905)
#define ERR_ENCRYPTION_REQUIRED     ((DWORD)906)
#define ERR_NO_CIPHERS              ((DWORD)907)
#define ERR_INVALID_PUBLIC_KEY      ((DWORD)908)
#define ERR_INVALID_SESSION_KEY     ((DWORD)909)
#define ERR_CONNECT_FAILED          ((DWORD)910)
#define ERR_MAILFORMED_COMMAND      ((DWORD)911)


//
// Parameter handler return codes
//

#define SYSINFO_RC_SUCCESS       0
#define SYSINFO_RC_UNSUPPORTED   1
#define SYSINFO_RC_ERROR         2


//
// Connection handle
//

typedef void *HCONN;


//
// Structure for holding enumeration results
//

typedef struct
{
   DWORD dwNumStrings;
   TCHAR **ppStringList;
} NETXMS_VALUES_LIST;


//
// Subagent's parameter information
//

typedef struct
{
   TCHAR szName[MAX_PARAM_NAME];
   LONG (* fpHandler)(TCHAR *, TCHAR *, TCHAR *);
   TCHAR *pArg;
   int iDataType;
   TCHAR szDescription[MAX_DB_STRING];
} NETXMS_SUBAGENT_PARAM;


//
// Subagent's enum information
//

typedef struct
{
   TCHAR szName[MAX_PARAM_NAME];
   LONG (* fpHandler)(TCHAR *, TCHAR *, NETXMS_VALUES_LIST *);
   TCHAR *pArg;
} NETXMS_SUBAGENT_ENUM;


//
// Subagent's action information
//

typedef struct
{
   TCHAR szName[MAX_PARAM_NAME];
   LONG (* fpHandler)(TCHAR *, NETXMS_VALUES_LIST *, TCHAR *);
   TCHAR *pArg;
   TCHAR szDescription[MAX_DB_STRING];
} NETXMS_SUBAGENT_ACTION;


//
// Subagent initialization structure
//

#define NETXMS_SUBAGENT_INFO_MAGIC     ((DWORD)0xAD000204)

class CSCPMessage;

typedef struct
{
   DWORD dwMagic;    // Magic number to check if subagent uses correct version of this structure
   TCHAR szName[MAX_SUBAGENT_NAME];
   TCHAR szVersion[32];
   void (* pUnloadHandler)(void);   // Called at subagent unload. Can be NULL.
   BOOL (* pCommandHandler)(DWORD dwCommand, CSCPMessage *pRequest,
                            CSCPMessage *pResponce);
   DWORD dwNumParameters;
   NETXMS_SUBAGENT_PARAM *pParamList;
   DWORD dwNumEnums;
   NETXMS_SUBAGENT_ENUM *pEnumList;
   DWORD dwNumActions;
   NETXMS_SUBAGENT_ACTION *pActionList;
} NETXMS_SUBAGENT_INFO;


//
// Inline functions for returning parameters
//

#ifdef __cplusplus
#ifndef LIBNETXMS_INLINE

#include <nms_agent.h>

inline void ret_string(TCHAR *rbuf, TCHAR *value)
{
   _tcsncpy(rbuf, value, MAX_RESULT_LENGTH);
}

inline void ret_int(TCHAR *rbuf, long value)
{
   _stprintf(rbuf, _T("%ld"), value);
}

inline void ret_uint(TCHAR *rbuf, unsigned long value)
{
   _stprintf(rbuf, _T("%lu"), value);
}

inline void ret_double(TCHAR *rbuf, double value)
{
   _stprintf(rbuf, _T("%f"), value);
}

inline void ret_int64(TCHAR *rbuf, INT64 value)
{
#ifdef _WIN32
   _stprintf(rbuf, _T("%I64d"), value);
#else    /* _WIN32 */
   _stprintf(rbuf, _T("%lld"), value);
#endif   /* _WIN32 */
}

inline void ret_uint64(TCHAR *rbuf, QWORD value)
{
#ifdef _WIN32
   _stprintf(rbuf, _T("%I64u"), value);
#else    /* _WIN32 */
   _stprintf(rbuf, _T("%llu"), value);
#endif   /* _WIN32 */
}

#endif   /* LIBNETXMS_INLINE */
#else    /* __cplusplus */

void LIBNETXMS_EXPORTABLE ret_string(TCHAR *rbuf, TCHAR *value);
void LIBNETXMS_EXPORTABLE ret_int(TCHAR *rbuf, long value);
void LIBNETXMS_EXPORTABLE ret_uint(TCHAR *rbuf, unsigned long value);
void LIBNETXMS_EXPORTABLE ret_double(TCHAR *rbuf, double value);
void LIBNETXMS_EXPORTABLE ret_int64(TCHAR *rbuf, INT64 value);
void LIBNETXMS_EXPORTABLE ret_uint64(TCHAR *rbuf, QWORD value);

#endif   /* __cplusplus */


//
// Functions from libnetxms
//

#ifdef __cplusplus
extern "C" {
#endif

BOOL LIBNETXMS_EXPORTABLE NxGetParameterArg(TCHAR *param, int index, TCHAR *arg, int maxSize);
void LIBNETXMS_EXPORTABLE NxAddResultString(NETXMS_VALUES_LIST *pList, TCHAR *pszString);
void LIBNETXMS_EXPORTABLE NxWriteAgentLog(int iLevel, TCHAR *pszFormat, ...);

#ifdef __cplusplus
}
#endif

#endif   /* _nms_agent_h_ */
