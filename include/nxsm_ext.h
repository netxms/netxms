/* $Id: nxsm_ext.h,v 1.4 2006-01-09 07:40:35 alk Exp $ */
/* 
** NetXMS - Network Management System
** Session Manager
** Copyright (C) 2005 Alex Kirhenshtein
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
**/

#ifndef __NXSM_EXT__H__
#define __NXSM_EXT__H__

enum
{
	CMD_NXSM_LOGIN,
	CMD_NXSM_CHECK_SESSION,
	CMD_NXSM_LOGOUT,

	CMD_NXSM_GET_OBJECTS,
	CMD_NXSM_GET_ALARMS,
	CMD_NXSM_ALARM_ACK,

	CMD_NXSM_STATUS,

	CMD_NXSM_LAST
};

enum
{
	VID_NXSM_LOGIN,
	VID_NXSM_PASSWORD,
	VID_NXSM_SID,
	VID_NXSM_DATA_SIZE,

	VID_NXSM_DATA = 128 // SHOULD BE LAST IN ENUM!
};


typedef struct t_NXSM_ALARM
{
	DWORD dwAlarmId;
	DWORD dwTimeStamp;
	WORD wSeverity;
	TCHAR szSourceObject[MAX_OBJECT_NAME];
	TCHAR szMessage[MAX_DB_STRING];
} NXSM_ALARM;

typedef struct t_W_NXSM_ALARM
{
	int count;
	NXSM_ALARM *data;
} W_NXSM_ALARM;

typedef struct
{
   DWORD dwNumAlarms;
	DWORD dwNumAlarmsNormal;
	DWORD dwNumAlarmsWarning;
	DWORD dwNumAlarmsMinor;
	DWORD dwNumAlarmsMajor;
	DWORD dwNumAlarmsCritical;
   DWORD dwNumObjects;
   DWORD dwNumNodes;
   DWORD dwNumDCI;
   DWORD dwNumClientSessions;
   DWORD dwServerUptime;
   DWORD dwServerProcessVMSize;
   DWORD dwServerProcessWorkSet;
   TCHAR szServerVersion[MAX_DB_STRING];
} NXSM_SERVER_STATS;


#endif // __NXSM_EXT__H__

//////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.3  2005/12/21 22:42:00  alk
+ get server stats

Revision 1.2  2005/12/21 03:05:40  alk
1) nxsmcl added to configure
2) Makefile.PL+autoconf -- looks like it works, but...
3) building on linux

Revision 1.1  2005/12/20 23:14:07  alk
nxsm_ext.h moved to global includes

Revision 1.4  2005/12/15 00:01:50  alk
session manager(and sm client) can handle login/logout/alarms list/alarm ack

Revision 1.3  2005/12/12 00:37:05  alk
initial version of web interface
*NOT WORKING*
if anyone can comment this, i would be happy:
c:\work\nms\include\nms_threads.h(65) : error C2054: expected '(' to follow 'inline'

Revision 1.2  2005/09/27 01:13:03  alk
license text block added


*/
