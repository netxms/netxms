/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: dbdrv.h
**
**/

#ifndef _dbdrv_h_
#define _dbdrv_h_

#include <nms_common.h>
#include <nms_util.h>

/**
 * API version
 */
#define DBDRV_API_VERSION           31

/**
 * Database driver entry point declaration
 */
#define DB_DRIVER_ENTRY_POINT(name, callTable) \
extern "C" __EXPORT_VAR(int drvAPIVersion); \
extern "C" __EXPORT_VAR(const char *drvName); \
extern "C" __EXPORT_VAR(DBDriverCallTable *drvCallTable); \
__EXPORT_VAR(int drvAPIVersion) = DBDRV_API_VERSION; \
__EXPORT_VAR(const char *drvName) = name; \
__EXPORT_VAR(DBDriverCallTable *drvCallTable) = &callTable;

/**
 * Max error text length
 */
#define DBDRV_MAX_ERROR_TEXT        1024

/**
 * Datatypes
 */
typedef void* DBDRV_CONNECTION;
typedef void* DBDRV_STATEMENT;
typedef void* DBDRV_RESULT;
typedef void* DBDRV_UNBUFFERED_RESULT;

/**
 * Driver call table
 */
struct DBDriverCallTable
{
   bool (*Initialize)(const char*);
   DBDRV_CONNECTION (*Connect)(const char *, const char *, const char *, const char *, const char *, WCHAR *);
   void (*Disconnect)(DBDRV_CONNECTION);
   bool (*SetPrefetchLimit)(DBDRV_CONNECTION, int);
   DBDRV_STATEMENT (*Prepare)(DBDRV_CONNECTION, const WCHAR *, bool, uint32_t *, WCHAR *);
   void (*FreeStatement)(DBDRV_STATEMENT);
   bool (*OpenBatch)(DBDRV_STATEMENT);
   void (*NextBatchRow)(DBDRV_STATEMENT);
   void (*Bind)(DBDRV_STATEMENT, int, int, int, void *, int);
   uint32_t (*Execute)(DBDRV_CONNECTION, DBDRV_STATEMENT, WCHAR *);
   uint32_t (*Query)(DBDRV_CONNECTION, const WCHAR *, WCHAR *);
   DBDRV_RESULT (*Select)(DBDRV_CONNECTION, const WCHAR *, uint32_t *, WCHAR *);
   DBDRV_UNBUFFERED_RESULT (*SelectUnbuffered)(DBDRV_CONNECTION, const WCHAR *, uint32_t *, WCHAR *);
   DBDRV_RESULT (*SelectPrepared)(DBDRV_CONNECTION, DBDRV_STATEMENT, uint32_t *, WCHAR *);
   DBDRV_UNBUFFERED_RESULT (*SelectPreparedUnbuffered)(DBDRV_CONNECTION, DBDRV_STATEMENT, uint32_t *, WCHAR *);
   bool (*Fetch)(DBDRV_UNBUFFERED_RESULT);
   int32_t (*GetFieldLength)(DBDRV_RESULT, int, int);
   int32_t (*GetFieldLengthUnbuffered)(DBDRV_UNBUFFERED_RESULT, int);
   WCHAR* (*GetField)(DBDRV_RESULT, int, int, WCHAR *, int);
   char* (*GetFieldUTF8)(DBDRV_RESULT, int, int, char *, int);
   WCHAR* (*GetFieldUnbuffered)(DBDRV_UNBUFFERED_RESULT, int, WCHAR *, int);
   char* (*GetFieldUnbufferedUTF8)(DBDRV_UNBUFFERED_RESULT, int, char *, int);
   int (*GetNumRows)(DBDRV_RESULT);
   void (*FreeResult)(DBDRV_RESULT);
   void (*FreeUnbufferedResult)(DBDRV_UNBUFFERED_RESULT);
   uint32_t (*Begin)(DBDRV_CONNECTION);
   uint32_t (*Commit)(DBDRV_CONNECTION);
   uint32_t (*Rollback)(DBDRV_CONNECTION);
   void (*Unload)();
   int (*GetColumnCount)(DBDRV_RESULT);
   const char* (*GetColumnName)(DBDRV_RESULT, int);
   int (*GetColumnCountUnbuffered)(DBDRV_UNBUFFERED_RESULT);
   const char* (*GetColumnNameUnbuffered)(DBDRV_UNBUFFERED_RESULT, int);
   StringBuffer (*PrepareString)(const TCHAR*, size_t);
   int (*IsTableExist)(DBDRV_CONNECTION, const WCHAR*);
};

//
// Error codes
//
#define DBERR_SUCCESS               0
#define DBERR_CONNECTION_LOST       1
#define DBERR_INVALID_HANDLE        2
#define DBERR_OTHER_ERROR           255

//
// DB binding buffer allocation types
//
#define DB_BIND_STATIC     0 // buffer is managed by caller and will be valid until after the query is executed
#define DB_BIND_TRANSIENT  1 // buffer will be duplicated by DB driver in DBBind()
#define DB_BIND_DYNAMIC    2 // DB Driver will call free() on buffer

//
// C and SQL types for parameter binding
//
#define DB_CTYPE_STRING       0
#define DB_CTYPE_INT32        1
#define DB_CTYPE_UINT32       2
#define DB_CTYPE_INT64        3
#define DB_CTYPE_UINT64       4
#define DB_CTYPE_DOUBLE       5
#define DB_CTYPE_UTF8_STRING  6

#define DB_SQLTYPE_VARCHAR    0
#define DB_SQLTYPE_INTEGER    1
#define DB_SQLTYPE_BIGINT     2
#define DB_SQLTYPE_DOUBLE     3
#define DB_SQLTYPE_TEXT       4

/**
 * DBIsTableExist return codes
 */
enum
{
   DBIsTableExist_Failure = -1,
   DBIsTableExist_NotFound = 0,
   DBIsTableExist_Found = 1
};

#endif   /* _dbdrv_h_ */
