/*
** nxdbmgr - NetXMS database manager
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
** File: upgrade_v30.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 30.102 to 31.0
 */
static bool H_UpgradeFromV102()
{
   CHK_EXEC(SetMajorSchemaVersion(31, 0));
   return true;
}

/**
 * Generate GSM modem notification channel configuration from SMSDrvConfig
 *
 * pszInitArgs format: portname,speed,databits,parity,stopbits,mode,blocksize,writedelay
 */
static StringBuffer CreateNCDrvConfig_GSM(TCHAR *portName)
{
   StringBuffer config;
   TCHAR *p;

   if ((p = _tcschr(portName, _T(','))) != NULL)
   {
      *p = 0; p++;
      config.appendFormattedString(_T("PortName=%s\n"), portName);
      int tmp = _tcstol(p, NULL, 10);
      if (tmp != 0)
      {
         config.appendFormattedString(_T("Speed=%d\n"), tmp);

         if ((p = _tcschr(p, _T(','))) != NULL)
         {
            *p = 0; p++;
            tmp = _tcstol(p, NULL, 10);
            if (tmp >= 5 && tmp <= 8)
            {
               config.appendFormattedString(_T("DataBits=%d\n"), tmp);

               // parity
               if ((p = _tcschr(p, _T(','))) != NULL)
               {
                  *p = 0; p++;
                  config.append(_T("Parity="));
                  config.append(p, 1);

                  // stop bits
                  if ((p = _tcschr(p, _T(','))) != NULL)
                  {
                     *p = 0; p++;
                     config.append(_T("\nStopBits="));
                     config.append(p, 1);

                     // Text or PDU mode
                     if ((p = _tcschr(p, _T(','))) != NULL)
                     {
                        *p = 0; p++;
                        config.append(_T("\nTextMode="));
                        if (*p == _T('T'))
                        {
                           config.append(_T("yes"));
                        }
                        else if (*p == _T('P'))
                        {
                           config.append(_T("no"));
                        }

                        // Use quotes
                        if ((p = _tcschr(p, _T(','))) != NULL)
                        {
                           *p = 0; p++;
                           config.append(_T("\nUseQuotes="));
                           if (*p == _T('N'))
                           {
                              config.append(_T("no"));
                           }
                           else
                           {
                              config.append(_T("yes"));
                           }

                           // block size
                           if ((p = _tcschr(p, _T(','))) != NULL)
                           {
                              *p = 0; p++;
                              int blockSize = _tcstol(p, NULL, 10);
                              if ((blockSize < 1) || (blockSize > 256))
                                 blockSize = 8;
                              config.appendFormattedString(_T("\nBlockSize=%d\n"), blockSize);

                              // write delay
                              if ((p = _tcschr(p, _T(','))) != NULL)
                              {
                                 *p = 0; p++;
                                 int writeDelay = _tcstol(p, NULL, 10);
                                 if ((writeDelay < 1) || (writeDelay > 10000))
                                    writeDelay = 100;
                                 config.appendFormattedString(_T("\nWriteDelay=%d"), writeDelay);
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }
   else
   {
      config.appendFormattedString(_T("PortName=%s\n"), portName);
   }
   return config;
}

/**
 * Generate remote NXAgent notification channel configuration from SMSDrvConfig
 */
static StringBuffer CreateNCDrvConfig_NXAgent(TCHAR *temp)
{
   StringBuffer config;
   TCHAR *ptr, *eptr;
   int field;
   const TCHAR *array[] = { _T("hostname"), _T("port"), _T("timeout"), _T("secret")};
   for(ptr = eptr = temp, field = 0; eptr != NULL; field++, ptr = eptr + 1)
   {
      eptr = _tcschr(ptr, ',');
      if (eptr != NULL)
         *eptr = 0;
      config.appendFormattedString(_T("%s=%s\n"), array[field], ptr);
   }
   return config;
}

/**
 * Generate default notification channel configuration from SMSDrvConfig
 */
static TCHAR *CreateNCDrvConfig_Default(TCHAR *oldConfiguration)
{
   TCHAR *tmp = _tcschr(oldConfiguration, _T(';'));
   while(tmp != NULL)
   {
      tmp[0] = _T('\n');
      tmp = _tcschr(tmp+1, _T(';'));
   }
   return oldConfiguration;
}

static void PrepareDriverNameAndConfig(TCHAR *driver, TCHAR *oldConfiguration, StringBuffer &newDriverName, StringBuffer &newConfiguration)
{
   //prepare driver name
   TCHAR *driverName = NULL;
   _tcslwr(driver);
   TCHAR *tmp = _tcsrchr(driver, FS_PATH_SEPARATOR_CHAR);
   if (tmp != NULL)
      driverName = tmp + 1;
   else
      driverName = driver;
   tmp = _tcsrchr(driverName, _T('.'));
   if (tmp != NULL)
   {
      tmp[0] = 0;
      if (!_tcscmp(tmp + 1, _T("so")))
      {
         tmp = _tcsrchr(driverName, _T('_'));
         if (tmp != NULL)
         {
            driverName = tmp + 1;
         }
      }
   }

   if (!_tcscmp(driverName, _T("anysms")))
   {
      newConfiguration = CreateNCDrvConfig_Default(oldConfiguration);
      newDriverName = _T("AnySMS");
   }
   else if (!_tcscmp(driverName, _T("dbemu")))// renamed to dbtable
   {
      newDriverName = _T("DBTable");
   }
   else if (!_tcscmp(driverName, _T("dummy")))
   {
      newDriverName = _T("Dummy");
   }
   else if (!_tcscmp(driverName, _T("generic")))
   {
      newConfiguration = CreateNCDrvConfig_GSM(oldConfiguration);
      newDriverName = _T("GSM");
   }
   else if(!_tcscmp(driverName, _T("kannel")))
   {
      newConfiguration = CreateNCDrvConfig_Default(oldConfiguration);
      newDriverName = _T("Kannel");
   }
   else if(!_tcscmp(driverName, _T("mymobile")))
   {
      newConfiguration = CreateNCDrvConfig_Default(oldConfiguration);
      newDriverName = _T("MyMobile");
   }
   else if(!_tcscmp(driverName, _T("nexmo")))
   {
      newConfiguration = CreateNCDrvConfig_Default(oldConfiguration);
      newDriverName = _T("Nexmo");
   }
   else if(!_tcscmp(driverName, _T("nxagent")))
   {
      newConfiguration = CreateNCDrvConfig_NXAgent(oldConfiguration);
      newDriverName = _T("NXAgent");
   }
   else if(!_tcscmp(driverName, _T("portech")))
   {
      newConfiguration = CreateNCDrvConfig_Default(oldConfiguration);
      newDriverName = _T("Portech");
   }
   else if(!_tcscmp(driverName, _T("slack")))
   {
      newConfiguration = CreateNCDrvConfig_Default(oldConfiguration);
      newDriverName = _T("Slack");
   }
   else if(!_tcscmp(driverName, _T("smseagle")))
   {
      newConfiguration = CreateNCDrvConfig_Default(oldConfiguration);
      newDriverName = _T("SMSEagle");
   }
   else if(!_tcscmp(driverName, _T("text2reach")))
   {
      newConfiguration = CreateNCDrvConfig_Default(oldConfiguration);
      newDriverName = _T("Text2Reach");
   }
   else if(!_tcscmp(driverName, _T("websms")))
   {
      newConfiguration = CreateNCDrvConfig_Default(oldConfiguration);
      newDriverName = _T("WebSMS");
   }
   else
   {
      newDriverName = driverName;
      newConfiguration = oldConfiguration;
   }
}

/**
 * Upgrade from 30.101 to 30.102
 */
static bool H_UpgradeFromV101()
{
   CHK_EXEC(SQLQuery(_T("UPDATE notification_channels SET driver_name='GSM' WHERE driver_name='Generic'"))); //Rename for Generic griver

   // Fix driver name used in action instead of notification channel name
   switch (g_dbSyntax)
   {
      case DB_SYNTAX_DB2:
      case DB_SYNTAX_ORACLE:
      case DB_SYNTAX_SQLITE:
         CHK_EXEC(SQLQuery(_T("UPDATE actions SET channel_name=(SELECT name FROM notification_channels WHERE driver_name=actions.channel_name) WHERE channel_name IN (SELECT driver_name FROM notification_channels)")));
         break;
      case DB_SYNTAX_MSSQL:
         CHK_EXEC(SQLQuery(_T("UPDATE actions SET actions.channel_name=notification_channels.name FROM actions INNER JOIN notification_channels ON actions.channel_name=notification_channels.driver_name")));
         break;
      case DB_SYNTAX_PGSQL:
         CHK_EXEC(SQLQuery(_T("UPDATE actions SET channel_name=notification_channels.name FROM notification_channels WHERE actions.channel_name=notification_channels.driver_name")));
         break;
      case DB_SYNTAX_MYSQL:
         CHK_EXEC(SQLQuery(_T("UPDATE actions INNER JOIN notification_channels ON actions.channel_name=notification_channels.driver_name SET channel_name=notification_channels.name")));
         break;
      default:
         break;
   }

   TCHAR driver[64];
   TCHAR name[64];
   TCHAR *oldConfiguration = NULL;
   StringBuffer newConfiguration;
   StringBuffer newDriverName;
   bool fixNeeded = false;

   DB_RESULT hResult = SQLSelect(_T("SELECT driver_name,name FROM notification_channels"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for (int i = 0; i < count; i++)
      {
         DBGetField(hResult, i, 0, driver, 64);
         if(driver[0] == FS_PATH_SEPARATOR_CHAR || (driver[0] >= _T('a') && driver[0] <= _T('z')) || (driver[0] >= _T('0') && driver[0] <= _T('9')))
         {
            DBGetField(hResult, i, 1, name, 64);
            fixNeeded = true;
            break;
         }
      }
      DBFreeResult(hResult);
   }

   if (fixNeeded)
   {
      DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("SELECT configuration FROM notification_channels WHERE name=?"));
      if(hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
         hResult = DBSelectPrepared(hStmt);
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) > 0)
               oldConfiguration = DBGetField(hResult, 0, 0, NULL, 0);
            DBFreeResult(hResult);
         }
         else if (!g_ignoreErrors)
           return false;

         DBFreeStatement(hStmt);
      }
      else if (!g_ignoreErrors)
      {
        return false;
      }

      if (oldConfiguration == NULL)
         oldConfiguration = _tcsdup(_T(""));

      PrepareDriverNameAndConfig(driver, oldConfiguration, newDriverName, newConfiguration);

      hStmt = DBPrepare(g_dbHandle, _T("UPDATE notification_channels SET driver_name=?,configuration=? WHERE name=?"));
      if (hStmt != NULL)
      {
        DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, static_cast<const TCHAR *>(newDriverName), DB_BIND_STATIC);
        DBBind(hStmt, 2, DB_SQLTYPE_TEXT, static_cast<const TCHAR *>(newConfiguration), DB_BIND_STATIC);
        DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, static_cast<const TCHAR *>(name), DB_BIND_STATIC);
        if (!SQLExecute(hStmt) && !g_ignoreErrors)
        {
           DBFreeStatement(hStmt);
           return false;
        }
        DBFreeStatement(hStmt);
      }
      else if (!g_ignoreErrors)
        return false;
   }

   MemFree(oldConfiguration);

   CHK_EXEC(SetMinorSchemaVersion(102));
   return true;
}

/**
 * Upgrade from 30.100 to 30.101
 */
static bool H_UpgradeFromV100()
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='AllowDirectNotifications' WHERE var_name='AllowDirectNotification'")));
   CHK_EXEC(SetMinorSchemaVersion(101));
   return true;
}

/**
 * Upgrade from 30.99 to 30.100
 */
static bool H_UpgradeFromV99()
{
   static const TCHAR *batch =
            _T("UPDATE object_tools SET tool_data='Loaded subagentsAgent.SubAgents' WHERE tool_id=8\n")
            _T("UPDATE object_tools SET tool_data='Current processesSystem.Processes' WHERE tool_id=13\n")
            _T("UPDATE object_tools SET tool_data='Supported tablesAgent.SupportedTables^(.*)' WHERE tool_id=23\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(100));
   return true;
}

/**
 * Upgrade from 30.98 to 30.99
 */
static bool H_UpgradeFromV98()
{
   static const TCHAR *batch =
            _T("UPDATE object_tools SET tool_name='&Info->&Agent->&Loaded subagents',tool_type=9,tool_data='Loaded subagentsAgent.SubAgents',description='Show information about loaded subagents' WHERE tool_id=8\n")
            _T("DELETE FROM object_tools_table_columns WHERE tool_id=8\n")
            _T("UPDATE object_tools SET description='Show information about actions supported by agent' WHERE tool_id=11\n")
            _T("UPDATE object_tools SET tool_type=9,description='Show information about ICMP targets configured on this agent' WHERE tool_id=12\n")
            _T("DELETE FROM object_tools_table_columns WHERE tool_id=12\n")
            _T("UPDATE object_tools SET tool_name='&Info->&Current processes',tool_type=9,tool_data='Current processesSystem.Processes',description='Show information about currently running processes' WHERE tool_id=13\n")
            _T("DELETE FROM object_tools_table_columns WHERE tool_id=13\n")
            _T("UPDATE object_tools SET description='Show information about active user sessions' WHERE tool_id=16\n")
            _T("UPDATE object_tools SET tool_filter='<objectMenuFilter><flags>2</flags></objectMenuFilter>' WHERE tool_id=18\n")
            _T("INSERT INTO object_tools (tool_id,guid,tool_name,tool_type,tool_data,flags,tool_filter,description,confirmation_text) ")
               _T("VALUES (23,'281e3601-1cc6-4969-93f2-dfb86f9380b9','&Info->&Agent->Supported &tables',3,")
               _T("'Supported tablesAgent.SupportedTables^(.*)',0,'<objectMenuFilter><flags>2</flags></objectMenuFilter>',")
               _T("'Show list of tables supported by agent','')\n")
            _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr)")
               _T("VALUES (23,0,'Parameter','',0,1)\n")
            _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (23,-2147483648)\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(99));
   return true;
}

/**
 * Upgrade from 30.97 to 30.98
 */
static bool H_UpgradeFromV97()
{
   static const TCHAR *batch =
            _T("UPDATE config SET default_value='https://tile.netxms.org/osm/' WHERE var_name='TileServerURL'\n")
            _T("UPDATE config SET var_value='https://tile.netxms.org/osm/' WHERE var_name='TileServerURL' AND (var_value='http://tile.openstreetmap.org/' OR var_value='https://maps.wikimedia.org/osm-intl/')\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(98));
   return true;
}

/**
 * Upgrade from 30.96 to 30.97
 */
static bool H_UpgradeFromV96()
{
   static const TCHAR *batch =
            _T("ALTER TABLE event_log ADD origin integer\n")
            _T("ALTER TABLE event_log ADD origin_timestamp integer\n")
            _T("UPDATE event_log SET origin=0,origin_timestamp=event_timestamp\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("event_log"), _T("origin")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("event_log"), _T("origin_timestamp")));
   CHK_EXEC(SetMinorSchemaVersion(97));
   return true;
}

/**
 * Upgrade from 30.95 to 30.96
 */
static bool H_UpgradeFromV95()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE snmp_trap_cfg ADD transformation_script $SQL:TEXT")));
   CHK_EXEC(SetMinorSchemaVersion(96));
   return true;
}

/**
 * Upgrade from 30.94 to 30.95
 */
static bool H_UpgradeFromV94()
{
   static const TCHAR *batch =
            _T("ALTER TABLE interfaces ADD phy_chassis integer\n")
            _T("ALTER TABLE interfaces ADD phy_pic integer\n")
            _T("UPDATE interfaces SET phy_chassis=0,phy_pic=0\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("interfaces"), _T("phy_slot"), _T("phy_module")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("interfaces"), _T("phy_chassis")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("interfaces"), _T("phy_pic")));
   CHK_EXEC(SetMinorSchemaVersion(95));
   return true;
}

/**
 * Upgrade from 30.93 to 30.94
 */
static bool H_UpgradeFromV93()
{
   static const TCHAR *batch =
            _T("ALTER TABLE items ADD related_object integer\n")
            _T("UPDATE items SET related_object=0\n")
            _T("ALTER TABLE dc_tables ADD related_object integer\n")
            _T("UPDATE dc_tables SET related_object=0\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("items"), _T("related_object")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dc_tables"), _T("related_object")));
   CHK_EXEC(SetMinorSchemaVersion(94));
   return true;
}

/**
 * Upgrade from 30.92 to 30.93
 */
static bool H_UpgradeFromV92()
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET need_server_restart=0,var_name='NetworkDiscovery.ActiveDiscovery.Interval' WHERE var_name='ActiveDiscoveryInterval'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET need_server_restart=0,var_name='NetworkDiscovery.PassiveDiscovery.Interval' WHERE var_name='DiscoveryPollingInterval'")));

   CHK_EXEC(CreateConfigParam(_T("NetworkDiscovery.ActiveDiscovery.Schedule"), _T(""),
            _T("Schedule used to start active network discovery poll in cron format."),
            NULL, 'S', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(93));
   return true;
}

/**
 * Upgrade from 30.91 to 30.92
 */
static bool H_UpgradeFromV91()
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE notification_channels (")
      _T("   name varchar(63) not null,")
      _T("   driver_name varchar(63) not null,")
      _T("   description  varchar(255) null,")
      _T("   configuration $SQL:TEXT null,")
      _T("PRIMARY KEY(name))")));

   CHK_EXEC(SQLQuery(_T("ALTER TABLE actions ADD channel_name varchar(63)")));

   TCHAR *driver = NULL;
   TCHAR *oldConfiguration = NULL;
   StringBuffer newConfiguration;
   StringBuffer newDriverName;

   DB_RESULT hResult = SQLSelect(_T("SELECT var_value FROM config WHERE var_name='SMSDriver'"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         driver = DBGetField(hResult, 0, 0, nullptr, 0);
      DBFreeResult(hResult);
   }

   if ((driver != NULL) && (driver[0] != 0) && _tcscmp(driver, _T("<none>")))
   {
      hResult = SQLSelect(_T("SELECT var_value FROM config WHERE var_name='SMSDrvConfig'"));
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
            oldConfiguration = DBGetField(hResult, 0, 0, nullptr, 0);
         DBFreeResult(hResult);
      }

      if (oldConfiguration == NULL)
         oldConfiguration = _tcsdup(_T(""));

      PrepareDriverNameAndConfig(driver, oldConfiguration, newDriverName, newConfiguration);

      DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("INSERT INTO notification_channels (name,driver_name,description,configuration) VALUES ('SMS',?,'Automatically migrated SMS driver configuration',?)"));
      if (hStmt != NULL)
      {
        DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, static_cast<const TCHAR *>(newDriverName), DB_BIND_STATIC);
        DBBind(hStmt, 2, DB_SQLTYPE_TEXT, static_cast<const TCHAR *>(newConfiguration), DB_BIND_STATIC);
        if (!SQLExecute(hStmt) && !g_ignoreErrors)
        {
           DBFreeStatement(hStmt);
           return false;
        }
        DBFreeStatement(hStmt);
      }
      else if (!g_ignoreErrors)
        return false;

      CHK_EXEC(SQLQuery(_T("UPDATE actions SET channel_name='SMS' WHERE action_type=3")));
   }

   static const TCHAR *batch =
            _T("DELETE FROM config WHERE var_name='SMSDriver'\n")
            _T("DELETE FROM config WHERE var_name='SMSDrvConfig'\n")
            _T("UPDATE config SET var_name='AllowDirectNotifications',description='Allow/disallow sending of notification via NetXMS server using nxnotify utility.' WHERE var_name='AllowDirectSMS'\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   MemFree(driver);
   MemFree(oldConfiguration);

   CHK_EXEC(SetMinorSchemaVersion(92));
   return true;
}

