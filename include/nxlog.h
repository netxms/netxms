/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: nxlog.h
**
**/

#ifndef _nxlog_h_
#define _nxlog_h_


//
// Constants
//

#define MAX_OBJECT_NAME          64
#define MAX_LOG_MSG_LENGTH       1024
#define MAX_SYSLOG_HOSTNAME_LEN  128
#define MAX_SYSLOG_TAG_LEN       33


//
// Syslog severity codes
//

#define SYSLOG_SEVERITY_EMERGENCY      0
#define SYSLOG_SEVERITY_ALERT          1
#define SYSLOG_SEVERITY_CRITICAL       2
#define SYSLOG_SEVERITY_ERROR          3
#define SYSLOG_SEVERITY_WARNING        4
#define SYSLOG_SEVERITY_NOTICE         5
#define SYSLOG_SEVERITY_INFORMATIONAL  6
#define SYSLOG_SEVERITY_DEBUG          7


//
// Policy flags
//

#define NX_LPPF_WINDOWS_EVENT_LOG      0x0001
#define NX_LPPF_REPORT_UNMATCHED       0x0002


//
// Syslog message structure
//

typedef struct
{
   QWORD qwMsgId;       // NetXMS internal message ID
   time_t tmTimeStamp;
   int nFacility;
   int nSeverity;
   UINT32 dwSourceObject;
   char szHostName[MAX_SYSLOG_HOSTNAME_LEN];
   char szTag[MAX_SYSLOG_TAG_LEN];
   char szMessage[MAX_LOG_MSG_LENGTH];
} NX_SYSLOG_RECORD;


#endif
