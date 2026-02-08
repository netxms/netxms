/*
** NetXMS database manager library
** Copyright (C) 2004-2019 Victor Kirhenshtein
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
** File: config.cpp
**
**/

#include "libnxdbmgr.h"

/**
 * Read string value from metadata table
 */
bool LIBNXDBMGR_EXPORTABLE DBMgrMetaDataReadStr(const TCHAR *variable, TCHAR *buffer, size_t bufferSize, const TCHAR *defaultValue)
{
   return DBMgrMetaDataReadStrEx(g_dbHandle, variable, buffer, bufferSize, defaultValue);
}

/**
 * Read string value from metadata table
 */
bool LIBNXDBMGR_EXPORTABLE DBMgrMetaDataReadStrEx(DB_HANDLE hdb, const TCHAR *variable, TCHAR *buffer, size_t bufferSize, const TCHAR *defaultValue)
{
   _tcslcpy(buffer, defaultValue, bufferSize);
   if (_tcslen(variable) > 127)
      return false;

   TCHAR szQuery[256];
   _sntprintf(szQuery, 256, _T("SELECT var_value FROM metadata WHERE var_name='%s'"), variable);
   DB_RESULT hResult = SQLSelectEx(hdb, szQuery);
   if (hResult == NULL)
      return false;

   bool success = false;
   if (DBGetNumRows(hResult) > 0)
   {
      DBGetField(hResult, 0, 0, buffer, bufferSize);
      success = true;
   }

   DBFreeResult(hResult);
   return success;
}

/**
 * Read integer value from configuration table
 */
int32_t LIBNXDBMGR_EXPORTABLE DBMgrMetaDataReadInt32(const TCHAR *variable, int32_t defaultValue)
{
   return DBMgrMetaDataReadInt32Ex(g_dbHandle, variable, defaultValue);
}

/**
 * Read integer value from configuration table
 */
int32_t LIBNXDBMGR_EXPORTABLE DBMgrMetaDataReadInt32Ex(DB_HANDLE hdb, const TCHAR *variable, int32_t defaultValue)
{
   TCHAR szBuffer[64];
   if (DBMgrMetaDataReadStrEx(hdb, variable, szBuffer, 64, _T("")))
      return _tcstol(szBuffer, NULL, 0);
   else
      return defaultValue;
}

/**
 * Write string value to metadata table
 */
bool LIBNXDBMGR_EXPORTABLE DBMgrMetaDataWriteStr(const wchar_t *variable, const wchar_t *value)
{
   // Check for variable existence
   DB_STATEMENT hStmt = DBPrepare(g_dbHandle, L"SELECT var_value FROM metadata WHERE var_name=?");
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   bool bVarExist = false;
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         bVarExist = true;
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);

   // Create or update variable value
   if (bVarExist)
   {
      hStmt = DBPrepare(g_dbHandle, L"UPDATE metadata SET var_value=? WHERE var_name=?");
      if (hStmt == NULL)
         return false;
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, value, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
   }
   else
   {
      hStmt = DBPrepare(g_dbHandle, L"INSERT INTO metadata (var_name,var_value) VALUES (?,?)");
      if (hStmt == NULL)
         return false;
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, variable, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, value, DB_BIND_STATIC);
   }
   bool success = SQLExecute(hStmt);
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Write integer value to metadata table
 */
bool LIBNXDBMGR_EXPORTABLE DBMgrMetaDataWriteInt32(const wchar_t *variable, int32_t value)
{
   wchar_t buffer[64];
   return DBMgrMetaDataWriteStr(variable, IntegerToString(value, buffer));
}

/**
 * Read string value from configuration table
 */
bool LIBNXDBMGR_EXPORTABLE DBMgrConfigReadStr(const TCHAR *variable, TCHAR *buffer, size_t bufferSize, const TCHAR *defaultValue)
{
   DB_RESULT hResult;
   TCHAR szQuery[256];
   BOOL bSuccess = FALSE;

   _tcslcpy(buffer, defaultValue, bufferSize);
   if (_tcslen(variable) > 127)
      return FALSE;

   _sntprintf(szQuery, 256, _T("SELECT var_value FROM config WHERE var_name='%s'"), variable);
   hResult = SQLSelect(szQuery);
   if (hResult == NULL)
      return FALSE;

   if (DBGetNumRows(hResult) > 0)
   {
      DBGetField(hResult, 0, 0, buffer, bufferSize);
      bSuccess = TRUE;
   }

   DBFreeResult(hResult);
   return bSuccess;
}

/**
 * Read integer value from configuration table
 */
int32_t LIBNXDBMGR_EXPORTABLE DBMgrConfigReadInt32(const TCHAR *variable, int32_t defaultValue)
{
   TCHAR buffer[64];
   if (DBMgrConfigReadStr(variable, buffer, 64, _T("")))
      return _tcstol(buffer, NULL, 0);
   else
      return defaultValue;
}

/**
 * Read unsigned long value from configuration table
 */
uint32_t LIBNXDBMGR_EXPORTABLE DBMgrConfigReadUInt32(const TCHAR *variable, uint32_t defaultValue)
{
   TCHAR buffer[64];
   if (DBMgrConfigReadStr(variable, buffer, 64, _T("")))
      return _tcstoul(buffer, NULL, 0);
   else
      return defaultValue;
}

/**
 * Read all configuration variables that match specific pattern
 */
StringMap LIBNXDBMGR_EXPORTABLE *DBMgrGetConfigurationVariables(const TCHAR *pattern)
{
   DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("SELECT var_name,var_value FROM config WHERE var_name LIKE ? ORDER BY var_name"));
   if (hStmt == nullptr)
      return nullptr;

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, pattern, DB_BIND_STATIC);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
   {
      DBFreeStatement(hStmt);
      return nullptr;
   }

   StringMap *variables = new StringMap();
   int count = DBGetNumRows(hResult);
   for (int i = 0; i < count; i++)
   {
      variables->setPreallocated(DBGetField(hResult, i, 0, nullptr, 0), DBGetField(hResult, i, 1, nullptr, 0));
   }

   DBFreeResult(hResult);
   DBFreeStatement(hStmt);

   return variables;
}