/**
 * Upgrade from 30.90 to 30.91
 */
static bool H_UpgradeFromV90()
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='ICMP.PingSize' WHERE var_name='IcmpPingSize'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='ICMP.PingTmeout' WHERE var_name='IcmpPingTimeout'")));

   CHK_EXEC(CreateConfigParam(_T("ICMP.CollectPollStatistics"), _T("1"),
            _T("Collect ICMP poll statistics for all nodes by default. When enabled ICMP ping is used on each status poll and response time and packet loss are collected."),
            NULL, 'B', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("ICMP.StatisticPeriod"), _T("60"),
            _T("Time period for collecting ICMP statistics (in number of polls)."),
            _T("polls"), 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("ICMP.PollingInterval"), _T("60"),
            _T("Interval between ICMP polls (in seconds)."),
            _T("seconds"), 'I', true, false, false, false));

   CHK_EXEC(CreateTable(
            _T("CREATE TABLE icmp_statistics (")
            _T("  object_id integer not null,")
            _T("  poll_target varchar(63) not null,")
            _T("  min_response_time integer not null,")
            _T("  max_response_time integer not null,")
            _T("  avg_response_time integer not null,")
            _T("  last_response_time integer not null,")
            _T("  sample_count integer not null,")
            _T("  raw_response_times $SQL:TEXT null,")
            _T("PRIMARY KEY(object_id,poll_target))")
            ));

   CHK_EXEC(CreateTable(
            _T("CREATE TABLE icmp_target_address_list (")
            _T("  node_id integer not null,")
            _T("  ip_addr varchar(48) not null,")
            _T("PRIMARY KEY(node_id,ip_addr))")
            ));

   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD icmp_poll_mode char(1)")));
   CHK_EXEC(SQLQuery(_T("UPDATE nodes SET icmp_poll_mode='0'")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("icmp_poll_mode")));

   CHK_EXEC(SetMinorSchemaVersion(91));
   return true;
}

/**
 * Upgrade from 30.89 to 30.90
 */
static bool H_UpgradeFromV89()
{
   CHK_EXEC(DBResizeColumn(g_dbHandle, _T("hardware_inventory"), _T("vendor"), 127, true));
   CHK_EXEC(DBResizeColumn(g_dbHandle, _T("hardware_inventory"), _T("model"), 127, true));
   CHK_EXEC(DBResizeColumn(g_dbHandle, _T("hardware_inventory"), _T("part_number"), 63, true));
   CHK_EXEC(DBResizeColumn(g_dbHandle, _T("hardware_inventory"), _T("serial_number"), 63, true));
   CHK_EXEC(DBResizeColumn(g_dbHandle, _T("nodes"), _T("snmp_sys_location"), 255, true));
   CHK_EXEC(SetMinorSchemaVersion(90));
   return true;
}

/**
 * Upgrade from 30.88 to 30.89
 */
static bool H_UpgradeFromV88()
{
   CHK_EXEC(DBResizeColumn(g_dbHandle, _T("software_inventory"), _T("version"), 63, false));
   CHK_EXEC(DBResizeColumn(g_dbHandle, _T("mapping_data"), _T("md_value"), 4000, true));
   CHK_EXEC(SetMinorSchemaVersion(89));
   return true;
}

/**
 * Upgrade from 30.87 to 30.88
 */
static bool H_UpgradeFromV87()
{
   CHK_EXEC(CreateEventTemplate(EVENT_DUPLICATE_NODE_DELETED, _T("SYS_DUPLICATE_NODE_DELETED"),
            SEVERITY_WARNING, EF_LOG, _T("b587cd90-d98b-432f-acfd-ff51f25c27d1"),
            _T("Duplicate node %5 is deleted (%7)"),
            _T("Generated when node is deleted by network discovery de-duplication process.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Original node object ID\r\n")
            _T("   2) Original node name\r\n")
            _T("   3) Original node primary host name\r\n")
            _T("   4) Duplicate node object ID\r\n")
            _T("   5) Duplicate node name\r\n")
            _T("   6) Duplicate node primary host name\r\n")
            _T("   7) Reason")
            ));

   CHK_EXEC(CreateEventTemplate(EVENT_DUPLICATE_IP_ADDRESS, _T("SYS_DUPLICATE_IP_ADDRESS"),
            SEVERITY_WARNING, EF_LOG, _T("f134169c-9f88-4390-9e0e-248b7f8a0013"),
            _T("IP address %1 already known on node %3 interface %4 with MAC %5 but found via %8 with MAC %6"),
            _T("Generated when possibly duplicate IP address is detected by network discovery process.\r\n")
            _T("Parameters:\r\n")
            _T("   1) IP address\r\n")
            _T("   2) Known node object ID\r\n")
            _T("   3) Known node name\r\n")
            _T("   4) Known interface name\r\n")
            _T("   5) Known MAC address\r\n")
            _T("   6) Discovered MAC address\r\n")
            _T("   7) Discovery source node object ID\r\n")
            _T("   8) Discovery source node name\r\n")
            _T("   9) Discovery data source")
            ));

   CHK_EXEC(SetMinorSchemaVersion(88));
   return true;
}

/**
 * Upgrade from 30.86 to 30.87
 */
static bool H_UpgradeFromV86()
{
   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      CHK_EXEC(DBRenameTable(g_dbHandle, _T("idata"), _T("idata_old")));
      CHK_EXEC(DBRenameTable(g_dbHandle, _T("tdata"), _T("tdata_old")));

      CHK_EXEC(CreateTable(
               _T("CREATE TABLE idata_sc_default (")
               _T("  item_id integer not null,")
               _T("  idata_timestamp integer not null,")
               _T("  idata_value varchar(255) null,")
               _T("  raw_value varchar(255) null,")
               _T("PRIMARY KEY(item_id,idata_timestamp))")
               ));
      CHK_EXEC(CreateTable(
               _T("CREATE TABLE idata_sc_7 (")
               _T("  item_id integer not null,")
               _T("  idata_timestamp integer not null,")
               _T("  idata_value varchar(255) null,")
               _T("  raw_value varchar(255) null,")
               _T("PRIMARY KEY(item_id,idata_timestamp))")
               ));
      CHK_EXEC(CreateTable(
               _T("CREATE TABLE idata_sc_30 (")
               _T("  item_id integer not null,")
               _T("  idata_timestamp integer not null,")
               _T("  idata_value varchar(255) null,")
               _T("  raw_value varchar(255) null,")
               _T("PRIMARY KEY(item_id,idata_timestamp))")
               ));
      CHK_EXEC(CreateTable(
               _T("CREATE TABLE idata_sc_90 (")
               _T("  item_id integer not null,")
               _T("  idata_timestamp integer not null,")
               _T("  idata_value varchar(255) null,")
               _T("  raw_value varchar(255) null,")
               _T("PRIMARY KEY(item_id,idata_timestamp))")
               ));
      CHK_EXEC(CreateTable(
               _T("CREATE TABLE idata_sc_180 (")
               _T("  item_id integer not null,")
               _T("  idata_timestamp integer not null,")
               _T("  idata_value varchar(255) null,")
               _T("  raw_value varchar(255) null,")
               _T("PRIMARY KEY(item_id,idata_timestamp))")
               ));
      CHK_EXEC(CreateTable(
               _T("CREATE TABLE idata_sc_other (")
               _T("  item_id integer not null,")
               _T("  idata_timestamp integer not null,")
               _T("  idata_value varchar(255) null,")
               _T("  raw_value varchar(255) null,")
               _T("PRIMARY KEY(item_id,idata_timestamp))")
               ));

      CHK_EXEC(CreateTable(
               _T("CREATE TABLE tdata_sc_default (")
               _T("  item_id integer not null,")
               _T("  tdata_timestamp integer not null,")
               _T("  tdata_value $SQL:TEXT null,")
               _T("PRIMARY KEY(item_id,tdata_timestamp))")
               ));
      CHK_EXEC(CreateTable(
               _T("CREATE TABLE tdata_sc_7 (")
               _T("  item_id integer not null,")
               _T("  tdata_timestamp integer not null,")
               _T("  tdata_value $SQL:TEXT null,")
               _T("PRIMARY KEY(item_id,tdata_timestamp))")
               ));
      CHK_EXEC(CreateTable(
               _T("CREATE TABLE tdata_sc_30 (")
               _T("  item_id integer not null,")
               _T("  tdata_timestamp integer not null,")
               _T("  tdata_value $SQL:TEXT null,")
               _T("PRIMARY KEY(item_id,tdata_timestamp))")
               ));
      CHK_EXEC(CreateTable(
               _T("CREATE TABLE tdata_sc_90 (")
               _T("  item_id integer not null,")
               _T("  tdata_timestamp integer not null,")
               _T("  tdata_value $SQL:TEXT null,")
               _T("PRIMARY KEY(item_id,tdata_timestamp))")
               ));
      CHK_EXEC(CreateTable(
               _T("CREATE TABLE tdata_sc_180 (")
               _T("  item_id integer not null,")
               _T("  tdata_timestamp integer not null,")
               _T("  tdata_value $SQL:TEXT null,")
               _T("PRIMARY KEY(item_id,tdata_timestamp))")
               ));
      CHK_EXEC(CreateTable(
               _T("CREATE TABLE tdata_sc_other (")
               _T("  item_id integer not null,")
               _T("  tdata_timestamp integer not null,")
               _T("  tdata_value $SQL:TEXT null,")
               _T("PRIMARY KEY(item_id,tdata_timestamp))")
               ));

      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('idata_sc_default', 'idata_timestamp', 'item_id', chunk_time_interval => 86400, number_partitions => 100)")));
      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('idata_sc_7', 'idata_timestamp', 'item_id', chunk_time_interval => 86400, number_partitions => 100)")));
      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('idata_sc_30', 'idata_timestamp', 'item_id', chunk_time_interval => 86400, number_partitions => 100)")));
      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('idata_sc_90', 'idata_timestamp', 'item_id', chunk_time_interval => 86400, number_partitions => 100)")));
      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('idata_sc_180', 'idata_timestamp', 'item_id', chunk_time_interval => 86400, number_partitions => 100)")));
      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('idata_sc_other', 'idata_timestamp', 'item_id', chunk_time_interval => 86400, number_partitions => 100)")));

      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('tdata_sc_default', 'tdata_timestamp', 'item_id', chunk_time_interval => 86400, number_partitions => 100)")));
      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('tdata_sc_7', 'tdata_timestamp', 'item_id', chunk_time_interval => 86400, number_partitions => 100)")));
      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('tdata_sc_30', 'tdata_timestamp', 'item_id', chunk_time_interval => 86400, number_partitions => 100)")));
      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('tdata_sc_90', 'tdata_timestamp', 'item_id', chunk_time_interval => 86400, number_partitions => 100)")));
      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('tdata_sc_180', 'tdata_timestamp', 'item_id', chunk_time_interval => 86400, number_partitions => 100)")));
      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('tdata_sc_other', 'tdata_timestamp', 'item_id', chunk_time_interval => 86400, number_partitions => 100)")));

      CHK_EXEC(SQLQuery(
            _T("CREATE VIEW idata AS")
            _T("   SELECT * FROM idata_sc_default")
            _T("   UNION ALL")
            _T("   SELECT * FROM idata_sc_7")
            _T("   UNION ALL")
            _T("   SELECT * FROM idata_sc_30")
            _T("   UNION ALL")
            _T("   SELECT * FROM idata_sc_90")
            _T("   UNION ALL")
            _T("   SELECT * FROM idata_sc_180")
            _T("   UNION ALL")
            _T("   SELECT * FROM idata_sc_other")));

      CHK_EXEC(SQLQuery(
            _T("CREATE VIEW tdata AS")
            _T("   SELECT * FROM tdata_sc_default")
            _T("   UNION ALL")
            _T("   SELECT * FROM tdata_sc_7")
            _T("   UNION ALL")
            _T("   SELECT * FROM tdata_sc_30")
            _T("   UNION ALL")
            _T("   SELECT * FROM tdata_sc_90")
            _T("   UNION ALL")
            _T("   SELECT * FROM tdata_sc_180")
            _T("   UNION ALL")
            _T("   SELECT * FROM tdata_sc_other")));

      RegisterOnlineUpgrade(30, 87);
   }
   else
   {
      CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("idata")));
      CHK_EXEC(DBDropColumn(g_dbHandle, _T("idata"), _T("node_id")));
      CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("idata"), _T("item_id,idata_timestamp")));

      CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("tdata")));
      CHK_EXEC(DBDropColumn(g_dbHandle, _T("tdata"), _T("node_id")));
      CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("tdata"), _T("item_id,tdata_timestamp")));
   }
   CHK_EXEC(SetMinorSchemaVersion(87));
   return true;
}

/**
 * Upgrade from 30.85 to 30.86
 */
