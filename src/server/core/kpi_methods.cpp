/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: kpi_methods.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG L"kpi"

/**
 * Lock for method registration
 */
static Mutex s_registrationLock;

/**
 * Find registered computation method by method ID and version. Returns 0 if method is not registered.
 */
static uint32_t FindComputationMethod(DB_HANDLE hdb, const MethodDescriptor& method)
{
   uint32_t id = 0;
   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT id FROM kpi_methods WHERE method_id=? AND version=?");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, method.methodId, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, method.version, DB_BIND_STATIC);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
            id = DBGetFieldULong(hResult, 0, 0);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   return id;
}

/**
 * Register computation method. Method is identified by method ID and version; record in method registry is created
 * if it does not exist yet. Registry records are never deleted, so ID stored with sample attributes can always be resolved.
 *
 * @return ID of method in registry or 0 on failure
 */
uint32_t NXCORE_EXPORTABLE RegisterComputationMethod(const MethodDescriptor& method)
{
   LockGuard lockGuard(s_registrationLock);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   uint32_t id = FindComputationMethod(hdb, method);
   if (id == 0)
   {
      DB_RESULT hResult = DBSelect(hdb, L"SELECT max(id) FROM kpi_methods");
      if (hResult != nullptr)
      {
         id = DBGetFieldULong(hResult, 0, 0) + 1;
         DBFreeResult(hResult);

         DB_STATEMENT hStmt = DBPrepare(hdb, L"INSERT INTO kpi_methods (id,method_id,version,engine,display_name,tier,coverage_level,registered_at) VALUES (?,?,?,?,?,?,?,?)");
         if (hStmt != nullptr)
         {
            wchar_t tier[2] = { static_cast<wchar_t>(method.tier), 0 };
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, method.methodId, DB_BIND_STATIC, 63);
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, method.version, DB_BIND_STATIC, 31);
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, method.engine, DB_BIND_STATIC, 63);
            DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, method.displayName, DB_BIND_STATIC, 255);
            DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, tier, DB_BIND_STATIC);
            DBBind(hStmt, 7, DB_SQLTYPE_DOUBLE, method.coverageLevel);
            DBBind(hStmt, 8, DB_SQLTYPE_BIGINT, static_cast<int64_t>(time(nullptr)));
            if (!DBExecute(hStmt))
               id = 0;
            DBFreeStatement(hStmt);
         }
         else
         {
            id = 0;
         }
      }

      if (id != 0)
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Computation method %s version %s registered with ID %u", method.methodId, method.version, id);
      else
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Cannot register computation method %s version %s", method.methodId, method.version);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return id;
}
