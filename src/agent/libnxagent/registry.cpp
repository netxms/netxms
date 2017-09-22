/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: registry.cpp
**
**/

#include "libnxagent.h"

/**
 * Read registry value as a string using provided attribute.
 *
 * @return attribute value or default value if no value found
 */
TCHAR LIBNXAGENT_EXPORTABLE *ReadRegistryAsString(const TCHAR *attr, TCHAR *buffer, int bufSize, const TCHAR *defaultValue)
{
   TCHAR *value = NULL;

   DB_HANDLE hdb = AgentGetLocalDatabaseHandle();
   if(hdb != NULL && attr != NULL)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT value FROM registry WHERE attribute=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, attr, DB_BIND_STATIC);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != NULL)
         {
            if(DBGetNumRows(hResult) > 0)
               value = DBGetField(hResult, 0, 0, buffer, bufSize);
            DBFreeResult(hResult);
         }
         DBFreeStatement(hStmt);
      }
   }

   if ((value == NULL) && (defaultValue != NULL))
   {
      if (buffer == NULL)
      {
         value = _tcsdup(defaultValue);
      }
      else
      {
         _tcslcpy(buffer, defaultValue, bufSize);
         value = buffer;
      }
   }
   return value;
}

/**
 * Read registry value as a INT32 using provided attribute.
 *
 * @return attribute value or default value in no value found
 */
INT32 LIBNXAGENT_EXPORTABLE ReadRegistryAsInt32(const TCHAR *attr, INT32 defaultValue)
{
   TCHAR buffer[MAX_DB_STRING];
   TCHAR *value = ReadRegistryAsString(attr, buffer, MAX_DB_STRING, NULL);
   if (value == NULL)
   {
      return defaultValue;
   }
   else
   {
      return _tcstol(buffer, NULL, 0);
   }
}

/**
 * Read registry value as a INT32 using provided attribute.
 *
 * @return attribute value or default value in no value found
 */
INT64 LIBNXAGENT_EXPORTABLE ReadRegistryAsInt64(const TCHAR *attr, INT64 defaultValue)
{
   TCHAR buffer[MAX_DB_STRING];
   TCHAR *value = ReadRegistryAsString(attr, buffer, MAX_DB_STRING, NULL);
   if (value == NULL)
   {
      return defaultValue;
   }
   else
   {
      return _tcstoll(buffer, NULL, 0);
   }
}

/**
 * Write registry value as a string. This method inserts new value or update existing.
 *
 * @return if this update/insert was successful
 */
bool LIBNXAGENT_EXPORTABLE WriteRegistry(const TCHAR *attr, const TCHAR *value)
{
   DB_HANDLE hdb = AgentGetLocalDatabaseHandle();
   if (_tcslen(attr) > 63 || hdb == NULL)
      return false;

   // Check for variable existence
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT value FROM registry WHERE attribute=?"));
   if (hStmt == NULL)
   {
      return false;
   }
   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, attr, DB_BIND_STATIC);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   bool varExist = false;
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         varExist = true;
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);

   // Create or update variable value
   if (varExist)
   {
      hStmt = DBPrepare(hdb, _T("UPDATE registry SET value=? WHERE attribute=?"));
      if (hStmt == NULL)
      {
         return false;
      }
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, value, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, attr, DB_BIND_STATIC);
   }
   else
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO registry (attribute,value) VALUES (?,?)"));
      if (hStmt == NULL)
      {
         return false;
      }
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, attr, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, value, DB_BIND_STATIC);
   }
   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Write registry value as a INT32. This method inserts new value or update existing.
 *
 * @return if this update/insert was successful
 */
bool LIBNXAGENT_EXPORTABLE WriteRegistry(const TCHAR *attr, INT32 value)
{
   TCHAR buffer[64];
   _sntprintf(buffer, 64, _T("%d"), value);
   return WriteRegistry(attr, buffer);
}

/**
 * Write registry value as a INT64. This method inserts new value or update existing.
 *
 * @return if this update/insert was successful
 */
bool LIBNXAGENT_EXPORTABLE WriteRegistry(const TCHAR *attr, INT64 value)
{
   TCHAR buffer[64];
   _sntprintf(buffer, 64, INT64_FMT, value);
   return WriteRegistry(attr, buffer);
}

/**
 * Delete registry entry
 *
 * @return if this delete was successful
 */
bool LIBNXAGENT_EXPORTABLE DeleteRegistryEntry(const TCHAR *attr)
{
   bool success = false;

   DB_HANDLE hdb = AgentGetLocalDatabaseHandle();
   if(hdb == NULL || attr == NULL)
      return false;

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM registry WHERE attribute=?"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, attr, DB_BIND_STATIC);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   return success;
}