static bool H_UpgradeFromV85()
{
   CHK_EXEC(CreateConfigParam(_T("Housekeeper.DisableCollectedDataCleanup"), _T("0"), _T("Disable automatic cleanup of collected DCI data during housekeeper run."), NULL, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(86));
   return true;
}

/**
 * Upgrade from 30.84 to 30.85
 */
static bool H_UpgradeFromV84()
{
   static const TCHAR *batch =
            _T("ALTER TABLE alarms ADD rule_guid varchar(36)\n")
            _T("ALTER TABLE alarms ADD event_tags varchar(2000)\n")
            _T("UPDATE event_log SET event_tags=user_tag\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("event_log"), _T("user_tag")));
   CHK_EXEC(SetMinorSchemaVersion(85));
   return true;
}

/**
 * Upgrade from 30.83 to 30.84
 */
static bool H_UpgradeFromV83()
{
   static const TCHAR *batch =
            _T("ALTER TABLE event_cfg ADD tags varchar(2000)\n")
            _T("ALTER TABLE event_log ADD event_tags varchar(2000)\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   DB_RESULT hResult = SQLSelect(_T("SELECT event_code,name FROM event_group_members INNER JOIN event_groups ON event_groups.id=event_group_members.group_id ORDER BY event_code"));
   if (hResult != NULL)
   {
      UINT32 currEventCode = 0;
      StringBuffer tags;
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 eventCode = DBGetFieldULong(hResult, i, 0);
         if (eventCode != currEventCode)
         {
            if (!tags.isEmpty())
            {
               StringBuffer query = _T("UPDATE event_cfg SET tags=");
               query.append(DBPrepareString(g_dbHandle, tags));
               query.append(_T(" WHERE event_code="));
               query.append(currEventCode);
               CHK_EXEC(SQLQuery(query));
               tags.clear();
            }
            currEventCode = eventCode;
         }

         TCHAR tag[64];
         DBGetField(hResult, i, 1, tag, 64);
         Trim(tag);
         if (tag[0] != 0)
         {
            if (!tags.isEmpty())
               tags.append(_T(','));
            tags.append(tag);
         }
      }
      DBFreeResult(hResult);

      if (!tags.isEmpty())
      {
         StringBuffer query = _T("UPDATE event_cfg SET tags=");
         query.append(DBPrepareString(g_dbHandle, tags));
         query.append(_T(" WHERE event_code="));
         query.append(currEventCode);
         CHK_EXEC(SQLQuery(query));
      }
   }
   else
   {
      if (!g_ignoreErrors)
         return false;
   }

   CHK_EXEC(SQLQuery(_T("DROP TABLE event_group_members")));
   CHK_EXEC(SQLQuery(_T("DROP TABLE event_groups")));

   CHK_EXEC(SetMinorSchemaVersion(84));
   return true;
}

/**
 * Upgrade from 30.82 to 30.83
 */
static bool H_UpgradeFromV82()
{
   if (!DBMgrMetaDataReadInt32(_T("CorrectFieldNamesV80"), 0))
   {
      CHK_EXEC(DBRenameColumn(g_dbHandle, _T("address_lists"), _T("comment"), _T("comments")));
      CHK_EXEC(DBRenameColumn(g_dbHandle, _T("usm_credentials"), _T("comment"), _T("comments")));
   }
   CHK_EXEC(SQLQuery(_T("DELETE FROM metadata WHERE var_name='CorrectFieldNamesV80'")));
   CHK_EXEC(SetMinorSchemaVersion(83));
   return true;
}

/**
 * Upgrade from 30.81 to 30.82
 */
static bool H_UpgradeFromV81()
{
   CHK_EXEC(SQLQuery(
         _T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
         _T("VALUES ('0517b64c-b1c3-43a1-8081-4b9dcc9433bb',20,'Hook::UpdateInterface','")
         _T("/* Available global variables:\r\n *  $node - current node, object of ''Node'' class\r\n")
         _T(" *  $interface - current interface, object of ''Interface'' class\n\n *\r\n * Expected return value:\r\n")
         _T(" *  none - returned value is ignored\r\n */\r\n')")));

   CHK_EXEC(SetMinorSchemaVersion(82));
   return true;
}

/**
 * Upgrade from 30.80 to 30.81
 */
static bool H_UpgradeFromV80()
{
   uint32_t discoveryEnabled = 0;
   uint32_t activeDiscoveryEnabled = 0;
   DB_RESULT hResult = SQLSelect(_T("SELECT var_value from config WHERE var_name='RunNetworkDiscovery'"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         discoveryEnabled = DBGetFieldLong(hResult, 0, 0);
      DBFreeResult(hResult);
   }
   hResult = SQLSelect(_T("SELECT var_value from config WHERE var_name='ActiveNetworkDiscovery'"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         activeDiscoveryEnabled = DBGetFieldLong(hResult, 0, 0);
      DBFreeResult(hResult);
   }

   static TCHAR batch[] =
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('NetworkDiscovery.Type','0','Disabled')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('NetworkDiscovery.Type','1','Passive only')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('NetworkDiscovery.Type','2','Active only')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('NetworkDiscovery.Type','3','Passive and active')\n")
      _T("DELETE FROM config WHERE var_name='RunNetworkDiscovery'\n")
      _T("DELETE FROM config WHERE var_name='ActiveNetworkDiscovery'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   const TCHAR *discoveryType =
            (discoveryEnabled ? (activeDiscoveryEnabled ? _T("3") : _T("1")) : _T("0"));

   CHK_EXEC(CreateConfigParam(_T("NetworkDiscovery.Type"), discoveryType, _T("Type of the network discovery."), NULL, 'C', true, true, false, true));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET default_value='0' WHERE var_name='NetworkDiscovery.Type'")));

   CHK_EXEC(SetMinorSchemaVersion(81));
   return true;
}

/**
 * Upgrade from 30.79 to 30.80
 */
static bool H_UpgradeFromV79()
{
   static const TCHAR *batch =
            _T("ALTER TABLE address_lists ADD comments varchar(255)\n")
            _T("ALTER TABLE usm_credentials ADD comments varchar(255)\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   // Indicator that field names are correct ("comments" instead of "comment")
   CHK_EXEC(DBMgrMetaDataWriteInt32(_T("CorrectFieldNamesV80"), 1));

   CHK_EXEC(DBRenameTable(g_dbHandle, _T("user_agent_notification"), _T("user_agent_notifications")));

   CHK_EXEC(SetMinorSchemaVersion(80));
   return true;
}

/**
 * Upgrade from 30.78 to 30.79
 */
static bool H_UpgradeFromV78()
{
   CHK_EXEC(CreateConfigParam(_T("DBWriter.MaxRecordsPerStatement"), _T("100"), _T("Maximum number of records per one SQL statement for delayed database writes."), _T("records/statement"), 'I', true, true, false, false));
   CHK_EXEC(SetMinorSchemaVersion(79));
   return true;
}

/**
 * Upgrade from 30.77 to 30.78 (changes also included into 22.59)
 */
static bool H_UpgradeFromV77()
{
   if (GetSchemaLevelForMajorVersion(22) < 59)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config_values SET var_description='AUTO' WHERE var_name='DefaultInterfaceExpectedState' AND var_value='1'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 59));
   }

   CHK_EXEC(SetMinorSchemaVersion(78));
   return true;
}

/**
 * Upgrade from 30.76 to 30.77
 */
static bool H_UpgradeFromV76()
{
   CHK_EXEC(DBRenameTable(g_dbHandle, _T("user_agent_messages"), _T("user_agent_notification")));

   CHK_EXEC(SetMinorSchemaVersion(77));
   return true;
}

/**
 * Upgrade from 30.75 to 30.76
 */
static bool H_UpgradeFromV75()
{
   DB_RESULT hResult = SQLSelect(_T("SELECT system_access FROM user_groups WHERE id=-2147483647"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) != 0)
      {
         UINT64 accessRights = DBGetFieldUInt64(hResult, 0, 0);
         accessRights |= SYSTEM_ACCESS_UA_NOTIFICATIONS;

         TCHAR query[256];
         _sntprintf(query, 256, _T("UPDATE user_groups SET system_access=") UINT64_FMT _T(" WHERE id=-2147483647"), accessRights);
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(hResult);
   }

   CHK_EXEC(SetMinorSchemaVersion(76));
   return true;
}

/**
 * Upgrade from 30.74 to 30.75
 */
static bool H_UpgradeFromV74()
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE user_agent_messages (")
      _T("   id integer not null,")
      _T("   message varchar(1023) null,")
      _T("   objects varchar(1023) not null,")
      _T("   start_time integer not null,")
      _T("   end_time integer not null,")
      _T("   recall char(1) not null,")
      _T("PRIMARY KEY(id))")));

   CHK_EXEC(CreateConfigParam(_T("UserAgent.DefaultMessageRetentionTime"), _T("10080"), _T("Default user agent message retention time (in minutes)."), _T("minutes"), 'I', true, false, true, false));
   CHK_EXEC(CreateConfigParam(_T("UserAgent.RetentionTime"), _T("30"), _T("User agent message historical data retention time (in days)."), _T("days"), 'I', true, false, false, false));

   CHK_EXEC(SetMinorSchemaVersion(75));
   return true;
}

/**
 * Upgrade from 30.73 to 30.74 (changes also included into 22.58)
 */
static bool H_UpgradeFromV73()
{
   if (GetSchemaLevelForMajorVersion(22) < 58)
   {
      CHK_EXEC(DBResizeColumn(g_dbHandle, _T("items"), _T("instance"), 1023, true));
      CHK_EXEC(DBResizeColumn(g_dbHandle, _T("items"), _T("instd_data"), 1023, true));
      CHK_EXEC(DBResizeColumn(g_dbHandle, _T("dc_tables"), _T("instance"), 1023, true));
      CHK_EXEC(DBResizeColumn(g_dbHandle, _T("dc_tables"), _T("instd_data"), 1023, true));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 58));
   }

   CHK_EXEC(SetMinorSchemaVersion(74));
   return true;
}

/**
 * Upgrade from 30.72 to 30.73
 */
static bool H_UpgradeFromV72()
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='AlarmListDisplayLimit'")));
   CHK_EXEC(SetMinorSchemaVersion(73));
   return true;
}

/**
 * Upgrade from 30.71 to 30.72
 */
static bool H_UpgradeFromV71()
{
   CHK_EXEC(CreateEventTemplate(EVENT_HARDWARE_COMPONENT_ADDED, _T("SYS_HARDWARE_COMPONENT_ADDED"),
            SEVERITY_NORMAL, EF_LOG, _T("3677d317-d5fc-4ba7-83a9-890db4aa7112"),
            _T("%1 %4 (%2) added"),
            _T("Generated when new hardware component is found.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Category\r\n")
            _T("   2) Type\r\n")
            _T("   3) Vendor\r\n")
            _T("   4) Model\r\n")
            _T("   5) Location\r\n")
            _T("   6) Part number\r\n")
            _T("   7) Serial number\r\n")
            _T("   8) Capacity\r\n")
            _T("   9) Description")
            ));

   CHK_EXEC(CreateEventTemplate(EVENT_HARDWARE_COMPONENT_REMOVED, _T("SYS_HARDWARE_COMPONENT_REMOVED"),
            SEVERITY_NORMAL, EF_LOG, _T("72904936-1d42-40c4-810c-e226ae44d7f1"),
            _T("%1 %4 (%2) removed"),
            _T("Generated when hardware component removal is detecetd.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Category\r\n")
            _T("   2) Type\r\n")
            _T("   3) Vendor\r\n")
            _T("   4) Model\r\n")
            _T("   5) Location\r\n")
            _T("   6) Part number\r\n")
            _T("   7) Serial number\r\n")
            _T("   8) Capacity\r\n")
            _T("   9) Description")
            ));

   CHK_EXEC(SetMinorSchemaVersion(72));
   return true;
}

/**
 * Upgrade from 30.70 to 30.71 (changes also included into 22.56)
 */
static bool H_UpgradeFromV70()
{
   if (GetSchemaLevelForMajorVersion(22) < 56)
   {
      CHK_EXEC(CreateTable(
         _T("CREATE TABLE zone_proxies (")
         _T("   object_id integer not null,")
         _T("   proxy_node integer not null,")
         _T("PRIMARY KEY(object_id,proxy_node))")));

      DB_RESULT hResult = SQLSelect(_T("SELECT id,proxy_node FROM zones"));
      if (hResult != NULL)
      {
         int count = DBGetNumRows(hResult);
         if (count > 0)
         {
            DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("INSERT INTO zone_proxies (object_id,proxy_node) VALUES (?,?)"));
            if (hStmt != NULL)
            {
               for(int i = 0; i < count; i++)
               {
                  DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 0));
                  DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 1));
                  if (!SQLExecute(hStmt) && !g_ignoreErrors)
                  {
                     DBFreeStatement(hStmt);
                     DBFreeResult(hResult);
                     return false;
                  }
               }
               DBFreeStatement(hStmt);
            }
            else if (!g_ignoreErrors)
            {
               DBFreeResult(hResult);
               return false;
            }
         }
         DBFreeResult(hResult);
      }
      else if (!g_ignoreErrors)
      {
         return false;
      }

      CHK_EXEC(DBDropColumn(g_dbHandle, _T("zones"), _T("proxy_node")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 56));
   }

   CHK_EXEC(SetMinorSchemaVersion(71));
   return true;
}

/**
 * Upgrade from 30.69 to 30.70
 */
static bool H_UpgradeFromV69()
{
   if (DBIsTableExist(g_dbHandle, _T("hardware_inventory")))
      CHK_EXEC(SQLQuery(_T("DROP TABLE hardware_inventory")));

   CHK_EXEC(CreateTable(
         _T("CREATE TABLE hardware_inventory (")
         _T("   node_id integer not null,")
         _T("   category integer not null,")
         _T("   component_index integer not null,")
         _T("   hw_type varchar(47) null,")
         _T("   vendor varchar(63) null,")
         _T("   model varchar(63) null,")
         _T("   location varchar(63) null,")
         _T("   capacity $SQL:INT64 null,")
         _T("   part_number varchar(63) null,")
         _T("   serial_number varchar(63) null,")
         _T("   description varchar(255) null,")
         _T("   PRIMARY KEY(node_id,category,component_index))")));

   CHK_EXEC(SetMinorSchemaVersion(70));
   return true;
}

/**
 * Upgrade from 30.68 to 30.69 (changes also included into 22.55)
 */
static bool H_UpgradeFromV68()
{
   if (GetSchemaLevelForMajorVersion(22) < 55)
   {
      CHK_EXEC(SQLQuery(
               _T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
               _T("VALUES ('4ec1a7bc-d46f-4df3-b846-e9dfd66571dc',19,'Hook::CreateSubnet',")
               _T("'/* Available global variables:\r\n *  $node - current node, object of ''Node'' class\r\n")
               _T(" *  $1 - current subnet, object of ''Subnet'' class\r\n")
               _T(" *\r\n * Expected return value:\r\n")
               _T(" *  true/false - boolean - whether subnet should be created\r\n")
               _T(" */\r\nreturn true;\r\n')")
            ));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 55));
   }

   CHK_EXEC(SetMinorSchemaVersion(69));
   return true;
}

/**
 * Upgrade from 30.67 to 30.68 (changes also included into 22.54)
 */
static bool H_UpgradeFromV67()
{
   if (GetSchemaLevelForMajorVersion(22) < 54)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE dct_threshold_instances ADD instance_id integer")));

      DB_RESULT hResult = SQLSelect(_T("SELECT threshold_id,instance,maint_copy FROM dct_threshold_instances"));
      if (hResult != NULL)
      {
         int count = DBGetNumRows(hResult);
         if (count > 0)
         {
            DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("UPDATE dct_threshold_instances SET instance_id=? WHERE threshold_id=? AND instance=? AND maint_copy=?"));
            if (hStmt != NULL)
            {
               TCHAR instance[256];
               UINT32 instanceId = 1;
               for(int i = 0; i < count; i++)
               {
                  UINT32 thresholdId = DBGetFieldULong(hResult, i, 0);
                  DBGetField(hResult, i, 1, instance, 256);
                  int maintCopy = DBGetFieldLong(hResult, i, 2);

                  DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, instanceId++);
                  DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, thresholdId);
                  DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, instance, DB_BIND_STATIC);
                  DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, maintCopy ? _T("1") : _T("0"), DB_BIND_STATIC);
                  if (!SQLExecute(hStmt) && !g_ignoreErrors)
                  {
                     DBFreeStatement(hStmt);
                     DBFreeResult(hResult);
                     return false;
                  }
               }
               DBFreeStatement(hStmt);
            }
            else if (!g_ignoreErrors)
            {
               DBFreeResult(hResult);
               return false;
            }
         }
         DBFreeResult(hResult);
      }
      else if (!g_ignoreErrors)
      {
         return false;
      }

      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dct_threshold_instances"), _T("instance_id")));
      CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("dct_threshold_instances")));
      CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("dct_threshold_instances"), _T("threshold_id,instance_id")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 54));
   }

   CHK_EXEC(SetMinorSchemaVersion(68));
   return true;
}

/**
 * Upgrade from 30.66 to 30.67 (changes also included into 22.53)
 */
static bool H_UpgradeFromV66()
{
   if (GetSchemaLevelForMajorVersion(22) < 53)
   {
      CHK_EXEC(DBDropColumn(g_dbHandle, _T("config"), _T("possible_values")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 53));
   }

   CHK_EXEC(SetMinorSchemaVersion(67));
   return true;
}

/**
 * Upgrade from 30.65 to 30.66 (changes also included into 22.52)
 */
static bool H_UpgradeFromV65()
{
   if (GetSchemaLevelForMajorVersion(22) < 52)
   {
      CHK_EXEC(CreateTable(
         _T("CREATE TABLE node_components (")
         _T("   node_id integer not null,")
         _T("   component_index integer not null,")
         _T("   parent_index integer not null,")
         _T("   position integer not null,")
         _T("   component_class integer not null,")
         _T("   if_index integer not null,")
         _T("   name varchar(255) null,")
         _T("   description varchar(255) null,")
         _T("   model varchar(255) null,")
         _T("   serial_number varchar(63) null,")
         _T("   vendor varchar(63) null,")
         _T("   firmware varchar(127) null,")
         _T("PRIMARY KEY(node_id,component_index))")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 52));
   }

   CHK_EXEC(SetMinorSchemaVersion(66));
   return true;
}

/**
 * Upgrade from 30.64 to 30.65 (changes also included into 22.51)
 */
static bool H_UpgradeFromV64()
{
   if (GetSchemaLevelForMajorVersion(22) < 51)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE thresholds ADD last_checked_value varchar(255)")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 51));
   }

   CHK_EXEC(SetMinorSchemaVersion(65));
   return true;
}

/**
 * Upgrade from 30.63 to 30.64 (changes also included into 22.50)
 */
static bool H_UpgradeFromV63()
{
   if (GetSchemaLevelForMajorVersion(22) < 50)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET description='Generated when node, cluster, or mobile device entered maintenance mode.\r\nParameters:\r\n   1) Comments' WHERE guid='5f6c8b1c-f162-413e-8028-80e7ad2c362d'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 50));
   }

   CHK_EXEC(SetMinorSchemaVersion(64));
   return true;
}

/**
 * Upgrade from 30.62 to 30.63 (changes also included into 22.49)
 */
static bool H_UpgradeFromV62()
{
   if (GetSchemaLevelForMajorVersion(22) < 49)
   {
      DB_RESULT hResult = SQLSelect(_T("SELECT system_access FROM user_groups WHERE id=-2147483647"));
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) != 0)
         {
            UINT64 accessRights = DBGetFieldUInt64(hResult, 0, 0);
            accessRights |= SYSTEM_ACCESS_IMPORT_CONFIGURATION;

            TCHAR query[256];
            _sntprintf(query, 256, _T("UPDATE user_groups SET system_access=") UINT64_FMT _T(" WHERE id=-2147483647"), accessRights);
            CHK_EXEC(SQLQuery(query));
         }
         DBFreeResult(hResult);
      }
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 49));
   }

   CHK_EXEC(SetMinorSchemaVersion(63));
   return true;
}

