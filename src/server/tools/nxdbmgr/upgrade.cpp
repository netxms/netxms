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
// Create configuration parameter if it doesn't exist
//

static BOOL CreateConfigParam(TCHAR *pszName, TCHAR *pszValue, int iVisible, int iNeedRestart)
{
   TCHAR szQuery[1024];
   DB_RESULT hResult;
   BOOL bVarExist = FALSE, bResult = TRUE;

   // Check for variable existence
   _stprintf(szQuery, _T("SELECT var_value FROM config WHERE var_name='%s'"), pszName);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bVarExist = TRUE;
      DBFreeResult(hResult);
   }

   if (!bVarExist)
   {
      _stprintf(szQuery, _T("INSERT INTO config (var_name,var_value,is_visible,"
                            "need_server_restart) VALUES ('%s','%s',%d,%d)"), 
                pszName, pszValue, iVisible, iNeedRestart);
      bResult = SQLQuery(szQuery);
   }
   return bResult;
}


//
// Upgrade from V18 to V19
//

static BOOL H_UpgradeFromV18(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE nodes ADD platform_name varchar(63)\n"
      "UPADTE nodes SET platform_name=''\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='19' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V17 to V18
//

static BOOL H_UpgradeFromV17(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE nodes DROP COLUMN inherit_access_rights\n"
      "ALTER TABLE nodes ADD agent_version varchar(63)\n"
      "UPDATE nodes SET agent_version='' WHERE is_agent=0\n"
      "UPDATE nodes SET agent_version='<unknown>' WHERE is_agent=1\n"
      "INSERT INTO event_cfg (event_code,event_name,severity,flags,"
         "message,description) SELECT event_id,name,severity,flags,"
         "message,description FROM events\n"
      "DROP TABLE events\n"
      "DROP TABLE event_group_members\n"
      "CREATE TABLE event_group_members (group_id integer not null,"
	      "event_code integer not null,	PRIMARY KEY(group_id,event_code))\n"
      "ALTER TABLE alarms ADD source_event_code integer\n"
      "UPDATE alarms SET source_event_code=source_event_id\n"
      "ALTER TABLE alarms DROP COLUMN source_event_id\n"
      "ALTER TABLE alarms ADD source_event_id bigint\n"
      "UPDATE alarms SET source_event_id=0\n"
      "ALTER TABLE snmp_trap_cfg ADD event_code integer not null default 0\n"
      "UPDATE snmp_trap_cfg SET event_code=event_id\n"
      "ALTER TABLE snmp_trap_cfg DROP COLUMN event_id\n"
      "DROP TABLE event_log\n"
      "CREATE TABLE event_log (event_id bigint not null,event_code integer,"
	      "event_timestamp integer,event_source integer,event_severity integer,"
	      "event_message varchar(255),root_event_id bigint default 0,"
	      "PRIMARY KEY(event_id))\n"
      "<END>";
   TCHAR szQuery[4096];
   DB_RESULT hResult;

   hResult = SQLSelect(_T("SELECT rule_id,event_id FROM policy_event_list"));
   if (hResult != NULL)
   {
      DWORD i, dwNumRows;

      if (!SQLQuery(_T("DROP TABLE policy_event_list")))
      {
         if (!g_bIgnoreErrors)
         {
            DBFreeResult(hResult);
            return FALSE;
         }
      }

      if (!SQLQuery(_T("CREATE TABLE policy_event_list ("
                       "rule_id integer not null,"
                       "event_code integer not null,"
                       "PRIMARY KEY(rule_id,event_code))")))
      {
         if (!g_bIgnoreErrors)
         {
            DBFreeResult(hResult);
            return FALSE;
         }
      }

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         _sntprintf(szQuery, 4096, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%ld,%ld)"),
                    DBGetFieldULong(hResult, i, 0), DBGetFieldULong(hResult, i, 1));
         if (!SQLQuery(szQuery))
            if (!g_bIgnoreErrors)
            {
               DBFreeResult(hResult);
               return FALSE;
            }
      }

      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return FALSE;
   }

   _sntprintf(szQuery, 4096, 
      _T("CREATE TABLE event_cfg (event_code integer not null,"
	      "event_name varchar(63) not null,severity integer,flags integer,"
	      "message varchar(255),description %s,PRIMARY KEY(event_code))"),
              g_pszSqlType[g_iSyntax][SQL_TYPE_TEXT]);
   if (!SQLQuery(szQuery))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   _sntprintf(szQuery, 4096,
      _T("CREATE TABLE modules (module_id integer not null,"
	      "module_name varchar(63),exec_name varchar(255),"
	      "module_flags integer not null default 0,description %s,"
	      "license_key varchar(255),PRIMARY KEY(module_id))"),
              g_pszSqlType[g_iSyntax][SQL_TYPE_TEXT]);
   if (!SQLQuery(szQuery))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='18' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V16 to V17
