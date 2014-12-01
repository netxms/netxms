/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: nms_alarm.h
**
**/

#ifndef _nms_alarm_h_
#define _nms_alarm_h_

/**
 * Functions
 */
bool InitAlarmManager();
void ShutdownAlarmManager();

void SendAlarmsToClient(UINT32 dwRqId, ClientSession *pSession);
void FillAlarmInfoMessage(NXCPMessage *pMsg, NXC_ALARM *pAlarm);
void DeleteAlarmNotes(DB_HANDLE hdb, UINT32 alarmId);
void DeleteAlarmEvents(DB_HANDLE hdb, UINT32 alarmId);

UINT32 NXCORE_EXPORTABLE GetAlarm(UINT32 dwAlarmId, NXCPMessage *msg);
ObjectArray<NXC_ALARM> NXCORE_EXPORTABLE *GetAlarms(UINT32 objectId);
UINT32 NXCORE_EXPORTABLE GetAlarmEvents(UINT32 dwAlarmId, NXCPMessage *msg);
NetObj NXCORE_EXPORTABLE *GetAlarmSourceObject(UINT32 dwAlarmId);
NetObj NXCORE_EXPORTABLE *GetAlarmSourceObject(const TCHAR *hdref);  
int GetMostCriticalStatusForObject(UINT32 dwObjectId);
void GetAlarmStats(NXCPMessage *pMsg);

void NXCORE_EXPORTABLE CreateNewAlarm(TCHAR *pszMsg, TCHAR *pszKey, int nState,
                                      int iSeverity, UINT32 dwTimeout,
									           UINT32 dwTimeoutEvent, Event *pEvent, UINT32 ackTimeout);
UINT32 NXCORE_EXPORTABLE AckAlarmById(UINT32 dwAlarmId, ClientSession *session, bool sticky, UINT32 acknowledgmentActionTime);
UINT32 NXCORE_EXPORTABLE AckAlarmByHDRef(const TCHAR *hdref, ClientSession *session, bool sticky, UINT32 acknowledgmentActionTime);
UINT32 NXCORE_EXPORTABLE ResolveAlarmById(UINT32 dwAlarmId, ClientSession *session, bool terminate);
void NXCORE_EXPORTABLE ResolveAlarmByKey(const TCHAR *pszKey, bool useRegexp, bool terminate, Event *pEvent);
UINT32 NXCORE_EXPORTABLE ResolveAlarmByHDRef(const TCHAR *hdref, ClientSession *session, bool terminate);
void NXCORE_EXPORTABLE DeleteAlarm(UINT32 dwAlarmId, bool objectCleanup);

UINT32 AddAlarmComment(const TCHAR *hdref, const TCHAR *text, UINT32 userId);
UINT32 UpdateAlarmComment(UINT32 alarmId, UINT32 noteId, const TCHAR *text, UINT32 userId);
UINT32 DeleteAlarmCommentByID(UINT32 alarmId, UINT32 noteId);
UINT32 GetAlarmComments(UINT32 alarmId, NXCPMessage *msg);

bool DeleteObjectAlarms(UINT32 objectId, DB_HANDLE hdb);

UINT32 NXCORE_EXPORTABLE ResolveAlarmByHDRef(const TCHAR *hdref);
UINT32 NXCORE_EXPORTABLE TerminateAlarmByHDRef(const TCHAR *hdref);
void LoadHelpDeskLink();
UINT32 CreateHelpdeskIssue(const TCHAR *description, TCHAR *hdref);
UINT32 OpenHelpdeskIssue(UINT32 alarmId, ClientSession *session, TCHAR *hdref);
UINT32 AddHelpdeskIssueComment(const TCHAR *hdref, const TCHAR *text);
UINT32 GetHelpdeskIssueUrlFromAlarm(UINT32 alarmId, TCHAR *url, size_t size);
UINT32 GetHelpdeskIssueUrl(const TCHAR *hdref, TCHAR *url, size_t size);
UINT32 UnlinkHelpdeskIssueById(UINT32 dwAlarmId, ClientSession *session);
UINT32 UnlinkHelpdeskIssueByHDRef(const TCHAR *hdref, ClientSession *session);


#endif
