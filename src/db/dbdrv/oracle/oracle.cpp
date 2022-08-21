/* 
** Oracle Database Driver
** Copyright (C) 2007-2022 Victor Kirhenshtein
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
** File: oracle.cpp
**
**/

#include "oracledrv.h"

static uint32_t Query(DBDRV_CONNECTION connection, const WCHAR *query, WCHAR *errorText);

#ifdef UNICODE_UCS4

/**
 * Convert wide character string to UCS-2 using internal buffer when possible
 */
inline UCS2CHAR *WideStringToUCS2(const WCHAR *str, UCS2CHAR *localBuffer, size_t size)
{
   size_t len = ucs4_ucs2len(str, -1);
   UCS2CHAR *buffer = (len <= size) ? localBuffer : MemAllocArrayNoInit<UCS2CHAR>(len);
   ucs4_to_ucs2(str, -1, buffer, len);
   return buffer;
}

/**
 * Free converted string if local buffer was not used
 */
inline void FreeConvertedString(UCS2CHAR *str, UCS2CHAR *localBuffer)
{
   if (str != localBuffer)
      MemFree(str);
}

#endif   /* UNICODE_UCS4 */

/**
 * Get error text from error handle
 */
static void GetErrorFromHandle(OCIError *handle, sb4 *errorCode, WCHAR *errorText)
{
#if UNICODE_UCS2
   OCIErrorGet(handle, 1, nullptr, errorCode, (text *)errorText, DBDRV_MAX_ERROR_TEXT, OCI_HTYPE_ERROR);
   errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#else
   UCS2CHAR buffer[DBDRV_MAX_ERROR_TEXT];

   OCIErrorGet(handle, 1, nullptr, errorCode, (text *)buffer, DBDRV_MAX_ERROR_TEXT, OCI_HTYPE_ERROR);
   buffer[DBDRV_MAX_ERROR_TEXT - 1] = 0;
   ucs2_to_ucs4(buffer, ucs2_strlen(buffer) + 1, errorText, DBDRV_MAX_ERROR_TEXT);
   errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif
   RemoveTrailingCRLFW(errorText);
}

/**
 * Set last error text
 */
static inline void SetLastError(DBDRV_CONNECTION connection)
{
   static_cast<ORACLE_CONN*>(connection)->lastErrorCode = 0;
   static_cast<ORACLE_CONN*>(connection)->lastErrorText[0] = 0;
   GetErrorFromHandle(static_cast<ORACLE_CONN*>(connection)->handleError, &static_cast<ORACLE_CONN*>(connection)->lastErrorCode, static_cast<ORACLE_CONN*>(connection)->lastErrorText);
}

/**
 * Check if last error was caused by lost connection to server
 */
static inline uint32_t IsConnectionError(DBDRV_CONNECTION connection)
{
   sb4 errorCode = static_cast<ORACLE_CONN*>(connection)->lastErrorCode;
   if ((errorCode == 1012) || (errorCode == 2396)) // ORA-01012 "not logged on" and ORA-02396 "exceeded maximum idle time"
      return DBERR_CONNECTION_LOST;

   ub4 nStatus = 0;
   OCIAttrGet(static_cast<ORACLE_CONN*>(connection)->handleServer, OCI_HTYPE_SERVER, &nStatus, nullptr, OCI_ATTR_SERVER_STATUS, static_cast<ORACLE_CONN*>(connection)->handleError);
   return (nStatus == OCI_SERVER_NOT_CONNECTED) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
}

/**
 * Check for successful completion
 */
static inline bool IsSuccess(sword code, ORACLE_CONN *conn = nullptr, const TCHAR *logMessage = nullptr)
{
   if (code == OCI_SUCCESS)
      return true;
   if (code != OCI_SUCCESS_WITH_INFO)
      return false;
   if (logMessage != nullptr)
   {
      WCHAR errorText[DBDRV_MAX_ERROR_TEXT];
      GetErrorFromHandle(conn->handleError, &conn->lastErrorCode, errorText);
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, logMessage, errorText);
   }
   return true;
}

/**
 * Global environment handle
 */
static OCIEnv *s_handleEnv = nullptr;

/**
 * Major OCI version
 */
static int s_ociVersionMajor = 0;

/**
 * Global option to disable statement caching
 */
static bool s_disableStatementCaching = false;

/**
 * Prepare string for using in SQL query - enclose in quotes and escape as needed
 */
static StringBuffer PrepareString(const TCHAR *str, size_t maxSize)
{
   StringBuffer out;
   out.append(_T('\''));
   for(const TCHAR *src = str; (*src != 0) && (maxSize > 0); src++, maxSize--)
   {
      if (*src == _T('\''))
         out.append(_T("''"), 2);
      else
         out.append(*src);
   }
   out.append(_T('\''));
   return out;
}

/**
 * Initialize driver
 */
static bool Initialize(const char *options)
{
   sword major, minor, update, patch, pupdate;
   OCIClientVersion(&major, &minor, &update, &patch, &pupdate);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("OCI version %d.%d.%d.%d.%d"), (int)major, (int)minor, (int)update, (int)patch, (int)pupdate);
   s_ociVersionMajor = (int)major;

   if (OCIEnvNlsCreate(&s_handleEnv, OCI_THREADED | OCI_NCHAR_LITERAL_REPLACE_OFF,
         nullptr, nullptr, nullptr, nullptr, 0, nullptr, OCI_UTF16ID, OCI_UTF16ID) != OCI_SUCCESS)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot allocate Oracle environment handle"));
      return false;
   }

   s_disableStatementCaching = ExtractNamedOptionValueAsBoolA(options, "disableStatementCaching", false);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("OCI statement caching is %s"), s_disableStatementCaching ? _T("DISABLED") : _T("ENABLED"));
	return true;
}

/**
 * Unload handler
 */
static void Unload()
{
   if (s_handleEnv != nullptr)
      OCIHandleFree(s_handleEnv, OCI_HTYPE_ENV);
	OCITerminate(OCI_DEFAULT);
}

/**
 * Destroy query result
 */
static void DestroyQueryResult(ORACLE_RESULT *result)
{
	int nCount = result->nCols * result->nRows;
	for(int i = 0; i < nCount; i++)
		MemFree(result->pData[i]);
	MemFree(result->pData);

	for(int i = 0; i < result->nCols; i++)
	   MemFree(result->columnNames[i]);
	MemFree(result->columnNames);

	MemFree(result);
}

/**
 * Connect to database
 */
