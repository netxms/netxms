/* $Id$ */
/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004, 2005, 2006 Victor Kirhenshtein
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
** File: reindex.cpp
**
**/

#include "nxdbmgr.h"


//
// Recreate index
//

static void RecreateIndex(const TCHAR *pszIndex, const TCHAR *pszTable, const TCHAR *pszColumns)
{
	TCHAR szQuery[1024];

	_tprintf(_T("Reindexing table %s by (%s)...\n"), pszTable, pszColumns);
	_stprintf(szQuery, _T("DROP INDEX %s"), pszIndex);
	DBQuery(g_hCoreDB, szQuery);
	_stprintf(szQuery, _T("CREATE INDEX %s ON %s(%s)"), pszIndex, pszTable, pszColumns);
	SQLQuery(szQuery);
}


//
// Reindex database
//

void ReindexDatabase(void)
{
	DB_RESULT hResult;
	LONG nVersion = 0;
	BOOL bLocked;
   TCHAR szLockStatus[MAX_DB_STRING], szLockInfo[MAX_DB_STRING];

   // Get database format version
   hResult = DBSelect(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name='DBFormatVersion'"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         nVersion = DBGetFieldLong(hResult, 0, 0);
      DBFreeResult(hResult);
   }
   if (nVersion < DB_FORMAT_VERSION)
   {
      _tprintf(_T("Your database has format version %d, this tool is compiled for version %d.\nUse \"upgrade\" command to upgrade your database first.\n"),
               nVersion, DB_FORMAT_VERSION);
		return;
   }
   else if (nVersion > DB_FORMAT_VERSION)
   {
		_tprintf(_T("Your database has format version %d, this tool is compiled for version %d.\n"
					 "You need to upgrade your server before using this database.\n"),
				   nVersion, DB_FORMAT_VERSION);
		return;
   }

   // Check if database is locked
   hResult = DBSelect(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name='DBLockStatus'"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         DBGetField(hResult, 0, 0, szLockStatus, MAX_DB_STRING);
         DecodeSQLString(szLockStatus);
         bLocked = _tcscmp(szLockStatus, _T("UNLOCKED"));
      }
      DBFreeResult(hResult);

      if (bLocked)
      {
         hResult = DBSelect(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name='DBLockInfo'"));
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               DBGetField(hResult, 0, 0, szLockInfo, MAX_DB_STRING);
               DecodeSQLString(szLockInfo);
            }
            DBFreeResult(hResult);
         }
      }
   }

   if (bLocked)
   {
      _tprintf(_T("Database is locked by server %s [%s]\n"), szLockStatus, szLockInfo);
		return;
   }

	if (g_iSyntax != DB_SYNTAX_ORACLE)
		RecreateIndex(_T("idx_raw_dci_values_item_id"), _T("raw_dci_values"), _T("item_id"));
	RecreateIndex(_T("idx_event_log_event_timestamp"), _T("event_log"), _T("event_timestamp"));
	RecreateIndex(_T("idx_thresholds_item_id"), _T("thresholds"), _T("item_id"));
	RecreateIndex(_T("idx_thresholds_sequence"), _T("thresholds"), _T("sequence_number"));
	RecreateIndex(_T("idx_alarm_change_log_alarm_id"), _T("alarm_change_log"), _T("alarm_id"));
	RecreateIndex(_T("idx_alarm_notes_alarm_id"), _T("alarm_notes"), _T("alarm_id"));
	RecreateIndex(_T("idx_syslog_msg_timestamp"), _T("syslog"), _T("msg_timestamp"));
	RecreateIndex(_T("idx_snmp_trap_log_trap_timestamp"), _T("snmp_trap_log"), _T("trap_timestamp"));
	RecreateIndex(_T("idx_address_lists_list_type"), _T("address_lists"), _T("list_type"));

	_tprintf(_T("Database reindexing complete.\n"));
}
