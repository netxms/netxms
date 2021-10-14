/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Raden Solutions
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
TCHAR LIBNXAGENT_EXPORTABLE *ReadRegistryAsString(const TCHAR *attr, TCHAR *buffer, size_t bufferSize, const TCHAR *defaultValue)
{
   TCHAR *value = nullptr;

   DB_HANDLE hdb = AgentGetLocalDatabaseHandle();
   if ((hdb != nullptr) && (attr != nullptr))
   {
      TCHAR query[256];
      _sntprintf(query, 256, _T("SELECT value FROM registry WHERE attribute=%s"), DBPrepareString(hdb, attr).cstr());
      DB_RESULT hResult = DBSelect(hdb, query);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
            value = DBGetField(hResult, 0, 0, buffer, bufferSize);
         DBFreeResult(hResult);
      }
   }

   if ((value == nullptr) && (defaultValue != nullptr))
   {
      if (buffer == nullptr)
      {
         value = MemCopyString(defaultValue);
      }
      else
      {
         _tcslcpy(buffer, defaultValue, bufferSize);
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
int32_t LIBNXAGENT_EXPORTABLE ReadRegistryAsInt32(const TCHAR *attr, int32_t defaultValue)
{
   TCHAR buffer[MAX_DB_STRING];
   TCHAR *value = ReadRegistryAsString(attr, buffer, MAX_DB_STRING, nullptr);
   return (value != nullptr) ? _tcstol(buffer, nullptr, 0) : defaultValue;
}

/**
 * Read registry value as a INT32 using provided attribute.
 *
 * @return attribute value or default value in no value found
 */
int64_t LIBNXAGENT_EXPORTABLE ReadRegistryAsInt64(const TCHAR *attr, int64_t defaultValue)
{
   TCHAR buffer[MAX_DB_STRING];
   TCHAR *value = ReadRegistryAsString(attr, buffer, MAX_DB_STRING, nullptr);
   return (value != nullptr) ? _tcstoll(buffer, nullptr, 0) : defaultValue;
}

/**
 * Write registry value as a string. This method inserts new value or update existing.
 *
 * @return if this update/insert was successful
 */
bool LIBNXAGENT_EXPORTABLE WriteRegistry(const TCHAR *attr, const TCHAR *value)
{
   if (_tcslen(attr) > 63)
      return false;

   DB_HANDLE hdb = AgentGetLocalDatabaseHandle();
   if (hdb == nullptr)
      return false;

   String pattr = DBPrepareString(hdb, attr);
   
   // Check for variable existence
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("SELECT value FROM registry WHERE attribute=%s"), pattr.cstr());
   DB_RESULT hResult = DBSelect(hdb, query);
   bool varExist = false;
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         varExist = true;
      DBFreeResult(hResult);
   }

   // Create or update variable value
   String pvalue = DBPrepareString(hdb, value);
   if (varExist)
   {
      _sntprintf(query, 1024, _T("UPDATE registry SET value=%s WHERE attribute=%s"), pvalue.cstr(), pattr.cstr());
   }
   else
   {
      _sntprintf(query, 1024, _T("INSERT INTO registry (attribute,value) VALUES (%s,%s)"), pattr.cstr(), pvalue.cstr());
   }
   return DBQuery(hdb, query);
}

/**
 * Write registry value as a INT32. This method inserts new value or update existing.
 *
 * @return if this update/insert was successful
 */
bool LIBNXAGENT_EXPORTABLE WriteRegistry(const TCHAR *attr, int32_t value)
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
bool LIBNXAGENT_EXPORTABLE WriteRegistry(const TCHAR *attr, int64_t value)
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
   if (attr == nullptr)
      return false;

   DB_HANDLE hdb = AgentGetLocalDatabaseHandle();
   if (hdb == nullptr)
      return false;

   TCHAR query[256];
   _sntprintf(query, 256, _T("DELETE FROM registry WHERE attribute=%s"), DBPrepareString(hdb, attr).cstr());
   return DBQuery(hdb, query);
}
