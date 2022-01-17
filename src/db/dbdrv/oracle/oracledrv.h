/** Oracle Database Driver
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
** File: oracledrv.h
**
**/

#ifndef _oracledrv_h_
#define _oracledrv_h_

#ifdef _WIN32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <windows.h>

#endif   /* _WIN32 */

#include <nms_common.h>
#include <nms_util.h>
#include <dbdrv.h>
#include <oci.h>

#define DEBUG_TAG _T("db.drv.oracle")

/**
 * Fetch buffer
 */
struct OracleFetchBuffer
{
	UCS2CHAR *data;
   OCILobLocator *lobLocator;
	ub2 length;
	ub2 code;
	sb2 isNull;
};

/**
 * Database connection
 */
struct ORACLE_CONN
{
	OCIServer *handleServer;
	OCISvcCtx *handleService;
	OCISession *handleSession;
	OCIError *handleError;
	Mutex *mutexQueryLock;
	int nTransLevel;
	sb4 lastErrorCode;
	WCHAR lastErrorText[DBDRV_MAX_ERROR_TEXT];
   ub4 prefetchLimit;
};

/**
 * Bind data
 */
struct OracleBind
{
   OCIBind *handle;
   void *data;
   OCILobLocator *lobLocator;
   OCIError *errorHandle;
   OCISvcCtx *serviceContext;
   OCINumber number;

   OracleBind(OCISvcCtx *_serviceContext, OCIError *_errorHandle)
   {
      handle = nullptr;
      data = nullptr;
      lobLocator = nullptr;
      serviceContext = _serviceContext;
      errorHandle = _errorHandle;
   }

   ~OracleBind()
   {
      freeTemporaryLob();
      MemFree(data);
   }

   bool createTemporaryLob(OCIEnv *envHandle)
   {
      if (OCIDescriptorAlloc(envHandle, (void **)&lobLocator, OCI_DTYPE_LOB, 0, nullptr) != OCI_SUCCESS)
         return false;
      if (OCILobCreateTemporary(serviceContext, errorHandle, lobLocator,
               OCI_DEFAULT, OCI_DEFAULT, OCI_TEMP_CLOB, FALSE, OCI_DURATION_SESSION) != OCI_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("OracleBind::createTemporaryLob(): cannot create temporary LOB locator"));
         OCIDescriptorFree(lobLocator, OCI_DTYPE_LOB);
         lobLocator = nullptr;
         return false;
      }
      return true;
   }

   void freeTemporaryLob()
   {
      if (lobLocator != nullptr)
      {
         OCILobFreeTemporary(serviceContext, errorHandle, lobLocator);
         OCIDescriptorFree(lobLocator, OCI_DTYPE_LOB);
         lobLocator = nullptr;
      }
   }
};

/**
 * Batch bind data for single column
 */
class OracleBatchBind
{
private:
   int m_cType;
   ub2 m_oraType;
   int m_size;
   int m_allocated;
   int m_elementSize;
   bool m_string;
   UCS2CHAR **m_strings;
   void *m_data;

public:
   OracleBatchBind(int cType, int sqlType);
   ~OracleBatchBind();

   void addRow();
   void set(void *value);
   void *getData();
   int getElementSize() { return m_elementSize; }
   int getCType() { return m_cType; }
   ub2 getOraType() { return m_oraType; }
};

/**
 * Statement
 */
struct ORACLE_STATEMENT
{
	ORACLE_CONN *connection;
	OCIStmt *handleStmt;
	OCIError *handleError;
	ObjectArray<OracleBind> *bindings;
   ObjectArray<OracleBatchBind> *batchBindings;
   bool batchMode;
   int batchSize;
};

/**
 * Result set
 */
struct ORACLE_RESULT
{
	int nRows;
	int nCols;
	WCHAR **pData;
	char **columnNames;
};

/**
 * Unbuffered result set
 */
struct ORACLE_UNBUFFERED_RESULT
{
   ORACLE_CONN *connection;
   OCIStmt *handleStmt;
   OracleFetchBuffer *pBuffers;
   int nCols;
   char **columnNames;
};

/**
 * Undocumented internal structure for column parameter handler
 */
struct OCI_PARAM_STRUCT_COLUMN_INFO
{
   unsigned char unknownFields[6 * sizeof(int) + 2 * sizeof(void *)];
   unsigned char attributes[sizeof(void *)];
   char *name;
};

/**
 * Undocumented internal structure for column parameter handler
 */
struct OCI_PARAM_STRUCT
{
   unsigned char unknownFields[3 * sizeof(void *)];
   OCI_PARAM_STRUCT_COLUMN_INFO *columnInfo;
};

#endif   /* _oracledrv_h_ */