static DBDRV_CONNECTION Connect(const char *host, const char *login, const char *password, const char *database, const char *schema, WCHAR *errorText)
{
	ORACLE_CONN *pConn = MemAllocStruct<ORACLE_CONN>();
	if (pConn != nullptr)
	{
      OCIHandleAlloc(s_handleEnv, (void **)&pConn->handleError, OCI_HTYPE_ERROR, 0, nullptr);
		OCIHandleAlloc(s_handleEnv, (void **)&pConn->handleServer, OCI_HTYPE_SERVER, 0, nullptr);
		UCS2CHAR *ucs2String = UCS2StringFromUTF8String(host);
		if (IsSuccess(OCIServerAttach(pConn->handleServer, pConn->handleError,
		                    (text *)ucs2String, (sb4)ucs2_strlen(ucs2String) * sizeof(UCS2CHAR), OCI_DEFAULT)))
		{
			MemFree(ucs2String);

			// Initialize service handle
			OCIHandleAlloc(s_handleEnv, (void **)&pConn->handleService, OCI_HTYPE_SVCCTX, 0, nullptr);
			OCIAttrSet(pConn->handleService, OCI_HTYPE_SVCCTX, pConn->handleServer, 0, OCI_ATTR_SERVER, pConn->handleError);

			// Initialize session handle
			OCIHandleAlloc(s_handleEnv, (void **)&pConn->handleSession, OCI_HTYPE_SESSION, 0, nullptr);
			ucs2String = UCS2StringFromUTF8String(login);
			OCIAttrSet(pConn->handleSession, OCI_HTYPE_SESSION, ucs2String, (ub4)ucs2_strlen(ucs2String) * sizeof(UCS2CHAR), OCI_ATTR_USERNAME, pConn->handleError);
			MemFree(ucs2String);
			ucs2String = UCS2StringFromUTF8String(password);
			OCIAttrSet(pConn->handleSession, OCI_HTYPE_SESSION, ucs2String, (ub4)ucs2_strlen(ucs2String) * sizeof(UCS2CHAR), OCI_ATTR_PASSWORD, pConn->handleError);

			// Authenticate
			if (IsSuccess(OCISessionBegin(pConn->handleService, pConn->handleError, pConn->handleSession,
			         OCI_CRED_RDBMS, s_disableStatementCaching ? OCI_DEFAULT : OCI_STMT_CACHE), pConn, _T("Connected to database with warning (%s)")))
			{
				OCIAttrSet(pConn->handleService, OCI_HTYPE_SVCCTX, pConn->handleSession, 0, OCI_ATTR_SESSION, pConn->handleError);
				pConn->mutexQueryLock = new Mutex();
				pConn->nTransLevel = 0;
				pConn->lastErrorCode = 0;
				pConn->lastErrorText[0] = 0;
            pConn->prefetchLimit = 10;

				if ((schema != nullptr) && (schema[0] != 0))
				{
					MemFree(ucs2String);
					ucs2String = UCS2StringFromUTF8String(schema);
					OCIAttrSet(pConn->handleSession, OCI_HTYPE_SESSION, ucs2String,
								  (ub4)ucs2_strlen(ucs2String) * sizeof(UCS2CHAR), OCI_ATTR_CURRENT_SCHEMA, pConn->handleError);
				}

            // LOB prefetch
            ub4 lobPrefetchSize = 16384;  // 16K
            OCIAttrSet(pConn->handleSession, OCI_HTYPE_SESSION, &lobPrefetchSize, 0, OCI_ATTR_DEFAULT_LOBPREFETCH_SIZE, pConn->handleError);

            // Setup session
            Query(pConn, L"ALTER SESSION SET NLS_LANGUAGE='AMERICAN' NLS_NUMERIC_CHARACTERS='.,'", nullptr);

            UCS2CHAR version[1024];
            if (IsSuccess(OCIServerVersion(pConn->handleService, pConn->handleError, (OraText *)version, sizeof(version), OCI_HTYPE_SVCCTX)))
            {
#ifdef UNICODE
#if UNICODE_UCS4
               WCHAR *wver = UCS4StringFromUCS2String(version);
               nxlog_debug_tag(DEBUG_TAG, 5, _T("Connected to %s"), wver);
               MemFree(wver);
#else
               nxlog_debug_tag(DEBUG_TAG, 5, _T("Connected to %s"), version);
#endif
#else
               char *mbver = MBStringFromUCS2String(version);
               nxlog_debug_tag(DEBUG_TAG, 5, _T("Connected to %s"), mbver);
               MemFree(mbver);
#endif
            }
         }
			else
			{
				GetErrorFromHandle(pConn->handleError, &pConn->lastErrorCode, errorText);
		      OCIServerDetach(pConn->handleServer, pConn->handleError, OCI_DEFAULT);
			   OCIHandleFree(pConn->handleService, OCI_HTYPE_SVCCTX);
				OCIHandleFree(pConn->handleServer, OCI_HTYPE_SERVER);
			   OCIHandleFree(pConn->handleError, OCI_HTYPE_ERROR);
			   MemFreeAndNull(pConn);
			}
		}
		else
		{
			GetErrorFromHandle(pConn->handleError, &pConn->lastErrorCode, errorText);
			OCIHandleFree(pConn->handleServer, OCI_HTYPE_SERVER);
			OCIHandleFree(pConn->handleError, OCI_HTYPE_ERROR);
         MemFreeAndNull(pConn);
		}
		MemFree(ucs2String);
	}
	else
	{
		wcscpy(errorText, L"Memory allocation error");
	}
   return pConn;
}

/**
 * Disconnect from database
 */
static void Disconnect(DBDRV_CONNECTION connection)
{
   auto c = static_cast<ORACLE_CONN*>(connection);
   OCISessionEnd(c->handleService, c->handleError, NULL, OCI_DEFAULT);
   OCIServerDetach(c->handleServer, c->handleError, OCI_DEFAULT);
   OCIHandleFree(c->handleSession, OCI_HTYPE_SESSION);
   OCIHandleFree(c->handleService, OCI_HTYPE_SVCCTX);
   OCIHandleFree(c->handleServer, OCI_HTYPE_SERVER);
   OCIHandleFree(c->handleError, OCI_HTYPE_ERROR);
   delete c->mutexQueryLock;
   MemFree(c);
}

/**
 * Set prefetch limit
 */
static bool SetPrefetchLimit(DBDRV_CONNECTION connection, int limit)
{
   static_cast<ORACLE_CONN*>(connection)->prefetchLimit = limit;
   return true;
}

/**
 * Convert query from NetXMS portable format to native Oracle format
 */
static const UCS2CHAR *ConvertQuery(const WCHAR *query, UCS2CHAR *localBuffer, size_t bufferSize)
{
   int count = NumCharsW(query, L'?');
   if (count == 0)
   {
#if UNICODE_UCS4
      return WideStringToUCS2(query, localBuffer, bufferSize);
#else
      return query;
#endif
   }

#if UNICODE_UCS4
   UCS2CHAR srcQueryBuffer[1024];
	UCS2CHAR *srcQuery = WideStringToUCS2(query, srcQueryBuffer, 1024);
#else
	const UCS2CHAR *srcQuery = query;
#endif

	size_t dstQuerySize = ucs2_strlen(srcQuery) + count * 3 + 1;
	UCS2CHAR *dstQuery = (dstQuerySize <= bufferSize) ? localBuffer : MemAllocArrayNoInit<UCS2CHAR>(dstQuerySize);

	bool inString = false;
	int pos = 1;
   const UCS2CHAR *src;
   UCS2CHAR *dst;
	for(src = srcQuery, dst = dstQuery; *src != 0; src++)
	{
		switch(*src)
		{
			case '\'':
				*dst++ = *src;
				inString = !inString;
				break;
			case '\\':
				*dst++ = *src++;
				*dst++ = *src;
				break;
			case '?':
				if (inString)
				{
					*dst++ = '?';
				}
				else
				{
					*dst++ = ':';
					if (pos < 10)
					{
						*dst++ = pos + '0';
					}
					else if (pos < 100)
					{
						*dst++ = pos / 10 + '0';
						*dst++ = pos % 10 + '0';
					}
					else
					{
						*dst++ = pos / 100 + '0';
						*dst++ = (pos % 100) / 10 + '0';
						*dst++ = pos % 10 + '0';
					}
					pos++;
				}
				break;
			default:
				*dst++ = *src;
				break;
		}
	}
	*dst = 0;
#if UNICODE_UCS4
	FreeConvertedString(srcQuery, srcQueryBuffer);
#endif
	return dstQuery;
}

/**
 * Prepare statement
 */
static DBDRV_STATEMENT Prepare(DBDRV_CONNECTION connection, const WCHAR *query, bool optimizeForReuse, uint32_t *errorCode, WCHAR *errorText)
{
	ORACLE_STATEMENT *stmt = nullptr;

	UCS2CHAR localBuffer[1024];
	const UCS2CHAR *ucs2Query = ConvertQuery(query, localBuffer, 1024);

	static_cast<ORACLE_CONN*>(connection)->mutexQueryLock->lock();
   OCIStmt *handleStmt;
	if (IsSuccess(OCIStmtPrepare2(static_cast<ORACLE_CONN*>(connection)->handleService, &handleStmt,
	         static_cast<ORACLE_CONN*>(connection)->handleError, (text *)ucs2Query,
	         (ub4)ucs2_strlen(ucs2Query) * sizeof(UCS2CHAR), nullptr, 0, OCI_NTV_SYNTAX, OCI_DEFAULT)))
	{
		stmt = MemAllocStruct<ORACLE_STATEMENT>();
		stmt->connection = static_cast<ORACLE_CONN*>(connection);
		stmt->handleStmt = handleStmt;
		stmt->bindings = new ObjectArray<OracleBind>(32, 32, Ownership::True);
      stmt->batchBindings = nullptr;
      stmt->batchMode = false;
      stmt->batchSize = 0;
		OCIHandleAlloc(s_handleEnv, (void **)&stmt->handleError, OCI_HTYPE_ERROR, 0, nullptr);
		*errorCode = DBERR_SUCCESS;
	}
	else
	{
		SetLastError(connection);
		*errorCode = IsConnectionError(connection);
	}

	if (errorText != nullptr)
	{
		wcslcpy(errorText, static_cast<ORACLE_CONN*>(connection)->lastErrorText, DBDRV_MAX_ERROR_TEXT);
	}
	static_cast<ORACLE_CONN*>(connection)->mutexQueryLock->unlock();

#if UNICODE_UCS2
	if ((ucs2Query != query) && (ucs2Query != localBuffer))
	   MemFree(const_cast<UCS2CHAR*>(ucs2Query));
#else
	FreeConvertedString(const_cast<UCS2CHAR*>(ucs2Query), localBuffer);
#endif
	return stmt;
}