/**
 * Upgrade from 30.61 to 30.62 (changes also included into 22.48)
 */
static bool H_UpgradeFromV61()
{
   if (GetSchemaLevelForMajorVersion(22) < 48)
   {
      static const TCHAR *batch =
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('ImportConfigurationOnStartup','0','Never')\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('ImportConfigurationOnStartup','1','Only missing elements')\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('ImportConfigurationOnStartup','2','Always')\n")
         _T("UPDATE config SET data_type='C' WHERE var_name='ImportConfigurationOnStartup'\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 48));
   }

   CHK_EXEC(SetMinorSchemaVersion(62));
   return true;
}

/**
 * Upgrade from 30.60 to 30.61 (changes also included into 22.47)
 */
static bool H_UpgradeFromV60()
{
   if (GetSchemaLevelForMajorVersion(22) < 47)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='Topology.AdHocRequest.ExpirationTime',description='Ad-hoc network topology request expiration time. Server will use cached result of previous request if it is newer than given interval.' WHERE var_name='TopologyExpirationTime'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='Topology.DefaultDiscoveryRadius',default_value='5',description='Default number of hops from seed node to be added to topology map.' WHERE var_name='TopologyDiscoveryRadius'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='Topology.PollingInterval' WHERE var_name='TopologyPollingInterval'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 47));
   }

   CHK_EXEC(SetMinorSchemaVersion(61));
   return true;
}

/**
 * Upgrade from 30.59 to 30.60 (changes also included into 22.46)
 */
static bool H_UpgradeFromV59()
{
   if (GetSchemaLevelForMajorVersion(22) < 46)
   {
      CHK_EXEC(CreateTable(
         _T("CREATE TABLE idata (")
         _T("   node_id integer not null,")
         _T("   item_id integer not null,")
         _T("   idata_timestamp integer not null,")
         _T("   idata_value varchar(255) null,")
         _T("   raw_value varchar(255) null,")
         _T("PRIMARY KEY(node_id,item_id,idata_timestamp))")));

      CHK_EXEC(CreateTable(
         _T("CREATE TABLE tdata (")
         _T("   node_id integer not null,")
         _T("   item_id integer not null,")
         _T("   tdata_timestamp integer not null,")
         _T("   tdata_value $SQL:TEXT null,")
         _T("PRIMARY KEY(node_id,item_id,tdata_timestamp))")));

      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('idata', 'idata_timestamp', 'node_id', chunk_time_interval => 604800, number_partitions => 100, migrate_data => true)")));
         CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('tdata', 'tdata_timestamp', 'node_id', chunk_time_interval => 604800, number_partitions => 100, migrate_data => true)")));
      }

      CHK_EXEC(SQLQuery(_T("INSERT INTO metadata (var_name,var_value) VALUES ('SingeTablePerfData','0')")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 46));
   }

   CHK_EXEC(SetMinorSchemaVersion(60));
   return true;
}

/**
 * Upgrade from 30.58 to 30.59 (changes also included into 22.45)
 */
static bool H_UpgradeFromV58()
{
   if (GetSchemaLevelForMajorVersion(22) < 45)
   {
      CHK_EXEC(CreateConfigParam(_T("Alarms.ResolveExpirationTime"), _T("0"),
               _T("Expiration time (in seconds) for resolved alarms. If set to non-zero, resolved and untouched alarms will be terminated automatically after given timeout."),
               _T("seconds"), 'I', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 45));
   }
   CHK_EXEC(SQLQuery(_T("UPDATE config SET units='seconds' WHERE var_name='Alarms.ResolveExpirationTime'")));

   CHK_EXEC(SetMinorSchemaVersion(59));
   return true;
}

/**
 * Upgrade from 30.57 to 30.58 (changes also included into 22.44)
 */
static bool H_UpgradeFromV57()
{
   if (GetSchemaLevelForMajorVersion(22) < 44)
   {
      CHK_EXEC(CreateConfigParam(_T("NetworkDiscovery.MergeDuplicateNodes"), _T("0"),
               _T("Enable/disable merge of duplicate nodes. When enabled, configuration of duplicate node(s) will be merged into original node and duplicate(s) will be deleted."),
               NULL, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 44));
   }

   CHK_EXEC(SetMinorSchemaVersion(58));
   return true;
}

/**
 * Upgrade from 30.56 to 30.57
 */
static bool H_UpgradeFromV56()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE software_inventory (")
         _T("   node_id integer not null,")
         _T("   name varchar(127) not null,")
         _T("   version varchar(47) not null,")
         _T("   vendor varchar(63) null,")
         _T("   install_date integer not null,")
         _T("   url varchar(255) null,")
         _T("   description varchar(255) null,")
         _T("   PRIMARY KEY(node_id,name,version))")));

   CHK_EXEC(SetMinorSchemaVersion(57));
   return true;
}

/**
 * Upgrade from 30.55 to 30.56
 */
static bool H_UpgradeFromV55()
{
   DBRenameTable(g_dbHandle, _T("ap_common"), _T("ap_common_old"));
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE ap_common (")
      _T("   policy_name varchar(63) not null,")
      _T("   owner_id integer not null,")
      _T("   policy_type varchar(31) not null,")
      _T("   file_content $SQL:TEXT null,")
      _T("   version integer not null,")
      _T("   guid varchar(36) not null,")
      _T("   PRIMARY KEY(guid))")));

   bool success = true;

   DB_RESULT hResult = SQLSelect(_T("SELECT id,policy_type,name,guid FROM ap_common_old a INNER JOIN object_properties p ON p.object_id=a.id WHERE policy_type IN (1,2)"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i=0; i < count; i++)
      {
         UINT32 id = DBGetFieldLong(hResult, i, 0);
         UINT32 type = DBGetFieldLong(hResult, i, 1);
         TCHAR name[MAX_OBJECT_NAME];
         DBGetField(hResult, i, 2, name, MAX_OBJECT_NAME);
         TCHAR guid[64];
         DBGetField(hResult, i, 3, guid, 64);

         TCHAR *content = NULL;

         TCHAR query[512];
         if (type == 1)
         {
            _sntprintf(query, 512, _T("SELECT file_content FROM ap_config_files WHERE policy_id=%d"), id);
            DB_RESULT policy = SQLSelect(query);
            if (policy != nullptr)
            {
               if(DBGetNumRows(policy) > 0)
                  content = DBGetField(policy, 0, 0, NULL, 0);
               DBFreeResult(policy);
            }
            else
            {
               success = false;
               break;
            }
         }
         else
         {
            _sntprintf(query, 512, _T("SELECT file_content FROM ap_log_parser WHERE policy_id=%d"), id);
            DB_RESULT policy = SQLSelect(query);
            if (policy != nullptr)
            {
               if(DBGetNumRows(policy) > 0)
                  content = DBGetField(policy, 0, 0, nullptr, 0);
               DBFreeResult(policy);
            }
            else
            {
               success = false;
               break;
            }
         }

         //Change every policy to template
         DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("INSERT INTO templates (id) VALUES (?)"));
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
            if (!(SQLExecute(hStmt)))
            {
               if (!g_ignoreErrors)
               {
                  success = false;
                  break;
               }
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
            break;
         }

         //Move common info to the new table
         DB_STATEMENT apCommon = DBPrepare(g_dbHandle, _T("INSERT INTO ap_common (policy_name,owner_id,policy_type,file_content,version,guid) VALUES (?,?,?,?,?,?)"));
         if (apCommon == NULL)
         {
            success = false;
            break;
         }

         DBBind(apCommon, 1, DB_SQLTYPE_TEXT, name, DB_BIND_STATIC);
         DBBind(apCommon, 2, DB_SQLTYPE_INTEGER, id);
         DBBind(apCommon, 3, DB_SQLTYPE_VARCHAR, type == 1 ? _T("AgentConfig") : _T("LogParserConfig"), DB_BIND_STATIC);
         DBBind(apCommon, 4, DB_SQLTYPE_VARCHAR, content, DB_BIND_STATIC);
         DBBind(apCommon, 5, DB_SQLTYPE_INTEGER, 0);
         DBBind(apCommon, 6, DB_SQLTYPE_VARCHAR, guid, DB_BIND_STATIC);
         if (!(SQLExecute(apCommon)))
         {
            if (!g_ignoreErrors)
            {
               success = false;
               break;
            }
         }
         DBFreeStatement(apCommon);
         MemFree(content);
      }
      DBFreeResult(hResult);
   }

   if (!success || (hResult == NULL))
   {
      if (!g_ignoreErrors)
         return false;
   }

   //ap_bindings move to template bindings dct_node_map (template_id,node_id)
   hResult = SQLSelect(_T("SELECT policy_id,node_id FROM ap_bindings"));
   if (hResult == nullptr)
   {
      if (!g_ignoreErrors)
         return false;
   }

   int count = DBGetNumRows(hResult);
   for(int i=0; i < count; i++)
   {
      DB_STATEMENT apBinding = DBPrepare(g_dbHandle, _T("INSERT INTO dct_node_map (template_id,node_id) VALUES (?,?)"));
      if(apBinding != NULL)
      {
         DBBind(apBinding, 1, DB_SQLTYPE_INTEGER, DBGetFieldLong(hResult, i, 0));
         DBBind(apBinding, 2, DB_SQLTYPE_INTEGER, DBGetFieldLong(hResult, i, 1));
         if (!(SQLExecute(apBinding)))
         {
            if (!g_ignoreErrors)
            {
               success = false;
               break;
            }
         }
         DBFreeStatement(apBinding);
      }
   }
   DBFreeResult(hResult);

   if (!success && !g_ignoreErrors)
      return false;

   CHK_EXEC(SQLQuery(_T("UPDATE container_members SET container_id=3 WHERE container_id=5")));
   CHK_EXEC(SQLQuery(_T("UPDATE object_containers SET object_class=9 WHERE object_class=15")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM object_containers WHERE id=5")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM object_custom_attributes WHERE object_id=5")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM object_urls WHERE object_id=5")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM responsible_users WHERE object_id=5")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM acl WHERE object_id=5")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM object_properties WHERE object_id=5")));

   CHK_EXEC(SQLQuery(_T("DELETE FROM scheduled_tasks WHERE taskid='Policy.Deploy'")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM scheduled_tasks WHERE taskid='Policy.Uninstall'")));

   CHK_EXEC(SQLQuery(_T("DROP TABLE ap_common_old")));
   CHK_EXEC(SQLQuery(_T("DROP TABLE ap_log_parser")));
   CHK_EXEC(SQLQuery(_T("DROP TABLE ap_config_files")));
   CHK_EXEC(SQLQuery(_T("DROP TABLE ap_bindings")));

   CHK_EXEC(SetMinorSchemaVersion(56));
   return true;
}

/**
 * Upgrade from 30.54 to 30.55 (changes also included into 22.43)
 */
static bool H_UpgradeFromV54()
{
   if (GetSchemaLevelForMajorVersion(22) < 43)
   {
      CHK_EXEC(CreateConfigParam(_T("NetworkDiscovery.EnableParallelProcessing"), _T("0"), _T("Enable/disable parallel processing of discovered addresses."), NULL, 'B', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Discovery.BaseSize"), _T("1"), _T("Base size for network discovery thread pool."), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Discovery.MaxSize"), _T("16"), _T("Maximum size for network discovery thread pool."), NULL, 'I', true, true, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 43));
   }
   CHK_EXEC(SetMinorSchemaVersion(55));
   return true;
}

/**
 * Upgrade from 30.53 to 30.54 (changes also included into 22.42)
 */
static bool H_UpgradeFromV53()
{
   if (GetSchemaLevelForMajorVersion(22) < 42)
   {
      CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("address_lists")));

      static const TCHAR *batch =
         _T("ALTER TABLE address_lists ADD zone_uin integer\n")
         _T("ALTER TABLE address_lists ADD proxy_id integer\n")
         _T("UPDATE address_lists SET zone_uin=0,proxy_id=0\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("address_lists"), _T("proxy_id")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("address_lists"), _T("zone_uin")));
      CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("address_lists"), _T("list_type,community_id,zone_uin,addr_type,addr1,addr2")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 42));
   }
   CHK_EXEC(SetMinorSchemaVersion(54));
   return true;
}

/**
 * Upgrade from 30.52 to 30.53
 */
static bool H_UpgradeFromV52()
{
   CHK_EXEC(CreateConfigParam(_T("DataCollection.StartupDelay"), _T("0"), _T("Enable/disable randomized data collection delays on server startup for evening server load distrubution."), NULL, 'B', true, true, false, false));
   CHK_EXEC(SetMinorSchemaVersion(53));
   return true;
}

/**
 * Upgrade from 30.51 to 30.52 (changes also included into 22.41)
 */
static bool H_UpgradeFromV51()
{
   if (GetSchemaLevelForMajorVersion(22) < 41)
   {
      CHK_EXEC(DBRenameColumn(g_dbHandle, _T("dct_threshold_instances"), _T("row_number"), _T("tt_row_number")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 41));
   }
   CHK_EXEC(SetMinorSchemaVersion(52));
   return true;
}

/**
 * Upgrade from 30.50 to 30.51 (changes also included into 22.40)
 */
static bool H_UpgradeFromV50()
{
   if (GetSchemaLevelForMajorVersion(22) < 40)
   {
      static const TCHAR *batch =
         _T("ALTER TABLE items ADD grace_period_start integer\n")
         _T("ALTER TABLE dc_tables ADD grace_period_start integer\n")
         _T("UPDATE items SET grace_period_start=0\n")
         _T("UPDATE dc_tables SET grace_period_start=0\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("items"), _T("grace_period_start")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dc_tables"), _T("grace_period_start")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 40));
   }
   CHK_EXEC(SetMinorSchemaVersion(51));
   return true;
}

/**
 * Upgrade from 30.49 to 30.50 (changes also included into 22.39)
 */
static bool H_UpgradeFromV49()
{
   if (GetSchemaLevelForMajorVersion(22) < 39)
   {
      TCHAR tmp[MAX_CONFIG_VALUE_LENGTH] = _T("");
      DB_RESULT hResult = SQLSelect(_T("SELECT var_value from config WHERE var_name='LdapMappingName'"));
      if (hResult != nullptr)
      {
         if(DBGetNumRows(hResult) > 0)
            DBGetField(hResult, 0, 0, tmp, MAX_CONFIG_VALUE_LENGTH);
      }

      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='LdapUserMappingName' WHERE var_name='LdapMappingName'")));
      CHK_EXEC(CreateConfigParam(_T("LdapGroupMappingName"), _T(""), _T("The name of an attribute whose value will be used as a group''s login name."), NULL, 'S', true, true, false, false));

      DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("UPDATE config SET var_value=? WHERE var_name='LdapGroupMappingName'"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_TEXT, tmp, DB_BIND_STATIC);
         CHK_EXEC(SQLExecute(hStmt));
         DBFreeStatement(hStmt);
      }
      else
      {
         if (!g_ignoreErrors)
            return false;
      }

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 39));
   }
   CHK_EXEC(SetMinorSchemaVersion(50));
   return true;
}

/**
 * Upgrade from 30.48 to 30.49 (changes also included into 22.38)
 */
static bool H_UpgradeFromV48()
{
   if (GetSchemaLevelForMajorVersion(22) < 38)
   {
      CHK_EXEC(CreateEventTemplate(EVENT_SERVER_STARTED, _T("SYS_SERVER_STARTED"), SEVERITY_NORMAL, EF_LOG, _T("32f3305b-1c1b-4597-9eb5-b74eca54330d"),
               _T("Server started"),
               _T("Generated when server initialization is completed.")
               ));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 38));
   }
   CHK_EXEC(SetMinorSchemaVersion(49));
   return true;
}

/**
 * Upgrade from 30.47 to 30.48 (changes also included into 22.36)
 */
static bool H_UpgradeFromV47()
{
   if (GetSchemaLevelForMajorVersion(22) < 36)
   {
      static const TCHAR *batch =
         _T("UPDATE object_properties SET state_before_maint=0 WHERE state_before_maint IS NULL\n")
         _T("UPDATE nodes SET port_rows=0 WHERE port_rows IS NULL\n")
         _T("UPDATE nodes SET port_numbering_scheme=0 WHERE port_numbering_scheme IS NULL\n")
         _T("UPDATE dct_threshold_instances SET row_number=0 WHERE row_number IS NULL\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("object_properties"), _T("state_before_maint")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("port_rows")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("port_numbering_scheme")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dct_threshold_instances"), _T("row_number")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 36));
   }
   CHK_EXEC(SetMinorSchemaVersion(48));
   return true;
}

/**
 * Upgrade from 30.46 to 30.47 (changes also included into 22.35)
 */
static bool H_UpgradeFromV46()
{
   if (GetSchemaLevelForMajorVersion(22) < 35)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='AgentTunnels.ListenPort',default_value='4703',description='TCP port number to listen on for incoming agent tunnel connections.' WHERE var_name='AgentTunnelListenPort'")));
      CHK_EXEC(CreateConfigParam(_T("AgentTunnels.ListenPort"), _T("4703"), _T("TCP port number to listen on for incoming agent tunnel connections."), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("Events.Correlation.TopologyBased"), _T("1"), _T("Enable/disable topology based event correlation."), NULL, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 35));
   }
   CHK_EXEC(SetMinorSchemaVersion(47));
   return true;
}

/*
 * Upgrade from 30.45 to 30.46
 */
