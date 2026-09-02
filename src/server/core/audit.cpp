/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Raden Solutions
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
#include <nms_users.h>
#include <openssl/hmac.h>

/**
 * Audit log HMAC key
 */
char g_auditLogKey[128] = "";

/**
 * Static data
 */
static VolatileCounter s_recordId = 0;
static InetAddress s_auditServerAddr;
static uint16_t s_auditServerPort;
static int s_auditFacility;
static int s_auditSeverity;
static char s_auditTag[MAX_SYSLOG_TAG_LEN];
static char s_localHostName[256];

/**
 * Getter for last used audit log record ID
 */
int32_t GetLastAuditRecordId()
{
   return s_recordId;
}

/**
 * Send syslog record to audit server
 */
static void SendSyslogRecord(const TCHAR *text)
{
   static char month[12][5] = { "Jan ", "Feb ", "Mar ", "Apr ",
                                "May ", "Jun ", "Jul ", "Aug ",
                                "Sep ", "Oct ", "Nov ", "Dec " };
   if (!s_auditServerAddr.isValidUnicast())
      return;

   time_t ts = time(nullptr);
#if HAVE_LOCALTIME_R
   struct tm tmbuffer;
   struct tm *now = localtime_r(&ts, &tmbuffer);
#else
   struct tm *now = localtime(&ts);
#endif

   char *mbText = nullptr;
   if (ConfigReadBoolean(_T("AuditLog.External.UseUTF8"), false))
   {
      mbText = UTF8StringFromWideString(text);
   }
   else
   {
      mbText = MBStringFromWideString(text);
   }
   char message[1025];
   snprintf(message, 1025, "<%d>%s %2d %02d:%02d:%02d %s %s %s", (s_auditFacility << 3) + s_auditSeverity, month[now->tm_mon],
         now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, s_localHostName, s_auditTag,
         (mbText != nullptr) ? mbText : reinterpret_cast<const char*>(text));
   MemFree(mbText);
   message[1024] = 0;

   SOCKET hSocket = CreateSocket(s_auditServerAddr.getFamily(), SOCK_DGRAM, 0);
   if (hSocket != INVALID_SOCKET)
	{
		SockAddrBuffer addr;
		s_auditServerAddr.fillSockAddr(&addr, s_auditServerPort);
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
   int32_t id = ConfigReadInt(_T("LastAuditRecordId"), s_recordId);
   if (id > s_recordId)
      s_recordId = id;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT max(record_id) FROM audit_log"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_recordId = std::max(DBGetFieldLong(hResult, 0, 0), static_cast<int32_t>(s_recordId));
      DBFreeResult(hResult);
   }
   s_recordId += static_cast<int32_t>(HAGetRecordIdGap());

	// External audit server
	TCHAR temp[256];
	ConfigReadStr(_T("AuditLog.External.Server"), temp, 256, _T("none"));
	if (_tcscmp(temp, _T("none")))
	{
		s_auditServerAddr = InetAddress::resolveHostName(temp);
		s_auditServerPort = (WORD)ConfigReadInt(_T("AuditLog.External.Port"), 514);
		s_auditFacility = ConfigReadInt(_T("AuditLog.External.Facility"), 13);  // default is log audit facility
		s_auditSeverity = ConfigReadInt(_T("AuditLog.External.Severity"), SYSLOG_SEVERITY_NOTICE);
		ConfigReadStrA(_T("AuditLog.External.Tag"), s_auditTag, MAX_SYSLOG_TAG_LEN, "netxmsd-audit");

		// Get local host name
#ifdef _WIN32
		DWORD size = 256;
		GetComputerNameA(s_localHostName, &size);
#else
		gethostname(s_localHostName, 256);
		s_localHostName[255] = 0;
		char *ptr = strchr(s_localHostName, '.');
		if (ptr != nullptr)
			*ptr = 0;
#endif

		SendSyslogRecord(_T("NetXMS server audit subsystem started"));
	}
	DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Write audit record
 */
void NXCORE_EXPORTABLE WriteAuditLog(const TCHAR *subsys, bool isSuccess, uint32_t userId, const TCHAR *workstation,
         session_id_t sessionId, uint32_t objectId, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   WriteAuditLogWithValues2(subsys, isSuccess, userId, workstation, sessionId, objectId, nullptr, nullptr, 0, format, args);
   va_end(args);
}

/**
 * Write audit record
 */
void NXCORE_EXPORTABLE WriteAuditLog2(const TCHAR *subsys, bool isSuccess, uint32_t userId, const TCHAR *workstation,
         session_id_t sessionId, uint32_t objectId, const TCHAR *format, va_list args)
{
   WriteAuditLogWithValues2(subsys, isSuccess, userId, workstation, sessionId, objectId, nullptr, nullptr, 0, format, args);
}

/**
 * Write audit record with old and new values
 */
void NXCORE_EXPORTABLE WriteAuditLogWithValues(const TCHAR *subsys, bool isSuccess, uint32_t userId, const TCHAR *workstation,
         session_id_t sessionId, uint32_t objectId, const TCHAR *oldValue, const TCHAR *newValue, char valueType, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   WriteAuditLogWithValues2(subsys, isSuccess, userId, workstation, sessionId, objectId, oldValue, newValue, valueType, format, args);
   va_end(args);
}

/**
 * Write audit record with old and new values in JSON format
 */
void NXCORE_EXPORTABLE WriteAuditLogWithJsonValues(const TCHAR *subsys, bool isSuccess, uint32_t userId, const TCHAR *workstation,
         session_id_t sessionId, uint32_t objectId, json_t *oldValue, json_t *newValue, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   WriteAuditLogWithJsonValues2(subsys, isSuccess, userId, workstation, sessionId, objectId, oldValue, newValue, format, args);
   va_end(args);
}

/**
 * Write audit record with old and new values and pre-formatted message text
 */
static void WriteAuditRecord(const TCHAR *subsys, bool isSuccess, uint32_t userId, const TCHAR *workstation,
         session_id_t sessionId, uint32_t objectId, const TCHAR *oldValue, const TCHAR *newValue, char valueType, const TCHAR *text)
{
	TCHAR recordId[16], _time[32], success[2], _userId[16], _sessionId[16], _objectId[16], _valueType[2], _hmac[SHA256_DIGEST_SIZE * 2 + 1];
   const TCHAR *values[13] = {
      recordId, _time, subsys, success, _userId,
      (workstation != nullptr) && (*workstation != 0) ? workstation : _T("SYSTEM"),
      _sessionId, _objectId, text, nullptr, nullptr, nullptr, nullptr };
   static int sqlTypes[13] = { DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_VARCHAR, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_VARCHAR, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_TEXT, -1, -1, -1, -1 };
   _sntprintf(recordId, 16, _T("%d"), InterlockedIncrement(&s_recordId));
   _sntprintf(_time, 32, INT64_FMT, static_cast<int64_t>(time(nullptr)));
   _sntprintf(success, 2, _T("%d"), isSuccess ? 1 : 0);
   _sntprintf(_userId, 16, _T("%d"), userId);
   _sntprintf(_sessionId, 16, _T("%d"), sessionId);
   _sntprintf(_objectId, 16, _T("%d"), objectId);
   _valueType[0] = valueType;
   _valueType[1] = 0;

   int bindCount = 9;
   StringBuffer query = _T("INSERT INTO audit_log (record_id,timestamp,subsystem,success,user_id,workstation,session_id,object_id,message");
   if (oldValue != nullptr)
   {
      query.append(_T(",old_value"));
      values[bindCount] = oldValue;
      sqlTypes[bindCount] = DB_SQLTYPE_TEXT;
      bindCount++;
   }
   if (newValue != nullptr)
   {
      query.append(_T(",new_value"));
      values[bindCount] = newValue;
      sqlTypes[bindCount] = DB_SQLTYPE_TEXT;
      bindCount++;
   }
   if ((oldValue != nullptr) || (newValue != nullptr))
   {
      query.append(_T(",value_type"));
      values[bindCount] = _valueType;
      sqlTypes[bindCount] = DB_SQLTYPE_VARCHAR;
      bindCount++;
   }

   // Calculate HMAC
   if (g_auditLogKey[0] != 0)
   {
#if OPENSSL_VERSION_NUMBER >= 0x30000000
      EVP_MAC *mac = EVP_MAC_fetch(nullptr, "HMAC", nullptr);
      EVP_MAC_CTX *ctx = EVP_MAC_CTX_new(mac);
#elif OPENSSL_VERSION_NUMBER >= 0x10100000
      HMAC_CTX *ctx = HMAC_CTX_new();
#else
      HMAC_CTX _ctx;
      HMAC_CTX *ctx = &_ctx;
#endif
      if (ctx != nullptr)
      {
#if OPENSSL_VERSION_NUMBER >= 0x30000000
         char digest[] = "SHA256";
         const OSSL_PARAM params[2] = { OSSL_PARAM_utf8_ptr("digest", digest, 0), OSSL_PARAM_END };
         if (EVP_MAC_init(ctx, reinterpret_cast<BYTE*>(g_auditLogKey), static_cast<int>(strlen(g_auditLogKey)), params))
#elif OPENSSL_VERSION_NUMBER >= 0x10000000
         if (HMAC_Init_ex(ctx, g_auditLogKey, static_cast<int>(strlen(g_auditLogKey)), EVP_sha256(), nullptr))
#else
         HMAC_Init_ex(ctx, g_auditLogKey, static_cast<int>(strlen(g_auditLogKey)), EVP_sha256(), nullptr);
#endif
         {
            char localBuffer[4096];
            for(int i = 0; i < bindCount; i++)
            {
               if (_tcslen(values[i]) < 1024)
               {
                  tchar_to_utf8(values[i], -1, localBuffer, 4096);
#if OPENSSL_VERSION_NUMBER >= 0x30000000
                  EVP_MAC_update(ctx, reinterpret_cast<BYTE*>(localBuffer), strlen(localBuffer));
#else
                  HMAC_Update(ctx, reinterpret_cast<BYTE*>(localBuffer), strlen(localBuffer));
#endif
               }
               else
               {
                  char *v = UTF8StringFromTString(values[i]);
#if OPENSSL_VERSION_NUMBER >= 0x30000000
                  EVP_MAC_update(ctx, reinterpret_cast<BYTE*>(v), strlen(v));
#else
                  HMAC_Update(ctx, reinterpret_cast<BYTE*>(v), strlen(v));
#endif
                  MemFree(v);
               }
            }
            BYTE hmac[SHA256_DIGEST_SIZE];
#if OPENSSL_VERSION_NUMBER >= 0x30000000
            size_t s;
            EVP_MAC_final(ctx, hmac, &s, SHA256_DIGEST_SIZE);
#else
            HMAC_Final(ctx, hmac, nullptr);
#endif

            BinToStr(hmac, SHA256_DIGEST_SIZE, _hmac);
            query.append(_T(",hmac"));
            values[bindCount] = _hmac;
            sqlTypes[bindCount] = DB_SQLTYPE_VARCHAR;
            bindCount++;
         }
#if OPENSSL_VERSION_NUMBER >= 0x30000000
         EVP_MAC_CTX_free(ctx);
         EVP_MAC_free(mac);
#elif OPENSSL_VERSION_NUMBER >= 0x10100000
         HMAC_CTX_free(ctx);
#else
         HMAC_CTX_cleanup(ctx);
#endif
      }
   }

   query.append(_T(") VALUES (?,?,?,?,?,?,?,?,?"));
   for(int i = 9; i < bindCount; i++)
      query.append(_T(",?"));
   query.append(_T(")"));

   QueueSQLRequest(query, bindCount, sqlTypes, values);

	NXCPMessage msg(CMD_AUDIT_RECORD, 0);
	msg.setField(VID_SUBSYSTEM, subsys);
	msg.setField(VID_SUCCESS_AUDIT, isSuccess);
	msg.setField(VID_USER_ID, userId);
	msg.setField(VID_WORKSTATION, workstation);
   msg.setField(VID_SESSION_ID, sessionId);
	msg.setField(VID_OBJECT_ID, objectId);
	msg.setField(VID_MESSAGE, text);
	NotifyClientSessions(msg, NXC_CHANNEL_AUDIT_LOG);

	if (s_auditServerAddr.isValidUnicast())
	{
		StringBuffer extText = _T("[");
		TCHAR buffer[MAX_USER_NAME];
      extText.append(ResolveUserId(userId, buffer, true));
		extText.append(_T('@'));
      extText.append(workstation);
      extText.append(_T("] "));
		extText.append(text);
		SendSyslogRecord(extText);
	}

	CALL_ALL_MODULES(pfProcessAuditRecord, (subsys, isSuccess, userId, workstation, sessionId, objectId, oldValue, newValue, valueType, text));
}

/**
 * Write audit record with old and new values
 */
void NXCORE_EXPORTABLE WriteAuditLogWithValues2(const TCHAR *subsys, bool isSuccess, uint32_t userId, const TCHAR *workstation,
         session_id_t sessionId, uint32_t objectId, const TCHAR *oldValue, const TCHAR *newValue, char valueType, const TCHAR *format, va_list args)
{
   StringBuffer text;
   text.appendFormattedStringV(format, args);
   WriteAuditRecord(subsys, isSuccess, userId, workstation, sessionId, objectId, oldValue, newValue, valueType, text);
}

/**
 * Maximum number of changed attributes listed in audit record message
 */
#define MAX_LISTED_ATTRIBUTES    10

/**
 * Maximum length of changed attribute list in audit record message
 */
#define MAX_ATTRIBUTE_LIST_LEN   512

/**
 * Maximum length of single attribute value in audit record message
 */
#define MAX_ATTRIBUTE_VALUE_LEN  40

/**
 * Attributes that are updated as a side effect of any modification and therefore
 * are not reported as changes.
 */
static const char *s_ignoredAttributes[] = { "timestamp", nullptr };

/**
 * Check if given top level attribute should be ignored
 */
static bool IsIgnoredAttribute(const char *name)
{
   for(int i = 0; s_ignoredAttributes[i] != nullptr; i++)
      if (!strcmp(s_ignoredAttributes[i], name))
         return true;
   return false;
}

/**
 * Append JSON value to attribute description
 */
static void AppendAttributeValue(StringBuffer *description, json_t *value)
{
   switch(json_typeof(value))
   {
      case JSON_STRING:
         {
            const char *s = json_string_value(value);
            size_t l = strlen(s);
            description->append(_T('"'));
            if (l > MAX_ATTRIBUTE_VALUE_LEN)
            {
               // Do not cut in the middle of multibyte UTF-8 character
               l = MAX_ATTRIBUTE_VALUE_LEN;
               while((l > 0) && ((s[l] & 0xC0) == 0x80))
                  l--;
               description->appendUtf8String(s, l);
               description->append(_T("..."));
            }
            else
            {
               description->appendUtf8String(s, l);
            }
            description->append(_T('"'));
         }
         break;
      case JSON_INTEGER:
         description->append(static_cast<int64_t>(json_integer_value(value)));
         break;
      case JSON_REAL:
         description->append(json_real_value(value));
         break;
      case JSON_TRUE:
         description->append(_T("true"));
         break;
      case JSON_FALSE:
         description->append(_T("false"));
         break;
      default:
         description->append(_T("null"));
         break;
   }
}

/**
 * Add description of changed attribute to the list. Value is not included for attributes
 * that has no scalar representation (objects and arrays).
 */
static void AddChangedAttribute(StringList *attributes, const TCHAR *prefix, const char *name, json_t *value)
{
   StringBuffer description;
   if (prefix != nullptr)
      description.append(prefix);
   description.appendUtf8String(name);
   if ((value != nullptr) && !json_is_object(value) && !json_is_array(value))
   {
      description.append(_T('='));
      AppendAttributeValue(&description, value);
   }
   attributes->add(description.cstr());
}

/**
 * Collect descriptions of attributes that are different in old and new object representation.
 * Nested objects are traversed recursively, arrays are compared as a whole.
 */
static void CollectChangedAttributes(json_t *oldValue, json_t *newValue, const TCHAR *prefix, StringList *attributes)
{
   void *it = json_object_iter(oldValue);
   while(it != nullptr)
   {
      const char *name = json_object_iter_key(it);
      json_t *ov = json_object_iter_value(it);
      it = json_object_iter_next(oldValue, it);

      if ((prefix == nullptr) && IsIgnoredAttribute(name))
         continue;

      json_t *nv = json_object_get(newValue, name);
      if (nv == nullptr)
      {
         AddChangedAttribute(attributes, prefix, name, nullptr);
      }
      else if (json_is_object(ov) && json_is_object(nv))
      {
         StringBuffer nestedPrefix;
         if (prefix != nullptr)
            nestedPrefix.append(prefix);
         nestedPrefix.appendUtf8String(name);
         nestedPrefix.append(_T('.'));
         CollectChangedAttributes(ov, nv, nestedPrefix.cstr(), attributes);
      }
      else if (!json_equal(ov, nv))
      {
         AddChangedAttribute(attributes, prefix, name, nv);
      }
   }

   it = json_object_iter(newValue);
   while(it != nullptr)
   {
      const char *name = json_object_iter_key(it);
      json_t *nv = json_object_iter_value(it);
      it = json_object_iter_next(newValue, it);

      if ((prefix == nullptr) && IsIgnoredAttribute(name))
         continue;

      if (json_object_get(oldValue, name) == nullptr)
         AddChangedAttribute(attributes, prefix, name, nv);
   }
}

/**
 * Append list of changed attributes to audit record message. Nothing is appended if changes
 * cannot be determined (for example if changed attribute is not part of object's JSON
 * representation because it contains sensitive data).
 */
static void AppendChangedAttributes(StringBuffer *text, json_t *oldValue, json_t *newValue)
{
   if (!json_is_object(oldValue) || !json_is_object(newValue))
      return;

   StringList attributes;
   CollectChangedAttributes(oldValue, newValue, nullptr, &attributes);
   if (attributes.isEmpty())
      return;

   if (attributes.size() <= MAX_LISTED_ATTRIBUTES)
   {
      StringBuffer list;
      for(int i = 0; i < attributes.size(); i++)
      {
         if (i > 0)
            list.append(_T(", "));
         list.append(attributes.get(i));
      }
      if (list.length() <= MAX_ATTRIBUTE_LIST_LEN)
      {
         text->append(_T(" ("));
         text->append(list);
         text->append(_T(')'));
         return;
      }
   }

   text->appendFormattedString((attributes.size() == 1) ? _T(" (%d attribute changed)") : _T(" (%d attributes changed)"), attributes.size());
}

/**
 * Write audit record with old and new values
 */
void NXCORE_EXPORTABLE WriteAuditLogWithJsonValues2(const TCHAR *subsys, bool isSuccess, uint32_t userId, const TCHAR *workstation,
         session_id_t sessionId, uint32_t objectId, json_t *oldValue, json_t *newValue, const TCHAR *format, va_list args)
{
   StringBuffer text;
   text.appendFormattedStringV(format, args);
   if ((oldValue != nullptr) && (newValue != nullptr))
      AppendChangedAttributes(&text, oldValue, newValue);

   char *js1 = (oldValue != nullptr) ? json_dumps(oldValue, JSON_SORT_KEYS | JSON_INDENT(3)) : MemCopyStringA("");
   char *js2 = (newValue != nullptr) ? json_dumps(newValue, JSON_SORT_KEYS | JSON_INDENT(3)) : MemCopyStringA("");
   WCHAR *js1w = WideStringFromUTF8String(js1);
   WCHAR *js2w = WideStringFromUTF8String(js2);
   WriteAuditRecord(subsys, isSuccess, userId, workstation, sessionId, objectId, js1w, js2w, 'J', text);
   MemFree(js1w);
   MemFree(js2w);
   MemFree(js1);
   MemFree(js2);
}
