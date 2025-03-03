/*
** NetXMS database manager library
** Copyright (C) 2004-2025 Raden Solutions
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
** File: xml_to_json.cpp
**
**/

#include "libnxdbmgr.h"
#include <xml_to_json.h>


/**
 * Convert network map XML configuration to JSON
 */
bool LIBNXDBMGR_EXPORTABLE ConvertXmlToJson(const TCHAR *table, const TCHAR *idColumn1, const TCHAR *idColumn2, const TCHAR *dataColumn)
{
   TCHAR reqest[512];
   _sntprintf(reqest, 512, _T("SELECT %s,%s%s%s FROM %s"), dataColumn, idColumn1, (idColumn1 != nullptr) ? L"," : L"", CHECK_NULL(idColumn2), table);

   TCHAR update[512];
   _sntprintf(update, 512, _T("UPDATE %s SET %s=? WHERE %s=?%s%s%s"), table, dataColumn, idColumn1, idColumn1 != nullptr ? L" AND " : L"", CHECK_NULL(idColumn2), idColumn1 != nullptr ? L"=?" : L"");

   DB_RESULT hResult = SQLSelect(reqest);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      DB_STATEMENT hStmt = DBPrepare(g_dbHandle, update, count > 1);
      if (hStmt != nullptr)
      {
         for (int i = 0; i < count; i++)
         {
            char *xmlText = DBGetFieldUTF8(hResult, i, 0, nullptr, 0);
            pugi::xml_document xml;
            if (!xml.load_buffer(xmlText, strlen(xmlText)))
            {
               MemFree(xmlText);
               continue; //ignore filed
            }

            json_t *result = XmlNodeToJson(xml.first_child());
            MemFree(xmlText);

            DBBind(hStmt, 1, DB_SQLTYPE_TEXT, result, DB_BIND_DYNAMIC);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 1));
            DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 2));
            if (!SQLExecute(hStmt) && !g_ignoreErrors)
            {
               DBFreeResult(hResult);
               DBFreeStatement(hStmt);
               return false;
            }
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         DBFreeResult(hResult);
         return false;
      }

      DBFreeResult(hResult);
   }
   else
   {
      return false;
   }
   return true;
}