static bool H_UpgradeFromV45()
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE auto_bind_target (")
      _T("   object_id integer not null,")
      _T("   bind_filter $SQL:TEXT null,")
      _T("   bind_flag char(1) null,")
      _T("   unbind_flag char(1) null,")
      _T("   PRIMARY KEY(object_id))")));

   CHK_EXEC(CreateTable(
      _T("CREATE TABLE versionable_object (")
      _T("   object_id integer not null,")
      _T("   version integer not null,")
      _T("   PRIMARY KEY(object_id))")));


   DB_STATEMENT stmtAutoBind = DBPrepare(g_dbHandle, _T("INSERT INTO auto_bind_target (object_id,bind_filter,bind_flag,unbind_flag) VALUES (?,?,?,?)"));
   DB_STATEMENT stmtVersion = DBPrepare(g_dbHandle, _T("INSERT INTO versionable_object (object_id,version) VALUES (?,?)"));
   DB_STATEMENT stmtFlags = DBPrepare(g_dbHandle, _T("UPDATE object_properties SET flags=? WHERE object_id=?"));
   //Template table upgrade
   bool success = true;
   DB_RESULT result = SQLSelect(_T("SELECT id,version,apply_filter FROM templates"));
   if (result != NULL)
   {
      if (stmtAutoBind != NULL && stmtVersion != NULL && stmtFlags != NULL)
      {
         int nRows = DBGetNumRows(result);
         for(int i = 0; i < nRows; i++)
         {
            UINT32 id = DBGetFieldULong(result, i, 0);
            //version
            DBBind(stmtVersion, 1, DB_SQLTYPE_INTEGER, id);
            DBBind(stmtVersion, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(result, i, 1));
            if (!(SQLExecute(stmtVersion)))
            {
               if (!g_ignoreErrors)
               {
                  success = false;
                  break;
               }
            }

            //flags
            UINT32 flags = 0;
            if(success)
            {
               TCHAR query[512];
               _sntprintf(query, 512, _T("SELECT flags FROM object_properties WHERE object_id=%d"), id);
               DB_RESULT flagResult = SQLSelect(query);
               if (flagResult != NULL)
               {
                  flags = DBGetFieldULong(flagResult, 0, 0);
                  DBBind(stmtFlags, 1, DB_SQLTYPE_INTEGER, (flags & ~3)); //remove auto apply flags
                  DBBind(stmtFlags, 2, DB_SQLTYPE_INTEGER, id);
                  if (!(SQLExecute(stmtFlags)))
                  {
                     if (!g_ignoreErrors)
                     {
                        DBFreeResult(flagResult);
                        success = false;
                        break;
                     }
                  }
                  DBFreeResult(flagResult);
               }
               else if (!g_ignoreErrors)
               {
                  success = false;
                  break;
               }
            }

            //autobind
            if(success)
            {
               DBBind(stmtAutoBind, 1, DB_SQLTYPE_INTEGER, id);
               DBBind(stmtAutoBind, 2, DB_SQLTYPE_TEXT, DBGetField(result, i, 2, NULL, 0), DB_BIND_DYNAMIC);
               DBBind(stmtAutoBind, 3, DB_SQLTYPE_VARCHAR, ((flags & 1) ? _T("1") : _T("0")), DB_BIND_STATIC);
               DBBind(stmtAutoBind, 4, DB_SQLTYPE_VARCHAR, ((flags & 2) ? _T("1") : _T("0")), DB_BIND_STATIC);
               if (!(SQLExecute(stmtAutoBind)))
               {
                  if (!g_ignoreErrors)
                  {
                     success = false;
                     break;
                  }
               }
            }
         }
      }
      else if (!g_ignoreErrors)
      {
         success = false;
      }
      DBFreeResult(result);
   }
   else if (!g_ignoreErrors)
      success = false;

   //Template table upgrade
   if(success)
   {
      result = SQLSelect(_T("SELECT id,auto_bind_filter FROM object_containers WHERE object_class=5"));
      if (result != NULL)
      {
         if (stmtAutoBind != NULL && stmtFlags != NULL)
         {
            int nRows = DBGetNumRows(result);
            for(int i = 0; i < nRows; i++)
            {
               UINT32 id = DBGetFieldULong(result, i, 0);
               //flags
               UINT32 flags = 0;
               TCHAR query[512];
               _sntprintf(query, 512, _T("SELECT flags FROM object_properties WHERE object_id=%d"), id);
               DB_RESULT flagResult = SQLSelect(query);
               if (flagResult != nullptr)
               {
                  flags = DBGetFieldULong(flagResult, 0, 0);
                  DBBind(stmtFlags, 1, DB_SQLTYPE_INTEGER, (flags & !3)); //remove auto apply flags
                  DBBind(stmtFlags, 2, DB_SQLTYPE_INTEGER, id);
                  if (!(SQLExecute(stmtFlags)))
                  {
                     if (!g_ignoreErrors)
                     {
                        DBFreeResult(flagResult);
                        success = false;
                        break;
                     }
                  }
                  DBFreeResult(flagResult);
               }
               else if (!g_ignoreErrors)
               {
                  success = false;
                  break;
               }

               //autobind
               if(success)
               {
                  DBBind(stmtAutoBind, 1, DB_SQLTYPE_INTEGER, id);
                  DBBind(stmtAutoBind, 2, DB_SQLTYPE_TEXT, DBGetField(result, i, 1, NULL, 0), DB_BIND_DYNAMIC);
                  DBBind(stmtAutoBind, 3, DB_SQLTYPE_VARCHAR, ((flags & 1) ? _T("1") : _T("0")), DB_BIND_STATIC);
                  DBBind(stmtAutoBind, 4, DB_SQLTYPE_VARCHAR, ((flags & 2) ? _T("1") : _T("0")), DB_BIND_STATIC);
                  if (!(SQLExecute(stmtAutoBind)))
                  {
                     if (!g_ignoreErrors)
                     {
                        success = false;
                        break;
                     }
                  }
               }
            }
         }
         DBFreeResult(result);
      }
      else if (!g_ignoreErrors)
         success = false;
   }

   //Policy table upgrade
   if(success)
   {
      result = SQLSelect(_T("SELECT id,version,deploy_filter FROM ap_common"));
      if (result != NULL)
      {
         if (stmtAutoBind != NULL && stmtVersion != NULL && stmtFlags != NULL)
         {
            int nRows = DBGetNumRows(result);
            for(int i = 0; i < nRows; i++)
            {
               UINT32 id = DBGetFieldULong(result, i, 0);
               //version
               DBBind(stmtVersion, 1, DB_SQLTYPE_INTEGER, id);
               DBBind(stmtVersion, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(result, i, 1));
               if (!(SQLExecute(stmtVersion)))
               {
                  if (!g_ignoreErrors)
                  {
                     success = false;
                     break;
                  }
               }

               //flags
               UINT32 flags = 0;
               if(success)
               {
                  TCHAR query[512];
                  _sntprintf(query, 512, _T("SELECT flags FROM object_properties WHERE object_id=%d"), id);
                  DB_RESULT flagResult = SQLSelect(query);
                  if (flagResult != NULL)
                  {
                     flags = DBGetFieldULong(flagResult, 0, 0);
                     DBBind(stmtFlags, 1, DB_SQLTYPE_INTEGER, (flags & ~3)); //remove auto apply flags
                     DBBind(stmtFlags, 2, DB_SQLTYPE_INTEGER, id);
                     if (!(SQLExecute(stmtFlags)))
                     {
                        if (!g_ignoreErrors)
                        {
                           DBFreeResult(flagResult);
                           success = false;
                           break;
                        }
                     }
                     DBFreeResult(flagResult);
                  }
                  else if (!g_ignoreErrors)
                  {
                     success = false;
                     break;
                  }
               }

               //autobind
               if(success)
               {
                  DBBind(stmtAutoBind, 1, DB_SQLTYPE_INTEGER, id);
                  DBBind(stmtAutoBind, 2, DB_SQLTYPE_TEXT, DBGetField(result, i, 2, NULL, 0), DB_BIND_DYNAMIC);
                  DBBind(stmtAutoBind, 3, DB_SQLTYPE_VARCHAR, ((flags & 1) ? _T("1") : _T("0")), DB_BIND_STATIC);
                  DBBind(stmtAutoBind, 4, DB_SQLTYPE_VARCHAR, ((flags & 2) ? _T("1") : _T("0")), DB_BIND_STATIC);
                  if (!(SQLExecute(stmtAutoBind)))
                  {
                     if (!g_ignoreErrors)
                     {
                        success = false;
                        break;
                     }
                  }
               }
            }
         }
         else if (!g_ignoreErrors)
         {
            success = false;
         }
         DBFreeResult(result);
      }
      else if (!g_ignoreErrors)
         success = false;
   }

   if(stmtAutoBind != NULL)
      DBFreeStatement(stmtAutoBind);
   if(stmtVersion != NULL)
      DBFreeStatement(stmtVersion);
   if(stmtFlags != NULL)
      DBFreeStatement(stmtFlags);

   if(!success)
      return false;

   CHK_EXEC(DBDropColumn(g_dbHandle, _T("templates"), _T("version")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("templates"), _T("apply_filter")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("ap_common"), _T("version")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("ap_common"), _T("deploy_filter")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("object_containers"), _T("auto_bind_filter")));

   CHK_EXEC(SetMinorSchemaVersion(46));
   return true;
}

/**
 * Upgrade from 30.44 to 30.45 (changes also included into 22.34)
 */
static bool H_UpgradeFromV44()
{
   if (GetSchemaLevelForMajorVersion(22) < 34)
   {
      static const TCHAR *batch =
         _T("ALTER TABLE event_log ADD raw_data $SQL:TEXT\n")
         _T("ALTER TABLE scheduled_tasks ADD task_key varchar(255)\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 34));
   }
   CHK_EXEC(SetMinorSchemaVersion(45));
   return true;
}

/**
 * Upgrade from 30.43 to 30.44 (changes also included into 22.33)
 */
static bool H_UpgradeFromV43()
{
   if (GetSchemaLevelForMajorVersion(22) < 33)
   {
      static const TCHAR *batch =
         _T("ALTER TABLE policy_action_list ADD timer_delay integer\n")
         _T("ALTER TABLE policy_action_list ADD timer_key varchar(127)\n")
         _T("UPDATE policy_action_list SET timer_delay=0\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("policy_action_list"), _T("timer_delay")));

      CHK_EXEC(CreateTable(
         _T("CREATE TABLE policy_timer_cancellation_list (")
         _T("   rule_id integer not null,")
         _T("   timer_key varchar(127) not null,")
         _T("   PRIMARY KEY(rule_id,timer_key))")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 33));
   }

   CHK_EXEC(SetMinorSchemaVersion(44));
   return true;
}

/**
 * Upgrade from 30.42 to 30.43 (changes also included into 22.32)
 */
static bool H_UpgradeFromV42()
{
   if (GetSchemaLevelForMajorVersion(22) < 32)
   {
      CHK_EXEC(CreateLibraryScript(17, _T("ee6dd107-982b-4ad1-980b-fc0cc7a03911"), _T("Hook::DiscoveryPoll"),
               _T("/* Available global variables:\r\n *  $node - current node, object of 'Node' type\r\n *\r\n * Expected return value:\r\n *  none - returned value is ignored\r\n */\r\n")));
      CHK_EXEC(CreateLibraryScript(18, _T("a02ea666-e1e9-4f98-a746-1c3ce19428e9"), _T("Hook::PostObjectCreate"),
               _T("/* Available global variables:\r\n *  $object - current object, one of 'NetObj' subclasses\r\n *  $node - current object if it is 'Node' class\r\n *\r\n * Expected return value:\r\n *  none - returned value is ignored\r\n */\r\n")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 32));
   }
   CHK_EXEC(SetMinorSchemaVersion(43));
   return true;
}

/**
 * Upgrade from 30.41 to 30.42 (changes also included into 22.31)
 */
static bool H_UpgradeFromV41()
{
   if (GetSchemaLevelForMajorVersion(22) < 31)
   {
      CHK_EXEC(CreateTable(
         _T("CREATE TABLE interface_vlan_list (")
         _T("   iface_id integer not null,")
         _T("   vlan_id integer not null,")
         _T("   PRIMARY KEY(iface_id,vlan_id))")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 31));
   }

   CHK_EXEC(SetMinorSchemaVersion(42));
   return true;
}

/**
 * Upgrade from 30.40 to 30.41 (changes also included into 22.30)
 */
static bool H_UpgradeFromV40()
{
   if (GetSchemaLevelForMajorVersion(22) < 30)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD hypervisor_type varchar(31)")));
      CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD hypervisor_info varchar(255)")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 30));
   }
   CHK_EXEC(SetMinorSchemaVersion(41));
   return true;
}

/**
 * Upgrade from 30.39 to 30.40 (changes also included into 22.29)
 */
static bool H_UpgradeFromV39()
{
   if (GetSchemaLevelForMajorVersion(22) < 29)
   {
      DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("UPDATE event_cfg SET description=? WHERE event_code=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_TEXT,
                  _T("Generated when agent tunnel is open and bound.\r\n")
                  _T("Parameters:\r\n")
                  _T("   1) Tunnel ID (tunnelId)\r\n")
                  _T("   2) Remote system IP address (ipAddress)\r\n")
                  _T("   3) Remote system name (systemName)\r\n")
                  _T("   4) Remote system FQDN (hostName)\r\n")
                  _T("   5) Remote system platform (platformName)\r\n")
                  _T("   6) Remote system information (systemInfo)\r\n")
                  _T("   7) Agent version (agentVersion)\r\n")
                  _T("   8) Agent ID (agentId)"),
                  DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, EVENT_TUNNEL_OPEN);
         bool success = DBExecute(hStmt);
         if (!success && !g_ignoreErrors)
         {
            DBFreeStatement(hStmt);
            return false;
         }

         DBBind(hStmt, 1, DB_SQLTYPE_TEXT,
                  _T("Generated when agent tunnel is closed.\r\n")
                  _T("Parameters:\r\n")
                  _T("   1) Tunnel ID (tunnelId)\r\n")
                  _T("   2) Remote system IP address (ipAddress)\r\n")
                  _T("   3) Remote system name (systemName)\r\n")
                  _T("   4) Remote system FQDN (hostName)\r\n")
                  _T("   5) Remote system platform (platformName)\r\n")
                  _T("   6) Remote system information (systemInfo)\r\n")
                  _T("   7) Agent version (agentVersion)\r\n")
                  _T("   8) Agent ID (agentId)"),
                  DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, EVENT_TUNNEL_CLOSED);
         success = DBExecute(hStmt);
         if (!success && !g_ignoreErrors)
         {
            DBFreeStatement(hStmt);
            return false;
         }

         DBBind(hStmt, 1, DB_SQLTYPE_TEXT,
                  _T("Generated when unbound agent tunnel is not bound or closed for more than configured threshold.\r\n")
                  _T("Parameters:\r\n")
                  _T("   1) Tunnel ID (tunnelId)\r\n")
                  _T("   2) Remote system IP address (ipAddress)\r\n")
                  _T("   3) Remote system name (systemName)\r\n")
                  _T("   4) Remote system FQDN (hostName)\r\n")
                  _T("   5) Remote system platform (platformName)\r\n")
                  _T("   6) Remote system information (systemInfo)\r\n")
                  _T("   7) Agent version (agentVersion)\r\n")
                  _T("   8) Agent ID (agentId)\r\n")
                  _T("   9) Configured idle timeout (idleTimeout)"),
                  DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, EVENT_UNBOUND_TUNNEL);
         success = DBExecute(hStmt);
         if (!success && !g_ignoreErrors)
         {
            DBFreeStatement(hStmt);
            return false;
         }

         DBFreeStatement(hStmt);
      }
      else
      {
         if (!g_ignoreErrors)
            return false;
      }

      CHK_EXEC(CreateEventTemplate(EVENT_TUNNEL_AGENT_ID_MISMATCH, _T("SYS_TUNNEL_AGENT_ID_MISMATCH"), SEVERITY_WARNING, EF_LOG, _T("30792e3d-f94a-4866-9616-457ba3ac276a"),
               _T("Agent ID %<nodeAgentId> on node do not match agent ID %<tunnelAgentId> on tunnel from %<systemName> (%<ipAddress>) at bind"),
               _T("Generated when agent ID mismatch detected during tunnel bind.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Tunnel ID (tunnelId)\r\n")
               _T("   2) Remote system IP address (ipAddress)\r\n")
               _T("   3) Remote system name (systemName)\r\n")
               _T("   4) Remote system FQDN (hostName)\r\n")
               _T("   5) Remote system platform (platformName)\r\n")
               _T("   6) Remote system information (systemInfo)\r\n")
               _T("   7) Agent version (agentVersion)\r\n")
               _T("   8) Tunnel agent ID (tunnelAgentId)\r\n")
               _T("   9) Node agent ID (nodeAgentId)")
               ));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 29));
   }
   CHK_EXEC(SetMinorSchemaVersion(40));
   return true;
}

/**
 * Upgrade from 30.38 to 30.39 (changes also included into 22.28)
 */
