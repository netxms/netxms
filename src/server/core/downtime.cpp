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
** File: downtime.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("downtime")

/**
 * Mark start of downtime for given object
 */
void StartDowntime(uint32_t objectId, String tag)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (DBBegin(hdb))
   {
      bool success;
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT start_time FROM active_downtimes WHERE object_id=? AND downtime_tag=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, tag, DB_BIND_STATIC);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            if (DBGetNumRows(hResult) != 0)
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Downtime \"%s\" for object [%u] already registered at %u"), tag.cstr(), objectId, DBGetFieldULong(hResult, 0, 0));
               success = false;
            }
            else
            {
               success = true;
            }
            DBFreeResult(hResult);
         }
         else
         {
            success = false;
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }

      time_t startTime = time(nullptr);
      if (success)
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO active_downtimes (object_id,downtime_tag,start_time) VALUES (?,?,?)"));
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, tag, DB_BIND_STATIC);
            DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(startTime));
            success = DBExecute(hStmt);
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }

      if (success)
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO downtime_log (object_id,start_time,end_time,downtime_tag) VALUES (?,?,0,?)"));
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(startTime));
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, tag, DB_BIND_STATIC);
            success = DBExecute(hStmt);
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }

      if (success)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Downtime \"%s\" for object [%u] is now registered"), tag.cstr(), objectId);
         DBCommit(hdb);
      }
      else
      {
         DBRollback(hdb);
      }
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Mark end of downtime for given object
 */
void EndDowntime(uint32_t objectId, String tag)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (DBBegin(hdb))
   {
      bool success;
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT start_time FROM active_downtimes WHERE object_id=? AND downtime_tag=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, tag, DB_BIND_STATIC);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            if (DBGetNumRows(hResult) != 0)
            {
               success = true;
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot end downtime \"%s\" for object [%u] because it is not registered"), tag.cstr(), objectId);
               success = false;
            }
            DBFreeResult(hResult);
         }
         else
         {
            success = false;
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }

      if (success)
      {
         hStmt = DBPrepare(hdb, _T("DELETE FROM active_downtimes WHERE object_id=? AND downtime_tag=?"));
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, tag, DB_BIND_STATIC);
            success = DBExecute(hStmt);
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }

      if (success)
      {
         hStmt = DBPrepare(hdb, _T("UPDATE downtime_log SET end_time=? WHERE object_id=? AND downtime_tag=? AND end_time=0"));
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(time(nullptr)));
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, objectId);
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, tag, DB_BIND_STATIC);
            success = DBExecute(hStmt);
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }

      if (success)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Downtime \"%s\" for object [%u] closed"), tag.cstr(), objectId);
         DBCommit(hdb);
      }
      else
      {
         DBRollback(hdb);
      }
   }
   DBConnectionPoolReleaseConnection(hdb);
}