/**
 * Open batch
 */
static bool OpenBatch(DBDRV_STATEMENT hStmt)
{
   auto stmt = static_cast<ORACLE_STATEMENT*>(hStmt);
   if (stmt->batchBindings != nullptr)
      stmt->batchBindings->clear();
   else
      stmt->batchBindings = new ObjectArray<OracleBatchBind>(32, 32, Ownership::True);
   stmt->batchMode = true;
   stmt->batchSize = 0;
   return true;
}

/**
 * Start next batch row
 */
static void NextBatchRow(DBDRV_STATEMENT hStmt)
{
   auto stmt = static_cast<ORACLE_STATEMENT*>(hStmt);
   if (!stmt->batchMode)
      return;

   for(int i = 0; i < stmt->batchBindings->size(); i++)
   {
	   OracleBatchBind *bind = stmt->batchBindings->get(i);
      if (bind != nullptr)
         bind->addRow();
   }
   stmt->batchSize++;
}

/**
 * Buffer sizes for different C types
 */
static uint32_t s_bufferSize[] = { 0, sizeof(int32_t), sizeof(uint32_t), sizeof(int64_t), sizeof(uint64_t), sizeof(double) };

/**
 * Corresponding Oracle types for C types
 */
static ub2 s_oracleType[] = { SQLT_STR, SQLT_INT, SQLT_UIN, SQLT_INT, SQLT_UIN, SQLT_FLT };

/**
 * Bind parameter to statement - normal mode (non-batch)
 */
static void BindNormal(ORACLE_STATEMENT *stmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
	OracleBind *bind = stmt->bindings->get(pos - 1);
	if (bind == nullptr)
	{
	   bind = new OracleBind(stmt->connection->handleService, stmt->handleError);
	   stmt->bindings->set(pos - 1, bind);
	}
	else
	{
	   MemFreeAndNull(bind->data);
	   bind->freeTemporaryLob();
	}

	UCS2CHAR *convertedString = nullptr;
	if ((sqlType == DB_SQLTYPE_TEXT) && ((cType == DB_CTYPE_STRING) || (cType == DB_CTYPE_UTF8_STRING)))
	{
	   ub4 textLen;
	   if (cType == DB_CTYPE_UTF8_STRING)
	   {
	      convertedString = UCS2StringFromUTF8String(static_cast<char*>(buffer));
	      textLen = static_cast<ub4>(ucs2_strlen(convertedString));
	   }
	   else
	   {
#if UNICODE_UCS4
	      convertedString = UCS2StringFromUCS4String(static_cast<WCHAR*>(buffer));
         textLen = static_cast<ub4>(ucs2_strlen(convertedString));
#else
         textLen = static_cast<ub4>(wcslen(static_cast<WCHAR*>(buffer)));
#endif
	   }
	   if (textLen > 2000)
	   {
         if (bind->createTemporaryLob(s_handleEnv))
         {
            if (OCILobWrite(bind->serviceContext, bind->errorHandle, bind->lobLocator, &textLen, 1,
                     (convertedString != nullptr) ? convertedString : buffer, textLen * sizeof(UCS2CHAR),
                     OCI_ONE_PIECE, nullptr, nullptr, OCI_UTF16ID, SQLCS_IMPLICIT) == OCI_SUCCESS)
            {
               OCIBindByPos(stmt->handleStmt, &bind->handle, stmt->handleError, pos, &bind->lobLocator,
                     static_cast<sb4>(sizeof(OCILobLocator*)), SQLT_CLOB, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT);
            }
            else
            {
               sb4 errorCode;
               WCHAR errorText[DBDRV_MAX_ERROR_TEXT];
               GetErrorFromHandle(stmt->handleError, &errorCode, errorText);
#ifdef UNICODE
               nxlog_debug_tag(DEBUG_TAG, 5, _T("BindNormal: cannot write to temporary LOB (%s)"), errorText);
#else
               nxlog_debug_tag(DEBUG_TAG, 5, _T("BindNormal: cannot write to temporary LOB (%S)"), errorText);
#endif
               bind->freeTemporaryLob();
            }
         }
         MemFree(convertedString);
         if (allocType == DB_BIND_DYNAMIC)
            MemFree(buffer);
	      return;
	   }
	   // Fallback to varchar binding
	}

	void *sqlBuffer;
	switch(cType)
	{
		case DB_CTYPE_STRING:
#if UNICODE_UCS4
			bind->data = (convertedString != nullptr) ? convertedString : UCS2StringFromUCS4String(static_cast<WCHAR*>(buffer));
			if (allocType == DB_BIND_DYNAMIC)
				MemFree(buffer);
			sqlBuffer = bind->data;
#else
         if (allocType == DB_BIND_TRANSIENT)
			{
				bind->data = MemCopyStringW(static_cast<WCHAR*>(buffer));
	         sqlBuffer = bind->data;
			}
			else
			{
				if (allocType == DB_BIND_DYNAMIC)
					bind->data = buffer;
            sqlBuffer = buffer;
			}
#endif
         OCIBindByPos(stmt->handleStmt, &bind->handle, stmt->handleError, pos, sqlBuffer,
               static_cast<sb4>((ucs2_strlen(static_cast<UCS2CHAR*>(sqlBuffer)) + 1) * sizeof(UCS2CHAR)),
               SQLT_STR, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT);
			break;
      case DB_CTYPE_UTF8_STRING:
         bind->data = (convertedString != nullptr) ? convertedString : UCS2StringFromUTF8String(static_cast<char*>(buffer));
         sqlBuffer = bind->data;
         if (allocType == DB_BIND_DYNAMIC)
            MemFree(buffer);
         OCIBindByPos(stmt->handleStmt, &bind->handle, stmt->handleError, pos, sqlBuffer,
               static_cast<sb4>((ucs2_strlen((UCS2CHAR *)sqlBuffer) + 1) * sizeof(UCS2CHAR)), SQLT_STR, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT);
         break;
		case DB_CTYPE_INT64:	// OCI prior to 11.2 cannot bind 64 bit integers
		   OCINumberFromInt(stmt->handleError, buffer, sizeof(int64_t), OCI_NUMBER_SIGNED, &bind->number);
         OCIBindByPos(stmt->handleStmt, &bind->handle, stmt->handleError, pos, &bind->number, sizeof(OCINumber),
               SQLT_VNU, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT);
			if (allocType == DB_BIND_DYNAMIC)
				MemFree(buffer);
			break;
		case DB_CTYPE_UINT64:	// OCI prior to 11.2 cannot bind 64 bit integers
         OCINumberFromInt(stmt->handleError, buffer, sizeof(int64_t), OCI_NUMBER_UNSIGNED, &bind->number);
         OCIBindByPos(stmt->handleStmt, &bind->handle, stmt->handleError, pos, &bind->number, sizeof(OCINumber),
               SQLT_VNU, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT);
         if (allocType == DB_BIND_DYNAMIC)
            MemFree(buffer);
         break;
		default:
		   switch(allocType)
		   {
			   case DB_BIND_STATIC:
				   sqlBuffer = buffer;
				   break;
			   case DB_BIND_DYNAMIC:
				   sqlBuffer = buffer;
				   bind->data = buffer;
				   break;
			   case DB_BIND_TRANSIENT:
				   sqlBuffer = MemCopyBlock(buffer, s_bufferSize[cType]);
				   bind->data = sqlBuffer;
				   break;
			   default:
				   return;	// Invalid call
		   }
		   OCIBindByPos(stmt->handleStmt, &bind->handle, stmt->handleError, pos, sqlBuffer, s_bufferSize[cType],
		         s_oracleType[cType], nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT);
		   break;
	}
}