static bool H_UpgradeFromV38()
{
   if (GetSchemaLevelForMajorVersion(22) < 28)
   {
      CHK_EXEC(CreateEventTemplate(EVENT_TUNNEL_OPEN, _T("SYS_TUNNEL_OPEN"), SEVERITY_NORMAL, EF_LOG, _T("2569c729-1f8c-4a13-9e75-a1d0c1995bc2"),
               _T("Agent tunnel from %<systemName> (%<ipAddress>) is open"),
               _T("Generated when agent tunnel is open and bound.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Tunnel ID (tunnelId)\r\n")
               _T("   2) Remote system IP address (ipAddress)\r\n")
               _T("   3) Remote system name (systemName)\r\n")
               _T("   4) Remote system FQDN (hostName)\r\n")
               _T("   5) Remote system platform (platformName)\r\n")
               _T("   6) Remote system information (systemInfo)\r\n")
               _T("   7) Agent version (agentVersion)")
               ));
      CHK_EXEC(CreateEventTemplate(EVENT_TUNNEL_CLOSED, _T("SYS_TUNNEL_CLOSED"), SEVERITY_WARNING, EF_LOG, _T("50a61266-710d-48d7-b620-9eaa0f85a94f"),
               _T("Agent tunnel from %<systemName> (%<ipAddress>) is closed"),
               _T("Generated when agent tunnel is closed.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Tunnel ID (tunnelId)\r\n")
               _T("   2) Remote system IP address (ipAddress)\r\n")
               _T("   3) Remote system name (systemName)\r\n")
               _T("   4) Remote system FQDN (hostName)\r\n")
               _T("   5) Remote system platform (platformName)\r\n")
               _T("   6) Remote system information (systemInfo)\r\n")
               _T("   7) Agent version (agentVersion)")
               ));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 28));
   }
   CHK_EXEC(SetMinorSchemaVersion(39));
   return true;
}

/**
 * Upgrade from 30.37 to 30.38 (changes also included into 22.27)
 */
static bool H_UpgradeFromV37()
{
   if (GetSchemaLevelForMajorVersion(22) < 27)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='Client.AlarmList.DisplayLimit' WHERE var_name='AlarmListDisplayLimit'")));
      CHK_EXEC(CreateConfigParam(_T("Client.ObjectBrowser.AutoApplyFilter"), _T("1"), _T("Enable or disable object browser's filter applying as user types (if disabled, user has to press ENTER to apply filter)."), NULL, 'B', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("Client.ObjectBrowser.FilterDelay"), _T("300"), _T("Delay between typing in object browser''s filter and applying it to object tree."), _T("milliseconds"), 'I', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("Client.ObjectBrowser.MinFilterStringLength"), _T("1"), _T("Minimal length of filter string in object browser required for automatic apply."), _T("characters"), 'I', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 27));
   }
   else
   {
      static const TCHAR *batch =
         _T("UPDATE config SET units='milliseconds' WHERE var_name='Client.ObjectBrowser.FilterDelay'\n")
         _T("UPDATE config SET units='characters' WHERE var_name='Client.ObjectBrowser.MinFilterStringLength'\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
   }
   CHK_EXEC(SetMinorSchemaVersion(38));
   return true;
}

/**
 * Upgrade from 30.36 to 30.37 (changes also included into 22.26)
 */
static bool H_UpgradeFromV36()
{
   if (GetSchemaLevelForMajorVersion(22) < 26)
   {
      CHK_EXEC(CreateTable(
         _T("CREATE TABLE certificate_action_log (")
         _T("   record_id integer not null,")
         _T("   timestamp integer not null,")
         _T("   operation integer not null,")
         _T("   user_id integer not null,")
         _T("   node_id integer not null,")
         _T("   node_guid varchar(36) null,")
         _T("   cert_type integer not null,")
         _T("   subject varchar(255) null,")
         _T("   serial integer null,")
         _T("   PRIMARY KEY(record_id))")));

      CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD agent_id varchar(36)")));
      CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD agent_cert_subject varchar(500)")));

      CHK_EXEC(CreateEventTemplate(EVENT_AGENT_ID_CHANGED, _T("SYS_AGENT_ID_CHANGED"), SEVERITY_WARNING, EF_LOG,
               _T("741f0abc-1e69-46e4-adbc-bf1c4ed8549a"),
               _T("Agent ID changed from %1 to %2"),
               _T("Generated when agent ID change detected.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Old agent ID\r\n")
               _T("   2) New agent ID")
               ));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 26));
   }
   CHK_EXEC(SetMinorSchemaVersion(37));
   return true;
}

/**
 * Upgrade from 30.35 to 30.36 (changes also included into 22.25)
 */
static bool H_UpgradeFromV35()
{
   if (GetSchemaLevelForMajorVersion(22) < 25)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE actions ADD guid varchar(36)")));
      CHK_EXEC(GenerateGUID(_T("actions"), _T("action_id"), _T("guid"), NULL));
      DBSetNotNullConstraint(g_dbHandle, _T("actions"), _T("guid"));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 25));
   }
   CHK_EXEC(SetMinorSchemaVersion(36));
   return true;
}

/**
 * Upgrade from 30.34 to 30.35 (changes also included into 22.24)
 */
static bool H_UpgradeFromV34()
{
   if (GetSchemaLevelForMajorVersion(22) < 24)
   {
      CHK_EXEC(DBRenameColumn(g_dbHandle, _T("dct_threshold_instances"), _T("row"), _T("row_number")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 24));
   }
   CHK_EXEC(SetMinorSchemaVersion(35));
   return true;
}

/**
 * Upgrade from 30.33 to 30.35 (changes also included into 22.23)
 */
static bool H_UpgradeFromV33()
{
   if (GetSchemaLevelForMajorVersion(22) < 23)
   {
      static const TCHAR *batch =
         _T("ALTER TABLE object_properties ADD state_before_maint integer\n")
         _T("ALTER TABLE dct_threshold_instances ADD row_number integer\n")
         _T("ALTER TABLE dct_threshold_instances ADD maint_copy char(1)\n")
         _T("ALTER TABLE thresholds ADD state_before_maint char(1)\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(DBDropColumn(g_dbHandle, _T("object_properties"), _T("maint_mode")));
      CHK_EXEC(SQLQuery(_T("UPDATE dct_threshold_instances SET maint_copy='0'")));
      CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("dct_threshold_instances")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dct_threshold_instances"), _T("maint_copy")));
      CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("dct_threshold_instances"), _T("threshold_id,instance,maint_copy")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 24));
   }
   CHK_EXEC(SetMinorSchemaVersion(35));
   return true;
}

/**
 * Upgrade from 30.32 to 30.33 (changes also included into 22.22)
 */
static bool H_UpgradeFromV32()
{
   if (GetSchemaLevelForMajorVersion(22) < 22)
   {
      CHK_EXEC(CreateConfigParam(_T("Alarms.IgnoreHelpdeskState"), _T("0"), _T("If set alarm helpdesk state will be ignored when resolving or terminating."), NULL, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 22));
   }
   CHK_EXEC(SetMinorSchemaVersion(33));
   return true;
}

/**
 * Upgrade from 30.31 to 30.32 (changes also included into 22.21)
 */
static bool H_UpgradeFromV31()
{
   if (GetSchemaLevelForMajorVersion(22) < 21)
   {
      static const TCHAR *batch =
         _T("ALTER TABLE alarms ADD zone_uin integer\n")
         _T("ALTER TABLE event_log ADD zone_uin integer\n")
         _T("ALTER TABLE snmp_trap_log ADD zone_uin integer\n")
         _T("ALTER TABLE syslog ADD zone_uin integer\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      RegisterOnlineUpgrade(22, 21);

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 21));
   }
   CHK_EXEC(SetMinorSchemaVersion(32));
   return true;
}

/**
 * Upgrade from 30.30 to 30.31 (changes also included into 22.20)
 */
static bool H_UpgradeFromV30()
{
   if (GetSchemaLevelForMajorVersion(22) < 20)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config_values SET var_description='Don''t use aliases' WHERE var_name='UseInterfaceAliases' AND var_value='0'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET description='Resolve node name using DNS, SNMP system name, or host name if current node name is it''s IP address.' WHERE var_name='ResolveNodeNames'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET description='Number of hops from seed node to be added to topology map.' WHERE var_name='TopologyDiscoveryRadius'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET description='Network topology information expiration time. Server will use cached topology information if it is newer than given interval.' WHERE var_name='TopologyExpirationTime'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 20));
   }
   CHK_EXEC(SetMinorSchemaVersion(31));
   return true;
}

/**
 * Upgrade from 30.29 to 30.30 (changes also included into 22.19)
 */
static bool H_UpgradeFromV29()
{
   if (GetSchemaLevelForMajorVersion(22) < 19)
   {
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Syncer.BaseSize"), _T("1"), _T("Base size for syncer thread pool."), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Syncer.MaxSize"), _T("1"), _T("Maximum size for syncer thread pool (value of 1 will disable pool creation)."), NULL, 'I', true, true, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 19));
   }
   CHK_EXEC(SQLQuery(_T("UPDATE config SET units='' WHERE var_name like 'ThreadPool.%'")));
   CHK_EXEC(SetMinorSchemaVersion(30));
   return true;
}

/**
 * Upgrade from 30.28 to 30.29 (changes also included into 22.18)
 */
static bool H_UpgradeFromV28()
{
   if (GetSchemaLevelForMajorVersion(22) < 18)
   {
      CHK_EXEC(CreateConfigParam(_T("DBWriter.RawDataFlushInterval"), _T("30"), _T("Interval between writes of accumulated raw DCI data to database."), _T("seconds"), 'I', true, true, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 18));
   }
   CHK_EXEC(SQLQuery(_T("UPDATE config SET units='seconds' WHERE var_name='DBWriter.RawDataFlushInterval'")));
   CHK_EXEC(SetMinorSchemaVersion(29));
   return true;
}

/**
 * Upgrade from 30.27 to 30.28 (changes also included into 22.17)
 */
static bool H_UpgradeFromV27()
{
   if (GetSchemaLevelForMajorVersion(22) < 17)
   {
      CHK_EXEC(CreateConfigParam(_T("DBWriter.DataQueues"), _T("1"), _T("Number of queues for DCI data writer."), NULL, 'I', true, true, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 17));
   }
   CHK_EXEC(SetMinorSchemaVersion(28));
   return true;
}

/**
 * Upgrade from 30.26 to 30.27
 */
static bool H_UpgradeFromV26()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE responsible_users (")
         _T("   object_id integer not null,")
         _T("   user_id integer not null,")
         _T("   PRIMARY KEY(object_id,user_id))")));
   CHK_EXEC(SetMinorSchemaVersion(27));
   return true;
}

/**
 * Upgrade from 30.25 to 30.26 (changes also included into 22.16)
 */
static bool H_UpgradeFromV25()
{
   if (GetSchemaLevelForMajorVersion(22) < 16)
   {
      CHK_EXEC(CreateConfigParam(_T("DataCollection.ScriptErrorReportInterval"), _T("86400"), _T("Minimal interval between reporting errors in data collection related script."), _T("seconds"), 'I', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 16));
   }
   CHK_EXEC(SQLQuery(_T("UPDATE config SET units='seconds' WHERE var_name='DataCollection.ScriptErrorReportInterval'")));
   CHK_EXEC(SetMinorSchemaVersion(26));
   return true;
}

/**
 * Upgrade from 30.24 to 30.25 (changes also included into 22.15)
 */
static bool H_UpgradeFromV24()
{
   if (GetSchemaLevelForMajorVersion(22) < 15)
   {
      CHK_EXEC(CreateConfigParam(_T("NXSL.EnableFileIOFunctions"), _T("0"), _T("Enable/disable server-side NXSL functions for file I/O (such as OpenFile, DeleteFile, etc.)."), NULL, 'B', true, true, false, false));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='NXSL.EnableContainerFunctions',description='Enable/disable server-side NXSL functions for container management (such as CreateContainer, RemoveContainer, BindObject, UnbindObject).' WHERE var_name='EnableNXSLContainerFunctions'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 15));
   }
   CHK_EXEC(SetMinorSchemaVersion(25));
   return true;
}

/**
 * Upgrade from 30.23 to 30.24 (changes also included into 22.14)
 */
static bool H_UpgradeFromV23()
{
   if (GetSchemaLevelForMajorVersion(22) < 14)
   {
      CHK_EXEC(CreateConfigParam(_T("DataCollection.OnDCIDelete.TerminateRelatedAlarms"), _T("1"), _T("Enable/disable automatic termination of related alarms when data collection item is deleted."), NULL, 'B', true, false, false, false));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Inactivity time after which user account will be blocked (0 to disable blocking).' WHERE var_name='BlockInactiveUserAccounts'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 14));
   }
   CHK_EXEC(SQLQuery(_T("UPDATE config SET units='days' WHERE var_name='BlockInactiveUserAccounts'")));
   CHK_EXEC(SetMinorSchemaVersion(24));
   return true;
}

/**
 * Upgrade from 30.22 to 30.23 (changes also included into 22.13)
 */
static bool H_UpgradeFromV22()
{
   if (GetSchemaLevelForMajorVersion(22) < 13)
   {
      CHK_EXEC(CreateEventTemplate(EVENT_UNBOUND_TUNNEL, _T("SYS_UNBOUND_TUNNEL"), SEVERITY_NORMAL, EF_LOG, _T("7f781ec2-a8f5-4c02-ad7f-9e5b0a223b87"),
               _T("Unbound agent tunnel from %<systemName> (%<ipAddress>) is idle for more than %<idleTimeout> seconds"),
               _T("Generated when unbound agent tunnel is not bound or closed for more than configured threshold.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Tunnel ID (tunnelId)\r\n")
               _T("   2) Remote system IP address (ipAddress)\r\n")
               _T("   3) Remote system name (systemName)\r\n")
               _T("   4) Remote system FQDN (hostName)\r\n")
               _T("   5) Remote system platform (platformName)\r\n")
               _T("   6) Remote system information (systemInfo)\r\n")
               _T("   7) Agent version (agentVersion)\r\n")
               _T("   8) Configured idle timeout (idleTimeout)")
               ));

      CHK_EXEC(CreateConfigParam(_T("AgentTunnels.NewNodesContainer"), _T(""), _T("Name of the container where nodes created automatically for unbound tunnels will be placed. If empty or missing, such nodes will be created in infrastructure services root."), NULL, 'S', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("AgentTunnels.UnboundTunnelTimeout"), _T("3600"), _T("Unbound agent tunnels inactivity timeout. If tunnel is not bound or closed after timeout, action defined by AgentTunnels.UnboundTunnelTimeoutAction parameter will be taken."), _T("seconds"), 'I', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("AgentTunnels.UnboundTunnelTimeoutAction"), _T("0"), _T("Action to be taken when unbound agent tunnel idle timeout expires."), NULL, 'C', true, false, false, false));

      static TCHAR batch[] =
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.UnboundTunnelTimeoutAction','0','Reset tunnel')\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.UnboundTunnelTimeoutAction','1','Generate event')\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.UnboundTunnelTimeoutAction','2','Bind tunnel to existing node')\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.UnboundTunnelTimeoutAction','3','Bind tunnel to existing node or create new node')\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET description='Generated when new node object added to the database.\r\nParameters:\r\n   1) Node origin (0 = created manually, 1 = created by network discovery, 2 = created by tunnel auto bind)' WHERE event_code=1")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 13));
   }
   CHK_EXEC(SetMinorSchemaVersion(23));
   return true;
}

/**
 * Upgrade from 30.21 to 30.22
 */