//

static BOOL H_UpgradeFromV16(void)
{
   static TCHAR m_szBatch[] =
      "DROP TABLE locks\n"
      "CREATE TABLE snmp_trap_cfg (trap_id integer not null,snmp_oid varchar(255) not null,"
	      "event_id integer not null,description varchar(255),PRIMARY KEY(trap_id))\n"
      "CREATE TABLE snmp_trap_pmap (trap_id integer not null,parameter integer not null,"
         "snmp_oid varchar(255),description varchar(255),PRIMARY KEY(trap_id,parameter))\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(500, 'SNMP_UNMATCHED_TRAP',	0, 1,	'SNMP trap received: %1 (Parameters: %2)',"
		   "'Generated when system receives an SNMP trap without match in trap "
         "configuration table#0D#0AParameters:#0D#0A   1) SNMP trap OID#0D#0A"
		   "   2) Trap parameters')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(501, 'SNMP_COLD_START', 0, 1, 'System was cold-started',"
		   "'Generated when system receives a coldStart SNMP trap#0D#0AParameters:#0D#0A"
		   "   1) SNMP trap OID')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(502, 'SNMP_WARM_START', 0, 1, 'System was warm-started',"
		   "'Generated when system receives a warmStart SNMP trap#0D#0A"
		   "Parameters:#0D#0A   1) SNMP trap OID')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(503, 'SNMP_LINK_DOWN', 3, 1, 'Link is down',"
		   "'Generated when system receives a linkDown SNMP trap#0D#0A"
		   "Parameters:#0D#0A   1) SNMP trap OID#0D#0A   2) Interface index')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(504, 'SNMP_LINK_UP', 0, 1, 'Link is up',"
		   "'Generated when system receives a linkUp SNMP trap#0D#0AParameters:#0D#0A"
		   "   1) SNMP trap OID#0D#0A   2) Interface index')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(505, 'SNMP_AUTH_FAILURE', 1, 1, 'SNMP authentication failure',"
		   "'Generated when system receives an authenticationFailure SNMP trap#0D#0A"
		   "Parameters:#0D#0A   1) SNMP trap OID')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(506, 'SNMP_EGP_NEIGHBOR_LOSS',	1, 1,	'EGP neighbor loss',"
		   "'Generated when system receives an egpNeighborLoss SNMP trap#0D#0A"
		   "Parameters:#0D#0A   1) SNMP trap OID')\n"
      "INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) "
         "VALUES (1,'.1.3.6.1.6.3.1.1.5.1',501,'Generic coldStart trap')\n"
      "INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) "
         "VALUES (2,'.1.3.6.1.6.3.1.1.5.2',502,'Generic warmStart trap')\n"
      "INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) "
         "VALUES (3,'.1.3.6.1.6.3.1.1.5.3',503,'Generic linkDown trap')\n"
      "INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) "
         "VALUES (4,'.1.3.6.1.6.3.1.1.5.4',504,'Generic linkUp trap')\n"
      "INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) "
         "VALUES (5,'.1.3.6.1.6.3.1.1.5.5',505,'Generic authenticationFailure trap')\n"
      "INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) "
         "VALUES (6,'.1.3.6.1.6.3.1.1.5.6',506,'Generic egpNeighborLoss trap')\n"
      "INSERT INTO snmp_trap_pmap (trap_id,parameter,snmp_oid,description) "
         "VALUES (3,1,'.1.3.6.1.2.1.2.2.1.1','Interface index')\n"
      "INSERT INTO snmp_trap_pmap (trap_id,parameter,snmp_oid,description) "
         "VALUES (4,1,'.1.3.6.1.2.1.2.2.1.1','Interface index')\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("DBLockStatus"), _T("UNLOCKED"), 0, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("DBLockInfo"), _T(""), 0, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("EnableSNMPTraps"), _T("1"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("SMSDriver"), _T("<none>"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("SMTPServer"), _T("localhost"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("SMTPFromAddr"), _T("netxms@localhost"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='17' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V15 to V16
//

static BOOL H_UpgradeFromV15(void)
{
   static TCHAR m_szBatch[] =
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(4005, 'DC_MAILBOX_TOO_LARGE', 1, 1,"
		   "'Mailbox #22%6#22 exceeds size limit (allowed size: %3; actual size: %4)',"
		   "'Custom data collection threshold event.#0D#0AParameters:#0D#0A"
		   "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(4006, 'DC_AGENT_VERSION_CHANGE',	0, 1,"
         "'NetXMS agent version was changed from %3 to %4',"
         "'Custom data collection threshold event.#0D#0AParameters:#0D#0A"
		   "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(4007, 'DC_HOSTNAME_CHANGE',	1, 1,"
         "'Host name was changed from %3 to %4',"
         "'Custom data collection threshold event.#0D#0AParameters:#0D#0A"
		   "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(4008, 'DC_FILE_CHANGE',	1, 1,"
         "'File #22%6#22 was changed',"
         "'Custom data collection threshold event.#0D#0AParameters:#0D#0A"
		   "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance')\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;
   if (!SQLQuery(_T("UPDATE config SET var_value='16' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;
   return TRUE;
}


//
// Upgrade from V14 to V15
//

static BOOL H_UpgradeFromV14(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE items ADD instance varchar(255)\n"
      "UPDATE items SET instance=''\n"
      "INSERT INTO config (var_name,var_value,is_visible,need_server_restart) VALUES "
         "('SMTPServer','localhost',1,0)\n"
      "INSERT INTO config (var_name,var_value,is_visible,need_server_restart) VALUES "
         "('SMTPFromAddr','netxms@localhost',1,0)\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(4003, 'DC_AGENT_RESTARTED',	0, 1,"
         "'NetXMS agent was restarted within last 5 minutes',"
         "'Custom data collection threshold event.#0D#0AParameters:#0D#0A"
		   "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(4004, 'DC_SERVICE_NOT_RUNNING', 3, 1,"
		   "'Service #22%6#22 is not running',"
		   "'Custom data collection threshold event.#0D#0AParameters:#0D#0A"
		   "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance')\n"
      "UPDATE events SET "
         "description='Generated when threshold value reached "
         "for specific data collection item.#0D#0A"
         "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance' WHERE "
         "event_id=17 OR (event_id>=4000 AND event_id<5000)\n"
      "UPDATE events SET "
         "description='Generated when threshold check is rearmed "
         "for specific data collection item.#0D#0A"
		   "Parameters:#0D#0A   1) Parameter name#0D#0A"
		   "   2) Item description#0D#0A   3) Data collection item ID' "
         "WHERE event_id=18\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;
   if (!SQLQuery(_T("UPDATE config SET var_value='15' WHERE var_name='DBFormatVersion'")))
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
   { 15, H_UpgradeFromV15 },
   { 16, H_UpgradeFromV16 },
   { 17, H_UpgradeFromV17 },
   { 0, NULL }
};


//
// Upgrade database to new version
//

void UpgradeDatabase(void)
{
   DB_RESULT hResult;
   long i, iVersion = 0;
   BOOL bLocked = FALSE;

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
      // Check if database is locked
      hResult = DBSelect(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name='DBLockStatus'"));
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
            bLocked = _tcscmp(DBGetField(hResult, 0, 0), _T("UNLOCKED"));
         DBFreeResult(hResult);
      }
      if (!bLocked)
      {
         // Upgrade database
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
      else
      {
         _tprintf(_T("Database is locked\n"));
      }
   }
}