/**
 * Batch bind - constructor
 */
OracleBatchBind::OracleBatchBind(int cType, int sqlType)
{
   m_cType = cType;
   m_size = 0;
   m_allocated = 256;
   if ((cType == DB_CTYPE_STRING) || (cType == DB_CTYPE_INT64) || (cType == DB_CTYPE_UINT64))
   {
      m_elementSize = sizeof(UCS2CHAR);
      m_string = true;
      m_oraType = (sqlType == DB_SQLTYPE_TEXT) ? SQLT_LNG : SQLT_STR;
      m_data = nullptr;
      m_strings = MemAllocArray<UCS2CHAR*>(m_allocated);
   }
   else
   {
      m_elementSize = s_bufferSize[cType];
      m_string = false;
      m_oraType = s_oracleType[cType];
      m_data = MemAllocZeroed(m_allocated * m_elementSize);
      m_strings = nullptr;
   }
}

/**
 * Batch bind - destructor
 */
OracleBatchBind::~OracleBatchBind()
{
   if (m_strings != nullptr)
   {
      for(int i = 0; i < m_size; i++)
         MemFree(m_strings[i]);
      MemFree(m_strings);
   }
   MemFree(m_data);
}

/**
 * Batch bind - add row
 */
void OracleBatchBind::addRow()
{
   if (m_size == m_allocated)
   {
      m_allocated += 256;
      if (m_string)
      {
         m_strings = MemRealloc(m_strings, m_allocated * sizeof(UCS2CHAR *));
         memset(m_strings + m_size, 0, (m_allocated - m_size) * sizeof(UCS2CHAR *));
      }
      else
      {
         m_data = MemRealloc(m_data, m_allocated * m_elementSize);
         memset((char *)m_data + m_size * m_elementSize, 0, (m_allocated - m_size) * m_elementSize);
      }
   }
   if (m_size > 0)
   {
      // clone last element
      if (m_string)
      {
         UCS2CHAR *p = m_strings[m_size - 1];
         m_strings[m_size] = (p != nullptr) ? ucs2_strdup(p) : nullptr;
      }
      else
      {
         memcpy((char *)m_data + m_size * m_elementSize, (char *)m_data + (m_size - 1) * m_elementSize, m_elementSize);
      }
   }
   m_size++;
}

/**
 * Batch bind - set value
 */
void OracleBatchBind::set(void *value)
{
   if (m_string)
   {
      MemFree(m_strings[m_size - 1]);
      m_strings[m_size - 1] = (UCS2CHAR *)value;
      if (value != NULL)
      {
         int l = (int)(ucs2_strlen((UCS2CHAR *)value) + 1) * sizeof(UCS2CHAR);
         if (l > m_elementSize)
            m_elementSize = l;
      }
   }
   else
   {
      memcpy((char *)m_data + (m_size - 1) * m_elementSize, value, m_elementSize);
   }
}

/**
 * Get data for OCI bind
 */
void *OracleBatchBind::getData()
{
   if (!m_string)
      return m_data;

   MemFree(m_data);
   m_data = MemAllocZeroed(m_size * m_elementSize);
   char *p = (char *)m_data;
   for(int i = 0; i < m_size; i++)
   {
      if (m_strings[i] == nullptr)
         continue;
      memcpy(p, m_strings[i], ucs2_strlen(m_strings[i]) * sizeof(UCS2CHAR));
      p += m_elementSize;
   }
   return m_data;
}

/**
 * Bind parameter to statement - batch mode
 */
static void BindBatch(ORACLE_STATEMENT *stmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
   if (stmt->batchSize == 0)
      return;  // no batch rows added yet

	OracleBatchBind *bind = stmt->batchBindings->get(pos - 1);
   if (bind == nullptr)
   {
      bind = new OracleBatchBind(cType, sqlType);
      stmt->batchBindings->set(pos - 1, bind);
      for(int i = 0; i < stmt->batchSize; i++)
         bind->addRow();
   }

   if (bind->getCType() != cType)
      return;

	void *sqlBuffer;
   switch(bind->getCType())
	{
		case DB_CTYPE_STRING:
#if UNICODE_UCS4
			sqlBuffer = UCS2StringFromUCS4String((WCHAR *)buffer);
         if (allocType == DB_BIND_DYNAMIC)
				MemFree(buffer);
#else
         if (allocType == DB_BIND_DYNAMIC)
         {
				sqlBuffer = buffer;
         }
         else
			{
				sqlBuffer = MemCopyStringW((WCHAR *)buffer);
			}
#endif
         bind->set(sqlBuffer);
			break;
      case DB_CTYPE_UTF8_STRING:
#if UNICODE_UCS4
         sqlBuffer = UCS2StringFromUTF8String((char *)buffer);
#else
         sqlBuffer = WideStringFromUTF8String((char *)buffer);
#endif
         if (allocType == DB_BIND_DYNAMIC)
            MemFree(buffer);
         bind->set(sqlBuffer);
         break;
		case DB_CTYPE_INT64:	// OCI prior to 11.2 cannot bind 64 bit integers
#ifdef UNICODE_UCS2
			sqlBuffer = malloc(64 * sizeof(WCHAR));
			swprintf((WCHAR *)sqlBuffer, 64, INT64_FMTW, *((INT64 *)buffer));
#else
			{
				char temp[64];
				snprintf(temp, 64, INT64_FMTA, *((INT64 *)buffer));
				sqlBuffer = UCS2StringFromMBString(temp);
			}
#endif
         bind->set(sqlBuffer);
			if (allocType == DB_BIND_DYNAMIC)
				MemFree(buffer);
			break;
		case DB_CTYPE_UINT64:	// OCI prior to 11.2 cannot bind 64 bit integers
#ifdef UNICODE_UCS2
			sqlBuffer = malloc(64 * sizeof(WCHAR));
			swprintf((WCHAR *)sqlBuffer, 64, UINT64_FMTW, *((QWORD *)buffer));
#else
			{
				char temp[64];
				snprintf(temp, 64, UINT64_FMTA, *((QWORD *)buffer));
				sqlBuffer = UCS2StringFromMBString(temp);
			}
#endif
         bind->set(sqlBuffer);
			if (allocType == DB_BIND_DYNAMIC)
			   MemFree(buffer);
			break;
		default:
         bind->set(buffer);
			if (allocType == DB_BIND_DYNAMIC)
				MemFree(buffer);
			break;
	}
}

/**
 * Bind parameter to statement
 */
static void Bind(DBDRV_STATEMENT hStmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
   if (static_cast<ORACLE_STATEMENT*>(hStmt)->batchMode)
      BindBatch(static_cast<ORACLE_STATEMENT*>(hStmt), pos, sqlType, cType, buffer, allocType);
   else
      BindNormal(static_cast<ORACLE_STATEMENT*>(hStmt), pos, sqlType, cType, buffer, allocType);
}

/**
 * Execute prepared non-select statement
 */
