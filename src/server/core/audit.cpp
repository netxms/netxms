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
static void SendNewRecord(ClientSession *session, void *arg)
{
   if (!session->isAuthenticated() || !session->isSubscribedTo(NXC_CHANNEL_AUDIT_LOG))
      return;

   UPDATE_INFO *pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
   pUpdate->dwCategory = INFO_CAT_AUDIT_RECORD;
   pUpdate->pData = new NXCPMessage((NXCPMessage *)arg);
   session->queueUpdate(pUpdate);
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
   WriteAuditLogWithValues2(subsys, isSuccess, userId, workstation, sessionId, objectId, NULL, NULL, format, args);
   va_end(args);
}

/**
 * Write audit record
 */
void NXCORE_EXPORTABLE WriteAuditLog2(const TCHAR *subsys, bool isSuccess, UINT32 userId,
                                      const TCHAR *workstation, int sessionId, UINT32 objectId,
                                      const TCHAR *format, va_list args)
{
   WriteAuditLogWithValues2(subsys, isSuccess, userId, workstation, sessionId, objectId, NULL, NULL, format, args);
}

/**
 * Write audit record with old and new values
 */
void NXCORE_EXPORTABLE WriteAuditLogWithValues(const TCHAR *subsys, bool isSuccess, UINT32 userId,
                                               const TCHAR *workstation, int sessionId, UINT32 objectId,
                                               const TCHAR *oldValue, const TCHAR *newValue,
                                               const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   WriteAuditLogWithValues2(subsys, isSuccess, userId, workstation, sessionId, objectId, oldValue, newValue, format, args);
   va_end(args);
}

/**
 * Write audit record with old and new values in JSON format
 */
void NXCORE_EXPORTABLE WriteAuditLogWithJsonValues(const TCHAR *subsys, bool isSuccess, UINT32 userId,
                                               const TCHAR *workstation, int sessionId, UINT32 objectId,
                                               json_t *oldValue, json_t *newValue,
                                               const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   WriteAuditLogWithJsonValues2(subsys, isSuccess, userId, workstation, sessionId, objectId, oldValue, newValue, format, args);
   va_end(args);
}

/**
 * Write audit record with old and new values
 */
void NXCORE_EXPORTABLE WriteAuditLogWithValues2(const TCHAR *subsys, bool isSuccess, UINT32 userId,
                                                const TCHAR *workstation, int sessionId, UINT32 objectId,
                                                const TCHAR *oldValue, const TCHAR *newValue,
                                                const TCHAR *format, va_list args)
{
	String text;
	text.appendFormattedStringV(format, args);

	TCHAR recordId[16], _time[32], success[2], _userId[16], _sessionId[16], _objectId[16];
   const TCHAR *values[11] = { recordId, _time, subsys, success, _userId, workstation, _sessionId, _objectId, (const TCHAR *)text, oldValue, newValue };
   _sntprintf(recordId, 16, _T("%d"), InterlockedIncrement(&m_recordId));
   _sntprintf(_time, 32, _T("%d"), (UINT32)time(NULL));
   _sntprintf(success, 2, _T("%d"), isSuccess);
   _sntprintf(_userId, 16, _T("%d"), userId);
   _sntprintf(_sessionId, 16, _T("%d"), sessionId);
   _sntprintf(_objectId, 16, _T("%d"), objectId);

   static int sqlTypes[11] = { DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_VARCHAR, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_VARCHAR, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_TEXT, DB_SQLTYPE_TEXT, DB_SQLTYPE_TEXT };
   if ((oldValue != NULL) && (newValue != NULL))
      QueueSQLRequest(_T("INSERT INTO audit_log (record_id,timestamp,subsystem,success,user_id,workstation,session_id,object_id,message,old_value,new_value) VALUES (?,?,?,?,?,?,?,?,?,?,?)"), 11, sqlTypes, values);
   else
      QueueSQLRequest(_T("INSERT INTO audit_log (record_id,timestamp,subsystem,success,user_id,workstation,session_id,object_id,message) VALUES (?,?,?,?,?,?,?,?,?)"), 9, sqlTypes, values);

	NXCPMessage msg;
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

/**
 * Write audit record with old and new values
 */
void NXCORE_EXPORTABLE WriteAuditLogWithJsonValues2(const TCHAR *subsys, bool isSuccess, UINT32 userId,
                                                    const TCHAR *workstation, int sessionId, UINT32 objectId,
                                                    json_t *oldValue, json_t *newValue,
                                                    const TCHAR *format, va_list args)
{
   char *js1 = (oldValue != NULL) ? json_dumps(oldValue, 0) : strdup("");
   char *js2 = (newValue != NULL) ? json_dumps(newValue, 0) : strdup("");
#ifdef UNICODE
   WCHAR *js1w = WideStringFromUTF8String(js1);
   WCHAR *js2w = WideStringFromUTF8String(js2);
   WriteAuditLogWithValues2(subsys, isSuccess, userId, workstation, sessionId, objectId, js1w, js2w, format, args);
   free(js1w);
   free(js2w);
#else
   WriteAuditLogWithValues2(subsys, isSuccess, userId, workstation, sessionId, objectId, js1, js2, format, args);
#endif
   free(js1);
   free(js2);
}