static bool H_UpgradeFromV21()
{
   static TCHAR batch[] =
      _T("ALTER TABLE config ADD units varchar(36)\n")
      _T("UPDATE config SET units='milliseconds' WHERE var_name='AgentCommandTimeout'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='AgentTunnels.UnboundTunnelTimeout'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='AgentUpgradeWaitTime'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='AlarmHistoryRetentionTime'\n")
      _T("UPDATE config SET units='alarms' WHERE var_name='AlarmListDisplayLimit'\n")
      _T("UPDATE config SET units='days' WHERE var_name='AuditLogRetentionTime'\n")
      _T("UPDATE config SET units='milliseconds' WHERE var_name='BeaconPollingInterval'\n")
      _T("UPDATE config SET units='milliseconds' WHERE var_name='BeaconTimeout'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='CapabilityExpirationTime'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='ConditionPollingInterval'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='ConfigurationPollingInterval'\n")
      _T("UPDATE config SET units='connections' WHERE var_name='DBConnectionPoolBaseSize'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='DBConnectionPoolCooldownTime'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='DBConnectionPoolMaxLifetime'\n")
      _T("UPDATE config SET units='connections' WHERE var_name='DBConnectionPoolMaxSize'\n")
      _T("UPDATE config SET units='records/transaction' WHERE var_name='DBWriter.MaxRecordsPerTransaction'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='DefaultDCIPollingInterval'\n")
      _T("UPDATE config SET units='days' WHERE var_name='DefaultDCIRetentionTime'\n")
      _T("UPDATE config SET units='days' WHERE var_name='DeleteUnreachableNodesPeriod'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='DiscoveryPollingInterval'\n")
      _T("UPDATE config SET units='days' WHERE var_name='EventLogRetentionTime'\n")
      _T("UPDATE config SET units='events/second' WHERE var_name='EventStormEventsPerSecond'\n")
      _T("UPDATE config SET units='logins' WHERE var_name='GraceLoginCount'\n")
      _T("UPDATE config SET units='size' WHERE var_name='IcmpPingSize'\n")
      _T("UPDATE config SET units='milliseconds' WHERE var_name='IcmpPingTimeout'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='InstancePollingInterval'\n")
      _T("UPDATE config SET units='days' WHERE var_name='InstanceRetentionTime'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='IntruderLockoutTime'\n")
      _T("UPDATE config SET units='days' WHERE var_name='JobHistoryRetentionTime'\n")
      _T("UPDATE config SET units='retries' WHERE var_name='JobRetryCount'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='KeepAliveInterval'\n")
      _T("UPDATE config SET units='milliseconds' WHERE var_name='LongRunningQueryThreshold'\n")
      _T("UPDATE config SET units='characters' WHERE var_name='MinPasswordLength'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='MinViewRefreshInterval'\n")
      _T("UPDATE config SET units='threads' WHERE var_name='NumberOfUpgradeThreads'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='OfflineDataRelevanceTime'\n")
      _T("UPDATE config SET units='polls' WHERE var_name='PollCountForStatusChange'\n")
      _T("UPDATE config SET units='retries' WHERE var_name='RADIUSNumRetries'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='RADIUSTimeout'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='RoutingTableUpdateInterval'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='ServerCommandOutputTimeout'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='StatusPollingInterval'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='SyncInterval'\n")
      _T("UPDATE config SET units='days' WHERE var_name='SyslogRetentionTime'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.Agent.BaseSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.Agent.MaxSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.DataCollector.BaseSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.DataCollector.MaxSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.Main.BaseSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.Main.MaxSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.Poller.BaseSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.Poller.MaxSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.Scheduler.BaseSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.Scheduler.MaxSize'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='ThresholdRepeatInterval'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='TopologyExpirationTime'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='TopologyPollingInterval'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetMinorSchemaVersion(22));
   return true;
}

/**
 * Upgrade from 30.20 to 30.21 (changes also included into 22.12)
 */
static bool H_UpgradeFromV20()
{
   if (GetSchemaLevelForMajorVersion(22) < 12)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE ap_common ADD deploy_filter $SQL:TEXT")));

      CHK_EXEC(CreateEventTemplate(EVENT_POLICY_AUTODEPLOY, _T("SYS_POLICY_AUTODEPLOY"), SEVERITY_NORMAL, EF_LOG, _T("f26d70b3-d48d-472c-b2ec-bfa7c20958ea"),
               _T("Agent policy %4 automatically deployed to node %2"),
               _T("Generated when agent policy deployed to node by autodeploy rule.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Node ID\r\n")
               _T("   2) Node name\r\n")
               _T("   3) Policy ID\r\n")
               _T("   4) Policy name")
               ));

      CHK_EXEC(CreateEventTemplate(EVENT_POLICY_AUTOUNINSTALL, _T("SYS_POLICY_AUTOUNINSTALL"), SEVERITY_NORMAL, EF_LOG, _T("2fbac886-2cfa-489f-bef1-364a38fa7062"),
               _T("Agent policy %4 automatically uninstalled from node %2"),
               _T("Generated when agent policy uninstalled from node by autodeploy rule.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Node ID\r\n")
               _T("   2) Node name\r\n")
               _T("   3) Policy ID\r\n")
               _T("   4) Policy name")
               ));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 12));
   }
   CHK_EXEC(SetMinorSchemaVersion(21));
   return true;
}

/**
 * Upgrade from 30.19 to 30.20 (changes also included into 22.11)
 */
static bool H_UpgradeFromV19()
{
   if (GetSchemaLevelForMajorVersion(22) < 11)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='Housekeeper.StartTime' WHERE var_name='HousekeeperStartTime'")));
      CHK_EXEC(CreateConfigParam(_T("Housekeeper.Throttle.HighWatermark"), _T("250000"), _T("High watermark for housekeeper throttling"), NULL, 'I', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("Housekeeper.Throttle.LowWatermark"), _T("50000"), _T("Low watermark for housekeeper throttling"), NULL, 'I', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 11));
   }
   CHK_EXEC(SetMinorSchemaVersion(20));
   return true;
}

/**
 * Upgrade from 30.18 to 30.19 (changes also included into 22.10)
 */
static bool H_UpgradeFromV18()
{
   if (GetSchemaLevelForMajorVersion(22) < 10)
   {
      static TCHAR batch[] =
         _T("UPDATE config SET var_name='ThreadPool.DataCollector.BaseSize' WHERE var_name='DataCollector.ThreadPool.BaseSize'\n")
         _T("UPDATE config SET var_name='ThreadPool.DataCollector.MaxSize' WHERE var_name='DataCollector.ThreadPool.MaxSize'\n")
         _T("UPDATE config SET var_name='ThreadPool.Poller.BaseSize',description='Base size for poller thread pool' WHERE var_name='PollerThreadPoolBaseSize'\n")
         _T("UPDATE config SET var_name='ThreadPool.Poller.MaxSize',description='Maximum size for poller thread pool' WHERE var_name='PollerThreadPoolMaxSize'\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Agent.BaseSize"), _T("4"), _T("Base size for agent connector thread pool"), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Agent.MaxSize"), _T("256"), _T("Maximum size for agent connector thread pool"), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Main.BaseSize"), _T("8"), _T("Base size for main server thread pool"), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Main.MaxSize"), _T("256"), _T("Maximum size for main server thread pool"), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Scheduler.BaseSize"), _T("1"), _T("Base size for scheduler thread pool"), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Scheduler.MaxSize"), _T("64"), _T("Maximum size for scheduler thread pool"), NULL, 'I', true, true, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 10));
   }
   CHK_EXEC(SetMinorSchemaVersion(19));
   return true;
}

/**
 * Upgrade from 30.17 to 30.18 (changes also included into 22.9)
 */
static bool H_UpgradeFromV17()
{
   if (GetSchemaLevelForMajorVersion(22) < 9)
   {
      CHK_EXEC(DBResizeColumn(g_dbHandle, _T("nodes"), _T("lldp_id"), 255, true));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 9));
   }
   CHK_EXEC(SetMinorSchemaVersion(18));
   return true;
}

/**
 * Upgrade from 30.16 to 30.17 (changes also included into 22.8)
 */
