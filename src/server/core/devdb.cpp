/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Raden Solutions
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
** File: devdb.cpp
**/

#include "nxcore.h"

/**
 * Lookup device port layout in database
 */
bool LookupDevicePortLayout(const SNMP_ObjectId& objectId, NDD_MODULE_LAYOUT *layout)
{
   bool success = false;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT numbering_scheme,row_count,layout_data FROM port_layouts WHERE device_oid=?"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, objectId.toString(), DB_BIND_TRANSIENT);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            layout->numberingScheme = DBGetFieldLong(hResult, 0, 0);
            layout->rows = DBGetFieldLong(hResult, 0, 1);
            if (layout->numberingScheme == NDD_PN_CUSTOM)
            {
               // TODO: custom layout parsing
            }
            nxlog_debug(5, _T("Successful port layout lookup for device %s: scheme=%d rows=%d"),
                     (const TCHAR *)objectId.toString(), layout->numberingScheme, layout->rows);
            success = true;
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return success;
}