static uint32_t Execute(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, WCHAR *errorText)
{
	uint32_t dwResult;

   auto stmt = static_cast<ORACLE_STATEMENT*>(hStmt);
   if (stmt->batchMode)
   {
      if (stmt->batchSize == 0)
      {
         stmt->batchMode = false;
         stmt->batchBindings->clear();
         return DBERR_SUCCESS;   // empty batch
      }

      for(int i = 0; i < stmt->batchBindings->size(); i++)
      {
         OracleBatchBind *b = stmt->batchBindings->get(i);
         if (b == nullptr)
            continue;

         OCIBind *handleBind = nullptr;
         OCIBindByPos(stmt->handleStmt, &handleBind, stmt->handleError, i + 1, b->getData(), 
               b->getElementSize(), b->getOraType(), nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT);
      }
   }

   static_cast<ORACLE_CONN*>(connection)->mutexQueryLock->lock();
   if (IsSuccess(OCIStmtExecute(static_cast<ORACLE_CONN*>(connection)->handleService, stmt->handleStmt, static_cast<ORACLE_CONN*>(connection)->handleError,
                      stmt->batchMode ? stmt->batchSize : 1, 0, nullptr, nullptr,
	                   (static_cast<ORACLE_CONN*>(connection)->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT)))
	{
		dwResult = DBERR_SUCCESS;
	}
	else
	{
		SetLastError(connection);
		dwResult = IsConnectionError(connection);
	}

	if (errorText != nullptr)
	{
		wcslcpy(errorText, static_cast<ORACLE_CONN*>(connection)->lastErrorText, DBDRV_MAX_ERROR_TEXT);
	}
	static_cast<ORACLE_CONN*>(connection)->mutexQueryLock->unlock();

   if (stmt->batchMode)
   {
      stmt->batchMode = false;
      stmt->batchBindings->clear();
   }

	return dwResult;
}

/**
 * Destroy prepared statement
 */
static void FreeStatement(DBDRV_STATEMENT hStmt)
{
   auto stmt = static_cast<ORACLE_STATEMENT*>(hStmt);
	stmt->connection->mutexQueryLock->lock();
   delete stmt->bindings;
   delete stmt->batchBindings;
	OCIStmtRelease(stmt->handleStmt, stmt->handleError, nullptr, 0, OCI_DEFAULT);
	OCIHandleFree(stmt->handleError, OCI_HTYPE_ERROR);
	stmt->connection->mutexQueryLock->unlock();
	MemFree(stmt);
}

/**
 * Perform non-SELECT query
 */
static uint32_t Query(DBDRV_CONNECTION connection, const WCHAR *query, WCHAR *errorText)
{
   uint32_t result;

#if UNICODE_UCS4
   UCS2CHAR localBuffer[1024];
   UCS2CHAR *ucs2Query = WideStringToUCS2(query, localBuffer, 1024);
#else
   const UCS2CHAR *ucs2Query = query;
#endif

   static_cast<ORACLE_CONN*>(connection)->mutexQueryLock->lock();
   OCISvcCtx *handleService = static_cast<ORACLE_CONN*>(connection)->handleService;
   OCIError *handleError = static_cast<ORACLE_CONN*>(connection)->handleError;
   OCIStmt *handleStmt;
	if (IsSuccess(OCIStmtPrepare2(handleService, &handleStmt, handleError, (text *)ucs2Query,
	                    (ub4)ucs2_strlen(ucs2Query) * sizeof(UCS2CHAR), nullptr, 0, OCI_NTV_SYNTAX, OCI_DEFAULT)))
	{
		if (IsSuccess(OCIStmtExecute(handleService, handleStmt, handleError, 1, 0, nullptr, nullptr,
		                   (static_cast<ORACLE_CONN*>(connection)->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT)))
		{
			result = DBERR_SUCCESS;
		}
		else
		{
			SetLastError(connection);
			result = IsConnectionError(connection);
		}
		OCIStmtRelease(handleStmt, handleError, nullptr, 0, OCI_DEFAULT);
	}
	else
	{
		SetLastError(connection);
		result = IsConnectionError(connection);
	}
	if (errorText != nullptr)
	{
		wcslcpy(errorText, static_cast<ORACLE_CONN*>(connection)->lastErrorText, DBDRV_MAX_ERROR_TEXT);
	}
	static_cast<ORACLE_CONN*>(connection)->mutexQueryLock->unlock();

#if UNICODE_UCS4
	FreeConvertedString(ucs2Query, localBuffer);
#endif
	return result;
}

/**
 * Get column name from parameter handle
 *
 * OCI library has memory leak when retrieving column name in UNICODE mode
 * Driver attempts to use workaround described in https://github.com/vrogier/ocilib/issues/31
 * (accessing internal OCI structure directly to avoid conversion to UTF-16 by OCI library)
 */
static char *GetColumnNameFromHandle(OCIParam *handleParam, OCIError *handleError)
{
   if ((s_ociVersionMajor == 11) || (s_ociVersionMajor == 12))
   {
      OCI_PARAM_STRUCT *p = (OCI_PARAM_STRUCT *)handleParam;
      if ((p->columnInfo != NULL) && (p->columnInfo->name != NULL) && (p->columnInfo->attributes[1] != 0))
      {
         size_t len = p->columnInfo->attributes[1];
         char *n = (char *)malloc(len + 1);
         memcpy(n, p->columnInfo->name, len);
         n[len] = 0;
         return n;
      }
   }

   // Use standard method as fallback
   text *colName;
   ub4 size;
   if (OCIAttrGet(handleParam, OCI_DTYPE_PARAM, &colName, &size, OCI_ATTR_NAME, handleError) == OCI_SUCCESS)
   {
      // We are in UTF-16 mode, so OCIAttrGet will return UTF-16 strings
      return MBStringFromUCS2String((UCS2CHAR *)colName);
   }
   else
   {
      return MemCopyStringA("");
   }
}

/**
 * Process SELECT results
 */
static ORACLE_RESULT *ProcessQueryResults(ORACLE_CONN *pConn, OCIStmt *handleStmt, uint32_t *errorCode)
{
	OCIDefine *handleDefine;
	ub4 nCount;
	ub2 nWidth;

	ORACLE_RESULT *pResult = MemAllocStruct<ORACLE_RESULT>();
	OCIAttrGet(handleStmt, OCI_HTYPE_STMT, &nCount, nullptr, OCI_ATTR_PARAM_COUNT, pConn->handleError);
	pResult->nCols = nCount;
	if (pResult->nCols > 0)
	{
      sword nStatus;

		// Prepare receive buffers and fetch column names
		pResult->columnNames = MemAllocArray<char*>(pResult->nCols);
		OracleFetchBuffer *pBuffers = MemAllocArray<OracleFetchBuffer>(pResult->nCols);
		for(int i = 0; i < pResult->nCols; i++)
		{
		   OCIParam *handleParam;
			if ((nStatus = OCIParamGet(handleStmt, OCI_HTYPE_STMT, pConn->handleError,
			                           (void **)&handleParam, (ub4)(i + 1))) == OCI_SUCCESS)
			{
				// Column name
            pResult->columnNames[i] = GetColumnNameFromHandle(handleParam, pConn->handleError);

				// Data size
            ub2 type = 0;
				OCIAttrGet(handleParam, OCI_DTYPE_PARAM, &type, NULL, OCI_ATTR_DATA_TYPE, pConn->handleError);
            if (type == OCI_TYPECODE_CLOB)
            {
               pBuffers[i].data = nullptr;
               OCIDescriptorAlloc(s_handleEnv, (void **)&pBuffers[i].lobLocator, OCI_DTYPE_LOB, 0, nullptr);
				   handleDefine = nullptr;
				   nStatus = OCIDefineByPos(handleStmt, &handleDefine, pConn->handleError, i + 1,
                                        &pBuffers[i].lobLocator, 0, SQLT_CLOB, &pBuffers[i].isNull, 
                                        nullptr, nullptr, OCI_DEFAULT);
            }
            else
            {
               pBuffers[i].lobLocator = nullptr;
				   OCIAttrGet(handleParam, OCI_DTYPE_PARAM, &nWidth, nullptr, OCI_ATTR_DATA_SIZE, pConn->handleError);
				   pBuffers[i].data = (UCS2CHAR *)MemAlloc((nWidth + 31) * sizeof(UCS2CHAR));
				   handleDefine = nullptr;
				   nStatus = OCIDefineByPos(handleStmt, &handleDefine, pConn->handleError, i + 1,
                                        pBuffers[i].data, (nWidth + 31) * sizeof(UCS2CHAR),
				                            SQLT_CHR, &pBuffers[i].isNull, &pBuffers[i].length,
				                            &pBuffers[i].code, OCI_DEFAULT);
            }
            if (nStatus != OCI_SUCCESS)
			   {
				   SetLastError(pConn);
				   *errorCode = IsConnectionError(pConn);
			   }
				OCIDescriptorFree(handleParam, OCI_DTYPE_PARAM);
			}
			else
			{
				SetLastError(pConn);
				*errorCode = IsConnectionError(pConn);
			}
		}

		// Fetch data
		if (nStatus == OCI_SUCCESS)
		{
			int nPos = 0;
			while(1)
			{
				nStatus = OCIStmtFetch2(handleStmt, pConn->handleError, 1, OCI_FETCH_NEXT, 0, OCI_DEFAULT);
				if (nStatus == OCI_NO_DATA)
				{
					*errorCode = DBERR_SUCCESS;	// EOF
					break;
				}
				if ((nStatus != OCI_SUCCESS) && (nStatus != OCI_SUCCESS_WITH_INFO))
				{
					SetLastError(pConn);
					*errorCode = IsConnectionError(pConn);
					break;
				}

				// New row
				pResult->nRows++;
				pResult->pData = MemRealloc(pResult->pData, sizeof(WCHAR *) * pResult->nCols * pResult->nRows);
				for(int i = 0; i < pResult->nCols; i++)
				{
					if (pBuffers[i].isNull)
					{
						pResult->pData[nPos] = (WCHAR *)MemCopyBlock("\0\0\0", sizeof(WCHAR));
					}
               else if (pBuffers[i].lobLocator != nullptr)
               {
                  ub4 length = 0;
                  OCILobGetLength(pConn->handleService, pConn->handleError, pBuffers[i].lobLocator, &length);
						pResult->pData[nPos] = MemAllocStringW(length + 1);
                  ub4 amount = length;
#if UNICODE_UCS4
                  UCS2CHAR *ucs2buffer = MemAllocArrayNoInit<UCS2CHAR>(length);
                  OCILobRead(pConn->handleService, pConn->handleError, pBuffers[i].lobLocator, &amount, 1, 
                             ucs2buffer, length * sizeof(UCS2CHAR), nullptr, nullptr, OCI_UCS2ID, SQLCS_IMPLICIT);
						ucs2_to_ucs4(ucs2buffer, amount, pResult->pData[nPos], length + 1);
                  MemFree(ucs2buffer);
#else
                  OCILobRead(pConn->handleService, pConn->handleError, pBuffers[i].lobLocator, &amount, 1, 
                             pResult->pData[nPos], (length + 1) * sizeof(WCHAR), NULL, NULL, OCI_UCS2ID, SQLCS_IMPLICIT);
#endif
						pResult->pData[nPos][amount] = 0;
               }
					else
					{
						int length = pBuffers[i].length / sizeof(UCS2CHAR);
						pResult->pData[nPos] = MemAllocArrayNoInit<WCHAR>(length + 1);
#if UNICODE_UCS4
						ucs2_to_ucs4(pBuffers[i].data, length, pResult->pData[nPos], length + 1);
#else
						memcpy(pResult->pData[nPos], pBuffers[i].data, pBuffers[i].length);
#endif
						pResult->pData[nPos][length] = 0;
					}
					nPos++;
				}
			}
		}

		// Cleanup
		for(int i = 0; i < pResult->nCols; i++)
      {
			MemFree(pBuffers[i].data);
         if (pBuffers[i].lobLocator != nullptr)
         {
            OCIDescriptorFree(pBuffers[i].lobLocator, OCI_DTYPE_LOB);
         }
      }
		MemFree(pBuffers);

		// Destroy results in case of error
		if (*errorCode != DBERR_SUCCESS)
		{
			DestroyQueryResult(pResult);
			pResult = nullptr;
		}
	}

	return pResult;
}

/**
 * Perform SELECT query
 */
static DBDRV_RESULT Select(DBDRV_CONNECTION connection, const WCHAR *query, uint32_t *errorCode, WCHAR *errorText)
{
	ORACLE_RESULT *pResult = nullptr;
	OCIStmt *handleStmt;

#if UNICODE_UCS4
	UCS2CHAR localBuffer[1024];
	UCS2CHAR *ucs2Query = WideStringToUCS2(query, localBuffer, 1024);
#else
	const UCS2CHAR *ucs2Query = query;
#endif

	auto conn = static_cast<ORACLE_CONN*>(connection);
	conn->mutexQueryLock->lock();
	if (IsSuccess(OCIStmtPrepare2(conn->handleService, &handleStmt, conn->handleError, (text *)ucs2Query,
	                    (ub4)ucs2_strlen(ucs2Query) * sizeof(UCS2CHAR), nullptr, 0, OCI_NTV_SYNTAX, OCI_DEFAULT)))
	{
      OCIAttrSet(handleStmt, OCI_HTYPE_STMT, &conn->prefetchLimit, 0, OCI_ATTR_PREFETCH_ROWS, conn->handleError);
		if (IsSuccess(OCIStmtExecute(conn->handleService, handleStmt, conn->handleError,
		                   0, 0, NULL, NULL, (conn->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT)))
		{
			pResult = ProcessQueryResults(conn, handleStmt, errorCode);
		}
		else
		{
			SetLastError(conn);
			*errorCode = IsConnectionError(conn);
		}
		OCIStmtRelease(handleStmt, conn->handleError, NULL, 0, OCI_DEFAULT);
	}
	else
	{
		SetLastError(conn);
		*errorCode = IsConnectionError(conn);
	}
	if (errorText != nullptr)
	{
		wcslcpy(errorText, conn->lastErrorText, DBDRV_MAX_ERROR_TEXT);
	}
	conn->mutexQueryLock->unlock();

#if UNICODE_UCS4
	FreeConvertedString(ucs2Query, localBuffer);
#endif
	return pResult;
}

/**
 * Perform SELECT query using prepared statement
 */
static DBDRV_RESULT SelectPrepared(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *pdwError, WCHAR *errorText)
{
	ORACLE_RESULT *pResult = nullptr;

	auto pConn = static_cast<ORACLE_CONN*>(connection);
	pConn->mutexQueryLock->lock();
	OCIStmt *handleStmt = static_cast<ORACLE_STATEMENT*>(hStmt)->handleStmt;
   OCIAttrSet(handleStmt, OCI_HTYPE_STMT, &pConn->prefetchLimit, 0, OCI_ATTR_PREFETCH_ROWS, pConn->handleError);
	if (IsSuccess(OCIStmtExecute(pConn->handleService, handleStmt, pConn->handleError,
	                   0, 0, NULL, NULL, (pConn->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT)))
	{
		pResult = ProcessQueryResults(pConn, handleStmt, pdwError);
	}
	else
	{
		SetLastError(pConn);
		*pdwError = IsConnectionError(pConn);
	}

	if (errorText != nullptr)
	{
		wcslcpy(errorText, pConn->lastErrorText, DBDRV_MAX_ERROR_TEXT);
	}
	pConn->mutexQueryLock->unlock();

	return pResult;
}

/**
 * Get field length from result
 */
static int32_t GetFieldLength(DBDRV_RESULT hResult, int nRow, int nColumn)
{
   auto result = static_cast<ORACLE_RESULT*>(hResult);
	if ((nRow >= 0) && (nRow < result->nRows) &&
		 (nColumn >= 0) && (nColumn < result->nCols))
		return (int32_t)wcslen(result->pData[result->nCols * nRow + nColumn]);
	return -1;
}

/**
 * Get field value from result
 */
static WCHAR *GetField(DBDRV_RESULT hResult, int nRow, int nColumn, WCHAR *pBuffer, int nBufLen)
{
   auto result = static_cast<ORACLE_RESULT*>(hResult);
   WCHAR *pValue = nullptr;
   if ((nRow < result->nRows) && (nRow >= 0) && (nColumn < result->nCols) && (nColumn >= 0))
   {
      wcslcpy(pBuffer, result->pData[nRow * result->nCols + nColumn], nBufLen);
      pValue = pBuffer;
   }
   return pValue;
}

/**
 * Get number of rows in result
 */
static int GetNumRows(DBDRV_RESULT hResult)
{
	return static_cast<ORACLE_RESULT*>(hResult)->nRows;
}

/**
 * Get column count in query result
 */
static int GetColumnCount(DBDRV_RESULT hResult)
{
	return static_cast<ORACLE_RESULT*>(hResult)->nCols;
}

/**
 * Get column name in query result
 */
static const char *GetColumnName(DBDRV_RESULT hResult, int column)
{
	return ((column >= 0) && (column < static_cast<ORACLE_RESULT*>(hResult)->nCols)) ? static_cast<ORACLE_RESULT*>(hResult)->columnNames[column] : nullptr;
}

/**
 * Free SELECT results
 */
static void FreeResult(DBDRV_RESULT hResult)
{
   DestroyQueryResult(static_cast<ORACLE_RESULT*>(hResult));
}

/**
 * Destroy unbuffered query result
 */
static void DestroyUnbufferedQueryResult(ORACLE_UNBUFFERED_RESULT *result, bool freeStatement)
{
   if (freeStatement)
      OCIStmtRelease(result->handleStmt, result->connection->handleError, nullptr, 0, OCI_DEFAULT);

   for(int i = 0; i < result->nCols; i++)
   {
      MemFree(result->pBuffers[i].data);
      if (result->pBuffers[i].lobLocator != nullptr)
      {
         OCIDescriptorFree(result->pBuffers[i].lobLocator, OCI_DTYPE_LOB);
      }
   }
   MemFree(result->pBuffers);

   for(int i = 0; i < result->nCols; i++)
      MemFree(result->columnNames[i]);
   MemFree(result->columnNames);

   MemFree(result);
}

/**
 * Process results of unbuffered query execution (prepare for fetching results)
 */
static ORACLE_UNBUFFERED_RESULT *ProcessUnbufferedQueryResults(ORACLE_CONN *pConn, OCIStmt *handleStmt, uint32_t *pdwError)
{
   ORACLE_UNBUFFERED_RESULT *result = MemAllocStruct<ORACLE_UNBUFFERED_RESULT>();
   result->handleStmt = handleStmt;
   result->connection = pConn;

   ub4 nCount;
   OCIAttrGet(result->handleStmt, OCI_HTYPE_STMT, &nCount, NULL, OCI_ATTR_PARAM_COUNT, pConn->handleError);
   result->nCols = nCount;
   if (result->nCols > 0)
   {
      // Prepare receive buffers and fetch column names
      result->columnNames = MemAllocArray<char*>(result->nCols);
      result->pBuffers = MemAllocArray<OracleFetchBuffer>(result->nCols);
      for(int i = 0; i < result->nCols; i++)
      {
         OCIParam *handleParam;

         result->pBuffers[i].isNull = 1;   // Mark all columns as NULL initially
         if (OCIParamGet(result->handleStmt, OCI_HTYPE_STMT, pConn->handleError,
                         (void **)&handleParam, (ub4)(i + 1)) == OCI_SUCCESS)
         {
            // Column name
            result->columnNames[i] = GetColumnNameFromHandle(handleParam, pConn->handleError);

            // Data size
            sword nStatus;
            ub2 type = 0;
            OCIAttrGet(handleParam, OCI_DTYPE_PARAM, &type, nullptr, OCI_ATTR_DATA_TYPE, pConn->handleError);
            OCIDefine *handleDefine;
            if (type == OCI_TYPECODE_CLOB)
            {
               result->pBuffers[i].data = nullptr;
               OCIDescriptorAlloc(s_handleEnv, (void **)&result->pBuffers[i].lobLocator, OCI_DTYPE_LOB, 0, nullptr);
               handleDefine = nullptr;
               nStatus = OCIDefineByPos(result->handleStmt, &handleDefine, pConn->handleError, i + 1,
                                        &result->pBuffers[i].lobLocator, 0, SQLT_CLOB, &result->pBuffers[i].isNull,
                                        nullptr, nullptr, OCI_DEFAULT);
            }
            else
            {
               ub2 nWidth;
               result->pBuffers[i].lobLocator = nullptr;
               OCIAttrGet(handleParam, OCI_DTYPE_PARAM, &nWidth, nullptr, OCI_ATTR_DATA_SIZE, pConn->handleError);
               result->pBuffers[i].data = (UCS2CHAR *)MemAlloc((nWidth + 31) * sizeof(UCS2CHAR));
               handleDefine = nullptr;
               nStatus = OCIDefineByPos(result->handleStmt, &handleDefine, pConn->handleError, i + 1,
                                        result->pBuffers[i].data, (nWidth + 31) * sizeof(UCS2CHAR),
                                        SQLT_CHR, &result->pBuffers[i].isNull, &result->pBuffers[i].length,
                                        &result->pBuffers[i].code, OCI_DEFAULT);
            }
            OCIDescriptorFree(handleParam, OCI_DTYPE_PARAM);
            if (nStatus == OCI_SUCCESS)
            {
               *pdwError = DBERR_SUCCESS;
            }
            else
            {
               SetLastError(pConn);
               *pdwError = IsConnectionError(pConn);
               DestroyUnbufferedQueryResult(result, false);
               result = nullptr;
               break;
            }
         }
         else
         {
            SetLastError(pConn);
            *pdwError = IsConnectionError(pConn);
            DestroyUnbufferedQueryResult(result, false);
            result = nullptr;
            break;
         }
      }
   }
   else
   {
      MemFree(result);
      result = nullptr;
   }

   return result;
}

/**
 * Perform unbuffered SELECT query
 */
static DBDRV_UNBUFFERED_RESULT SelectUnbuffered(DBDRV_CONNECTION connection, const WCHAR *query, uint32_t *errorCode, WCHAR *errorText)
{
   ORACLE_UNBUFFERED_RESULT *result = nullptr;

#if UNICODE_UCS4
   UCS2CHAR localBuffer[1024];
	UCS2CHAR *ucs2Query = WideStringToUCS2(query, localBuffer, 1024);
#else
	const UCS2CHAR *ucs2Query = query;
#endif

   auto pConn = static_cast<ORACLE_CONN*>(connection);
	pConn->mutexQueryLock->lock();

	OCIStmt *handleStmt;
	if (IsSuccess(OCIStmtPrepare2(pConn->handleService, &handleStmt, pConn->handleError, (text *)ucs2Query,
	                    (ub4)ucs2_strlen(ucs2Query) * sizeof(UCS2CHAR), nullptr, 0, OCI_NTV_SYNTAX, OCI_DEFAULT)))
	{
      OCIAttrSet(handleStmt, OCI_HTYPE_STMT, &pConn->prefetchLimit, 0, OCI_ATTR_PREFETCH_ROWS, pConn->handleError);
		if (IsSuccess(OCIStmtExecute(pConn->handleService, handleStmt, pConn->handleError,
		                   0, 0, NULL, NULL, (pConn->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT)))
		{
		   result = ProcessUnbufferedQueryResults(pConn, handleStmt, errorCode);
		}
		else
		{
			SetLastError(pConn);
			*errorCode = IsConnectionError(pConn);
		}
	}
	else
	{
		SetLastError(pConn);
		*errorCode = IsConnectionError(pConn);
	}

#if UNICODE_UCS4
	FreeConvertedString(ucs2Query, localBuffer);
#endif

	if ((*errorCode == DBERR_SUCCESS) && (result != nullptr))
		return result;

	// On failure, unlock query mutex and do cleanup
	OCIStmtRelease(handleStmt, pConn->handleError, nullptr, 0, OCI_DEFAULT);
	if (errorText != nullptr)
	{
		wcslcpy(errorText, pConn->lastErrorText, DBDRV_MAX_ERROR_TEXT);
	}
	pConn->mutexQueryLock->unlock();
	return nullptr;
}

/**
 * Perform SELECT query using prepared statement
 */
static DBDRV_UNBUFFERED_RESULT SelectPreparedUnbuffered(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *pdwError, WCHAR *errorText)
{
   ORACLE_UNBUFFERED_RESULT *result = nullptr;
   auto pConn = static_cast<ORACLE_CONN*>(connection);

   pConn->mutexQueryLock->lock();

   OCIStmt *handleStmt = static_cast<ORACLE_STATEMENT*>(hStmt)->handleStmt;
   OCIAttrSet(handleStmt, OCI_HTYPE_STMT, &pConn->prefetchLimit, 0, OCI_ATTR_PREFETCH_ROWS, pConn->handleError);
   if (IsSuccess(OCIStmtExecute(pConn->handleService, handleStmt, pConn->handleError,
                      0, 0, NULL, NULL, (pConn->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT)))
   {
      result = ProcessUnbufferedQueryResults(pConn, handleStmt, pdwError);
   }
   else
   {
      SetLastError(pConn);
      *pdwError = IsConnectionError(pConn);
   }

   if ((*pdwError == DBERR_SUCCESS) && (result != nullptr))
      return result;

   // On failure, unlock query mutex and do cleanup
   if (errorText != nullptr)
   {
      wcslcpy(errorText, pConn->lastErrorText, DBDRV_MAX_ERROR_TEXT);
   }
   pConn->mutexQueryLock->unlock();
   return nullptr;
}

/**
 * Fetch next result line from unbuffered SELECT results
 */
static bool Fetch(DBDRV_UNBUFFERED_RESULT hResult)
{
   auto result = static_cast<ORACLE_UNBUFFERED_RESULT*>(hResult);
	bool success;
	sword rc = OCIStmtFetch2(result->handleStmt, result->connection->handleError, 1, OCI_FETCH_NEXT, 0, OCI_DEFAULT);
	if ((rc == OCI_SUCCESS) || (rc == OCI_SUCCESS_WITH_INFO))
	{
		success = true;
	}
	else
	{
		SetLastError(result->connection);
		success = false;
	}
	return success;
}

/**
 * Get field length from current row in unbuffered query result
 */
static int32_t GetFieldLengthUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column)
{
   auto result = static_cast<ORACLE_UNBUFFERED_RESULT*>(hResult);

	if ((column < 0) || (column >= result->nCols))
		return 0;

	if (result->pBuffers[column].isNull)
		return 0;

   if (result->pBuffers[column].lobLocator != nullptr)
   {
      ub4 length = 0;
      OCILobGetLength(result->connection->handleService, result->connection->handleError, result->pBuffers[column].lobLocator, &length);
      return static_cast<int32_t>(length);
   }

	return static_cast<int32_t>(result->pBuffers[column].length / sizeof(UCS2CHAR));
}

/**
 * Get field from current row in unbuffered query result
 */
static WCHAR *GetFieldUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int nColumn, WCHAR *buffer, int bufferSize)
{
   auto result = static_cast<ORACLE_UNBUFFERED_RESULT*>(hResult);

	if ((nColumn < 0) || (nColumn >= result->nCols))
		return nullptr;

	if (result->pBuffers[nColumn].isNull)
	{
		*buffer = 0;
	}
   else if (result->pBuffers[nColumn].lobLocator != nullptr)
   {
      ub4 length = 0;
      OCILobGetLength(result->connection->handleService, result->connection->handleError, result->pBuffers[nColumn].lobLocator, &length);

		int nLen = std::min(bufferSize - 1, (int)length);
      ub4 amount = nLen;
#if UNICODE_UCS4
      UCS2CHAR *ucs2buffer = MemAllocArrayNoInit<UCS2CHAR>(nLen);
      OCILobRead(result->connection->handleService, result->connection->handleError, result->pBuffers[nColumn].lobLocator, &amount, 1,
                 ucs2buffer, nLen * sizeof(UCS2CHAR), nullptr, nullptr, OCI_UCS2ID, SQLCS_IMPLICIT);
		ucs2_to_ucs4(ucs2buffer, nLen, buffer, nLen);
      MemFree(ucs2buffer);
#else
      OCILobRead(result->connection->handleService, result->connection->handleError, result->pBuffers[nColumn].lobLocator, &amount, 1,
                 buffer, bufferSize * sizeof(WCHAR), nullptr, nullptr, OCI_UCS2ID, SQLCS_IMPLICIT);
#endif
		buffer[nLen] = 0;
   }
	else
	{
		int nLen = std::min(bufferSize - 1, ((int)(result->pBuffers[nColumn].length / sizeof(UCS2CHAR))));
#if UNICODE_UCS4
		ucs2_to_ucs4(result->pBuffers[nColumn].data, nLen, buffer, nLen + 1);
#else
		memcpy(buffer, result->pBuffers[nColumn].data, nLen * sizeof(WCHAR));
#endif
		buffer[nLen] = 0;
	}

	return buffer;
}

/**
 * Get column count in unbuffered query result
 */
static int GetColumnCountUnbuffered(DBDRV_UNBUFFERED_RESULT hResult)
{
	return static_cast<ORACLE_UNBUFFERED_RESULT*>(hResult)->nCols;
}

/**
 * Get column name in unbuffered query result
 */
static const char *GetColumnNameUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column)
{
	return ((column >= 0) && (column < static_cast<ORACLE_UNBUFFERED_RESULT*>(hResult)->nCols)) ? static_cast<ORACLE_UNBUFFERED_RESULT*>(hResult)->columnNames[column] : nullptr;
}

/**
 * Destroy result of unbuffered query
 */
static void FreeUnbufferedResult(DBDRV_UNBUFFERED_RESULT hResult)
{
	Mutex *mutex = static_cast<ORACLE_UNBUFFERED_RESULT*>(hResult)->connection->mutexQueryLock;
	DestroyUnbufferedQueryResult(static_cast<ORACLE_UNBUFFERED_RESULT*>(hResult), true);
	mutex->unlock();
}

/**
 * Begin transaction
 */
static uint32_t Begin(DBDRV_CONNECTION connection)
{
   auto pConn = static_cast<ORACLE_CONN*>(connection);
	pConn->mutexQueryLock->lock();
	pConn->nTransLevel++;
	pConn->mutexQueryLock->unlock();
	return DBERR_SUCCESS;
}

/**
 * Commit transaction
 */
static uint32_t Commit(DBDRV_CONNECTION connection)
{
   uint32_t rc;

   auto pConn = static_cast<ORACLE_CONN*>(connection);
	pConn->mutexQueryLock->lock();
	if (pConn->nTransLevel > 0)
	{
		if (IsSuccess(OCITransCommit(pConn->handleService, pConn->handleError, OCI_DEFAULT)))
		{
			rc = DBERR_SUCCESS;
			pConn->nTransLevel = 0;
		}
		else
		{
			SetLastError(pConn);
			rc = IsConnectionError(pConn);
		}
	}
	else
	{
		rc = DBERR_SUCCESS;
	}
	pConn->mutexQueryLock->unlock();
	return rc;
}

/**
 * Rollback transaction
 */
static uint32_t Rollback(DBDRV_CONNECTION connection)
{
   uint32_t rc;

   auto pConn = static_cast<ORACLE_CONN*>(connection);
	pConn->mutexQueryLock->lock();
	if (pConn->nTransLevel > 0)
	{
		if (IsSuccess(OCITransRollback(pConn->handleService, pConn->handleError, OCI_DEFAULT)))
		{
			rc = DBERR_SUCCESS;
			pConn->nTransLevel = 0;
		}
		else
		{
			SetLastError(pConn);
			rc = IsConnectionError(pConn);
		}
	}
	else
	{
		rc = DBERR_SUCCESS;
	}
	pConn->mutexQueryLock->unlock();
	return rc;
}

/**
 * Check if table exist
 */
static int IsTableExist(DBDRV_CONNECTION connection, const WCHAR *name)
{
   WCHAR query[256];
   swprintf(query, 256, L"SELECT count(*) FROM user_tables WHERE table_name=upper('%ls')", name);
   uint32_t error;
   int rc = DBIsTableExist_Failure;
   ORACLE_RESULT *result = static_cast<ORACLE_RESULT*>(Select(connection, query, &error, nullptr));
   if (result != nullptr)
   {
      if ((result->nRows > 0) && (result->nCols > 0))
      {
         rc = (wcstol(result->pData[0], nullptr, 10) > 0) ? DBIsTableExist_Found : DBIsTableExist_NotFound;
      }
      else
      {
         rc = DBIsTableExist_NotFound;
      }
      DestroyQueryResult(result);
   }
   return rc;
}

/**
 * Driver call table
 */
static DBDriverCallTable s_callTable =
{
   Initialize,
   Connect,
   Disconnect,
   SetPrefetchLimit,
   Prepare,
   FreeStatement,
   OpenBatch,
   NextBatchRow,
   Bind,
   Execute,
   Query,
   Select,
   SelectUnbuffered,
   SelectPrepared,
   SelectPreparedUnbuffered,
   Fetch,
   GetFieldLength,
   GetFieldLengthUnbuffered,
   GetField,
   nullptr, // GetFieldUTF8
   GetFieldUnbuffered,
   nullptr, // GetFieldUnbufferedUTF8
   GetNumRows,
   FreeResult,
   FreeUnbufferedResult,
   Begin,
   Commit,
   Rollback,
   Unload,
   GetColumnCount,
   GetColumnName,
   GetColumnCountUnbuffered,
   GetColumnNameUnbuffered,
   PrepareString,
   IsTableExist
};

DB_DRIVER_ENTRY_POINT("ORACLE", s_callTable)

#ifdef _WIN32

/**
 * DLL Entry point
 */
bool WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return true;
}

#endif
