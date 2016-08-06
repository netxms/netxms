/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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

/**
 * Static data
 */
static VolatileCounter m_recordId = 1;
static InetAddress m_auditServerAddr;
static WORD m_auditServerPort;
static int m_auditFacility;
static int m_auditSeverity;
static char m_auditTag[MAX_SYSLOG_TAG_LEN];
static char m_localHostName[256];

/**
 * Send syslog record to audit server
 */
static void SendSyslogRecord(const TCHAR *text)
{
   static char month[12][5] = { "Jan ", "Feb ", "Mar ", "Apr ",
                                "May ", "Jun ", "Jul ", "Aug ",
                                "Sep ", "Oct ", "Nov ", "Dec " };
	if (!m_auditServerAddr.isValidUnicast())
		return;

	time_t ts = time(NULL);
	struct tm *now = localtime(&ts);

   char message[1025];
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

   SOCKET hSocket = socket(m_auditServerAddr.getFamily(), SOCK_DGRAM, 0);
   if (hSocket != INVALID_SOCKET)
	{
		SockAddrBuffer addr;
		m_auditServerAddr.fillSockAddr(&addr, m_auditServerPort);
		sendto(hSocket, message, (int)strlen(message), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
		shutdown(hSocket, SHUT_RDWR);
		closesocket(hSocket);
	}
}

/**
 * Initalize audit log
 */
void InitAuditLog()
{
	DB_RESULT hResult;

	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   hResult = DBSelect(hdb, _T("SELECT max(record_id) FROM audit_log"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_recordId = DBGetFieldULong(hResult, 0, 0) + 1;
      DBFreeResult(hResult);
   }

	// External audit server
	TCHAR temp[256];
	ConfigReadStr(_T("ExternalAuditServer"), temp, 256, _T("none"));
	if (_tcscmp(temp, _T("none")))
	{
		m_auditServerAddr = InetAddress::resolveHostName(temp);
		m_auditServerPort = (WORD)ConfigReadInt(_T("ExternalAuditPort"), 514);
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
	DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Handler for EnumerateSessions()
 */
static void SendNewRecord(ClientSession *pSession, void *pArg)
{
   UPDATE_INFO *pUpdate;

   if (pSession->isAuthenticated() && pSession->isSubscribedTo(NXC_CHANNEL_AUDIT_LOG))
	{
      pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
      pUpdate->dwCategory = INFO_CAT_AUDIT_RECORD;
      pUpdate->pData = new NXCPMessage((NXCPMessage *)pArg);
      pSession->queueUpdate(pUpdate);
	}
}

/**
 * Write audit record
 */
void NXCORE_EXPORTABLE WriteAuditLog(const TCHAR *subsys, bool isSuccess, UINT32 userId,
                                     const TCHAR *workstation, int sessionId, UINT32 objectId,
                                     const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   WriteAuditLog2(subsys, isSuccess, userId, workstation, sessionId, objectId, format, args);
   va_end(args);
}

/**
 * Write audit record
 */
void NXCORE_EXPORTABLE WriteAuditLog2(const TCHAR *subsys, bool isSuccess, UINT32 userId,
                                      const TCHAR *workstation, int sessionId, UINT32 objectId,
                                      const TCHAR *format, va_list args)
{
	String text, query;
	NXCPMessage msg;

	text.appendFormattedStringV(format, args);

	query.appendFormattedString(_T("INSERT INTO audit_log (record_id,timestamp,subsystem,success,user_id,workstation,session_id,object_id,message) VALUES(%d,") TIME_T_FMT _T(",%s,%d,%d,%s,%d,%d,%s)"),
      InterlockedIncrement(&m_recordId), time(NULL), (const TCHAR *)DBPrepareString(g_dbDriver, subsys), isSuccess ? 1 : 0,
		userId, (const TCHAR *)DBPrepareString(g_dbDriver, workstation), sessionId, objectId, (const TCHAR *)DBPrepareString(g_dbDriver, text));
	QueueSQLRequest(query);

	msg.setCode(CMD_AUDIT_RECORD);
	msg.setField(VID_SUBSYSTEM, subsys);
	msg.setField(VID_SUCCESS_AUDIT, (WORD)isSuccess);
	msg.setField(VID_USER_ID, userId);
	msg.setField(VID_WORKSTATION, workstation);
   msg.setField(VID_SESSION_ID, sessionId);
	msg.setField(VID_OBJECT_ID, objectId);
	msg.setField(VID_MESSAGE, (const TCHAR *)text);
	EnumerateClientSessions(SendNewRecord, &msg);

	if (m_auditServerAddr.isValidUnicast())
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
			extText.appendFormattedString(_T("{%d}"), userId);
		}

		extText.appendFormattedString(_T("@%s] "), workstation);

		extText += (const TCHAR *)text;
		SendSyslogRecord((const TCHAR *)extText);
	}
}
