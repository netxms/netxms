/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: upgrade.cpp
**
**/

#include "nxdbmgr.h"


//
// Execute SQL batch
//

static BOOL SQLBatch(TCHAR *pszBatch)
{
   TCHAR *pszQuery, *ptr;

   pszQuery = pszBatch;
   while(1)
   {
      ptr = _tcschr(pszQuery, _T('\n'));
      if (ptr != NULL)
         *ptr = 0;
      if (!_tcscmp(pszQuery, _T("<END>")))
         break;
      if (!DBQuery(g_hCoreDB, pszQuery))
      {
#ifdef _WIN32
         _tprintf(_T("SQL query failed:\n"));
         SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0E);
         _tprintf(_T("%s\n"), pszQuery);
         SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);
#else
         _tprintf(_T("SQL query failed:\n%s\n"), pszQuery);
#endif
         if (!g_bIgnoreErrors)
            return FALSE;
      }
      ptr++;
      pszQuery = ptr;
   }
   return TRUE;
}


//
// Upgrade from V14 to V15
//

static BOOL H_UpgradeFromV14(void)
{
   static TCHAR m_szBatch[] = 
      "ALTER TABLE items ADD COLUMN instance varchar(255)\n"
      "INSERT INTO config (var_name,var_value,is_visible,need_server_restart) VALUES "
         "('SMTPServer','localhost',1,0)\n"
      "INSERT INTO config (var_name,var_value,is_visible,need_server_restart) VALUES "
         "('SMTPFromAddr','netxms@localhost',1,0)\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(4003, 'DC_AGENT_RESTARTED',	0 , 1,"
         "'NetXMS agent was restarted within last 5 minutes',"
         "'Custom data collection threshold event.#0D#0AParameters:#0D#0A"
		   "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance')\n"
      "UPDATE events SET "
         "description='   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance' WHERE "
         "event_id=17 OR (event_id>=4000 AND event_id<5000)\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;
   if (!SQLQuery(_T("UPDATE CONFIG SET var_value='15' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;
   return TRUE;
}


//
// Upgrade map
//

static struct
{
   int iVersion;
   BOOL (* fpProc)(void);
} m_dbUpgradeMap[] =
{
   { 14, H_UpgradeFromV14 },
   { 0, NULL }
};


//
// Upgrade database to new version
//

void UpgradeDatabase(void)
{
   DB_RESULT hResult;
   long i, iVersion = 0;

   _tprintf(_T("Upgrading database...\n"));

   // Get database format version
   hResult = DBSelect(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name='DBFormatVersion'"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         iVersion = DBGetFieldLong(hResult, 0, 0);
      DBFreeResult(hResult);
   }
   if (iVersion == DB_FORMAT_VERSION)
   {
      _tprintf(_T("Your database format is up to date\n"));
   }
   else if (iVersion > DB_FORMAT_VERSION)
   {
       _tprintf(_T("Your database has format version %d, this tool is compiled for version %d.\n"
                   "You need to upgrade your server before using this database.\n"),
                iVersion, DB_FORMAT_VERSION);
   }
   else
   {
      while(iVersion < DB_FORMAT_VERSION)
      {
         // Find upgrade procedure
         for(i = 0; m_dbUpgradeMap[i].fpProc != NULL; i++)
            if (m_dbUpgradeMap[i].iVersion == iVersion)
               break;
         if (m_dbUpgradeMap[i].fpProc == NULL)
         {
            _tprintf(_T("Unable to find upgrade procedure for version %d\n"), iVersion);
            break;
         }
         printf("Upgrading from version %d to %d\n", iVersion, iVersion + 1);
         if (m_dbUpgradeMap[i].fpProc())
            iVersion++;
         else
            break;
      }

      _tprintf(_T("Database upgrade %s\n"), (iVersion == DB_FORMAT_VERSION) ? _T("succeeded") : _T("failed"));
   }
}
