/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: audit.cpp
**
**/

#include "nxcore.h"


//
// Static data
//

static DWORD m_dwRecordId = 1;
static DWORD m_auditServerAddr = 0;
static WORD m_auditServerPort;
static int m_auditFacility;
static int m_auditSeverity;
static char m_auditTag[MAX_SYSLOG_TAG_LEN];
static char m_localHostName[256];


//
// Send syslog record to audit server
//

static void SendSyslogRecord(const TCHAR *text)
{
   static char month[12][5] = { "Jan ", "Feb ", "Mar ", "Apr ",
                                "May ", "Jun ", "Jul ", "Aug ",
                                "Sep ", "Oct ", "Nov ", "Dec " };
	char message[1025];

	if (m_auditServerAddr == 0)
		return;

	time_t ts = time(NULL);
	struct tm *now = localtime(&ts);

#ifdef UNICODE
	char *mbText = MBStringFromWideString(text);
	snprintf(message, 1025, "<%d>%s %2d %02d:%02d:%02d %s %s %s", (m_auditFacility << 3) + m_auditSeverity, month[now->tm_mon],
	         now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, m_localHostName, m_auditTag, mbText);
	free(mbText);
#else
	snprintf(message, 1025, "<%d>%s %2d %02d:%02d:%02d %s %s %s", (m_auditFacility << 3) + m_auditSeverity, month[now->tm_mon],
	         now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, m_localHostName, m_auditTag, text);
#endif
	message[1024] = 0;

   SOCKET hSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (hSocket != 0)
	{
		struct sockaddr_in addr;

		memset(&addr, 0, sizeof(struct sockaddr_in));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = m_auditServerAddr;
		addr.sin_port = m_auditServerPort;

		sendto(hSocket, message, (int)strlen(message), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
		shutdown(hSocket, SHUT_RDWR);
		closesocket(hSocket);
	}
}


//
// Initalize audit log
//

void InitAuditLog()
{
	DB_RESULT hResult;

   hResult = DBSelect(g_hCoreDB, _T("SELECT max(record_id) FROM audit_log"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwRecordId = DBGetFieldULong(hResult, 0, 0) + 1;
      DBFreeResult(hResult);
   }

	// External audit server
	TCHAR temp[256];
	ConfigReadStr(_T("ExternalAuditServer"), temp, 256, _T("none"));
	if (_tcscmp(temp, _T("none")))
	{
		m_auditServerAddr = ResolveHostName(temp);
		m_auditServerPort = htons((WORD)ConfigReadInt(_T("ExternalAuditPort"), 514));
		m_auditFacility = ConfigReadInt(_T("ExternalAuditFacility"), 13);  // default is log audit facility
		m_auditSeverity = ConfigReadInt(_T("ExternalAuditSeverity"), SYSLOG_SEVERITY_NOTICE);
		ConfigReadStrA(_T("ExternalAuditTag"), m_auditTag, MAX_SYSLOG_TAG_LEN, "netxmsd-audit");

		// Get local host name
#ifdef _WIN32
		DWORD size = 256;
		GetComputerNameA(m_localHostName, &size);
#else
		gethostname(m_localHostName, 256);
		m_localHostName[255] = 0;
		char *ptr = strchr(m_localHostName, '.');
		if (ptr != NULL)
			*ptr = 0;
#endif

		SendSyslogRecord(_T("NetXMS server audit subsystem started"));
	}
}


//
// Handler for EnumerateSessions()
//

static void SendNewRecord(ClientSession *pSession, void *pArg)
{
   UPDATE_INFO *pUpdate;

   if (pSession->isAuthenticated() && pSession->isSubscribed(NXC_CHANNEL_AUDIT_LOG))
	{
      pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
      pUpdate->dwCategory = INFO_CAT_AUDIT_RECORD;
      pUpdate->pData = new CSCPMessage((CSCPMessage *)pArg);
      pSession->queueUpdate(pUpdate);
	}
}


//
// Write audit record
//

void NXCORE_EXPORTABLE WriteAuditLog(const TCHAR *subsys, BOOL isSuccess, DWORD userId,
                                     const TCHAR *workstation, DWORD objectId,
                                     const TCHAR *format, ...)
{
	String text, query;
	va_list args;
	CSCPMessage msg;

	va_start(args, format);
	text.addFormattedStringV(format, args);
	va_end(args);

	query.addFormattedString(_T("INSERT INTO audit_log (record_id,timestamp,subsystem,success,user_id,workstation,object_id,message) VALUES(%d,") TIME_T_FMT _T(",%s,%d,%d,%s,%d,%s)"),
		       m_dwRecordId++, time(NULL), (const TCHAR *)DBPrepareString(g_hCoreDB, subsys), isSuccess ? 1 : 0, 
		       userId, (const TCHAR *)DBPrepareString(g_hCoreDB, workstation), objectId, (const TCHAR *)DBPrepareString(g_hCoreDB, text));
	QueueSQLRequest(query);

	msg.SetCode(CMD_AUDIT_RECORD);
	msg.SetVariable(VID_SUBSYSTEM, subsys);
	msg.SetVariable(VID_SUCCESS_AUDIT, (WORD)isSuccess);
	msg.SetVariable(VID_USER_ID, userId);
	msg.SetVariable(VID_WORKSTATION, workstation);
	msg.SetVariable(VID_OBJECT_ID, objectId);
	msg.SetVariable(VID_MESSAGE, (const TCHAR *)text);
	EnumerateClientSessions(SendNewRecord, &msg);

	if (m_auditServerAddr != 0)
	{
		String extText;
		TCHAR buffer[256];

		extText = _T("[");
		if (ResolveUserId(userId, buffer, 256))
		{
			extText += buffer;
		}
		else
		{
			extText.addFormattedString(_T("{%d}"), userId);
		}

		extText.addFormattedString(_T("@%s] "), workstation);

		extText += (const TCHAR *)text;
		SendSyslogRecord((const TCHAR *)extText);
	}
}