static bool H_UpgradeFromV16()
{
   if (GetSchemaLevelForMajorVersion(22) < 8)
   {
      static TCHAR batch[] =
         _T("ALTER TABLE nodes ADD rack_image_rear varchar(36)\n")
         _T("ALTER TABLE chassis ADD rack_image_rear varchar(36)\n")
         _T("UPDATE nodes SET rack_image_rear='00000000-0000-0000-0000-000000000000'\n")
         _T("UPDATE chassis SET rack_image_rear='00000000-0000-0000-0000-000000000000'\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBRenameColumn(g_dbHandle, _T("nodes"), _T("rack_image"), _T("rack_image_front")));
      CHK_EXEC(DBRenameColumn(g_dbHandle, _T("chassis"), _T("rack_image"), _T("rack_image_front")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 8));
   }
   CHK_EXEC(SetMinorSchemaVersion(17));
   return true;
}

/**
 * Upgrade from 30.15 to 30.16
 */
static bool H_UpgradeFromV15()
{
   static TCHAR batch[] =
      _T("UPDATE config SET default_value='2' WHERE var_name='DefaultEncryptionPolicy'\n")
      _T("UPDATE config SET var_value='2' WHERE var_name='DefaultEncryptionPolicy' AND var_value!='3'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Upgrade from 30.14 to 30.15
 */
static bool H_UpgradeFromV14()
{
   if (GetSchemaLevelForMajorVersion(22) < 6)
   {
      static TCHAR batch[] =
         _T("ALTER TABLE racks ADD passive_elements $SQL:TEXT\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 6));
   }
   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade from 30.13 to 30.14 (changes also included into 22.5)
 */
static bool H_UpgradeFromV13()
{
   if (GetSchemaLevelForMajorVersion(22) < 5)
   {
      static const TCHAR *batch =
               _T("ALTER TABLE items ADD instance_retention_time integer\n")
               _T("ALTER TABLE dc_tables ADD instance_retention_time integer\n")
               _T("UPDATE items SET instance_retention_time=-1\n")
               _T("UPDATE dc_tables SET instance_retention_time=-1\n")
               _T("INSERT INTO config (var_name,var_value,default_value,is_visible,need_server_restart,is_public,data_type,description) ")
               _T("VALUES ('InstanceRetentionTime','0','0',1,1,'Y','I','Default retention time (in days) for missing DCI instances')\n")
               _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("items"), _T("instance_retention_time")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dc_tables"), _T("instance_retention_time")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 5));
   }
   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Upgrade from 30.12 to 30.13  (changes also included into 21.5 and 22.4)
 */
static bool H_UpgradeFromV12()
{
   if ((GetSchemaLevelForMajorVersion(21) < 5) && (GetSchemaLevelForMajorVersion(22) < 4))
   {
      static const TCHAR *batch =
               _T("ALTER TABLE nodes ADD rack_orientation integer\n")
               _T("ALTER TABLE chassis ADD rack_orientation integer\n")
               _T("UPDATE nodes SET rack_orientation=0\n")
               _T("UPDATE chassis SET rack_orientation=0\n")
               _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("rack_orientation")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("chassis"), _T("rack_orientation")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 4));
      CHK_EXEC(SetSchemaLevelForMajorVersion(21, 5));
   }
   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

/**
 * Upgrade from 30.11 to 30.12
 */
static bool H_UpgradeFromV11()
{
   if (GetSchemaLevelForMajorVersion(22) < 3)
   {
      CHK_EXEC(CreateTable(
            _T("CREATE TABLE dci_access (")
            _T("   dci_id integer not null,")
            _T("   user_id integer not null,")
            _T("   PRIMARY KEY(dci_id,user_id))")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 3));
   }

   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 30.10 to 30.11
 */
static bool H_UpgradeFromV10()
{
   CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='CREATE TABLE idata_%d (item_id integer not null,idata_timestamp integer not null,idata_value varchar(255) null,raw_value varchar(255) null)' WHERE var_name='IDataTableCreationCommand'")));
   if (DBMgrMetaDataReadInt32(_T("SingeTablePerfData"), 0) == 0)
   {
      IntegerArray<UINT32> *targets = GetDataCollectionTargets();
      for(int i = 0; i < targets->size(); i++)
      {
         TCHAR query[256];
         _sntprintf(query, 256, _T("ALTER TABLE idata_%d ADD raw_value varchar(255)"), targets->get(i));
         CHK_EXEC(SQLQuery(query));
      }
      delete targets;
   }
   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 30.9 to 30.10
 */
static bool H_UpgradeFromV9()
{
   static const TCHAR *batch =
            _T("ALTER TABLE snmp_communities ADD zone integer null\n")
            _T("ALTER TABLE usm_credentials ADD zone integer null\n")
            _T("ALTER TABLE zones ADD snmp_ports varchar(512) null\n")
            _T("UPDATE snmp_communities SET zone=-1\n")
            _T("UPDATE usm_credentials SET zone=-1\n")
            _T("UPDATE zones SET snmp_ports=''\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   DBSetNotNullConstraint(g_dbHandle, _T("snmp_communities"), _T("zone"));
   DBSetNotNullConstraint(g_dbHandle, _T("usm_credentials"), _T("zone"));

   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 30.8 to 30.9 (changes also included into 22.2)
 */
static bool H_UpgradeFromV8()
{
   if (GetSchemaLevelForMajorVersion(22) < 2)
   {
      CHK_EXEC(CreateConfigParam(_T("DBWriter.MaxRecordsPerTransaction"), _T("1000"), _T("Maximum number of records per one transaction for delayed database writes."), NULL, 'I', true, true, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 2));
   }
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 30.7 to 30.8 (changes also included into 22.1)
 */
static bool H_UpgradeFromV7()
{
   if (GetSchemaLevelForMajorVersion(22) < 1)
   {
      int count = DBMgrConfigReadInt32(_T("NumberOfDataCollectors"), 250);
      TCHAR value[64];
      _sntprintf(value, 64,_T("%d"), std::max(250, count));
      CHK_EXEC(CreateConfigParam(_T("DataCollector.ThreadPool.BaseSize"), _T("10"), _T("Base size for data collector thread pool."), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("DataCollector.ThreadPool.MaxSize"), value, _T("Maximum size for data collector thread pool."), NULL, 'I', true, true, false, false));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET default_value='250' WHERE var_name='DataCollector.ThreadPool.MaxSize'")));
      CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='NumberOfDataCollectors'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 1));
   }
   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 30.6 to 30.7 (changes also included into 21.4 and all 22.x)
 */
static bool H_UpgradeFromV6()
{
   if ((GetSchemaLevelForMajorVersion(21) < 4) && (GetSchemaLevelForMajorVersion(22) < 1))
   {
      DB_RESULT hResult = SQLSelect(_T("SELECT access_rights,object_id FROM acl WHERE user_id=-2147483647")); // Get group Admins object acl
      if (hResult != nullptr)
      {
         DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("UPDATE acl SET access_rights=? WHERE user_id=-2147483647 AND object_id=? "));
         if (hStmt != nullptr)
         {
            int nRows = DBGetNumRows(hResult);
            UINT32 rights;
            for(int i = 0; i < nRows; i++)
            {
               rights = DBGetFieldULong(hResult, i, 0);
               if (rights & OBJECT_ACCESS_READ)
               {
                  rights |= (OBJECT_ACCESS_READ_AGENT | OBJECT_ACCESS_READ_SNMP | OBJECT_ACCESS_SCREENSHOT);
                  DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, rights);
                  DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 1));

                  if (!SQLExecute(hStmt))
                  {
                     if (!g_ignoreErrors)
                     {
                        DBFreeStatement(hStmt);
                        DBFreeResult(hResult);
                        return FALSE;
                     }
                  }
               }
            }

            DBFreeStatement(hStmt);
         }
         else if (!g_ignoreErrors)
            return FALSE;
         DBFreeResult(hResult);
      }
      else if (!g_ignoreErrors)
         return false;
      CHK_EXEC(SetSchemaLevelForMajorVersion(21, 4));
   }
   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 30.5 to 30.6 (changes also included into 21.3 and all 22.x)
 */
static bool H_UpgradeFromV5()
{
   if ((GetSchemaLevelForMajorVersion(21) < 3) && (GetSchemaLevelForMajorVersion(22) < 1))
   {
      static const TCHAR *batch =
               _T("UPDATE nodes SET fail_time_snmp=0 WHERE fail_time_snmp IS NULL\n")
               _T("UPDATE nodes SET fail_time_agent=0 WHERE fail_time_agent IS NULL\n")
               _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("fail_time_snmp")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("fail_time_agent")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(21, 3));
   }
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 30.4 to 30.5 (changes also included into 21.2 and all 22.x)
 */
static bool H_UpgradeFromV4()
{
   if ((GetSchemaLevelForMajorVersion(21) < 2) && (GetSchemaLevelForMajorVersion(22) < 1))
   {
      static const TCHAR *batch =
               _T("ALTER TABLE nodes ADD fail_time_snmp integer\n")
               _T("ALTER TABLE nodes ADD fail_time_agent integer\n")
               _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(SetSchemaLevelForMajorVersion(21, 2));
   }
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Move object flags from old to new tables
 */
static bool MoveFlagsFromOldTables(const TCHAR *tableName)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT id,flags FROM %s"), tableName);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult != nullptr)
   {
      DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("UPDATE object_properties SET flags=? WHERE object_id=?"));
      if (hStmt != nullptr)
      {
         int nRows = DBGetNumRows(hResult);
         for(int i = 0; i < nRows; i++)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 1));
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 0));

            if (!SQLExecute(hStmt))
            {
               if (!g_ignoreErrors)
               {
                  DBFreeStatement(hStmt);
                  DBFreeResult(hResult);
                  return FALSE;
               }
            }
         }
         DBFreeStatement(hStmt);
      }
      else if (!g_ignoreErrors)
      {
         return false;
      }
      DBFreeResult(hResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(DBDropColumn(g_dbHandle, tableName, _T("flags")));
   return true;
}

/**
 * Move single flag
 */
static inline void MoveFlag(uint32_t oldVar, uint32_t *newVar, uint32_t oldFlag, uint32_t newFlag)
{
   *newVar |= ((oldVar & oldFlag) != 0) ? newFlag : 0;
}

/**
 * Move node flags
 */
static void MoveNodeFlags(uint32_t oldFlag, uint32_t *flags, bool withSnmpConfLock)
{
   MoveFlag(oldFlag, flags, 0x10000000, 0x00000001);  // disable status poll
   MoveFlag(oldFlag, flags, 0x20000000, 0x00000002);  // disable configuration poll
   MoveFlag(oldFlag, flags, 0x80000000, 0x00000004);  // disable data collection
   MoveFlag(oldFlag, flags, 0x00000080, 0x00010000);  // external gateway
   if (withSnmpConfLock)
   {
      MoveFlag(oldFlag, flags, 0x00200000, 0x02000000);  // SNMP settings locked
   }
   MoveFlag(oldFlag, flags, 0x00400000, 0x00020000);  // disable discovery poll
   MoveFlag(oldFlag, flags, 0x00800000, 0x00040000);  // disable topology poll
   MoveFlag(oldFlag, flags, 0x01000000, 0x00080000);  // disable SNMP
   MoveFlag(oldFlag, flags, 0x02000000, 0x00100000);  // disable NXCP
   MoveFlag(oldFlag, flags, 0x04000000, 0x00200000);  // disable ICMP
   MoveFlag(oldFlag, flags, 0x08000000, 0x00400000);  // force encryption
   MoveFlag(oldFlag, flags, 0x40000000, 0x00800000);  // disable routing poll
}

/**
 * Move node capabilities flags
 */
static void MoveNodeCapabilities(uint32_t oldFlag, uint32_t *capabilities, bool withSnmpConfLock)
{
   MoveFlag(oldFlag, capabilities, 0x00000001, NC_IS_SNMP);
   MoveFlag(oldFlag, capabilities, 0x00000002, NC_IS_NATIVE_AGENT);
   MoveFlag(oldFlag, capabilities, 0x00000004, NC_IS_BRIDGE);
   MoveFlag(oldFlag, capabilities, 0x00000008, NC_IS_ROUTER);
   MoveFlag(oldFlag, capabilities, 0x00000010, NC_IS_LOCAL_MGMT);
   MoveFlag(oldFlag, capabilities, 0x00000020, NC_IS_PRINTER);
   MoveFlag(oldFlag, capabilities, 0x00000040, NC_IS_OSPF);
   MoveFlag(oldFlag, capabilities, 0x00000100, 0x80);  // NC_IS_CPSNMP
   MoveFlag(oldFlag, capabilities, 0x00000200, NC_IS_CDP);
   MoveFlag(oldFlag, capabilities, 0x00000400, NC_IS_NDP);
   MoveFlag(oldFlag, capabilities, 0x00000800, NC_IS_LLDP);
   MoveFlag(oldFlag, capabilities, 0x00001000, NC_IS_VRRP);
   MoveFlag(oldFlag, capabilities, 0x00002000, NC_HAS_VLANS);
   MoveFlag(oldFlag, capabilities, 0x00004000, NC_IS_8021X);
   MoveFlag(oldFlag, capabilities, 0x00008000, NC_IS_STP);
   MoveFlag(oldFlag, capabilities, 0x00010000, NC_HAS_ENTITY_MIB);
   MoveFlag(oldFlag, capabilities, 0x00020000, NC_HAS_IFXTABLE);
   MoveFlag(oldFlag, capabilities, 0x00040000, NC_HAS_AGENT_IFXCOUNTERS);
   MoveFlag(oldFlag, capabilities, 0x00080000, NC_HAS_WINPDH);
   MoveFlag(oldFlag, capabilities, 0x00100000, NC_IS_WIFI_CONTROLLER);
   if (!withSnmpConfLock)
   {
      MoveFlag(oldFlag, capabilities, 0x00200000, NC_IS_SMCLP);
   }
}

/**
 * Move node state flags
 */
static void MoveNodeState(uint32_t oldRuntime, uint32_t *state)
{
   MoveFlag(oldRuntime, state, 0x000004, DCSF_UNREACHABLE);
   MoveFlag(oldRuntime, state, 0x000008, NSF_AGENT_UNREACHABLE);
   MoveFlag(oldRuntime, state, 0x000010, NSF_SNMP_UNREACHABLE);
   MoveFlag(oldRuntime, state, 0x000200, 0x00020000);  // NSF_CPSNMP_UNREACHABLE
   MoveFlag(oldRuntime, state, 0x008000, DCSF_NETWORK_PATH_PROBLEM);
   MoveFlag(oldRuntime, state, 0x020000, NSF_CACHE_MODE_NOT_SUPPORTED);
}

/**
 * Move sensor state flags
 */
static void MoveSensorState(uint32_t oldFlag, uint32_t oldRuntime, uint32_t *status)
{
   MoveFlag(oldFlag, status, 0x00000001, 0x00010000);
   MoveFlag(oldFlag, status, 0x00000002, 0x00020000);
   MoveFlag(oldFlag, status, 0x00000004, 0x00040000);
   MoveFlag(oldFlag, status, 0x00000008, 0x00080000);
   MoveFlag(oldRuntime, status, 0x000004, DCSF_UNREACHABLE);
}

/**
 * Upgrade from 30.3 to 30.4
 */
static bool H_UpgradeFromV3()
{
   static const TCHAR *batch =
            _T("ALTER TABLE object_properties ADD flags integer\n")
            _T("ALTER TABLE object_properties ADD state integer\n")
            _T("ALTER TABLE nodes ADD capabilities integer\n")
            _T("UPDATE object_properties SET flags=0,state=0\n")
            _T("UPDATE nodes SET capabilities=0\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("object_properties"), _T("flags")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("object_properties"), _T("state")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("capabilities")));

   // move flags from old tables to the new one
   CHK_EXEC(MoveFlagsFromOldTables(_T("interfaces")));
   CHK_EXEC(MoveFlagsFromOldTables(_T("templates")));
   CHK_EXEC(MoveFlagsFromOldTables(_T("chassis")));
   CHK_EXEC(MoveFlagsFromOldTables(_T("object_containers")));
   CHK_EXEC(MoveFlagsFromOldTables(_T("network_maps")));
   if (GetSchemaLevelForMajorVersion(22) >= 12)
   {
      CHK_EXEC(MoveFlagsFromOldTables(_T("ap_common")));
   }

   // create special behavior for node and sensor, cluster node
   bool withSnmpConfLock = (GetSchemaLevelForMajorVersion(22) >= 37);
   DB_RESULT hResult = SQLSelect(_T("SELECT id,runtime_flags FROM nodes"));
   if (hResult != nullptr)
   {
      DB_STATEMENT stmtNetObj = DBPrepare(g_dbHandle, _T("UPDATE object_properties SET flags=?, state=? WHERE object_id=?"));
      DB_STATEMENT stmtNode = DBPrepare(g_dbHandle, _T("UPDATE nodes SET capabilities=? WHERE id=?"));
      if ((stmtNetObj != nullptr) && (stmtNode != nullptr))
      {
         int nRows = DBGetNumRows(hResult);
         for(int i = 0; i < nRows; i++)
         {
            uint32_t id = DBGetFieldULong(hResult, i, 0);
            uint32_t oldFlags = 0;
            uint32_t oldRuntime = DBGetFieldULong(hResult, i, 1);
            uint32_t flags = 0;
            uint32_t state = 0;
            uint32_t capabilities = 0;
            TCHAR query[256];
            _sntprintf(query, 256, _T("SELECT node_flags FROM nodes WHERE id=%d"), id);
            DB_RESULT flagResult = SQLSelect(query);
            if (DBGetNumRows(flagResult) >= 1)
            {
               oldFlags = DBGetFieldULong(flagResult, 0, 0);
            }
            else
            {
               if (!g_ignoreErrors)
               {
                  DBFreeStatement(stmtNetObj);
                  DBFreeStatement(stmtNode);
                  DBFreeResult(hResult);
                  return false;
               }
            }
            MoveNodeFlags(oldFlags, &flags, withSnmpConfLock);
            MoveNodeCapabilities(oldFlags, &capabilities, withSnmpConfLock);
            MoveNodeState(oldRuntime, &state);

            DBBind(stmtNetObj, 1, DB_SQLTYPE_INTEGER, flags);
            DBBind(stmtNetObj, 2, DB_SQLTYPE_INTEGER, state);
            DBBind(stmtNetObj, 3, DB_SQLTYPE_INTEGER, id);

            DBBind(stmtNode, 1, DB_SQLTYPE_INTEGER, capabilities);
            DBBind(stmtNode, 2, DB_SQLTYPE_INTEGER, id);

            if (!(SQLExecute(stmtNetObj)))
            {
               if (!g_ignoreErrors)
               {
                  DBFreeStatement(stmtNetObj);
                  DBFreeStatement(stmtNode);
                  DBFreeResult(hResult);
                  return false;
               }
            }

            if (!SQLExecute(stmtNode))
            {
               if (!g_ignoreErrors)
               {
                  DBFreeStatement(stmtNetObj);
                  DBFreeStatement(stmtNode);
                  DBFreeResult(hResult);
                  return false;
               }
            }
         }
         DBFreeStatement(stmtNetObj);
         DBFreeStatement(stmtNode);
      }
      else
      {
         if (stmtNetObj != nullptr)
            DBFreeStatement(stmtNetObj);
         if (stmtNode != nullptr)
            DBFreeStatement(stmtNode);
         if (!g_ignoreErrors)
            return false;
      }
      DBFreeResult(hResult);
   }
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("nodes"), _T("runtime_flags")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("nodes"), _T("node_flags")));

   //sensor
   hResult = SQLSelect(_T("SELECT id,runtime_flags,flags FROM sensors"));
   if (hResult != nullptr)
   {
      DB_STATEMENT stmt = DBPrepare(g_dbHandle, _T("UPDATE object_properties SET status=? WHERE object_id=?"));
      if (stmt != nullptr)
      {
         int nRows = DBGetNumRows(hResult);
         for(int i = 0; i < nRows; i++)
         {
            uint32_t status = 0;
            MoveSensorState(DBGetFieldULong(hResult, i, 2), DBGetFieldULong(hResult, i, 1), &status);

            DBBind(stmt, 1, DB_SQLTYPE_INTEGER, status);
            DBBind(stmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 0));

            if (!(SQLExecute(stmt)))
            {
               if (!g_ignoreErrors)
               {
                  DBFreeStatement(stmt);
                  DBFreeResult(hResult);
                  return false;
               }
            }
         }
         DBFreeStatement(stmt);
      }
      else if (!g_ignoreErrors)
      {
         return false;
      }
      DBFreeResult(hResult);
   }
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("sensors"), _T("runtime_flags")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("sensors"), _T("flags")));

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 30.2 to 30.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("event_groups"), _T("range_start")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("event_groups"), _T("range_end")));

   static const TCHAR *batch =
            _T("ALTER TABLE event_groups ADD guid varchar(36) null\n")
            _T("UPDATE event_groups SET guid='04b326c0-5cc0-411f-8587-2836cb87c920' WHERE id=-2147483647\n")
            _T("UPDATE event_groups SET guid='b61859c6-1768-4a61-a0cf-eed07d688f66' WHERE id=-2147483646\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   DBSetNotNullConstraint(g_dbHandle, _T("event_groups"), _T("guid"));

   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 30.1 to 30.2
 */
static bool H_UpgradeFromV1()
{
   static const TCHAR *batch =
            _T("ALTER TABLE users ADD created integer null\n")
            _T("ALTER TABLE user_groups ADD created integer null\n")
            _T("UPDATE users SET created=0\n")
            _T("UPDATE user_groups SET created=0\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("users"), _T("created")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("user_groups"), _T("created")));

   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 30.0 to 30.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE sensors (")
      _T("  id integer not null,")
      _T("  proxy_node integer not null,")
      _T("  flags integer not null,")
      _T("  mac_address varchar(16) null,")
      _T("  device_class integer not null,")
      _T("  vendor varchar(128) null,")
      _T("  communication_protocol integer not null,")
      _T("  xml_config varchar(4000) null,")
      _T("  xml_reg_config varchar(4000) null,")
      _T("  serial_number varchar(256) null,")
      _T("  device_address varchar(256) null,")
      _T("  meta_type varchar(256) null,")
      _T("  description varchar(512) null,")
      _T("  last_connection_time integer not null,")
      _T("  frame_count integer not null,")
      _T("  signal_strenght integer not null,")
      _T("  signal_noise integer not null,")
      _T("  frequency integer not null,")
      _T("  runtime_flags integer null,")
      _T("PRIMARY KEY(id))")));

   CHK_EXEC(SetMinorSchemaVersion(1));
   return true;
}

/**
 * Upgrade map
 */
static struct
{
   int version;
   int nextMajor;
   int nextMinor;
   bool (* upgradeProc)();
} s_dbUpgradeMap[] =
{
   { 102, 31, 0, H_UpgradeFromV102 },
   { 101, 30, 102, H_UpgradeFromV101 },
   { 100, 30, 101, H_UpgradeFromV100 },
   { 99, 30, 100, H_UpgradeFromV99 },
   { 98, 30, 99, H_UpgradeFromV98 },
   { 97, 30, 98, H_UpgradeFromV97 },
   { 96, 30, 97, H_UpgradeFromV96 },
   { 95, 30, 96, H_UpgradeFromV95 },
   { 94, 30, 95, H_UpgradeFromV94 },
   { 93, 30, 94, H_UpgradeFromV93 },
   { 92, 30, 93, H_UpgradeFromV92 },
   { 91, 30, 92, H_UpgradeFromV91 },
   { 90, 30, 91, H_UpgradeFromV90 },
   { 89, 30, 90, H_UpgradeFromV89 },
   { 88, 30, 89, H_UpgradeFromV88 },
   { 87, 30, 88, H_UpgradeFromV87 },
   { 86, 30, 87, H_UpgradeFromV86 },
   { 85, 30, 86, H_UpgradeFromV85 },
   { 84, 30, 85, H_UpgradeFromV84 },
   { 83, 30, 84, H_UpgradeFromV83 },
   { 82, 30, 83, H_UpgradeFromV82 },
   { 81, 30, 82, H_UpgradeFromV81 },
   { 80, 30, 81, H_UpgradeFromV80 },
   { 79, 30, 80, H_UpgradeFromV79 },
   { 78, 30, 79, H_UpgradeFromV78 },
   { 77, 30, 78, H_UpgradeFromV77 },
   { 76, 30, 77, H_UpgradeFromV76 },
   { 75, 30, 76, H_UpgradeFromV75 },
   { 74, 30, 75, H_UpgradeFromV74 },
   { 73, 30, 74, H_UpgradeFromV73 },
   { 72, 30, 73, H_UpgradeFromV72 },
   { 71, 30, 72, H_UpgradeFromV71 },
   { 70, 30, 71, H_UpgradeFromV70 },
   { 69, 30, 70, H_UpgradeFromV69 },
   { 68, 30, 69, H_UpgradeFromV68 },
   { 67, 30, 68, H_UpgradeFromV67 },
   { 66, 30, 67, H_UpgradeFromV66 },
   { 65, 30, 66, H_UpgradeFromV65 },
   { 64, 30, 65, H_UpgradeFromV64 },
   { 63, 30, 64, H_UpgradeFromV63 },
   { 62, 30, 63, H_UpgradeFromV62 },
   { 61, 30, 62, H_UpgradeFromV61 },
   { 60, 30, 61, H_UpgradeFromV60 },
   { 59, 30, 60, H_UpgradeFromV59 },
   { 58, 30, 59, H_UpgradeFromV58 },
   { 57, 30, 58, H_UpgradeFromV57 },
   { 56, 30, 57, H_UpgradeFromV56 },
   { 55, 30, 56, H_UpgradeFromV55 },
   { 54, 30, 55, H_UpgradeFromV54 },
   { 53, 30, 54, H_UpgradeFromV53 },
   { 52, 30, 53, H_UpgradeFromV52 },
   { 51, 30, 52, H_UpgradeFromV51 },
   { 50, 30, 51, H_UpgradeFromV50 },
   { 49, 30, 50, H_UpgradeFromV49 },
   { 48, 30, 49, H_UpgradeFromV48 },
   { 47, 30, 48, H_UpgradeFromV47 },
   { 46, 30, 47, H_UpgradeFromV46 },
   { 45, 30, 46, H_UpgradeFromV45 },
   { 44, 30, 45, H_UpgradeFromV44 },
   { 43, 30, 44, H_UpgradeFromV43 },
   { 42, 30, 43, H_UpgradeFromV42 },
   { 41, 30, 42, H_UpgradeFromV41 },
   { 40, 30, 41, H_UpgradeFromV40 },
   { 39, 30, 40, H_UpgradeFromV39 },
   { 38, 30, 39, H_UpgradeFromV38 },
   { 37, 30, 38, H_UpgradeFromV37 },
   { 36, 30, 37, H_UpgradeFromV36 },
   { 35, 30, 36, H_UpgradeFromV35 },
   { 34, 30, 35, H_UpgradeFromV34 },
   { 33, 30, 35, H_UpgradeFromV33 },
   { 32, 30, 33, H_UpgradeFromV32 },
   { 31, 30, 32, H_UpgradeFromV31 },
   { 30, 30, 31, H_UpgradeFromV30 },
   { 29, 30, 30, H_UpgradeFromV29 },
   { 28, 30, 28, H_UpgradeFromV28 },
   { 27, 30, 28, H_UpgradeFromV27 },
   { 26, 30, 27, H_UpgradeFromV26 },
   { 25, 30, 26, H_UpgradeFromV25 },
   { 24, 30, 25, H_UpgradeFromV24 },
   { 23, 30, 24, H_UpgradeFromV23 },
   { 22, 30, 23, H_UpgradeFromV22 },
   { 21, 30, 22, H_UpgradeFromV21 },
   { 20, 30, 21, H_UpgradeFromV20 },
   { 19, 30, 20, H_UpgradeFromV19 },
   { 18, 30, 19, H_UpgradeFromV18 },
   { 17, 30, 18, H_UpgradeFromV17 },
   { 16, 30, 17, H_UpgradeFromV16 },
   { 15, 30, 16, H_UpgradeFromV15 },
   { 14, 30, 15, H_UpgradeFromV14 },
   { 13, 30, 14, H_UpgradeFromV13 },
   { 12, 30, 13, H_UpgradeFromV12 },
   { 11, 30, 12, H_UpgradeFromV11 },
   { 10, 30, 11, H_UpgradeFromV10 },
   { 9,  30, 10, H_UpgradeFromV9 },
   { 8,  30, 9,  H_UpgradeFromV8 },
   { 7,  30, 8,  H_UpgradeFromV7 },
   { 6,  30, 7,  H_UpgradeFromV6 },
   { 5,  30, 6,  H_UpgradeFromV5 },
   { 4,  30, 5,  H_UpgradeFromV4 },
   { 3,  30, 4,  H_UpgradeFromV3 },
   { 2,  30, 3,  H_UpgradeFromV2 },
   { 1,  30, 2,  H_UpgradeFromV1 },
   { 0,  30, 1,  H_UpgradeFromV0 },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V30()
{
   INT32 major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while(major == 30)
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 30.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 30.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
      DBBegin(g_dbHandle);
      if (s_dbUpgradeMap[i].upgradeProc())
      {
         DBCommit(g_dbHandle);
         if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
            return false;
      }
      else
      {
         _tprintf(_T("Rolling back last stage due to upgrade errors...\n"));
         DBRollback(g_dbHandle);
         return false;
      }
   }
   return true;
}
