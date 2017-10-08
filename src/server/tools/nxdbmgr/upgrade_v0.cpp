/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2017 Victor Kirhenshtein
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
** File: upgrade_v0.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Externals
 */
BOOL MigrateMaps();
bool ConvertTDataTables();

/**
 * Re-create TDATA tables
 */
static BOOL RecreateTData(const TCHAR *className, bool multipleTables, bool indexFix)
{
   TCHAR query[1024];
   _sntprintf(query, 256, _T("SELECT id FROM %s"), className);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         bool recreateTables = true;
         DWORD id = DBGetFieldULong(hResult, i, 0);

         if (indexFix)
         {
            _sntprintf(query, 256, _T("SELECT count(*) FROM dc_tables WHERE node_id=%d"), id);
            DB_RESULT hResultCount = SQLSelect(query);
            if (hResultCount != NULL)
            {
               recreateTables = (DBGetFieldLong(hResultCount, 0, 0) == 0);
               DBFreeResult(hResultCount);
            }

            if (!recreateTables)
            {
               _sntprintf(query, 256, _T("CREATE INDEX idx_tdata_rec_%d_id ON tdata_records_%d(record_id)"), id, id);
               if (!SQLQuery(query))
               {
                  if (!g_bIgnoreErrors)
                  {
                     DBFreeResult(hResult);
                     return FALSE;
                  }
               }
            }
         }

         if (recreateTables)
         {
            if (multipleTables)
            {
               _sntprintf(query, 1024, _T("DROP TABLE tdata_rows_%d\nDROP TABLE tdata_records_%d\nDROP TABLE tdata_%d\n<END>"), id, id, id);
            }
            else
            {
               _sntprintf(query, 256, _T("DROP TABLE tdata_%d\n<END>"), id);
            }
            if (!SQLBatch(query))
            {
               if (!g_bIgnoreErrors)
               {
                  DBFreeResult(hResult);
                  return FALSE;
               }
            }

            if (!CreateTDataTable(id))
            {
               if (!g_bIgnoreErrors)
               {
                  DBFreeResult(hResult);
                  return FALSE;
               }
            }
         }
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return FALSE;
   }
   return TRUE;
}

/**
 * Convert network masks from dotted decimal format to number of bits
 */
static BOOL ConvertNetMasks(const TCHAR *table, const TCHAR *column, const TCHAR *idColumn, const TCHAR *idColumn2 = NULL, const TCHAR *condition = NULL)
{
   TCHAR query[256];

   if (idColumn2 != NULL)
      _sntprintf(query, 256, _T("SELECT %s,%s,%s FROM %s"), idColumn, column, idColumn2, table);
   else
      _sntprintf(query, 256, _T("SELECT %s,%s FROM %s"), idColumn, column, table);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult == NULL)
      return FALSE;

   bool success = DBDropColumn(g_hCoreDB, table, column);

   if (success)
   {
      _sntprintf(query, 256, _T("ALTER TABLE %s ADD %s integer"), table, column);
      success = SQLQuery(query);
   }

   if (success)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; (i < count) && success; i++)
      {
         if (idColumn2 != NULL)
         {
            TCHAR id2[256];
            _sntprintf(query, 256, _T("UPDATE %s SET %s=%d WHERE %s=%d AND %s='%s'"),
               table, column, BitsInMask(DBGetFieldIPAddr(hResult, i, 1)), idColumn, DBGetFieldLong(hResult, i, 0),
               idColumn2, DBGetField(hResult, i, 2, id2, 256));
         }
         else
         {
            _sntprintf(query, 256, _T("UPDATE %s SET %s=%d WHERE %s=%d"),
               table, column, BitsInMask(DBGetFieldIPAddr(hResult, i, 1)), idColumn, DBGetFieldLong(hResult, i, 0));
         }
         success = SQLQuery(query);
      }
   }

   DBFreeResult(hResult);
   return success;
}

/**
 * Convert object tool macros to new format
 */
static bool ConvertObjectToolMacros(UINT32 id, const TCHAR *text, const TCHAR *column)
{
   if (_tcschr(text, _T('%')) == NULL)
      return true; // nothing to convert

   String s;
   for(const TCHAR *p = text; *p != 0; p++)
   {
      if (*p == _T('%'))
      {
         TCHAR name[256];
         int i = 0;
         for(p++; (*p != _T('%')) && (*p != 0); p++)
         {
            if (i < 255)
               name[i++] = *p;
         }
         if (*p == 0)
            break;  // malformed string
         name[i] = 0;
         if (!_tcscmp(name, _T("ALARM_ID")))
         {
            s.append(_T("%Y"));
         }
         else if (!_tcscmp(name, _T("ALARM_MESSAGE")))
         {
            s.append(_T("%A"));
         }
         else if (!_tcscmp(name, _T("ALARM_SEVERITY")))
         {
            s.append(_T("%s"));
         }
         else if (!_tcscmp(name, _T("ALARM_SEVERITY_TEXT")))
         {
            s.append(_T("%S"));
         }
         else if (!_tcscmp(name, _T("ALARM_STATE")))
         {
            s.append(_T("%y"));
         }
         else if (!_tcscmp(name, _T("OBJECT_ID")))
         {
            s.append(_T("%I"));
         }
         else if (!_tcscmp(name, _T("OBJECT_IP_ADDR")))
         {
            s.append(_T("%a"));
         }
         else if (!_tcscmp(name, _T("OBJECT_NAME")))
         {
            s.append(_T("%n"));
         }
         else if (!_tcscmp(name, _T("USERNAME")))
         {
            s.append(_T("%U"));
         }
         else
         {
            s.append(_T("%{"));
            s.append(name);
            s.append(_T('}'));
         }
      }
      else
      {
         s.append(*p);
      }
   }

   String query = _T("UPDATE object_tools SET ");
   query.append(column);
   query.append(_T('='));
   query.append(DBPrepareString(g_hCoreDB, s));
   query.append(_T(" WHERE tool_id="));
   query.append(id);
   return SQLQuery(query);
}

/**
 * Create library script
 */
static bool CreateLibraryScript(UINT32 id, const TCHAR *name, const TCHAR *code)
{
   // Check if script exists
   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT script_id FROM script_library WHERE script_id=%d OR script_name=%s"),
              id, (const TCHAR *)DBPrepareString(g_hCoreDB, name));
   DB_RESULT hResult = SQLSelect(query);
   if (hResult == NULL)
      return false;
   bool exist = (DBGetNumRows(hResult) > 0);
   DBFreeResult(hResult);
   if (exist)
      return true;

   DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("INSERT INTO script_library (script_id,script_name,script_code) VALUES (?,?,?)"));
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
   DBBind(hStmt, 3, DB_SQLTYPE_TEXT, code, DB_BIND_STATIC);

   bool success = SQLExecute(hStmt);
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Set schema version
 */
static bool SetSchemaVersion(int version)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("UPDATE metadata SET var_value='%d' WHERE var_name='SchemaVersion'"), version);
   return SQLQuery(query);
}

/**
 * Finish legacy versions upgrade to 21.x
 */
static bool FinishLegacyUpgradeToV21(INT32 minor)
{
   _tprintf(_T("Upgrading from version 0.700 to 21.%d\n"), minor);

   const TCHAR *batch =
      _T("INSERT INTO metadata (var_name,var_value) VALUES ('SchemaVersionMajor','21')\n")
      _T("INSERT INTO metadata (var_name,var_value) VALUES ('SchemaVersionLevel.0','700')\n")
      _T("UPDATE metadata SET var_value='700' WHERE var_name='SchemaVersion'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   TCHAR query[256];
   _sntprintf(query, 256, _T("INSERT INTO metadata (var_name,var_value) VALUES ('SchemaVersionMinor','%d')"), minor);
   CHK_EXEC(SQLQuery(query));

   return true;
}

/**
 * Upgrade from V462 to V700
 */
static BOOL H_UpgradeFromV462(int currVersion, int newVersion)
{
   CHK_EXEC(FinishLegacyUpgradeToV21(4));
   return TRUE;
}

/**
 * Upgrade from V461 to V700
 */
static BOOL H_UpgradeFromV461(int currVersion, int newVersion)
{
   CHK_EXEC(FinishLegacyUpgradeToV21(3));
   return TRUE;
}

/**
 * Upgrade from V460 to V700
 */
static BOOL H_UpgradeFromV460(int currVersion, int newVersion)
{
   CHK_EXEC(FinishLegacyUpgradeToV21(2));
   return TRUE;
}

/**
 * Upgrade from V459 to V700
 */
static BOOL H_UpgradeFromV459(int currVersion, int newVersion)
{
   CHK_EXEC(FinishLegacyUpgradeToV21(1));
   return TRUE;
}

/**
 * Upgrade from V458 to V459
 */
static BOOL H_UpgradeFromV458(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE dci_summary_tables ADD table_dci_name varchar(255)")));
   CHK_EXEC(SetSchemaVersion(459));
   return TRUE;
}

/**
 * Upgrade from V457 to V458
 */
static BOOL H_UpgradeFromV457(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',need_server_restart=0 WHERE var_name='DeleteUnreachableNodesPeriod'")));
   CHK_EXEC(CreateConfigParam(_T("LongRunningQueryThreshold"), _T("0"), _T("Threshold in milliseconds to report long running SQL queries (0 to disable)"), 'I', true, true, false, false));
   CHK_EXEC(SetSchemaVersion(458));
   return TRUE;
}

/**
 * Upgrade from V456 to V457
 */
static BOOL H_UpgradeFromV456(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='EnableAdminInterface'")));
   CHK_EXEC(SetSchemaVersion(457));
   return TRUE;
}

/**
 * Upgrade from V455 to V456
 */
static BOOL H_UpgradeFromV455(int currVersion, int newVersion)
{
   CHK_EXEC(
      CreateEventTemplate(EVENT_PACKAGE_INSTALLED, _T("SYS_PACKAGE_INSTALLED"),
         SEVERITY_NORMAL, EF_LOG, _T("92e5cf98-a415-4414-9ad8-d155dac77e96"),
         _T("Package %1 %2 installed"),
         _T("Generated when new software package is found.\r\n")
         _T("Parameters:\r\n")
         _T("   1) Package name\r\n")
         _T("   2) Package version"))
      );

   CHK_EXEC(
      CreateEventTemplate(EVENT_PACKAGE_UPDATED, _T("SYS_PACKAGE_UPDATED"),
         SEVERITY_NORMAL, EF_LOG, _T("9d5878c1-525e-4cab-8f02-2a6c46d7fc36"),
         _T("Package %1 updated from %3 to %2"),
         _T("Generated when software package version change is detected.\r\n")
         _T("Parameters:\r\n")
         _T("   1) Package name\r\n")
         _T("   2) New package version\r\n")
         _T("   3) Old package version"))
      );

   CHK_EXEC(
      CreateEventTemplate(EVENT_PACKAGE_REMOVED, _T("SYS_PACKAGE_REMOVED"),
         SEVERITY_NORMAL, EF_LOG, _T("6ada4ea4-43e4-4444-9d19-ef7366110bb9"),
         _T("Package %1 %2 removed"),
         _T("Generated when software package removal is detected.\r\n")
         _T("Parameters:\r\n")
         _T("   1) Package name\r\n")
         _T("   2) Last known package version"))
      );

   int ruleId = NextFreeEPPruleID();
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event) ")
                           _T("VALUES (%d,'2bb3df47-482b-4e4b-9b49-8c72c6b33011',7944,'Generate alarm on software package changes','%%m',5,'SW_PKG_%%i_%%<name>','',0,%d)"),
                           ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 256, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_PACKAGE_INSTALLED);
   CHK_EXEC(SQLQuery(query));
   _sntprintf(query, 256, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_PACKAGE_UPDATED);
   CHK_EXEC(SQLQuery(query));
   _sntprintf(query, 256, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_PACKAGE_REMOVED);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetSchemaVersion(456));
   return TRUE;
}

/**
 * Upgrade from V454 to V455
 */
static BOOL H_UpgradeFromV454(int currVersion, int newVersion)
{
   static const TCHAR *batch =
            _T("ALTER TABLE interfaces ADD parent_iface integer\n")
            _T("UPDATE interfaces SET parent_iface=0\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("interfaces"), _T("parent_iface")));

   CHK_EXEC(SetSchemaVersion(455));
   return TRUE;
}

/**
 * Upgrade from V453 to V454
 */
static BOOL H_UpgradeFromV453(int currVersion, int newVersion)
{
   static const TCHAR *batch =
            _T("ALTER TABLE config ADD default_value varchar(2000) null\n")
            _T("UPDATE config SET default_value='7200' WHERE var_name='ActiveDiscoveryInterval'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='ActiveNetworkDiscovery'\n")
            _T("UPDATE config SET default_value='4000' WHERE var_name='AgentCommandTimeout'\n")
            _T("UPDATE config SET default_value='netxms' WHERE var_name='AgentDefaultSharedSecret'\n")
            _T("UPDATE config SET default_value='600' WHERE var_name='AgentUpgradeWaitTime'\n")
            _T("UPDATE config SET default_value='180' WHERE var_name='AlarmHistoryRetentionTime'\n")
            _T("UPDATE config SET default_value='4096' WHERE var_name='AlarmListDisplayLimit'\n")
            _T("UPDATE config SET default_value='0 0 * * *' WHERE var_name='AlarmSummaryEmailSchedule'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='AllowDirectSMS'\n")
            _T("UPDATE config SET default_value='63' WHERE var_name='AllowedCiphers'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='AllowTrapVarbindsConversion'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='AnonymousFileAccess'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='ApplyDCIFromTemplateToDisabledDCI'\n")
            _T("UPDATE config SET default_value='90' WHERE var_name='AuditLogRetentionTime'\n")
            _T("UPDATE config SET default_value='1000' WHERE var_name='BeaconPollingInterval'\n")
            _T("UPDATE config SET default_value='1000' WHERE var_name='BeaconTimeout'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='BlockInactiveUserAccounts'\n")
            _T("UPDATE config SET default_value='604800' WHERE var_name='CapabilityExpirationTime'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='CaseInsensitiveLoginNames'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='CheckTrustedNodes'\n")
            _T("UPDATE config SET default_value='4701' WHERE var_name='ClientListenerPort'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='ClusterContainerAutoBind'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='ClusterTemplateAutoApply'\n")
            _T("UPDATE config SET default_value='60' WHERE var_name='ConditionPollingInterval'\n")
            _T("UPDATE config SET default_value='3600' WHERE var_name='ConfigurationPollingInterval'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='DashboardDataExportEnableInterpolation'\n")
            _T("UPDATE config SET default_value='10' WHERE var_name='DBConnectionPoolBaseSize'\n")
            _T("UPDATE config SET default_value='300' WHERE var_name='DBConnectionPoolCooldownTime'\n")
            _T("UPDATE config SET default_value='14400' WHERE var_name='DBConnectionPoolMaxLifetime'\n")
            _T("UPDATE config SET default_value='30' WHERE var_name='DBConnectionPoolMaxSize'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='DBLockPID'\n")
            _T("UPDATE config SET default_value='UNLOCKED' WHERE var_name='DBLockStatus'\n")
            _T("UPDATE config SET default_value='2' WHERE var_name='DefaultAgentCacheMode'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='DefaultAgentProtocolCompressionMode'\n")
            _T("UPDATE config SET default_value='dd.MM.yyyy' WHERE var_name='DefaultConsoleDateFormat'\n")
            _T("UPDATE config SET default_value='HH:mm' WHERE var_name='DefaultConsoleShortTimeFormat'\n")
            _T("UPDATE config SET default_value='HH:mm:ss' WHERE var_name='DefaultConsoleTimeFormat'\n")
            _T("UPDATE config SET default_value='60' WHERE var_name='DefaultDCIPollingInterval'\n")
            _T("UPDATE config SET default_value='30' WHERE var_name='DefaultDCIRetentionTime'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='DefaultEncryptionPolicy'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='DefaultInterfaceExpectedState'\n")
            _T("UPDATE config SET default_value='0xffffff' WHERE var_name='DefaultMapBackgroundColor'\n")
            _T("UPDATE config SET default_value='24' WHERE var_name='DefaultSubnetMaskIPv4'\n")
            _T("UPDATE config SET default_value='64' WHERE var_name='DefaultSubnetMaskIPv6'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='DeleteAlarmsOfDeletedObject'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='DeleteEmptySubnets'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='DeleteEventsOfDeletedObject'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='DeleteUnreachableNodesPeriod'\n")
            _T("UPDATE config SET default_value='none' WHERE var_name='DiscoveryFilter'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='DiscoveryFilterFlags'\n")
            _T("UPDATE config SET default_value='900' WHERE var_name='DiscoveryPollingInterval'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='EnableAdminInterface'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='EnableAgentRegistration'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='EnableAuditLog'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='EnableCheckPointSNMP'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='EnableReportingServer'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='EnableEventStormDetection'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='EnableISCListener'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='EnableNXSLContainerFunctions'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='EnableObjectTransactions'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='EnableSNMPTraps'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='EnableAlarmSummaryEmails'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='EnableSyslogReceiver'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='EnableTimedAlarmAck'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='EnableXMPPConnector'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='EnableZoning'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='EscapeLocalCommands'\n")
            _T("UPDATE config SET default_value='90' WHERE var_name='EventLogRetentionTime'\n")
            _T("UPDATE config SET default_value='15' WHERE var_name='EventStormDuration'\n")
            _T("UPDATE config SET default_value='100' WHERE var_name='EventStormEventsPerSecond'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='ExtendedLogQueryAccessControl'\n")
            _T("UPDATE config SET default_value='13' WHERE var_name='ExternalAuditFacility'\n")
            _T("UPDATE config SET default_value='514' WHERE var_name='ExternalAuditPort'\n")
            _T("UPDATE config SET default_value='none' WHERE var_name='ExternalAuditServer'\n")
            _T("UPDATE config SET default_value='5' WHERE var_name='ExternalAuditSeverity'\n")
            _T("UPDATE config SET default_value='netxmsd-audit' WHERE var_name='ExternalAuditTag'\n")
            _T("UPDATE config SET default_value='100' WHERE var_name='FirstFreeObjectId'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='FixedStatusValue'\n")
            _T("UPDATE config SET default_value='5' WHERE var_name='GraceLoginCount'\n")
            _T("UPDATE config SET default_value='none' WHERE var_name='HelpDeskLink'\n")
            _T("UPDATE config SET default_value='02:00' WHERE var_name='HousekeeperStartTime'\n")
            _T("UPDATE config SET default_value='46' WHERE var_name='IcmpPingSize'\n")
            _T("UPDATE config SET default_value='1500' WHERE var_name='IcmpPingTimeout'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='ImportConfigurationOnStartup'\n")
            _T("UPDATE config SET default_value='600' WHERE var_name='InstancePollingInterval'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='InternalCA'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='IntruderLockoutThreshold'\n")
            _T("UPDATE config SET default_value='30' WHERE var_name='IntruderLockoutTime'\n")
            _T("UPDATE config SET default_value='Task' WHERE var_name='JiraIssueType'\n")
            _T("UPDATE config SET default_value='netxms' WHERE var_name='JiraLogin'\n")
            _T("UPDATE config SET default_value='NETXMS' WHERE var_name='JiraProjectCode'\n")
            _T("UPDATE config SET default_value='http://localhost' WHERE var_name='JiraServerURL'\n")
            _T("UPDATE config SET default_value='90' WHERE var_name='JobHistoryRetentionTime'\n")
            _T("UPDATE config SET default_value='5' WHERE var_name='JobRetryCount'\n")
            _T("UPDATE config SET default_value='60' WHERE var_name='KeepAliveInterval'\n")
            _T("UPDATE config SET default_value='ldap://localhost:389' WHERE var_name='LdapConnectionString'\n")
            _T("UPDATE config SET default_value='displayName' WHERE var_name='LdapMappingFullName'\n")
            _T("UPDATE config SET default_value='1000' WHERE var_name='LdapPageSize'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='LdapSyncInterval'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='LdapUserDeleteAction'\n")
            _T("UPDATE config SET default_value='60000' WHERE var_name='LockTimeout'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='LogAllSNMPTraps'\n")
            _T("UPDATE config SET default_value='utf8' WHERE var_name='MailEncoding'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='MinPasswordLength'\n")
            _T("UPDATE config SET default_value='1000' WHERE var_name='MinViewRefreshInterval'\n")
            _T("UPDATE config SET default_value='4747' WHERE var_name='MobileDeviceListenerPort'\n")
            _T("UPDATE config SET default_value='25' WHERE var_name='NumberOfDataCollectors'\n")
            _T("UPDATE config SET default_value='10' WHERE var_name='NumberOfUpgradeThreads'\n")
            _T("UPDATE config SET default_value='86400' WHERE var_name='OfflineDataRelevanceTime'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='PasswordComplexity'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='PasswordExpiration'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='PasswordHistoryLength'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='PollCountForStatusChange'\n")
            _T("UPDATE config SET default_value='10' WHERE var_name='PollerThreadPoolBaseSize'\n")
            _T("UPDATE config SET default_value='250' WHERE var_name='PollerThreadPoolMaxSize'\n")
            _T("UPDATE config SET default_value='PAP' WHERE var_name='RADIUSAuthMethod'\n")
            _T("UPDATE config SET default_value='5' WHERE var_name='RADIUSNumRetries'\n")
            _T("UPDATE config SET default_value='1645' WHERE var_name='RADIUSPort'\n")
            _T("UPDATE config SET default_value='1645' WHERE var_name='RADIUSSecondaryPort'\n")
            _T("UPDATE config SET default_value='netxms' WHERE var_name='RADIUSSecondarySecret'\n")
            _T("UPDATE config SET default_value='none' WHERE var_name='RADIUSSecondaryServer'\n")
            _T("UPDATE config SET default_value='netxms' WHERE var_name='RADIUSSecret'\n")
            _T("UPDATE config SET default_value='none' WHERE var_name='RADIUSServer'\n")
            _T("UPDATE config SET default_value='3' WHERE var_name='RADIUSTimeout'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='ReceiveForwardedEvents'\n")
            _T("UPDATE config SET default_value='localhost' WHERE var_name='ReportingServerHostname'\n")
            _T("UPDATE config SET default_value='4710' WHERE var_name='ReportingServerPort'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='ResolveDNSToIPOnStatusPoll'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='ResolveNodeNames'\n")
            _T("UPDATE config SET default_value='300' WHERE var_name='RoutingTableUpdateInterval'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='RunNetworkDiscovery'\n")
            _T("UPDATE config SET default_value='60' WHERE var_name='ServerCommandOutputTimeout'\n")
            _T("UPDATE config SET default_value='<none>' WHERE var_name='SMSDriver'\n")
            _T("UPDATE config SET default_value='161' WHERE var_name='SNMPPorts'\n")
            _T("UPDATE config SET default_value='1500' WHERE var_name='SNMPRequestTimeout'\n")
            _T("UPDATE config SET default_value='90' WHERE var_name='SNMPTrapLogRetentionTime'\n")
            _T("UPDATE config SET default_value='162' WHERE var_name='SNMPTrapPort'\n")
            _T("UPDATE config SET default_value='netxms@localhost' WHERE var_name='SMTPFromAddr'\n")
            _T("UPDATE config SET default_value='NetXMS Server' WHERE var_name='SMTPFromName'\n")
            _T("UPDATE config SET default_value='25' WHERE var_name='SMTPPort'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='SMTPRetryCount'\n")
            _T("UPDATE config SET default_value='localhost' WHERE var_name='SMTPServer'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='StatusCalculationAlgorithm'\n")
            _T("UPDATE config SET default_value='60' WHERE var_name='StatusPollingInterval'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='StatusPropagationAlgorithm'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='StatusShift'\n")
            _T("UPDATE config SET default_value='75' WHERE var_name='StatusSingleThreshold'\n")
            _T("UPDATE config SET default_value='503C2814' WHERE var_name='StatusThresholds'\n")
            _T("UPDATE config SET default_value='01020304' WHERE var_name='StatusTranslation'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='StrictAlarmStatusFlow'\n")
            _T("UPDATE config SET default_value='60' WHERE var_name='SyncInterval'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='SyncNodeNamesWithDNS'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='SyslogIgnoreMessageTimestamp'\n")
            _T("UPDATE config SET default_value='514' WHERE var_name='SyslogListenPort'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='SyslogNodeMatchingPolicy'\n")
            _T("UPDATE config SET default_value='90' WHERE var_name='SyslogRetentionTime'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='ThresholdRepeatInterval'\n")
            _T("UPDATE config SET default_value='http://tile.openstreetmap.org/' WHERE var_name='TileServerURL'\n")
            _T("UPDATE config SET default_value='3' WHERE var_name='TopologyDiscoveryRadius'\n")
            _T("UPDATE config SET default_value='900' WHERE var_name='TopologyExpirationTime'\n")
            _T("UPDATE config SET default_value='1800' WHERE var_name='TopologyPollingInterval'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='TrapSourcesInAllZones'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='UseDNSNameForDiscoveredNodes'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='UseFQDNForNodeNames'\n")
            _T("UPDATE config SET default_value='1' WHERE var_name='UseIfXTable'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='UseInterfaceAliases'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='UseSNMPTrapsForDiscovery'\n")
            _T("UPDATE config SET default_value='0' WHERE var_name='UseSyslogForDiscovery'\n")
            _T("UPDATE config SET default_value='netxms@localhost' WHERE var_name='XMPPLogin'\n")
            _T("UPDATE config SET default_value='netxms' WHERE var_name='XMPPPassword'\n")
            _T("UPDATE config SET default_value='5222' WHERE var_name='XMPPPort'\n")
            _T("UPDATE config SET default_value='localhost' WHERE var_name='XMPPServer'\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetSchemaVersion(454));
   return TRUE;
}

/**
 * Upgrade from V452 to V453
 */
static BOOL H_UpgradeFromV452(int currVersion, int newVersion)
{
   static const TCHAR *batch =
            _T("ALTER TABLE audit_log ADD value_diff $SQL:TEXT\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(453));
   return TRUE;
}

/**
 * Upgrade from V451 to V452
 */
static BOOL H_UpgradeFromV451(int currVersion, int newVersion)
{
   static const TCHAR *batch =
            _T("ALTER TABLE audit_log ADD old_value $SQL:TEXT\n")
            _T("ALTER TABLE audit_log ADD new_value $SQL:TEXT\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(452));
   return TRUE;
}

/**
 * Upgrade from V450 to V451
 */
static BOOL H_UpgradeFromV450(int currVersion, int newVersion)
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE object_access_snapshot (")
      _T("  user_id integer not null,")
      _T("  object_id integer not null,")
      _T("  access_rights integer not null,")
      _T("PRIMARY KEY(user_id,object_id))")));

   CHK_EXEC(SetSchemaVersion(451));
   return TRUE;
}

/**
 * Upgrade from V449 to V450
 */
static BOOL H_UpgradeFromV449(int currVersion, int newVersion)
{
   static const TCHAR *batch =
            _T("ALTER TABLE dct_thresholds ADD sample_count integer\n")
            _T("UPDATE dct_thresholds SET sample_count=1\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   DBSetNotNullConstraint(g_hCoreDB, _T("dct_thresholds"), _T("sample_count"));

   CHK_EXEC(CreateTable(
      _T("CREATE TABLE dct_threshold_instances (")
      _T("  threshold_id integer not null,")
      _T("  instance varchar(255) not null,")
      _T("  match_count integer not null,")
      _T("  is_active char(1) not null,")
      _T("PRIMARY KEY(threshold_id,instance))")));

   CHK_EXEC(SetSchemaVersion(450));
   return TRUE;
}

/**
 * Upgrade from V448 to V449
 */
static BOOL H_UpgradeFromV448(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("SMTPPort"), _T("25"), _T("Port used by SMTP server"), 'I', true, false, false, false));
   CHK_EXEC(SetSchemaVersion(449));
   return TRUE;
}

/**
 * Upgrade from V447 to V448
 */
static BOOL H_UpgradeFromV447(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD lldp_id varchar(63)")));
   CHK_EXEC(SetSchemaVersion(448));
   return TRUE;
}

/**
 * Upgrade from V446 to V447
 */
static BOOL H_UpgradeFromV446(int currVersion, int newVersion)
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE port_layouts (")
      _T("  device_oid varchar(255) not null,")
      _T("  numbering_scheme char(1) not null,")
      _T("  row_count char(1) not null,")
      _T("  layout_data varchar(4000) null,")
      _T("PRIMARY KEY(device_oid))")));

   CHK_EXEC(SetSchemaVersion(447));
   return TRUE;
}

/**
 * Upgrade from V445 to V446
 */
static BOOL H_UpgradeFromV445(int currVersion, int newVersion)
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE network_map_seed_nodes (")
      _T("  map_id integer not null,")
      _T("  seed_node_id integer not null,")
      _T("PRIMARY KEY(map_id,seed_node_id))")));

   DB_RESULT hResult = DBSelect(g_hCoreDB, _T("SELECT id,seed FROM network_maps"));
   DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("INSERT INTO network_map_seed_nodes (map_id,seed_node_id) VALUES (?,?)"));
   if (hResult != NULL)
   {
      if (hStmt != NULL)
      {
         int nRows = DBGetNumRows(hResult);
         for(int i = 0; i < nRows; i++)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 0));
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 1));

            if (!SQLExecute(hStmt))
            {
               if (!g_bIgnoreErrors)
               {
                  DBFreeStatement(hStmt);
                  DBFreeResult(hResult);
                  return FALSE;
               }
            }
         }

         CHK_EXEC(DBDropColumn(g_hCoreDB, _T("network_maps"), _T("seed")));
         DBFreeStatement(hStmt);
      }
      DBFreeResult(hResult);
   }

   CHK_EXEC(SetSchemaVersion(446));
   return TRUE;
}

/**
 * Upgrade from V444 to V445
 */
static BOOL H_UpgradeFromV444(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD tunnel_id varchar(36) null")));
   CHK_EXEC(SetSchemaVersion(445));
   return TRUE;
}

/**
 * Upgrade from V443 to V444
 */
static BOOL H_UpgradeFromV443(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("GraceLoginCount"), _T("5"), _T("User's grace login count"), 'I', true, false, false, false));
   CHK_EXEC(SetSchemaVersion(444));
   return TRUE;
}

/**
 * Upgrade from V442 to V443
 */
static BOOL H_UpgradeFromV442(int currVersion, int newVersion)
{
   static const TCHAR *batch =
            _T("ALTER TABLE dc_tables ADD instance varchar(255) null\n")
            _T("ALTER TABLE dc_tables ADD instd_method integer null\n")
            _T("UPDATE dc_tables SET instd_method=0\n")
            _T("ALTER TABLE dc_tables ADD instd_data varchar(255) null\n")
            _T("ALTER TABLE dc_tables ADD instd_filter $SQL:TEXT null\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   DBSetNotNullConstraint(g_hCoreDB, _T("dc_tables"), _T("instd_method"));
   CHK_EXEC(SetSchemaVersion(443));
   return TRUE;
}

/**
 * Upgrade from V441 to V442
 */
static BOOL H_UpgradeFromV441(int currVersion, int newVersion)
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE object_urls (")
      _T("  object_id integer not null,")
      _T("  url_id integer not null,")
      _T("  url varchar(2000) null,")
      _T("  description varchar(2000) null,")
      _T("  PRIMARY KEY(object_id,url_id))")));
   CHK_EXEC(SetSchemaVersion(442));
   return TRUE;
}

/**
 * Upgrade from V440 to V441
 */
static BOOL H_UpgradeFromV440(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("TopologyPollingInterval"), _T("1800"), _T("Interval in seconds between topology polls"), 'I', true, true, false, false));
   CHK_EXEC(SetSchemaVersion(441));
   return TRUE;
}

/**
 * Upgrade from V439 to V440
 */
static BOOL H_UpgradeFromV439(int currVersion, int newVersion)
{
   static const TCHAR *batch =
            _T("UPDATE config SET description='Enable/disable automatic deletion of subnet objects without any nodes within.' WHERE var_name='DeleteEmptySubnets'\n")
            _T("UPDATE config SET description='Instance polling interval (in seconds).' WHERE var_name='InstancePollingInterval'\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(440));
   return TRUE;
}

/**
 * Upgrade from V438 to V439
 */
static BOOL H_UpgradeFromV438(int currVersion, int newVersion)
{
	static const TCHAR *batch =
            _T("UPDATE config SET data_type='S' WHERE var_name='LdapUserUniqueId'\n")
            _T("UPDATE config SET data_type='S' WHERE var_name='LdapGroupUniqueId'\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(439));
   return TRUE;
}

/**
 * Upgrade from V437 to V438
 */
static BOOL H_UpgradeFromV437(int currVersion, int newVersion)
{
   static const TCHAR *batch =
            _T("ALTER TABLE snmp_trap_cfg ADD guid varchar(36)\n")
            _T("UPDATE snmp_trap_cfg SET guid='5d01e7e5-edbb-46ce-b53c-f7f64d1bf8ff' WHERE trap_id=1\n")
            _T("UPDATE snmp_trap_cfg SET guid='c5464919-fd76-4624-9c21-b6ab73d9df80' WHERE trap_id=2\n")
            _T("UPDATE snmp_trap_cfg SET guid='44d3b32e-33c5-4a39-b2ad-990a1120155d' WHERE trap_id=3\n")
            _T("UPDATE snmp_trap_cfg SET guid='c9660f48-a4b3-41c8-b3f9-e9a6a8129db5' WHERE trap_id=4\n")
            _T("UPDATE snmp_trap_cfg SET guid='4b422ba6-4b45-4881-931a-ed38dc798f9f' WHERE trap_id=5\n")
            _T("UPDATE snmp_trap_cfg SET guid='bd8b6971-a3e4-4cad-9c70-3a33e61e0913' WHERE trap_id=6\n")
            _T("ALTER TABLE script_library ADD guid varchar(36)\n")
            _T("UPDATE script_library SET guid='3b7bddce-3505-42ff-ac60-6a48a64bd0ae' WHERE script_id=1\n")
            _T("UPDATE script_library SET guid='2fb9212b-97e6-40e7-b434-2df4f7e8f6aa' WHERE script_id=2\n")
            _T("UPDATE script_library SET guid='38696a00-c519-438c-8cbd-4b3a0cba4af1' WHERE script_id=3\n")
            _T("UPDATE script_library SET guid='efe50915-47b2-43d8-b4f4-2c09a44970c3' WHERE script_id=4\n")
            _T("UPDATE script_library SET guid='7837580c-4054-40f2-981f-7185797fe7d7' WHERE script_id=11\n")
            _T("UPDATE script_library SET guid='f7d1bc7e-4046-4ee4-adb2-718f7361984d' WHERE script_id=12\n")
            _T("UPDATE script_library SET guid='048fcf32-765b-4702-9c70-f012f62d5a90' WHERE script_id=13\n")
            _T("UPDATE script_library SET guid='d515c10f-a5c9-4f41-afcd-9ddc8845f288' WHERE script_id=14\n")
            _T("UPDATE script_library SET guid='7cd1c471-2f14-4fae-8743-8899fed64d18' WHERE script_id=15\n")
            _T("UPDATE script_library SET guid='befdb083-ac68-481d-a7b7-127e11c3fae0' WHERE script_id=16\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   DB_RESULT hResult = DBSelect(g_hCoreDB, _T("SELECT trap_id FROM snmp_trap_cfg WHERE guid IS NULL"));
   if (hResult != NULL)
   {
      DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("UPDATE snmp_trap_cfg SET guid=? WHERE trap_id=?"));
      if (hStmt != NULL)
      {
         int numRows = DBGetNumRows(hResult);
         for(int i = 0; i < numRows; i++)
         {
            uuid guid = uuid::generate();
            DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 0));

            if (!SQLExecute(hStmt))
            {
               if (!g_bIgnoreErrors)
               {
                  DBFreeStatement(hStmt);
                  DBFreeResult(hResult);
                  return FALSE;
               }
            }
         }
         DBFreeStatement(hStmt);
      }
      DBFreeResult(hResult);
   }

   hResult = DBSelect(g_hCoreDB, _T("SELECT guid,script_id FROM script_library WHERE guid IS NULL"));
   if (hResult != NULL)
   {
      DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("UPDATE script_library SET guid=? WHERE script_id=?"));
      if (hStmt != NULL)
      {
         int numRows = DBGetNumRows(hResult);
         for(int i = 0; i < numRows; i++)
         {
            uuid guid = uuid::generate();
            DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 1));
            if (!SQLExecute(hStmt))
            {
               if (!g_bIgnoreErrors)
               {
                  DBFreeStatement(hStmt);
                  DBFreeResult(hResult);
                  return FALSE;
               }
            }
         }
         DBFreeStatement(hStmt);
      }
      DBFreeResult(hResult);
   }

   CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("snmp_trap_cfg"), _T("guid")));
   CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("script_library"), _T("guid")));
   CHK_EXEC(SetSchemaVersion(438));
   return TRUE;
}

/**
 * Upgrade from V436 to V437
 */
static BOOL H_UpgradeFromV436(int currVersion, int newVersion)
{
   CHK_EXEC(
      CreateEventTemplate(EVENT_ROUTING_LOOP_DETECTED, _T("SYS_ROUTING_LOOP_DETECTED"),
         SEVERITY_MAJOR, EF_LOG, _T("98276f42-dc85-41a5-b449-6ba83d1a71b7"),
         _T("Routing loop detected for destination %3 (selected route %6/%7 via %9)"),
         _T("Generated when server detects routing loop during network path trace.\r\n")
         _T("Source of the event is node which routes packet back to already passed hop.\r\n")
         _T("Parameters:\r\n")
         _T("   1) Protocol (IPv4 or IPv6)\r\n")
         _T("   2) Path trace destination node ID\r\n")
         _T("   3) Path trace destination address\r\n")
         _T("   4) Path trace source node ID\r\n")
         _T("   5) Path trace source node address\r\n")
         _T("   6) Routing prefix (subnet address)\r\n")
         _T("   7) Routing prefix length (subnet mask length)\r\n")
         _T("   8) Next hop node ID\r\n")
         _T("   9) Next hop address"))
      );
   CHK_EXEC(SetSchemaVersion(437));
   return TRUE;
}

/**
 * Upgrade from V435 to V436
 */
static BOOL H_UpgradeFromV435(int currVersion, int newVersion)
{
   static const TCHAR *batch =
            _T("ALTER TABLE nodes ADD agent_comp_mode char(1)\n")
            _T("UPDATE nodes SET agent_comp_mode='0'\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("nodes"), _T("agent_comp_mode")));
   CHK_EXEC(SetSchemaVersion(436));
   return TRUE;
}

/**
 * Upgrade from V434 to V435
 */
static BOOL H_UpgradeFromV434(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("DefaultAgentProtocolCompressionMode"), _T("1"), _T("Default agent protocol compression mode"), 'C', true, false, false, false));
   static const TCHAR *batch =
            _T("UPDATE config SET data_type='C',description='Default agent cache mode' WHERE var_name='DefaultAgentCacheMode'\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('DefaultAgentCacheMode','1','On')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('DefaultAgentCacheMode','2','Off')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('DefaultAgentProtocolCompressionMode','1','Enabled')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('DefaultAgentProtocolCompressionMode','2','Disabled')\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(435));
   return TRUE;
}

/**
 * Upgrade from V433 to V434
 */
static BOOL H_UpgradeFromV433(int currVersion, int newVersion)
{
   CHK_EXEC(
      CreateEventTemplate(EVENT_IF_EXPECTED_STATE_UP, _T("SYS_IF_EXPECTED_STATE_UP"),
         SEVERITY_NORMAL, EF_LOG, _T("4997c3f5-b332-4077-8e99-983142f0e193"),
         _T("Expected state for interface \"%2\" set to UP"),
         _T("Generated when interface expected state set to UP.\r\n")
         _T("Please note that source of event is node, not an interface itself.\r\n")
         _T("Parameters:\r\n")
         _T("   1) Interface index\r\n")
         _T("   2) Interface name"))
      );

   CHK_EXEC(
      CreateEventTemplate(EVENT_IF_EXPECTED_STATE_DOWN, _T("SYS_IF_EXPECTED_STATE_DOWN"),
         SEVERITY_NORMAL, EF_LOG, _T("75de536c-4861-4f19-ba56-c43d814431d7"),
         _T("Expected state for interface \"%2\" set to DOWN"),
         _T("Generated when interface expected state set to DOWN.\r\n")
         _T("Please note that source of event is node, not an interface itself.\r\n")
         _T("Parameters:\r\n")
         _T("   1) Interface index\r\n")
         _T("   2) Interface name"))
      );

   CHK_EXEC(
      CreateEventTemplate(EVENT_IF_EXPECTED_STATE_IGNORE, _T("SYS_IF_EXPECTED_STATE_IGNORE"),
         SEVERITY_NORMAL, EF_LOG, _T("0e488c0e-3340-4e02-ad96-b999b8392e55"),
         _T("Expected state for interface \"%2\" set to IGNORE"),
         _T("Generated when interface expected state set to IGNORE.\r\n")
         _T("Please note that source of event is node, not an interface itself.\r\n")
         _T("Parameters:\r\n")
         _T("   1) Interface index\r\n")
         _T("   2) Interface name"))
      );

   CHK_EXEC(AddEventToEPPRule(_T("6f46d451-ee66-4563-8747-d129877df24d"), EVENT_IF_EXPECTED_STATE_DOWN));
   CHK_EXEC(AddEventToEPPRule(_T("6f46d451-ee66-4563-8747-d129877df24d"), EVENT_IF_EXPECTED_STATE_IGNORE));
   CHK_EXEC(AddEventToEPPRule(_T("ecc3fb57-672d-489d-a0ef-4214ea896e0f"), EVENT_IF_EXPECTED_STATE_UP));
   CHK_EXEC(AddEventToEPPRule(_T("ecc3fb57-672d-489d-a0ef-4214ea896e0f"), EVENT_IF_EXPECTED_STATE_IGNORE));

   CHK_EXEC(SetSchemaVersion(434));
   return TRUE;
}

/**
 * Upgrade from V432 to V433
 */
static BOOL H_UpgradeFromV432(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("UseSyslogForDiscovery"), _T("0"), _T("Use syslog messages for new node discovery."), 'B', true, false, false, false));
   CHK_EXEC(SetSchemaVersion(433));
   return TRUE;
}

/**
 * Upgrade from V431 to V432
 */
static BOOL H_UpgradeFromV431(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_syslog_source ON syslog(source_object_id)")));
   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_snmp_trap_log_oid ON snmp_trap_log(object_id)")));
   CHK_EXEC(SetSchemaVersion(432));
   return TRUE;
}

/**
 * Upgrade from V430 to V431
 */
static BOOL H_UpgradeFromV430(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM config_values WHERE var_name='SNMPPorts'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Comma separated list of UDP ports used by SNMP capable devices.' WHERE var_name='SNMPPorts'")));
   CHK_EXEC(SetSchemaVersion(431));
   return TRUE;
}

/**
 * Upgrade from V429 to V430
 */
static BOOL H_UpgradeFromV429(int currVersion, int newVersion)
{
   CHK_EXEC(CreateTable(
            _T("CREATE TABLE policy_pstorage_actions (")
            _T("rule_id integer not null,")
            _T("ps_key varchar(255) not null,")
            _T("value varchar(2000) null,")
            _T("action integer not null,")
            _T("PRIMARY KEY(rule_id,ps_key,action))")));

   CHK_EXEC(CreateTable(
            _T("CREATE TABLE persistent_storage (")
            _T("entry_key varchar(255) not null,")
            _T("value varchar(2000) null,")
            _T("PRIMARY KEY(entry_key))")));

   //Move previous attrs form situations to pstorage
   DB_RESULT hResult = SQLSelect(_T("SELECT event_policy.rule_id,situations.name,event_policy.situation_instance,")
                                 _T("policy_situation_attr_list.attr_name,policy_situation_attr_list.attr_value ")
                                 _T("FROM event_policy,situations,policy_situation_attr_list ")
                                 _T("WHERE event_policy.rule_id=policy_situation_attr_list.rule_id ")
                                 _T("AND situations.id=policy_situation_attr_list.situation_id"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("INSERT INTO policy_pstorage_actions (rule_id,ps_key,value,action) VALUES (?,?,?,1)"));
      if (hStmt != NULL)
      {
         String key;
         for(int i = 0; i < count; i++)
         {
            TCHAR tmp[256];
            DBGetField(hResult, i, 1, tmp, 256);
            key.append(tmp);
            DBGetField(hResult, i, 2, tmp, 256);
            key.append(_T("."));
            key.append(tmp);
            DBGetField(hResult, i, 3, tmp, 256);
            key.append(_T("."));
            key.append(tmp);
            DBGetField(hResult, i, 4, tmp, 256);

            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, DBGetFieldLong(hResult, i, 0));
            DBBind(hStmt, 2, DB_SQLTYPE_TEXT, key, DB_BIND_STATIC, 512);
            DBBind(hStmt, 3, DB_SQLTYPE_TEXT, tmp, DB_BIND_STATIC);
            if (!SQLExecute(hStmt))
            {
               if (!g_bIgnoreErrors)
               {
                  DBFreeStatement(hStmt);
                  DBFreeResult(hResult);
                  return FALSE;
               }
            }
            key.clear();
         }
         DBFreeStatement(hStmt);
      }
      else if (!g_bIgnoreErrors)
      {
         DBFreeStatement(hStmt);
         DBFreeResult(hResult);
         return FALSE;
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return false;
   }

   CHK_EXEC(SQLQuery(_T("DROP TABLE situations")));
   CHK_EXEC(SQLQuery(_T("DROP TABLE policy_situation_attr_list")));
   CHK_EXEC(DBDropColumn(g_hCoreDB, _T("event_policy"), _T("situation_id")));
   CHK_EXEC(DBDropColumn(g_hCoreDB, _T("event_policy"), _T("situation_instance")));
   CHK_EXEC(SetSchemaVersion(430));
   return TRUE;
}

/**
 *  Upgrade from V428 to V429
 */
static BOOL H_UpgradeFromV428(int currVersion, int newVersion)
{
   CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("config"), _T("description"), 450, true));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Interval in seconds between active network discovery polls.' WHERE var_name='ActiveDiscoveryInterval'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable active network discovery. This setting is change by Network Discovery GUI' WHERE var_name='ActiveNetworkDiscovery'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Timeout in milliseconds for commands sent to agent. If agent did not respond to command within given number of seconds, \ncommand considered as failed.' WHERE var_name='AgentCommandTimeout'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='String that will be used as a shared secret in case if agent will required authentication.' WHERE var_name='AgentDefaultSharedSecret'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Maximum wait time in seconds for agent restart after upgrade. If agent cannot be contacted after this time period, \nupgrade process is considered as failed.' WHERE var_name='AgentUpgradeWaitTime'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='A number of days the server keeps an alarm history in the database.' WHERE var_name='AlarmHistoryRetentionTime'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Maximum alarm count that will be displayed on Alarm Browser page. Alarms that exceed this count will not be shown.' WHERE var_name='AlarmListDisplayLimit'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='A semicolon separated list of alarm summary e-mail recipient addresses.' WHERE var_name='AlarmSummaryEmailRecipients'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Schedule for sending alarm summary e-mails in cron format' WHERE var_name='AlarmSummaryEmailSchedule'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Allow/disallow sending of SMS via NetXMS server using nxsms utility.' WHERE var_name='AllowDirectSMS'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='A bitmask for encryption algorithms allowed in the server(sum the values to allow multiple algorithms at once): \n\t*1 - AES256 \n\t*2 - Blowfish-256 \n\t*4 - IDEA \n\t*8 - 3DES\n\t*16 - AES128\n\t*32 - Blowfish-128' WHERE var_name='AllowedCiphers'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='' WHERE var_name='AllowTrapVarbindsConversion'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='' WHERE var_name='AnonymousFileAccess'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable applying all DCIs from a template to the node, including disabled ones.' WHERE var_name='ApplyDCIFromTemplateToDisabledDCI'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Retention time in days for the records in audit log. All records older than specified will be deleted by housekeeping process.' WHERE var_name='AuditLogRetentionTime'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Comma-separated list of hosts to be used as beacons for checking NetXMS server network connectivity. Either DNS names \nor IP addresses can be used. This list is pinged by NetXMS server and if none of the hosts have responded, server considers that connection \nwith network is lost and generates specific event.' WHERE var_name='BeaconHosts'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Interval in milliseconds between beacon hosts polls.' WHERE var_name='BeaconPollingInterval'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Timeout in milliseconds to consider beacon host unreachable.' WHERE var_name='BeaconTimeout'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='' WHERE var_name='BlockInactiveUserAccounts'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='CapabilityExpirationTime'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable case insensitive login names' WHERE var_name='CaseInsensitiveLoginNames'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable trusted nodes check' WHERE var_name='CheckTrustedNodes'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='The server port for incoming client connections (such as management console).' WHERE var_name='ClientListenerPort'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable container auto binding for clusters.' WHERE var_name='ClusterContainerAutoBind'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable template auto apply for clusters.' WHERE var_name='ClusterTemplateAutoApply'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Interval in seconds between polling (re-evaluating) of condition objects.' WHERE var_name='ConditionPollingInterval'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Interval in seconds between configuration polls.' WHERE var_name='ConfigurationPollingInterval'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable data interpolation in dashboard data export.' WHERE var_name='DashboardDataExportEnableInterpolation'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='A number of connections to the database created on server startup.' WHERE var_name='DBConnectionPoolBaseSize'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='DBConnectionPoolCooldownTime'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='DBConnectionPoolMaxLifetime'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='A maximum number of connections in the connection pool.' WHERE var_name='DBConnectionPoolMaxSize'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='' WHERE var_name='DBLockInfo'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='DBLockPID'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='' WHERE var_name='DBLockStatus'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='DefaultAgentCacheMode'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Default date display format for GUI.' WHERE var_name='DefaultConsoleDateFormat'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Default short time display format for GUI.' WHERE var_name='DefaultConsoleShortTimeFormat'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Default long time display format for GUI.' WHERE var_name='DefaultConsoleTimeFormat'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Default polling interval for newly created DCI (in seconds).' WHERE var_name='DefaultDCIPollingInterval'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Default retention time for newly created DCI (in days).' WHERE var_name='DefaultDCIRetentionTime'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='C',description='Set the default encryption policy for communications with agents.' WHERE var_name='DefaultEncryptionPolicy'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='C',description='Default expected state for new interface objects.' WHERE var_name='DefaultInterfaceExpectedState'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='H',description='Default background color for new network map objects.' WHERE var_name='DefaultMapBackgroundColor'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Default mask for synthetic IPv4 subnets.' WHERE var_name='DefaultSubnetMaskIPv4'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Default mask for synthetic IPv6 subnets.' WHERE var_name='DefaultSubnetMaskIPv6'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable automatic alarm removal of an object when it is deleted' WHERE var_name='DeleteAlarmsOfDeletedObject'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable automatic deletion of subnet objects without any nodes within. When enabled, empty subnets will be deleted \nby housekeeping process.' WHERE var_name='DeleteEmptySubnets'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable automatic event removal of an object when it is deleted.' WHERE var_name='DeleteEventsOfDeletedObject'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Delete nodes which were unreachable for a number of days specified by this parameter.' WHERE var_name='DeleteUnreachableNodesPeriod'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='' WHERE var_name='DiscoveryFilter'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='DiscoveryFilterFlags'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Interval in seconds between passive network discovery polls.' WHERE var_name='DiscoveryPollingInterval'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='' WHERE var_name='EnableAdminInterface'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable agent self-registration' WHERE var_name='EnableAgentRegistration'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable audit log.' WHERE var_name='EnableAuditLog'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='' WHERE var_name='EnableCheckPointSNMP'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='' WHERE var_name='EnableReportingServer'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='' WHERE var_name='EnableEventStormDetection'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable Inter-Server Communications Listener.' WHERE var_name='EnableISCListener'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable server-side NXSL functions for container management (such as CreateContainer, RemoveContainer, BindObject, \nUnbindObject).' WHERE var_name='EnableNXSLContainerFunctions.'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='' WHERE var_name='EnableObjectTransactions'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable SNMP trap processing.' WHERE var_name='EnableSNMPTraps'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable alarm summary e-mails.' WHERE var_name='EnableAlarmSummaryEmails'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable receiving of syslog messages.' WHERE var_name='EnableSyslogReceiver'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='' WHERE var_name='EnableTimedAlarmAck'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable XMPP connector (required to enable XMPP message sending).' WHERE var_name='EnableXMPPConnector'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable zoning support' WHERE var_name='EnableZoning'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='' WHERE var_name='EscapeLocalCommands'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='The retention time of event logs' WHERE var_name='EventLogRetentionTime'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='EventStormDuration'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Event storm events per second' WHERE var_name='EventStormEventsPerSecond'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable extended access control in log queries.' WHERE var_name='ExtendedLogQueryAccessControl'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Syslog facility to be used in audit log records sent to external server.' WHERE var_name='ExternalAuditFacility'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='UDP port of external syslog server to send audit records to.' WHERE var_name='ExternalAuditPort'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='External syslog server to send audit records to. If set to ‘’none’‘, external audit logging is disabled.' WHERE var_name='ExternalAuditServer'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Syslog severity to be used in audit log records sent to external server.' WHERE var_name='ExternalAuditSeverity'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Syslog tag to be used in audit log records sent to external server.' WHERE var_name='ExternalAuditTag'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='FirstFreeObjectId'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='FixedStatusValue'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='' WHERE var_name='HelpDeskLink'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Time when housekeeper starts. Housekeeper deletes old log lines, old DCI data, cleans removed objects and does VACUUM for \nPostgreSQL.' WHERE var_name='HousekeeperStartTime'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Size of ICMP packets (in bytes, excluding IP header size) used for status polls.' WHERE var_name='IcmpPingSize'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Timeout for ICMP ping used for status polls (in milliseconds).' WHERE var_name='IcmpPingTimeout'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Import configuration from local files on server startup.' WHERE var_name='ImportConfigurationOnStartup'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Instance polling interval (in milliseconds).' WHERE var_name='InstancePollingInterval'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable internal certificate authority.' WHERE var_name='InternalCA'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='IntruderLockoutThreshold'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='IntruderLockoutTime'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='' WHERE var_name='JiraIssueType'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Jira login name.' WHERE var_name='JiraLogin'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Jira password' WHERE var_name='JiraPassword'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Jira project code' WHERE var_name='JiraProjectCode'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='' WHERE var_name='JiraProjectComponent'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='The URL of the Jira server' WHERE var_name='JiraServerURL'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='JobHistoryRetentionTime'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Maximum mumber of job execution retrys.' WHERE var_name='JobRetryCount'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Interval in seconds between sending keep alive packets to connected clients.' WHERE var_name='KeepAliveInterval'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Specifies which object class represents group objects. If the found entry is not of user or group class, it will be ignored.' WHERE var_name='LdapGroupClass'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='LdapGroupUniqueId'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='The LdapConnectionString configuration parameter may be a comma- or whitespace-separated list of URIs containing only the \nschema, the host, and the port fields. Format: schema://host:port.' WHERE var_name='LdapConnectionString'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='The name of an attribute whose value will be used as a user`s description.' WHERE var_name='LdapMappingDescription'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='The name of an attribute whose value will be used as a user`s full name.' WHERE var_name='LdapMappingFullName'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='The name of an attribute whose value will be used as a user`s login name.' WHERE var_name='LdapMappingName'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='The maximum amount of records that can be returned in one search page.' WHERE var_name='LdapPageSize'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='The DN of the entry at which to start the search.' WHERE var_name='LdapSearchBase'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='A string representation of the filter to apply in the search.' WHERE var_name='LdapSearchFilter'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='The synchronization interval (in minutes) between the NetXMS server and the LDAP server. If the parameter is set to 0, no \nsynchronization will take place.' WHERE var_name='LdapSyncInterval'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='User login for LDAP synchronization.' WHERE var_name='LdapSyncUser'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='User password for LDAP synchronization.' WHERE var_name='LdapSyncUserPassword'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='The object class which represents user objects. If the found entry is not of user or group class, it will be ignored.' WHERE var_name='LdapUserClass'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='C',description='This parameter specifies what should be done while synchronizing with a deleted user/group from LDAP.' WHERE var_name='LdapUserDeleteAction'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='LdapUserUniqueId'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='LockTimeout'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Log all SNMP traps.' WHERE var_name='LogAllSNMPTraps'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Encoding for e-mails generated by NetXMS server.' WHERE var_name='MailEncoding'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Message to be shown when a user logs into the console.' WHERE var_name='MessageOfTheDay'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Default minimum password length for a NetXMS user. The default applied only if per-user setting is not defined.' WHERE var_name='MinPasswordLength'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='MinViewRefreshInterval'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='MobileDeviceListenerPort'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='The number of threads used for data collection.' WHERE var_name='NumberOfDataCollectors'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='The number of threads used to perform agent upgrades (i.e. maximum number of parallel upgrades).' WHERE var_name='NumberOfUpgradeThreads'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Time period in seconds within which received offline data still relevant for threshold validation.' WHERE var_name='OfflineDataRelevanceTime'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Set of flags to enforce password complexity.' WHERE var_name='PasswordComplexity'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Password expiration time in days. If set to 0, password expiration is disabled.' WHERE var_name='PasswordExpiration'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Number of previous passwords to keep. Users are not allowed to set password if it matches one from previous passwords list.' WHERE var_name='PasswordHistoryLength'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='The number of consecutive unsuccessful polls required to declare interface as down.' WHERE var_name='PollCountForStatusChange'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='The base thread pool size.' WHERE var_name='PollerThreadPoolBaseSize'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Maximum thread pool size.' WHERE var_name='PollerThreadPoolMaxSize'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='RADIUS authentication method to be used (PAP, CHAP, MS-CHAPv1, MS-CHAPv2).' WHERE var_name='RADIUSAuthMethod'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='The number of retries for RADIUS authentication.' WHERE var_name='RADIUSNumRetries'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Port number used for connection to primary RADIUS server.' WHERE var_name='RADIUSPort'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Port number used for connection to secondary RADIUS server.' WHERE var_name='RADIUSSecondaryPort'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Shared secret used for communication with secondary RADIUS server.' WHERE var_name='RADIUSSecondarySecret'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Host name or IP address of secondary RADIUS server.' WHERE var_name='RADIUSSecondaryServer'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Shared secret used for communication with primary RADIUS server.' WHERE var_name='RADIUSSecret'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Host name or IP address of primary RADIUS server.' WHERE var_name='RADIUSServer'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Timeout in seconds for requests to RADIUS server.' WHERE var_name='RADIUSTimeout'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable reception of events forwarded by another NetXMS server. Please note that for external event reception ISC listener \nshould be enabled as well.' WHERE var_name='ReceiveForwardedEvents'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='The hostname of the reporting server.' WHERE var_name='ReportingServerHostname'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='The port of the reporting server.' WHERE var_name='ReportingServerPort'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Resolve DNS to IP on status poll.' WHERE var_name='ResolveDNSToIPOnStatusPoll'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='' WHERE var_name='ResolveNodeNames'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Interval in seconds between reading routing table from node.' WHERE var_name='RoutingTableUpdateInterval'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable automatic network discovery process. *This setting is changed by Network Discovery in the GUI*.' WHERE var_name='RunNetworkDiscovery'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='H',description='Identification color for this server' WHERE var_name='ServerColor'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='ServerCommandOutputTimeout'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Name of this server' WHERE var_name='ServerName'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Mobile phone driver to be used for sending SMS.' WHERE var_name='SMSDriver'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='SMS driver parameters. For ‘’generic’’ driver, it should be the name of COM port device.' WHERE var_name='SMSDrvConfig'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='The address used for sending mail from.' WHERE var_name='SMTPFromAddr'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='The name used as the sender.' WHERE var_name='SMTPFromName'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Number of retries for sending mail.' WHERE var_name='SMTPRetryCount'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='TCP port for SMTP server.' WHERE var_name='SNMPPorts'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='An SMTP server used for sending mail.' WHERE var_name='SMTPServer'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Timeout in milliseconds for SNMP requests sent by NetXMS server.' WHERE var_name='SNMPRequestTimeout'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='The time how long SNMP trap logs are retained.' WHERE var_name='SNMPTrapLogRetentionTime'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Port used for SNMP traps.' WHERE var_name='SNMPTrapPort'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='StatusCalculationAlgorithm'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Interval in seconds between status polls.' WHERE var_name='StatusPollingInterval'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='C',description='Algorithm for status propagation (how object’s status affects its child object statuses).' WHERE var_name='StatusPropagationAlgorithm'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='StatusShift'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='StatusSingleThreshold'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='' WHERE var_name='StatusThresholds'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='' WHERE var_name='StatusTranslation'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable strict alarm status flow (alarm can be terminated only after it has been resolved).' WHERE var_name='StrictAlarmStatusFlow'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Interval in seconds between writing object changes to the database.' WHERE var_name='SyncInterval'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable synchronization of node names with DNS on each configuration poll.' WHERE var_name='SyncNodeNamesWithDNS'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Ignore timestamp received in syslog messages and always use server time.' WHERE var_name='SyslogIgnoreMessageTimestamp'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='UDP port used by built-in syslog server.' WHERE var_name='SyslogListenPort'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='C',description='Node matching policy for built-in syslog daemon.' WHERE var_name='SyslogNodeMatchingPolicy'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Retention time in days for records in syslog. All records older than specified will be deleted by housekeeping process.' WHERE var_name='SyslogRetentionTime'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='System-wide interval in seconds for resending threshold violation events. Value of 0 disables event resending.' WHERE var_name='ThresholdRepeatInterval'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='The URL for the Tile server.' WHERE var_name='TileServerURL'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='TopologyDiscoveryRadius'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='' WHERE var_name='TopologyExpirationTime'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Search all zones to match trap/syslog source address to node.' WHERE var_name='TrapSourcesInAllZones'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable the use of DNS name instead of IP address as primary name for newly discovered nodes.' WHERE var_name='UseDNSNameForDiscoveredNodes'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable the use of fully qualified domain names as primary names for newly discovered nodes.' WHERE var_name='UseFQDNForNodeNames'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Enable/disable the use of SNMP ifXTable instead of ifTable for interface configuration polling.' WHERE var_name='UseIfXTable'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='C',description='Control usage of interface aliases (or descriptions).' WHERE var_name='UseInterfaceAliases'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='B',description='Use SNMP trap information for new node discovery.' WHERE var_name='UseSNMPTrapsForDiscovery'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Login name that will be used to authentication on XMPP server.' WHERE var_name='XMPPLogin'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='Password that will be used to authentication on XMPP server.' WHERE var_name='XMPPPassword'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='XMPP connection port.' WHERE var_name='XMPPPort'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='S',description='XMPP connection server.' WHERE var_name='XMPPServer'")));

   CHK_EXEC(CreateTable(
               _T("CREATE TABLE config_values (")
               _T("var_name varchar(63) not null,")
               _T("var_value varchar(15) not null,")
               _T("var_description varchar(255) null,")
               _T("PRIMARY KEY(var_name,var_value))")));

   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value) VALUES ('ClientListenerPort','65535')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value) VALUES ('ExternalAuditPort','65535')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value) VALUES ('MobileDeviceListenerPort','65535')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value) VALUES ('RADIUSPort','65535')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value) VALUES ('RADIUSSecondaryPort','65535')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value) VALUES ('ReportingServerPort','65535')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value) VALUES ('SNMPPorts','65535')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value) VALUES ('SNMPTrapPort','65535')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value) VALUES ('SyslogListenPort','65535')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value) VALUES ('XMPPPort','65535')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value) VALUES ('AllowedCiphers','63')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('DefaultInterfaceExpectedState','0','UP')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('DefaultInterfaceExpectedState','1','DOWN')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('DefaultInterfaceExpectedState','2','IGNORE')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('DefaultEncryptionPolicy','0','Disabled')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('DefaultEncryptionPolicy','1','Allowed')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('DefaultEncryptionPolicy','2','Preferred')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('DefaultEncryptionPolicy','3','Required')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('StatusPropagationAlgorithm','0','Default')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('StatusPropagationAlgorithm','1','Unchanged')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('StatusPropagationAlgorithm','2','Fixed')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('StatusPropagationAlgorithm','3','Relative')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('StatusPropagationAlgorithm','4','Translated')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('UseInterfaceAliases','0','Don`t use aliases')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('UseInterfaceAliases','1','Use aliases when possible')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('UseInterfaceAliases','2','Concatenate alias and name')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('UseInterfaceAliases','3','Concatenate name and alias')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SyslogNodeMatchingPolicy','0','IP, then hostname')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SyslogNodeMatchingPolicy','1','Hostname, then IP')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('LdapUserDeleteAction','0','Delete user')")));
   CHK_EXEC(SQLQuery(_T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('LdapUserDeleteAction','1','Disable user')")));

   CHK_EXEC(SetSchemaVersion(429));
   return TRUE;
}

/**
 * Upgrade from V427 to V428
 */
static BOOL H_UpgradeFromV427(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("ClusterContainerAutoBind"), _T("0"), _T("Enable/disable container auto binding for clusters"), 'B', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("ClusterTemplateAutoApply"), _T("0"), _T("Enable/disable template auto apply for clusters"), 'B', true, false, false, false));
   CHK_EXEC(SetSchemaVersion(428));
   return TRUE;
}

/**
 * Upgrade from V426 to V427
 */
static BOOL H_UpgradeFromV426(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD port_rows integer")));
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD port_numbering_scheme integer")));
   CHK_EXEC(SetSchemaVersion(427));
   return TRUE;
}

/**
 * Upgrade from V425 to V426
 */
static BOOL H_UpgradeFromV425(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("EnableAlarmSummaryEmails"), _T(""), _T("Enable alarm summary e-mails"), 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("AlarmSummaryEmailSchedule"), _T(""), _T("Schedule for sending alarm summary e-mails in cron format"), 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("AlarmSummaryEmailRecipients"), _T(""), _T("A semicolon separated list of alarm summary e-mail recipient addresses"), 'S', true, false, false, false));
   CHK_EXEC(SetSchemaVersion(426));
   return TRUE;
}

/**
 * Upgrade from V424 to V425
 */
static BOOL H_UpgradeFromV424(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE scheduled_tasks ADD comments varchar(255)")));
   CHK_EXEC(SetSchemaVersion(425));
   return TRUE;
}

/**
 * Upgrade from V423 to V424
 */
static BOOL H_UpgradeFromV423(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("MessageOfTheDay"), _T(""), _T("Message to be shown when a user logs into the console"), 'S', true, false, false, false));
   CHK_EXEC(SetSchemaVersion(424));
   return TRUE;
}

/**
 * Upgrade from V422 to V423
 */
static BOOL H_UpgradeFromV422(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("RADIUSAuthMethod"), _T("PAP"), _T("RADIUS authentication method to be used (PAP, CHAP, MS-CHAPv1, MS-CHAPv2)"), 'S', true, false, false, false));
   CHK_EXEC(SetSchemaVersion(423));
   return TRUE;
}

/**
 * Upgrade from V421 to V422
 */
static BOOL H_UpgradeFromV421(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("ServerColor"), _T(""), _T("Identification color for this server"), 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("ServerName"), _T(""), _T("Name of this server"), 'S', true, false, false, false));
   CHK_EXEC(SetSchemaVersion(422));
   return TRUE;
}

/**
 * Upgrade from V420 to V421
 */
static BOOL H_UpgradeFromV420(int currVersion, int newVersion)
{
   DB_RESULT hResult = SQLSelect(_T("SELECT access_rights,user_id,object_id FROM acl"));
   if (hResult != NULL)
   {
      UINT32 rights;
      DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("UPDATE acl SET access_rights=? WHERE user_id=? AND object_id=?"));
      if (hStmt != NULL)
      {
         for(int i = 0; i < DBGetNumRows(hResult); i++)
         {
            rights = DBGetFieldULong(hResult, i, 0);
            if ((rights & 0x2) == 0x2)
            {
               rights = rights | 0x8000; // Add Manage maintenance access right to all users which have Modify object access right
               DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, rights);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 1));
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 2));
               SQLExecute(hStmt);
            }
         }
         DBFreeStatement(hStmt);
      }
      else
      {
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

   CHK_EXEC(SetSchemaVersion(421));
   return TRUE;
}

/**
 * Upgrade from V419 to V420
 */
static BOOL H_UpgradeFromV419(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("EnableReportingServer"), _T("0"), 1, 1));
   CHK_EXEC(CreateConfigParam(_T("ReportingServerHostname"), _T("localhost"), 1, 1));
   CHK_EXEC(CreateConfigParam(_T("ReportingServerPort"), _T("4710"), 1, 1));
   CHK_EXEC(SetSchemaVersion(420));
   return TRUE;
}

/**
 * Upgrade from V418 to V419
 */
static BOOL H_UpgradeFromV418(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE items ADD npe_name varchar(15)")));
   CHK_EXEC(SetSchemaVersion(419));
   return TRUE;
}

/**
 * Upgrade from V417 to V418
 */
static BOOL H_UpgradeFromV417(int currVersion, int newVersion)
{
   // Update in object tools objectToolFilter objectMenuFilter
   // move object tool flags to filter structure
   DB_RESULT hResult = SQLSelect(_T("SELECT tool_id,flags,tool_filter FROM object_tools"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         int filteringFlag = 0;
         int objectToolFlag = DBGetFieldLong(hResult, i, 1);

         // Separate and reorder flags for filter and for object tools
         for(int j = 1; j < 5; j = j << 1) //REQUIRES_SNMP 0x01, REQUIRES_AGENT 0x02, REQUIRES_OID_MATCH 0x04
         {
            if ((objectToolFlag & j) != 0)
            {
               objectToolFlag = objectToolFlag & ~j;
               filteringFlag = filteringFlag | j;
            }
         }

         if ((objectToolFlag & 0x08) != 0) //ASK_CONFIRMATION
         {
            objectToolFlag = objectToolFlag & ~0x08;
            objectToolFlag = objectToolFlag | 0x01;
         }

         if ((objectToolFlag & 0x10) != 0) //GENERATES_OUTPUT
         {
            objectToolFlag = objectToolFlag & ~0x10;
            objectToolFlag = objectToolFlag | 0x02;
         }

         if ((objectToolFlag & 0x20) != 0) //DISABLED
         {
            objectToolFlag = objectToolFlag & ~0x20;
            objectToolFlag = objectToolFlag | 0x04;
         }

         if ((objectToolFlag & 0x40) != 0) //SHOW_IN_COMMANDS
         {
            objectToolFlag = objectToolFlag & ~0x40;
            objectToolFlag = objectToolFlag | 0x08;
         }

         if ((objectToolFlag & 0x80) != 0) //REQUIRES_NODE_OS_MATCH
         {
            objectToolFlag = objectToolFlag & ~0x80;
            filteringFlag = filteringFlag | 0x08;
         }

         if ((objectToolFlag & 0x100) != 0) //REQUIRES_TEMPLATE_MATCH
         {
            objectToolFlag = objectToolFlag & ~0x100;
            filteringFlag = filteringFlag | 0x10;
         }

         if ((objectToolFlag & 0x10000) != 0) //SNMP_INDEXED_BY_VALUE
         {
            objectToolFlag = objectToolFlag & ~0x10000;
            objectToolFlag = objectToolFlag | 0x10;
         }

         if ((objectToolFlag & 0x20000) != 0) //REQUIRES_WORKSTATION_OS_MATCH
         {
            objectToolFlag = objectToolFlag & ~0x20000;
            filteringFlag = filteringFlag | 0x20;
         }

         // Add filter flags to XML
         TCHAR *xml = DBGetField(hResult, i, 2, NULL, 0);
         size_t len = (xml != NULL) ? _tcslen(xml) + 1024 : 1024;
         TCHAR *tmp = (TCHAR *)malloc(len * sizeof(TCHAR));
         TCHAR *ptr = (xml != NULL) ? _tcsrchr(xml, '<') : NULL;
         if (ptr != NULL)
         {
            *ptr = 0;
            _sntprintf(tmp, len, _T("%s<flags>%d</flags></objectMenuFilter>"), xml, filteringFlag);
            _tcsncpy(tmp, _T("<objectMenuFilter"), 17); //Change main tag name
         }
         else
         {
            _sntprintf(tmp, len, _T("<objectMenuFilter><flags>%d</flags></objectMenuFilter>"), filteringFlag);
         }

         DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("UPDATE object_tools SET flags=?,tool_filter=? WHERE tool_id=?"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectToolFlag);
            DBBind(hStmt, 2, DB_SQLTYPE_TEXT, tmp, DB_BIND_DYNAMIC);
            DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 0));
            if (!SQLExecute(hStmt))
            {
               if (!g_bIgnoreErrors)
               {
                  free(xml);
                  DBFreeStatement(hStmt);
                  DBFreeResult(hResult);
                  return FALSE;
               }
            }
         }
         else
         {
            free(tmp);
            if (!g_bIgnoreErrors)
            {
               free(xml);
               DBFreeResult(hResult);
               return FALSE;
            }
         }
         free(xml);
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return false;
   }

   CHK_EXEC(SetSchemaVersion(418));
   return TRUE;
}

/**
 * Upgrade from V416 to V417
 */
static BOOL H_UpgradeFromV416(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("JiraIssueType"), _T("Task"), true, false));
   CHK_EXEC(CreateConfigParam(_T("JiraLogin"), _T("netxms"), true, true));
   CHK_EXEC(CreateConfigParam(_T("JiraPassword"), _T(""), true, true));
   CHK_EXEC(CreateConfigParam(_T("JiraProjectCode"), _T("NETXMS"), true, false));
   CHK_EXEC(CreateConfigParam(_T("JiraProjectComponent"), _T(""), true, false));
   CHK_EXEC(CreateConfigParam(_T("JiraServerURL"), _T("http://localhost"), true, true));

   CHK_EXEC(SetSchemaVersion(417));
   return TRUE;
}

/**
 * Upgrade from V415 to V416
 */
static BOOL H_UpgradeFromV415(int currVersion, int newVersion)
{
   CHK_EXEC(CreateTable(
            _T("CREATE TABLE alarm_categories (")
            _T("id integer not null,")
            _T("name varchar(63) null,")
            _T("descr varchar(255) null,")
            _T("PRIMARY KEY(id))")));
   CHK_EXEC(CreateTable(
            _T("CREATE TABLE alarm_category_acl (")
            _T("category_id integer not null,")
            _T("user_id integer not null,")
            _T("PRIMARY KEY(category_id,user_id))")));
   CHK_EXEC(CreateTable(
            _T("CREATE TABLE alarm_category_map (")
            _T("alarm_id integer not null,")
            _T("category_id integer not null,")
            _T("PRIMARY KEY(alarm_id,category_id))")));

   CHK_EXEC(SQLQuery(
			   _T("ALTER TABLE alarms ADD alarm_category_ids varchar(255)")));

   // Add view all alarms system access to user group - Everyone
   DB_RESULT hResult = SQLSelect(_T("SELECT system_access FROM user_groups WHERE name='Everyone'"));
   if (hResult != NULL)
   {
      UINT64 sysAccess = DBGetFieldUInt64(hResult, 0, 0);
      sysAccess = sysAccess | SYSTEM_ACCESS_VIEW_ALL_ALARMS;

      TCHAR query[MAX_DB_STRING];
      _sntprintf(query, MAX_DB_STRING, _T("UPDATE user_groups SET system_access=") UINT64_FMT _T(" WHERE name='Everyone'"), sysAccess);
      CHK_EXEC(SQLQuery(query));

      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return false;
   }

   CHK_EXEC(SetSchemaVersion(416));
   return TRUE;
}

/**
 * Upgrade from V414 to V415
 */
static BOOL H_UpgradeFromV414(int currVersion, int newVersion)
{
   CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("dci_summary_tables"), _T("menu_path")));
   CHK_EXEC(SetSchemaVersion(415));
   return TRUE;
}

/**
 * Upgrade from V413 to V414
 */
static BOOL H_UpgradeFromV413(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='EnableSyslogReceiver' WHERE var_name='EnableSyslogDaemon'")));
   CHK_EXEC(SetSchemaVersion(414));
   return TRUE;
}

/**
 * Upgrade from V412 to V413
 */
static BOOL H_UpgradeFromV412(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("UPDATE users SET name='system',description='Built-in system account',full_name='',flags=12 WHERE id=0")));

   TCHAR password[128];
   DB_RESULT hResult = SQLSelect(_T("SELECT password FROM users WHERE id=0"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         DBGetField(hResult, 0, 0, password, 128);
      else
         _tcscpy(password, _T("3A445C0072CD69D9030CC6644020E5C4576051B1")); // Use default password "netxms"
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return false;
   }

   UINT32 userId = 1;
   hResult = SQLSelect(_T("SELECT max(id) FROM users"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         userId = DBGetFieldULong(hResult, 0, 0) + 1;
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return false;
   }

   uuid guid = uuid::generate();
   TCHAR query[1024];
   _sntprintf(query, 1024,
     _T("INSERT INTO users (id,name,password,system_access,flags,full_name,")
     _T("   description,grace_logins,auth_method,guid,")
     _T("   cert_mapping_method,cert_mapping_data,")
     _T("   auth_failures,last_passwd_change,min_passwd_length,")
     _T("   disabled_until,last_login) VALUES (%d,'admin','%s',274877906943,0,")
     _T("   '','Default administrator account',5,0,'%s',0,'',0,0,0,0,0)"),
     userId, password, (const TCHAR *)guid.toString());
   CHK_EXEC(SQLQuery(query));

   for(int i = 1; i < 10; i++)
   {
      _sntprintf(query, 256, _T("INSERT INTO acl (object_id,user_id,access_rights) VALUES (%d,%d,32767)"), i, userId);
      CHK_EXEC(SQLQuery(query));
   }

   //Add adming to Admin group if exists
   UINT32 groupId = 0;
   hResult = SQLSelect(_T("SELECT id FROM user_groups WHERE name='Admins'"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         groupId = DBGetFieldULong(hResult, 0, 0);
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return false;
   }
   if(groupId != 0)
   {
      _sntprintf(query, 256, _T("INSERT INTO user_group_members (group_id,user_id) VALUES (%d,%d)"), groupId, userId);
      CHK_EXEC(SQLQuery(query));
   }

   //Update non object ACL
   _sntprintf(query, 256, _T("UPDATE object_tools_acl SET user_id=%d WHERE user_id=0"), userId);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 256, _T("UPDATE graph_acl SET user_id=%d WHERE user_id=0"), userId);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 256, _T("UPDATE graphs SET owner_id=%d WHERE owner_id=0"), userId);
   CHK_EXEC(SQLQuery(query));


   CHK_EXEC(SetSchemaVersion(413));
   return TRUE;
}

/**
 * Upgrade from V411 to V412
 */
static BOOL H_UpgradeFromV411(int currVersion, int newVersion)
{
   DB_RESULT hResult = SQLSelect(_T("SELECT id,zone_guid,agent_proxy,snmp_proxy,icmp_proxy,ssh_proxy FROM zones"));
   if (hResult != NULL)
   {
      CHK_EXEC(SQLQuery(_T("DROP TABLE zones")));
      CHK_EXEC(CreateTable(
               _T("CREATE TABLE zones (")
               _T("id integer not null,")
               _T("zone_guid integer not null,")
               _T("proxy_node integer not null,")
               _T("PRIMARY KEY(id))")));
      int count = DBGetNumRows(hResult);
      if (count > 0)
      {
         TCHAR query[256];
         for(int i = 0; i < count; i++)
         {
            UINT32 proxy = 0;
            for(int j = 2; (j < 6) && (proxy == 0); j++)
               proxy = DBGetFieldULong(hResult, i, j);
            _sntprintf(query, 256, _T("INSERT INTO zones (id,zone_guid,proxy_node) VALUES (%d,%d,%d)"),
                       DBGetFieldLong(hResult, i, 0), DBGetFieldLong(hResult, i, 1), (int)proxy);
            CHK_EXEC(SQLQuery(query));
         }
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return false;
   }
   CHK_EXEC(SetSchemaVersion(412));
   return TRUE;
}

/**
 * Upgrade from V410 to V411
 */
static BOOL H_UpgradeFromV410(int currVersion, int newVersion)
{
   //check if tdata upgrade was already done, then just delete "TdataTableUpdated" metadata parameter
   if (!MetaDataReadInt(_T("TdataTableUpdated"), 0))
   {
      StringMap savedMetadata;
      DB_RESULT hResult = SQLSelect(_T("SELECT var_name,var_value FROM metadata WHERE var_name LIKE 'TDataTableCreationCommand_%' OR var_name LIKE 'TDataIndexCreationCommand_%'"));
      if (hResult != NULL)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            savedMetadata.setPreallocated(DBGetField(hResult, i, 0, NULL, 0), DBGetField(hResult, i, 1, NULL, 0));
         }
         DBFreeResult(hResult);
      }
      else if (!g_bIgnoreErrors)
      {
         return false;
      }

      static const TCHAR *batch =
         _T("DELETE FROM metadata WHERE var_name LIKE 'TDataTableCreationCommand_%' OR var_name LIKE 'TDataIndexCreationCommand_%'\n")
         _T("INSERT INTO metadata (var_name,var_value) VALUES ('TDataTableCreationCommand_0','CREATE TABLE tdata_%d (item_id integer not null,tdata_timestamp integer not null,tdata_value $SQL:TEXT null)')\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      if (g_dbSyntax == DB_SYNTAX_MSSQL)
         CHK_EXEC(SQLQuery(_T("INSERT INTO metadata (var_name,var_value) VALUES ('TDataIndexCreationCommand_0','CREATE CLUSTERED INDEX idx_tdata_%d ON tdata_%d(item_id,tdata_timestamp)')")));
      else
         CHK_EXEC(SQLQuery(_T("INSERT INTO metadata (var_name,var_value) VALUES ('TDataIndexCreationCommand_0','CREATE INDEX idx_tdata_%d ON tdata_%d(item_id,tdata_timestamp)')")));

      // table conversion will require multiple commits
      DBCommit(g_hCoreDB);
      if (!ConvertTDataTables())
      {
         if (!g_bIgnoreErrors)
         {
            // Restore metadata
            SQLQuery(_T("DELETE FROM metadata WHERE var_name LIKE 'TDataTableCreationCommand_%' OR var_name LIKE 'TDataIndexCreationCommand_%'"));
            StringList *keys = savedMetadata.keys();
            for(int i = 0; i < keys->size(); i++)
            {
               TCHAR query[4096];
               _sntprintf(query, 4096, _T("INSERT INTO metadata (var_name,var_value) VALUES (%s,%s)"),
                          (const TCHAR *)DBPrepareString(g_hCoreDB, keys->get(i)),
                          (const TCHAR *)DBPrepareString(g_hCoreDB, savedMetadata.get(keys->get(i))));
               SQLQuery(query);
            }
            return false;
         }
      }

      DBBegin(g_hCoreDB);
   }
   else
   {
      CHK_EXEC(SQLQuery(_T("DELETE FROM metadata WHERE var_name='TdataTableUpdated'")));
   }
   CHK_EXEC(SetSchemaVersion(411));
   return TRUE;
}

/**
 * Upgrade from V409 to V410
 */
static BOOL H_UpgradeFromV409(int currVersion, int newVersion)
{
   static const TCHAR *batch =
      _T("ALTER TABLE zones ADD ssh_proxy integer\n")
      _T("UPDATE zones SET ssh_proxy=0\n")
      _T("ALTER TABLE nodes ADD ssh_login varchar(63)\n")
      _T("ALTER TABLE nodes ADD ssh_password varchar(63)\n")
      _T("ALTER TABLE nodes ADD ssh_proxy integer\n")
      _T("UPDATE nodes SET ssh_proxy=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("nodes"), _T("ssh_proxy")));
   CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("zones"), _T("ssh_proxy")));

   CHK_EXEC(SetSchemaVersion(410));
   return TRUE;
}

/**
 * Upgrade from V408 to V409
 */
static BOOL H_UpgradeFromV408(int currVersion, int newVersion)
{
   static const TCHAR *batch =
      _T("ALTER TABLE nodes ADD node_type integer\n")
      _T("ALTER TABLE nodes ADD node_subtype varchar(127)\n")
      _T("UPDATE nodes SET node_type=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("nodes"), _T("node_type")));

   CHK_EXEC(SetSchemaVersion(409));
   return TRUE;
}

/**
 * Upgrade from V407 to V408
 */
static BOOL H_UpgradeFromV407(int currVersion, int newVersion)
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE chassis (")
         _T("   id integer not null,")
         _T("   flags integer not null,")
         _T("   controller_id integer not null,")
         _T("   rack_id integer not null,")
         _T("   rack_image varchar(36) null,")
         _T("   rack_position integer not null,")
         _T("   rack_height integer not null,")
         _T("   PRIMARY KEY(id))")
       ));

   static const TCHAR *batch =
      _T("ALTER TABLE nodes ADD chassis_id integer\n")
      _T("UPDATE nodes SET chassis_id=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("nodes"), _T("chassis_id")));

   CHK_EXEC(SetSchemaVersion(408));
   return TRUE;
}

/**
 * Upgrade from V406 to V407
 */
static BOOL H_UpgradeFromV406(int currVersion, int newVersion)
{
   DBResizeColumn(g_hCoreDB, _T("user_groups"), _T("ldap_unique_id"), 64, true);
   CHK_EXEC(SetSchemaVersion(407));
   return TRUE;
}

/**
 * Upgrade from V405 to V406
 */
static BOOL H_UpgradeFromV405(int currVersion, int newVersion)
{
   static const TCHAR *batch =
      _T("ALTER TABLE nodes ADD syslog_msg_count $SQL:INT64\n")
      _T("ALTER TABLE nodes ADD snmp_trap_count $SQL:INT64\n")
      _T("UPDATE nodes SET syslog_msg_count=0,snmp_trap_count=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(406));
   return TRUE;
}

/**
 * Upgrade from V404 to V405
 */
static BOOL H_UpgradeFromV404(int currVersion, int newVersion)
{
   CHK_EXEC(CreateEventTemplate(EVENT_AGENT_LOG_PROBLEM, _T("SYS_AGENT_LOG_PROBLEM"),
            SEVERITY_MAJOR, EF_LOG, _T("262057ca-357a-4a4d-9b78-42ae96e490a1"),
      _T("Problem with agent log: %2"),
      _T("Generated on status poll if agent reports problem with log file.\r\n")
      _T("Parameters:\r\n")
      _T("    1) Status code\r\n")
      _T("    2) Description")));
   CHK_EXEC(CreateEventTemplate(EVENT_AGENT_LOCAL_DATABASE_PROBLEM, _T("SYS_AGENT_LOCAL_DATABASE_PROBLEM"),
            SEVERITY_MAJOR, EF_LOG, _T("d02b63f1-1151-429e-adb9-1dfbb3a31b32"),
      _T("Problem with agent local database: %2"),
      _T("Generated on status poll if agent reports local database problem.\r\n")
      _T("Parameters:\r\n")
      _T("    1) Status code\r\n")
      _T("    2) Description")));

	int ruleId = NextFreeEPPruleID();
   TCHAR query[1024];
	_sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event,situation_id,situation_instance) ")
                           _T("VALUES (%d,'19bd89ba-8bb2-4915-8546-a1ecc650dedd',7944,'Generate an alarm when there is problem with agent log','%%m',5,'SYS_AGENT_LOG_PROBLEM_%%i','',0,%d,0,'')"),
                           ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));
   _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_AGENT_LOG_PROBLEM);
   CHK_EXEC(SQLQuery(query));

   ruleId = NextFreeEPPruleID();
	_sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event,situation_id,situation_instance) ")
                           _T("VALUES (%d,'cff7fe6b-2ad1-4c18-8a8f-4d397d44fe04',7944,'Generate an alarm when there is problem with agent local database','%%m',5,'SYS_AGENT_LOCAL_DATABASE_PROBLEM_%%i','',0,%d,0,'')"),
                           ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));
   _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_AGENT_LOCAL_DATABASE_PROBLEM);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetSchemaVersion(405));
   return TRUE;
}

/**
 * Upgrade from V403 to V404
 */
static BOOL H_UpgradeFromV403(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("SyslogIgnoreMessageTimestamp"), _T("0"),
            _T("Ignore timestamp received in syslog messages and always use server time"),
            'B', true, false, false, false));
   CHK_EXEC(SetSchemaVersion(404));
   return TRUE;
}

/**
 * Upgrade from V402 to V403
 */
static BOOL H_UpgradeFromV402(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("DROP TABLE policy_time_range_list")));
   CHK_EXEC(SQLQuery(_T("DROP TABLE time_ranges")));
   CHK_EXEC(SetSchemaVersion(403));
   return TRUE;
}

/**
 * Upgrade from V401 to V402
 */
static BOOL H_UpgradeFromV401(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_event_log_source ON event_log(event_source)")));
   CHK_EXEC(SetSchemaVersion(402));
   return TRUE;
}

/**
 * Upgrade from V400 to V401
 */
static BOOL H_UpgradeFromV400(int currVersion, int newVersion)
{
   CHK_EXEC(CreateEventTemplate(EVENT_LDAP_SYNC_ERROR, _T("SYS_LDAP_SYNC_ERROR"),
            SEVERITY_MAJOR, EF_LOG, _T("f7e8508d-1503-4736-854b-1e5b8b0ad1f2"),
      _T("LDAP sync error: %5"),
      _T("Generated when LDAP synchronization error occurs.\r\n")
      _T("Parameters:\r\n")
      _T("    1) User ID\r\n")
      _T("    2) User GUID\r\n")
      _T("    3) User LDAP DN\r\n")
      _T("    4) User name\r\n")
      _T("    5) Problem description")));

	int ruleId = NextFreeEPPruleID();
   TCHAR query[1024];
	_sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event,situation_id,situation_instance) VALUES (%d,'417648af-5361-49a5-9471-6ef31e857b2d',7944,'Generate an alarm when error occurred during LDAP synchronization','%%m',5,'SYS_LDAP_SYNC_ERROR_%%2','',0,%d,0,'')"), ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));
   _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_LDAP_SYNC_ERROR);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SQLQuery(_T("ALTER TABLE users ADD ldap_unique_id varchar(64)")));
   CHK_EXEC(SQLQuery(_T("ALTER TABLE user_groups ADD ldap_unique_id varchar(64)")));
   CHK_EXEC(CreateConfigParam(_T("LdapUserUniqueId"), _T(""), true, false, false));
   CHK_EXEC(CreateConfigParam(_T("LdapGroupUniqueId"), _T(""), true, false, false));

   CHK_EXEC(SetSchemaVersion(401));
   return TRUE;
}

/**
 * Upgrade from V399 to V400
 */
static BOOL H_UpgradeFromV399(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("JobRetryCount"), _T("5"), _T("Maximum mumber of job execution retrys"), 'I', true, false, false, false));
   CHK_EXEC(SetSchemaVersion(400));
   return TRUE;
}

/**
 * Upgrade from V398 to V399
 */
static BOOL H_UpgradeFromV398(int currVersion, int newVersion)
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE config_repositories (")
         _T("   id integer not null,")
         _T("   url varchar(1023) not null,")
         _T("   auth_token varchar(63) null,")
         _T("   description varchar(1023) null,")
         _T("   PRIMARY KEY(id))")
       ));

   CHK_EXEC(SetSchemaVersion(399));
   return TRUE;
}

/**
 * Upgrade from V397 to V398
 */
static BOOL H_UpgradeFromV397(int currVersion, int newVersion)
{
   CHK_EXEC(CreateTable(
            _T("CREATE TABLE currency_codes (")
            _T("   numeric_code char(3) not null,")
            _T("   alpha_code char(3) not null,")
            _T("   description varchar(127) not null,")
            _T("   exponent integer not null,")
            _T("   PRIMARY KEY(numeric_code))")
         ));

   CHK_EXEC(CreateTable(
            _T("CREATE TABLE country_codes (")
            _T("   numeric_code char(3) not null,")
            _T("   alpha_code char(2) not null,")
            _T("   alpha3_code char(3) not null,")
            _T("   name varchar(127) not null,")
            _T("   PRIMARY KEY(numeric_code))")
         ));

   static const TCHAR *batch1 =
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('008', 'ALL', 'Lek', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('012', 'DZD', 'Algerian Dinar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('031', 'AZM', 'Azerbaijanian Manat', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('032', 'ARS', 'Argentine Peso', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('036', 'AUD', 'Australian Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('044', 'BSD', 'Bahamian Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('048', 'BHD', 'Bahraini Dinar', 3)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('050', 'BDT', 'Taka', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('051', 'AMD', 'Armenian Dram', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('052', 'BBD', 'Barbados Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('060', 'BMD', 'Bermudian Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('064', 'BTN', 'Ngultrum', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('068', 'BOB', 'Boliviano', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('072', 'BWP', 'Pula', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('084', 'BZD', 'Belize Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('090', 'SBD', 'Solomon Islands Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('096', 'BND', 'Brunei Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('100', 'BGL', 'Lev', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('104', 'MMK', 'Kyat', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('108', 'BIF', 'Burundi Franc', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('116', 'KHR', 'Riel', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('124', 'CAD', 'Canadian Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('132', 'CVE', 'Cape Verde Escudo', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('136', 'KYD', 'Cayman Islands Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('144', 'LKR', 'Sri Lanka Rupee', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('152', 'CLP', 'Chilean Peso', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('156', 'CNY', 'Yuan Renminbi', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('170', 'COP', 'Colombian Peso', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('174', 'KMF', 'Comoro Franc', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('188', 'CRC', 'Costa Rican Colon', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('191', 'HRK', 'Croatian Kuna', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('192', 'CUP', 'Cuban Peso', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('196', 'CYP', 'Cyprus Pound', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('203', 'CZK', 'Czech Koruna', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('208', 'DKK', 'Danish Krone', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('214', 'DOP', 'Dominican Peso', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('222', 'SVC', 'El Salvador Colon', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('230', 'ETB', 'Ethiopian Birr', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('232', 'ERN', 'Nakfa', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('233', 'EEK', 'Estonian Kroon', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('238', 'FKP', 'Falkland Islands Pound', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('242', 'FJD', 'Fiji Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('262', 'DJF', 'Djibouti Franc', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('270', 'GMD', 'Dalasi', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('288', 'GHC', 'Cedi', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('292', 'GIP', 'Gibraltar Pound', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('320', 'GTQ', 'Quetzal', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('324', 'GNF', 'Guinea Franc', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('328', 'GYD', 'Guyana Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('332', 'HTG', 'Gourde', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('340', 'HNL', 'Lempira', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('344', 'HKD', 'Hong Kong Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('348', 'HUF', 'Forint', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('352', 'ISK', 'Iceland Krona', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('356', 'INR', 'Indian Rupee', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('360', 'IDR', 'Rupiah', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('364', 'IRR', 'Iranian Rial', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('368', 'IQD', 'Iraqi Dinar', 3)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('376', 'ILS', 'New Israeli Sheqel', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('388', 'JMD', 'Jamaican Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('392', 'JPY', 'Yen', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('398', 'KZT', 'Tenge', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('400', 'JOD', 'Jordanian Dinar', 3)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('404', 'KES', 'Kenyan Shilling', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('408', 'KPW', 'North Korean Won', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('410', 'KRW', 'Won', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('414', 'KWD', 'Kuwaiti Dinar', 3)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('417', 'KGS', 'Som', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('418', 'LAK', 'Kip', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('422', 'LBP', 'Lebanese Pound', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('426', 'LSL', 'Lesotho Loti', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('428', 'LVL', 'Latvian Lats', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('430', 'LRD', 'Liberian Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('434', 'LYD', 'Lybian Dinar', 3)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('440', 'LTL', 'Lithuanian Litas', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('446', 'MOP', 'Pataca', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('450', 'MGF', 'Malagasy Franc', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('454', 'MWK', 'Kwacha', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('458', 'MYR', 'Malaysian Ringgit', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('462', 'MVR', 'Rufiyaa', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('470', 'MTL', 'Maltese Lira', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('478', 'MRO', 'Ouguiya', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('480', 'MUR', 'Mauritius Rupee', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('484', 'MXN', 'Mexican Peso', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('496', 'MNT', 'Tugrik', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('498', 'MDL', 'Moldovan Leu', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('504', 'MAD', 'Moroccan Dirham', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('508', 'MZM', 'Metical', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('512', 'OMR', 'Rial Omani', 3)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('516', 'NAD', 'Namibia Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('524', 'NPR', 'Nepalese Rupee', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('532', 'ANG', 'Netherlands Antillan Guilder', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('533', 'AWG', 'Aruban Guilder', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('548', 'VUV', 'Vatu', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('554', 'NZD', 'New Zealand Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('558', 'NIO', 'Cordoba Oro', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('566', 'NGN', 'Naira', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('578', 'NOK', 'Norvegian Krone', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('586', 'PKR', 'Pakistan Rupee', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('590', 'PAB', 'Balboa', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('598', 'PGK', 'Kina', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('600', 'PYG', 'Guarani', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('604', 'PEN', 'Nuevo Sol', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('608', 'PHP', 'Philippine Peso', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('624', 'GWP', 'Guinea-Bissau Peso', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('634', 'QAR', 'Qatari Rial', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('642', 'ROL', 'Leu', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('643', 'RUB', 'Russian Ruble', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('646', 'RWF', 'Rwanda Franc', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('654', 'SHP', 'Saint Helena Pound', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('678', 'STD', 'Dobra', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('682', 'SAR', 'Saudi Riyal', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('690', 'SCR', 'Seychelles Rupee', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('694', 'SLL', 'Leone', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('702', 'SGD', 'Singapore Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('703', 'SKK', 'Slovak Koruna', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('704', 'VND', 'Dong', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('706', 'SOS', 'Somali Shilling', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('710', 'ZAR', 'Rand', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('716', 'ZWD', 'Zimbabwe Dollar', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('728', 'SSP', 'South Sudanese pound', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('740', 'SRG', 'Suriname Guilder', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('748', 'SZL', 'Lilangeni', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('752', 'SEK', 'Swedish Krona', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('756', 'CHF', 'Swiss Franc', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('760', 'SYP', 'Syrian Pound', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('764', 'THB', 'Baht', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('776', 'TOP', 'Paanga', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('780', 'TTD', 'Trinidad and Tobago Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('784', 'AED', 'UAE Dirham', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('788', 'TND', 'Tunisian Dinar', 3)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('795', 'TMM', 'Manat', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('800', 'UGX', 'Uganda Shilling', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('807', 'MKD', 'Denar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('810', 'RUR', 'Russian Ruble', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('818', 'EGP', 'Egyptian Pound', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('826', 'GBP', 'Pound Sterling', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('834', 'TZS', 'Tanzanian Shilling', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('840', 'USD', 'US Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('858', 'UYU', 'Peso Uruguayo', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('860', 'UZS', 'Uzbekistan Sum', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('862', 'VEB', 'Bolivar', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('882', 'WST', 'Tala', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('886', 'YER', 'Yemeni Rial', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('891', 'CSD', 'Serbian Dinar', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('894', 'ZMK', 'Kwacha', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('901', 'TWD', 'New Taiwan Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('933', 'BYN', 'Belarussian New Ruble', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('934', 'TMT', 'Turkmenistani Manat', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('937', 'VEF', 'Venezuelan Bolivar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('938', 'SDG', 'Sudanese Pound', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('941', 'RSD', 'Serbian Dinar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('943', 'MZN', 'Mozambican Metical', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('944', 'AZN', 'Azerbaijani Manat', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('946', 'RON', 'New Romanian Leu', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('949', 'TRY', 'New Turkish Lira', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('950', 'XAF', 'CFA Franc BEAC', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('951', 'XCD', 'East Carribbean Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('952', 'XOF', 'CFA Franc BCEAO', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('953', 'XPF', 'CFP Franc', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('967', 'ZMW', 'Zambian Kwacha', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('968', 'SRD', 'Surinamese Dollar', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('969', 'MGA', 'Malagasy Ariary', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('971', 'AFN', 'Afghani', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('972', 'TJS', 'Somoni', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('973', 'AOA', 'Kwanza', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('974', 'BYR', 'Belarussian Ruble', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('975', 'BGN', 'Bulgarian Lev', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('976', 'CDF', 'Franc Congolais', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('977', 'BAM', 'Convertible Marks', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('978', 'EUR', 'Euro', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('979', 'MXV', 'Mexican Unidad de Inversion (UDI)', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('980', 'UAH', 'Hryvnia', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('981', 'GEL', 'Lari', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('984', 'BOV', 'Mvdol', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('985', 'PLN', 'Zloty', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('986', 'BRL', 'Brazilian Real', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('990', 'CLF', 'Unidades de Fomento', 0)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('997', 'USN', 'US dollar (next day funds code)', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('998', 'USS', 'US dollar (same day funds code)', 2)\n")
      _T("INSERT INTO currency_codes (numeric_code, alpha_code, description, exponent) VALUES ('999', 'XXX', 'No currency', 0)\n")
      _T("<END>");
   static const TCHAR *batch2 =
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('AD','AND','020','Andorra')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('AE','ARE','784','United Arab Emirates')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('AF','AFG','004','Afghanistan')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('AG','ATG','028','Antigua and Barbuda')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('AI','AIA','660','Anguilla')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('AL','ALB','008','Albania')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('AM','ARM','051','Armenia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('AN','ANT','530','Netherlands Antilles')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('AO','AGO','024','Angola')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('AQ','ATA','010','Antarctica')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('AR','ARG','032','Argentina')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('AS','ASM','016','American Samoa')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('AT','AUT','040','Austria')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('AU','AUS','036','Australia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('AW','ABW','533','Aruba')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('AX','ALA','248','Aland Islands')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('AZ','AZE','031','Azerbaijan')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BA','BIH','070','Bosnia and Herzegovina')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BB','BRB','052','Barbados')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BD','BGD','050','Bangladesh')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BE','BEL','056','Belgium')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BF','BFA','854','Burkina Faso')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BG','BGR','100','Bulgaria')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BH','BHR','048','Bahrain')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BI','BDI','108','Burundi')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BJ','BEN','204','Benin')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BL','BLM','652','Saint-Barthelemy')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BM','BMU','060','Bermuda')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BN','BRN','096','Brunei Darussalam')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BO','BOL','068','Bolivia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BQ','BES','535','Bonaire, Sint Eustatius and Saba')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BR','BRA','076','Brazil')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BS','BHS','044','Bahamas')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BT','BTN','064','Bhutan')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BV','BVT','074','Bouvet Island')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BW','BWA','072','Botswana')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BY','BLR','112','Belarus')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('BZ','BLZ','084','Belize')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CA','CAN','124','Canada')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CC','CCK','166','Cocos (Keeling) Islands')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CD','COD','180','Congo, Democratic Republic of the')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CF','CAF','140','Central African Republic')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CG','COG','178','Congo (Brazzaville)')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CH','CHE','756','Switzerland')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CI','CIV','384','Cote d''Ivoire')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CK','COK','184','Cook Islands')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CL','CHL','152','Chile')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CM','CMR','120','Cameroon')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CN','CHN','156','China')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CO','COL','170','Colombia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CR','CRI','188','Costa Rica')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CU','CUB','192','Cuba')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CV','CPV','132','Cape Verde')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CW','CUW','531','Curacao')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CX','CXR','162','Christmas Island')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CY','CYP','196','Cyprus')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('CZ','CZE','203','Czech Republic')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('DE','DEU','276','Germany')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('DJ','DJI','262','Djibouti')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('DK','DNK','208','Denmark')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('DM','DMA','212','Dominica')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('DO','DOM','214','Dominican Republic')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('DZ','DZA','012','Algeria')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('EC','ECU','218','Ecuador')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('EE','EST','233','Estonia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('EG','EGY','818','Egypt')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('EH','ESH','732','Western Sahara')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('ER','ERI','232','Eritrea')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('ES','ESP','724','Spain')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('ET','ETH','231','Ethiopia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('FI','FIN','246','Finland')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('FJ','FJI','242','Fiji')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('FK','FLK','238','Falkland Islands (Malvinas)')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('FM','FSM','583','Micronesia, Federated States of')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('FO','FRO','234','Faroe Islands')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('FR','FRA','250','France')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GA','GAB','266','Gabon')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GB','GBR','826','United Kingdom')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GD','GRD','308','Grenada')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GE','GEO','268','Georgia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GF','GUF','254','French Guiana')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GG','GGY','831','Guernsey')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GH','GHA','288','Ghana')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GI','GIB','292','Gibraltar')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GL','GRL','304','Greenland')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GM','GMB','270','Gambia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GN','GIN','324','Guinea')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GP','GLP','312','Guadeloupe')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GQ','GNQ','226','Equatorial Guinea')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GR','GRC','300','Greece')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GS','SGS','239','South Georgia and the South Sandwich Islands')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GT','GTM','320','Guatemala')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GU','GUM','316','Guam')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GW','GNB','624','Guinea-Bissau')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('GY','GUY','328','Guyana')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('HK','HKG','344','Hong Kong, Special Administrative Region of China')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('HM','HMD','334','Heard Island and Mcdonald Islands')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('HN','HND','340','Honduras')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('HR','HRV','191','Croatia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('HT','HTI','332','Haiti')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('HU','HUN','348','Hungary')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('ID','IDN','360','Indonesia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('IE','IRL','372','Ireland')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('IL','ISR','376','Israel')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('IM','IMN','833','Isle of Man')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('IN','IND','356','India')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('IO','IOT','086','British Indian Ocean Territory')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('IQ','IRQ','368','Iraq')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('IR','IRN','364','Iran, Islamic Republic of')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('IS','ISL','352','Iceland')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('IT','ITA','380','Italy')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('JE','JEY','832','Jersey')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('JM','JAM','388','Jamaica')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('JO','JOR','400','Jordan')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('JP','JPN','392','Japan')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('KE','KEN','404','Kenya')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('KG','KGZ','417','Kyrgyzstan')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('KH','KHM','116','Cambodia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('KI','KIR','296','Kiribati')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('KM','COM','174','Comoros')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('KN','KNA','659','Saint Kitts and Nevis')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('KP','PRK','408','Korea, Democratic People''s Republic of')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('KR','KOR','410','Korea, Republic of')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('KW','KWT','414','Kuwait')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('KY','CYM','136','Cayman Islands')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('KZ','KAZ','398','Kazakhstan')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('LA','LAO','418','Lao PDR')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('LB','LBN','422','Lebanon')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('LC','LCA','662','Saint Lucia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('LI','LIE','438','Liechtenstein')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('LK','LKA','144','Sri Lanka')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('LR','LBR','430','Liberia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('LS','LSO','426','Lesotho')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('LT','LTU','440','Lithuania')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('LU','LUX','442','Luxembourg')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('LV','LVA','428','Latvia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('LY','LBY','434','Libya')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MA','MAR','504','Morocco')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MC','MCO','492','Monaco')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MD','MDA','498','Moldova')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('ME','MNE','499','Montenegro')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MF','MAF','663','Saint-Martin (French part)')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MG','MDG','450','Madagascar')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MH','MHL','584','Marshall Islands')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MK','MKD','807','Macedonia, Republic of')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('ML','MLI','466','Mali')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MM','MMR','104','Myanmar')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MN','MNG','496','Mongolia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MO','MAC','446','Macao, Special Administrative Region of China')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MP','MNP','580','Northern Mariana Islands')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MQ','MTQ','474','Martinique')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MR','MRT','478','Mauritania')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MS','MSR','500','Montserrat')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MT','MLT','470','Malta')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MU','MUS','480','Mauritius')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MV','MDV','462','Maldives')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MW','MWI','454','Malawi')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MX','MEX','484','Mexico')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MY','MYS','458','Malaysia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('MZ','MOZ','508','Mozambique')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('NA','NAM','516','Namibia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('NC','NCL','540','New Caledonia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('NE','NER','562','Niger')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('NF','NFK','574','Norfolk Island')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('NG','NGA','566','Nigeria')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('NI','NIC','558','Nicaragua')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('NL','NLD','528','Netherlands')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('NO','NOR','578','Norway')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('NP','NPL','524','Nepal')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('NR','NRU','520','Nauru')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('NU','NIU','570','Niue')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('NZ','NZL','554','New Zealand')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('OM','OMN','512','Oman')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('PA','PAN','591','Panama')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('PE','PER','604','Peru')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('PF','PYF','258','French Polynesia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('PG','PNG','598','Papua New Guinea')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('PH','PHL','608','Philippines')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('PK','PAK','586','Pakistan')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('PL','POL','616','Poland')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('PM','SPM','666','Saint Pierre and Miquelon')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('PN','PCN','612','Pitcairn')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('PR','PRI','630','Puerto Rico')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('PS','PSE','275','Palestinian Territory, Occupied')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('PT','PRT','620','Portugal')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('PW','PLW','585','Palau')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('PY','PRY','600','Paraguay')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('QA','QAT','634','Qatar')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('RE','REU','638','Reunion')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('RO','ROU','642','Romania')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('RS','SRB','688','Serbia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('RU','RUS','643','Russian Federation')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('RW','RWA','646','Rwanda')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SA','SAU','682','Saudi Arabia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SB','SLB','090','Solomon Islands')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SC','SYC','690','Seychelles')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SD','SDN','729','Sudan')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SE','SWE','752','Sweden')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SG','SGP','702','Singapore')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SH','SHN','654','Saint Helena')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SI','SVN','705','Slovenia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SJ','SJM','744','Svalbard and Jan Mayen Islands')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SK','SVK','703','Slovakia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SL','SLE','694','Sierra Leone')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SM','SMR','674','San Marino')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SN','SEN','686','Senegal')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SO','SOM','706','Somalia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SR','SUR','740','Suriname')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SS','SSD','728','South Sudan')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('ST','STP','678','Sao Tome and Principe')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SV','SLV','222','El Salvador')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SX','SXM','534','Sint Maarten (Dutch part)')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SY','SYR','760','Syrian Arab Republic (Syria)')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('SZ','SWZ','748','Swaziland')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('TC','TCA','796','Turks and Caicos Islands')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('TD','TCD','148','Chad')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('TF','ATF','260','French Southern Territories')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('TG','TGO','768','Togo')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('TH','THA','764','Thailand')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('TJ','TJK','762','Tajikistan')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('TK','TKL','772','Tokelau')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('TL','TLS','626','Timor-Leste')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('TM','TKM','795','Turkmenistan')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('TN','TUN','788','Tunisia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('TO','TON','776','Tonga')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('TR','TUR','792','Turkey')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('TT','TTO','780','Trinidad and Tobago')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('TV','TUV','798','Tuvalu')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('TW','TWN','158','Taiwan')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('TZ','TZA','834','Tanzania')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('UA','UKR','804','Ukraine')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('UG','UGA','800','Uganda')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('UM','UMI','581','United States Minor Outlying Islands')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('US','USA','840','United States of America')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('UY','URY','858','Uruguay')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('UZ','UZB','860','Uzbekistan')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('VA','VAT','336','Holy See (Vatican City State)')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('VC','VCT','670','Saint Vincent and Grenadines')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('VE','VEN','862','Venezuela')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('VG','VGB','092','British Virgin Islands')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('VI','VIR','850','Virgin Islands, US')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('VN','VNM','704','Viet Nam')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('VU','VUT','548','Vanuatu')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('WF','WLF','876','Wallis and Futuna Islands')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('WS','WSM','882','Samoa')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('YE','YEM','887','Yemen')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('YT','MYT','175','Mayotte')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('ZA','ZAF','710','South Africa')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('ZM','ZMB','894','Zambia')\n")
      _T("INSERT INTO country_codes (alpha_code,alpha3_code,numeric_code,name) VALUES ('ZW','ZWE','716','Zimbabwe')\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch1));
   CHK_EXEC(SQLBatch(batch2));

   CHK_EXEC(SetSchemaVersion(398));
   return TRUE;
}

/**
 * Upgrade from V396 to V397
 */
static BOOL H_UpgradeFromV396(int currVersion, int newVersion)
{
   static GUID_MAPPING eventGuidMapping[] = {
      { EVENT_NODE_ADDED, _T("8d34acfd-dad6-4f6e-b6a8-1189683591ef") },
      { EVENT_SUBNET_ADDED, _T("75fc3f8b-768f-46b4-bf44-72949436a679") },
      { EVENT_INTERFACE_ADDED, _T("33cb8f9c-9427-459c-8a71-45c73f5cc183") },
      { EVENT_INTERFACE_UP, _T("09ee209a-0e75-434f-b8c8-399d93305d7b") },
      { EVENT_INTERFACE_DOWN, _T("d9a6d46d-97f8-48eb-a86a-2c0f6b150d0d") },
      { EVENT_INTERFACE_UNKNOWN, _T("ecb47be1-f911-4c1f-9b00-d0d21694071d") },
      { EVENT_INTERFACE_DISABLED, _T("2f3123a2-425f-47db-9c6b-9bc05a7fba2d") },
      { EVENT_INTERFACE_TESTING, _T("eb500e5c-3560-4394-8f5f-80aa67036f13") },
      { EVENT_INTERFACE_UNEXPECTED_UP, _T("ff21a165-9253-4ecc-929a-ffd1e388d504") },
      { EVENT_INTERFACE_EXPECTED_DOWN, _T("911358f4-d2a1-4465-94d7-ce4bc5c38860") },
      { EVENT_NODE_NORMAL, _T("03bc11c0-ec20-43be-be45-e60846f744dc") },
      { EVENT_NODE_WARNING, _T("1c80deab-aafb-43a7-93a7-1330dd563b47") },
      { EVENT_NODE_MINOR, _T("84eaea00-4ed7-41eb-9079-b783e5c60651") },
      { EVENT_NODE_MAJOR, _T("27035c88-c27a-4c16-97b3-4658d34a5f63") },
      { EVENT_NODE_CRITICAL, _T("8f2e98f8-1cd4-4e12-b41f-48b5c60ebe8e") },
      { EVENT_NODE_UNKNOWN, _T("6933cce0-fe1f-4123-817f-af1fb9f0eab4") },
      { EVENT_NODE_UNMANAGED, _T("a8356ba7-51b7-4487-b74e-d12132db233c") },
      { EVENT_NODE_CAPABILITIES_CHANGED, _T("b04e39f5-d3a7-4d9a-b594-37132f5eaf34") },
      { EVENT_SNMP_FAIL, _T("d2fc3b0c-1215-4a92-b8f3-47df5d753602") },
      { EVENT_AGENT_FAIL, _T("ba484457-3594-418e-a72a-65336055d025") },
      { EVENT_INTERFACE_DELETED, _T("ad7e9856-e361-4095-9361-ccc462d93624") },
      { EVENT_THRESHOLD_REACHED, _T("05161c3d-7ceb-406f-af0a-af5c77f324a5") },
      { EVENT_THRESHOLD_REARMED, _T("25eef3a7-6158-4c5e-b4e3-8a7aa7ade73c") },
      { EVENT_SUBNET_DELETED, _T("af188eb3-e84f-4fd9-aecf-f1ba934a9f1a") },
      { EVENT_THREAD_HANGS, _T("df247d13-a63a-43fe-bb02-cb41718ee387") },
      { EVENT_THREAD_RUNNING, _T("5589f6ce-7133-44db-8e7a-e1452d636a9a") },
      { EVENT_SMTP_FAILURE, _T("1e376009-0d26-4b86-87a2-f4715a02fb38") },
      { EVENT_MAC_ADDR_CHANGED, _T("61916ef0-1eee-4df7-a95b-150928d47962") },
      { EVENT_INCORRECT_NETMASK, _T("86c08c55-416e-4ac4-bf2b-302b5fddbd68") },
      { EVENT_NODE_DOWN, _T("ce34f0d0-5b21-48c2-8788-8ed5ee547023") },
      { EVENT_NODE_UP, _T("05f180b6-62e7-4bc4-8a8d-34540214254b") },
      { EVENT_SERVICE_DOWN, _T("89caacb5-d2cf-493b-862f-cddbfecac5b6") },
      { EVENT_SERVICE_UP, _T("ab35e7c7-2428-44db-ad43-57fe551bb8cc") },
      { EVENT_SERVICE_UNKNOWN, _T("d891adae-49fe-4442-a8f3-0ca37ca8820a") },
      { EVENT_SMS_FAILURE, _T("c349bf75-458a-4d43-9c27-f71ea4bb06e2") },
      { EVENT_SNMP_OK, _T("a821086b-1595-40db-9148-8d770d30a54b") },
      { EVENT_AGENT_OK, _T("9c15299a-f2e3-4440-84c5-b17dea87ae2a") },
      { EVENT_SCRIPT_ERROR, _T("2cc78efe-357a-4278-932f-91e36754c775") },
      { EVENT_CONDITION_ACTIVATED, _T("16a86780-b73a-4601-929c-0c503bd06401") },
      { EVENT_CONDITION_DEACTIVATED, _T("926d15d2-9761-4bb6-a1ce-64175303796f") },
      { EVENT_DB_CONNECTION_LOST, _T("0311e9c8-8dcf-4a5b-9036-8cff034409ff") },
      { EVENT_DB_CONNECTION_RESTORED, _T("d36259a7-5f6b-4e3c-bb6f-17d1f8ac950d") },
      { EVENT_CLUSTER_RESOURCE_MOVED, _T("44abe5f3-a7c9-4fbd-8d10-53be172eae83") },
      { EVENT_CLUSTER_RESOURCE_DOWN, _T("c3b1d4b5-2e41-4a2f-b379-9d74ebba3a25") },
      { EVENT_CLUSTER_RESOURCE_UP, _T("ef6fff96-0cbb-4030-aeba-2473a80c6568") },
      { EVENT_CLUSTER_DOWN, _T("8f14d0f7-08d4-4422-92f4-469e5eef93ef") },
      { EVENT_CLUSTER_UP, _T("4a9cdb65-aa44-42f2-99b0-1e302aec10f6") },
      { EVENT_ALARM_TIMEOUT, _T("4ae4f601-327b-4ef8-9740-8600a1ba2acd") },
      { EVENT_LOG_RECORD_MATCHED, _T("d9326159-5c60-410f-990e-fae88df7fdd4") },
      { EVENT_EVENT_STORM_DETECTED, _T("c98d8575-d134-4044-ba67-75c5f5d0f6e0") },
      { EVENT_EVENT_STORM_ENDED, _T("dfd5e3ba-3182-4deb-bc32-9e6b8c1c6546") },
      { EVENT_NETWORK_CONNECTION_LOST, _T("3cb0921a-87a1-46e4-8be1-82ad2dda0015") },
      { EVENT_NETWORK_CONNECTION_RESTORED, _T("1c61b3e0-389a-47ac-8469-932a907392bc") },
      { EVENT_DB_QUERY_FAILED, _T("5f35d646-63b6-4dcd-b94a-e2ccd060686a") },
      { EVENT_DCI_UNSUPPORTED, _T("28367b5b-1541-4526-8cbe-91a17ed31ba4") },
      { EVENT_DCI_DISABLED, _T("50196042-0619-4420-9471-16b7c25c0213") },
      { EVENT_DCI_ACTIVE, _T("740b6810-b355-46f4-a921-65118504af18") },
      { EVENT_IP_ADDRESS_CHANGED, _T("517b6d2a-f5c6-46aa-969d-48e62e05e3bf") },
      { EVENT_CONTAINER_AUTOBIND, _T("611133e2-1f76-446f-b278-9d500a823611") },
      { EVENT_CONTAINER_AUTOUNBIND, _T("e57603be-0d81-41aa-b07c-12d08640561c") },
      { EVENT_TEMPLATE_AUTOAPPLY, _T("bf084945-f928-4428-8c6c-d2965addc832") },
      { EVENT_TEMPLATE_AUTOREMOVE, _T("8f7a4b4a-d0a2-4404-9b66-fdbc029f42cf") },
      { EVENT_NODE_UNREACHABLE, _T("47bba2ce-c795-4e56-ad44-cbf05741e1ff") },
      { EVENT_TABLE_THRESHOLD_ACTIVATED, _T("c08a1cfe-3128-40c2-99cc-378e7ef91f79") },
      { EVENT_TABLE_THRESHOLD_DEACTIVATED, _T("479085e7-e1d1-4f2a-9d96-1d522f51b26a") },
      { EVENT_IF_PEER_CHANGED, _T("a3a5c1df-9d96-4e98-9e06-b3157dbf39f0") },
      { EVENT_AP_ADOPTED, _T("5aaee261-0c5d-44e0-b2f0-223bbba5297d") },
      { EVENT_AP_UNADOPTED, _T("846a3581-aad1-4e17-9c55-9bd2e6b1247b") },
      { EVENT_AP_DOWN, _T("2c8c6208-d3ab-4b8c-926a-872f4d8abcee") },
      { EVENT_IF_MASK_CHANGED, _T("f800e593-057e-4aec-9e47-be0f7718c5c4") },
      { EVENT_IF_IPADDR_ADDED, _T("475bdca6-543e-410b-9aff-c217599e0fe6") },
      { EVENT_IF_IPADDR_DELETED, _T("ef477387-eb50-4a1a-bf90-717502b9873c") },
      { EVENT_MAINTENANCE_MODE_ENTERED, _T("5f6c8b1c-f162-413e-8028-80e7ad2c362d") },
      { EVENT_MAINTENANCE_MODE_LEFT, _T("cab06848-a622-430d-8b4c-addeea732657") },
      { EVENT_SNMP_UNMATCHED_TRAP, _T("fc3613f7-d151-4221-9acd-d28b6f804335") },
      { EVENT_SNMP_COLD_START, _T("39920e99-97bd-4d61-a462-43f89ba6fbdf") },
      { EVENT_SNMP_WARM_START, _T("0aa888c1-eba6-4fe7-a37a-b85f2b373bdc") },
      { EVENT_SNMP_LINK_DOWN, _T("b71338cc-137d-473c-a0f1-6b131086af56") },
      { EVENT_SNMP_LINK_UP, _T("03da14a7-e39c-4a46-a7cb-4bf77ec7936c") },
      { EVENT_SNMP_AUTH_FAILURE, _T("37020cb0-dde7-487b-9cfb-0d5ee771aa13") },
      { EVENT_SNMP_EGP_NL, _T("aecf5fa4-390c-4125-be10-df8b0e669fe1") },
      { 0, NULL }
   };

   CHK_EXEC(SQLQuery(_T("ALTER TABLE event_cfg ADD guid varchar(36)")));
   CHK_EXEC(GenerateGUID(_T("event_cfg"), _T("event_code"), _T("guid"), eventGuidMapping));
   CHK_EXEC(SetSchemaVersion(397));
   return TRUE;
}

/**
 * Upgrade from V395 to V396
 */
static BOOL H_UpgradeFromV395(int currVersion, int newVersion)
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE ap_log_parser (")
      _T("   policy_id integer not null,")
      _T("   file_content $SQL:TEXT null,")
      _T("   PRIMARY KEY(policy_id))")));

   CHK_EXEC(SetSchemaVersion(396));
   return TRUE;
}

/**
 * Upgrade from V394 to V395
 */
static BOOL H_UpgradeFromV394(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET need_server_restart='1' WHERE var_name='OfflineDataRelevanceTime'")));
   CHK_EXEC(SetSchemaVersion(395));
   return TRUE;
}

/**
 * Upgrade from V393 to V394
 */
static BOOL H_UpgradeFromV393(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("OfflineDataRelevanceTime"), _T("86400"), _T("Time period in seconds within which received offline data still relevant for threshold validation"), 'I', true, false, false, false));
   CHK_EXEC(SetSchemaVersion(394));
   return TRUE;
}

/**
 * Upgrade from V392 to V393
 */
static BOOL H_UpgradeFromV392(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("DefaultInterfaceExpectedState"), _T("0"), _T("Default expected state for new interface objects"), 'C', true, false, false, false));
   CHK_EXEC(SetSchemaVersion(393));
   return TRUE;
}

/**
 * Upgrade from V391 to V392
 */
static BOOL H_UpgradeFromV391(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("ImportConfigurationOnStartup"), _T("0"), _T("Import configuration from local files on server startup"), 'B', true, true, false, false));
   CHK_EXEC(SetSchemaVersion(392));
   return TRUE;
}

/**
 * Upgrade from V390 to V391
 */
static BOOL H_UpgradeFromV390(int currVersion, int newVersion)
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE zmq_subscription (")
      _T("   object_id integer not null,")
      _T("   subscription_type char(1) not null,")
      _T("   ignore_items integer not null,")
      _T("   items $SQL:TEXT,")
      _T("   PRIMARY KEY(object_id, subscription_type))")));

   CHK_EXEC(SetSchemaVersion(391));
   return TRUE;
}

/**
 * Upgrade from V389 to V390
 */
static BOOL H_UpgradeFromV389(int currVersion, int newVersion)
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE object_containers (")
      _T("   id integer not null,")
      _T("   object_class integer not null,")
      _T("   flags integer not null,")
      _T("   auto_bind_filter $SQL:TEXT null,")
      _T("   PRIMARY KEY(id))")));

   DB_RESULT hResult = SQLSelect(_T("SELECT id,object_class,flags,auto_bind_filter FROM containers"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      if (count > 0)
      {
         DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("INSERT INTO object_containers (id,object_class,flags,auto_bind_filter) VALUES (?,?,?,?)"));
         if (hStmt != NULL)
         {
            for(int i = 0; i < count; i++)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 0));
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldLong(hResult, i, 1));
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 2));
               DBBind(hStmt, 4, DB_SQLTYPE_TEXT, DBGetField(hResult, i, 3, NULL, 0), DB_BIND_DYNAMIC);
               if (!SQLExecute(hStmt))
               {
                  if (!g_bIgnoreErrors)
                  {
                     DBFreeStatement(hStmt);
                     DBFreeResult(hResult);
                     return FALSE;
                  }
               }
            }
            DBFreeStatement(hStmt);
         }
         else if (!g_bIgnoreErrors)
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

   CHK_EXEC(SQLQuery(_T("DROP TABLE containers")));
   CHK_EXEC(SQLQuery(_T("DROP TABLE container_categories")));
   CHK_EXEC(SetSchemaVersion(390));
   return TRUE;
}

/**
 * Upgrade from V388 to V389
 */
static BOOL H_UpgradeFromV388(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE racks ADD top_bottom_num char(1)\n")
      _T("UPDATE racks SET top_bottom_num='0'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(389));
   return TRUE;
}

/**
 * Upgrade from V387 to V388
 */
static BOOL H_UpgradeFromV387(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE alarms ADD dci_id integer\n")
      _T("UPDATE alarms SET dci_id=0\n")
      _T("ALTER TABLE event_log ADD dci_id integer\n")
      _T("UPDATE event_log SET dci_id=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(388));
   return TRUE;
}

/**
 * Upgrade from V386 to V387
 */
static BOOL H_UpgradeFromV386(int currVersion, int newVersion)
{
   DB_RESULT hResult = SQLSelect(_T("SELECT id,flags,filter FROM network_maps WHERE filter IS NOT NULL"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         TCHAR *filter = DBGetField(hResult, i, 2, NULL, 0);
         if ((filter != NULL) && (filter[0] != 0))
         {
            TCHAR query[256];
            _sntprintf(query, 256, _T("UPDATE network_maps SET flags=%d WHERE id=%d"),
               DBGetFieldULong(hResult, i, 1) | MF_FILTER_OBJECTS, DBGetFieldULong(hResult, i, 0));
            CHK_EXEC(SQLQuery(query));
         }
         free(filter);
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return FALSE;
   }
   CHK_EXEC(SetSchemaVersion(387));
   return TRUE;
}

/**
 * Upgrade from V385 to V386
 */
static BOOL H_UpgradeFromV385(int currVersion, int newVersion)
{
   TCHAR query[1024];
   int ruleId = NextFreeEPPruleID();

   if (!IsEventPairInUse(EVENT_THREAD_HANGS, EVENT_THREAD_RUNNING))
   {
      _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event,situation_id,situation_instance) VALUES (%d,'ea1dee96-b42e-499c-a992-0b0f9e4874b9',7944,'Generate an alarm when one of the system threads hangs or stops unexpectedly','%%m',5,'SYS_THREAD_HANG_%%1','',0,%d,0,'')"), ruleId, EVENT_ALARM_TIMEOUT);
      CHK_EXEC(SQLQuery(query));
      _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_THREAD_HANGS);
      CHK_EXEC(SQLQuery(query));
      ruleId++;
      _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event,situation_id,situation_instance) VALUES (%d,'f0c5a6b2-7427-45e5-8333-7d60d2b408e6',7944,'Terminate the alarm when one of the system threads which previously hanged or stopped unexpectedly returned to the running state','%%m',6,'SYS_THREAD_HANG_%%1','',0,%d,0,'')"), ruleId, EVENT_ALARM_TIMEOUT);
      CHK_EXEC(SQLQuery(query));
      _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_THREAD_RUNNING);
      CHK_EXEC(SQLQuery(query));
      ruleId++;
   }

   if (!IsEventPairInUse(EVENT_MAINTENANCE_MODE_ENTERED, EVENT_MAINTENANCE_MODE_LEFT))
   {
      _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event,situation_id,situation_instance) VALUES (%d,'ed3397a8-a496-4534-839b-5a6fc77c167c',7944,'Generate an alarm when the object enters the maintenance mode','%%m',5,'MAINTENANCE_MODE_%%i','',0,%d,0,'')"), ruleId, EVENT_ALARM_TIMEOUT);
      CHK_EXEC(SQLQuery(query));
      _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_MAINTENANCE_MODE_ENTERED);
      CHK_EXEC(SQLQuery(query));
      ruleId++;
      _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event,situation_id,situation_instance) VALUES (%d,'20a0f4a5-d90e-4961-ba88-a65b9ee45d07',7944,'Terminate the alarm when the object leaves the maintenance mode','%%m',6,'MAINTENANCE_MODE_%%i','',0,%d,0,'')"), ruleId, EVENT_ALARM_TIMEOUT);
      CHK_EXEC(SQLQuery(query));
      _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_MAINTENANCE_MODE_LEFT);
      CHK_EXEC(SQLQuery(query));
      ruleId++;
   }

   if (!IsEventPairInUse(EVENT_AGENT_FAIL, EVENT_AGENT_OK))
   {
      _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event,situation_id,situation_instance) VALUES (%d,'c6f66840-4758-420f-a27d-7bbf7b66a511',7944,'Generate an alarm if the NetXMS agent on the node stops responding','%%m',5,'AGENT_UNREACHABLE_%%i','',0,%d,0,'')"), ruleId, EVENT_ALARM_TIMEOUT);
      CHK_EXEC(SQLQuery(query));
      _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_AGENT_FAIL);
      CHK_EXEC(SQLQuery(query));
      ruleId++;
      _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event,situation_id,situation_instance) VALUES (%d,'9fa60260-c2ec-4371-b4e4-f5346b1d8400',7944,'Terminate the alarm if the NetXMS agent on the node start responding again','%%m',6,'AGENT_UNREACHABLE_%%i','',0,%d,0,'')"), ruleId, EVENT_ALARM_TIMEOUT);
      CHK_EXEC(SQLQuery(query));
      _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_AGENT_OK);
      CHK_EXEC(SQLQuery(query));
      ruleId++;
   }

   if (!IsEventPairInUse(EVENT_SNMP_FAIL, EVENT_SNMP_OK))
   {
      _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event,situation_id,situation_instance) VALUES (%d,'20ef861f-b8e4-4e04-898e-e57d13863661',7944,'Generate an alarm if the SNMP agent on the node stops responding','%%m',5,'SNMP_UNREACHABLE_%%i','',0,%d,0,'')"), ruleId, EVENT_ALARM_TIMEOUT);
      CHK_EXEC(SQLQuery(query));
      _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_SNMP_FAIL);
      CHK_EXEC(SQLQuery(query));
      ruleId++;
      _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event,situation_id,situation_instance) VALUES (%d,'33f6193a-e103-4f5f-8bee-870bbcc08066',7944,'Terminate the alarm if the SNMP agent on the node start responding again','%%m',6,'SNMP_UNREACHABLE_%%i','',0,%d,0,'')"), ruleId, EVENT_ALARM_TIMEOUT);
      CHK_EXEC(SQLQuery(query));
      _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_SNMP_OK);
      CHK_EXEC(SQLQuery(query));
      ruleId++;
   }

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET severity='3' WHERE event_code=14 OR event_code=15")));
   CHK_EXEC(SetSchemaVersion(386));
   return TRUE;
}


/**
 * Upgrade from V384 to V385
 */
static BOOL H_UpgradeFromV384(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET is_visible=1,need_server_restart=0 WHERE var_name='SNMPPorts'")));
   CHK_EXEC(SetSchemaVersion(385));
   return TRUE;
}

/**
 * Upgrade from V383 to V384
 */
static BOOL H_UpgradeFromV383(int currVersion, int newVersion)
{
   CHK_EXEC(ConvertStrings(_T("graphs"), _T("graph_id"), _T("config")));
   static TCHAR batch[] =
      _T("ALTER TABLE graphs ADD flags integer\n")
      _T("ALTER TABLE graphs ADD filters $SQL:TEXT\n")
      _T("UPDATE graphs SET flags=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(384));
   return TRUE;
}

/**
 * Upgrade from V382 to V383
 */
static BOOL H_UpgradeFromV382(int currVersion, int newVersion)
{
   CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("nodes"), _T("primary_ip"), 48, false));
   CHK_EXEC(SetSchemaVersion(383));
   return TRUE;
}

/**
 * Upgrade from V381 to V382
 */
static BOOL H_UpgradeFromV381(int currVersion, int newVersion)
{
   CHK_EXEC(CreateLibraryScript(11, _T("Hook::StatusPoll"), _T("/* Available global variables:\r\n *  $node - current node, object of 'Node' type\r\n *\r\n * Expected return value:\r\n *  none - returned value is ignored\r\n */\r\n")));
   CHK_EXEC(CreateLibraryScript(12, _T("Hook::ConfigurationPoll"), _T("/* Available global variables:\r\n *  $node - current node, object of 'Node' type\r\n *\r\n * Expected return value:\r\n *  none - returned value is ignored\r\n */\r\n")));
   CHK_EXEC(CreateLibraryScript(13, _T("Hook::InstancePoll"), _T("/* Available global variables:\r\n *  $node - current node, object of 'Node' type\r\n *\r\n * Expected return value:\r\n *  none - returned value is ignored\r\n */\r\n")));
   CHK_EXEC(CreateLibraryScript(14, _T("Hook::TopologyPoll"), _T("/* Available global variables:\r\n *  $node - current node, object of 'Node' type\r\n *\r\n * Expected return value:\r\n *  none - returned value is ignored\r\n */\r\n")));
   CHK_EXEC(CreateLibraryScript(15, _T("Hook::CreateInterface"), _T("/* Available global variables:\r\n *  $node - current node, object of 'Node' type\r\n *  $1 - current interface, object of 'Interface' type\r\n *\r\n * Expected return value:\r\n *  true/false - boolean - whether interface should be created\r\n */\r\nreturn true;\r\n")));
   CHK_EXEC(CreateLibraryScript(16, _T("Hook::AcceptNewNode"), _T("/* Available global variables:\r\n *  $ipAddr - IP address of the node being processed\r\n *  $ipNetMask - netmask of the node being processed\r\n *  $macAddr - MAC address of the node being processed\r\n *  $zoneId - zone ID of the node being processed\r\n *\r\n * Expected return value:\r\n *  true/false - boolean - whether node should be created\r\n */\r\nreturn true;\r\n")));
   CHK_EXEC(SetSchemaVersion(382));
   return TRUE;
}

/**
 * Upgrade from V380 to V381
 */
static BOOL H_UpgradeFromV380(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE items ADD guid varchar(36)\n")
      _T("ALTER TABLE dc_tables ADD guid varchar(36)\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(GenerateGUID(_T("items"), _T("item_id"), _T("guid"), NULL));
   CHK_EXEC(GenerateGUID(_T("dc_tables"), _T("item_id"), _T("guid"), NULL));
   CHK_EXEC(SetSchemaVersion(381));
   return TRUE;
}

/**
 * Upgrade from V379 to V380
 */
static BOOL H_UpgradeFromV379(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("UPDATE object_properties SET maint_event_id=0 WHERE maint_event_id IS NULL\n")
      _T("UPDATE nodes SET last_agent_comm_time=0 WHERE last_agent_comm_time IS NULL\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(380));
   return TRUE;
}

/**
 * Upgrade from V378 to V379
 */
static BOOL H_UpgradeFromV378(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='NumberOfDatabaseWriters'")));
   CHK_EXEC(SetSchemaVersion(379));
   return TRUE;
}

/**
 * Upgrade from V377 to V378
 */
static BOOL H_UpgradeFromV377(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("DELETE FROM config WHERE var_name='EnableMultipleDBConnections'\n")
      _T("UPDATE config SET var_name='DBConnectionPoolBaseSize' WHERE var_name='ConnectionPoolBaseSize'\n")
      _T("UPDATE config SET var_name='DBConnectionPoolMaxSize' WHERE var_name='ConnectionPoolMaxSize'\n")
      _T("UPDATE config SET var_name='DBConnectionPoolCooldownTime' WHERE var_name='ConnectionPoolCooldownTime'\n")
      _T("UPDATE config SET var_name='DBConnectionPoolMaxLifetime' WHERE var_name='ConnectionPoolMaxLifetime'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(378));
   return TRUE;
}

/**
 * Upgrade from V376 to V377
 */
static BOOL H_UpgradeFromV376(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("DefaultSubnetMaskIPv4"), _T("24"), _T("Default mask for synthetic IPv4 subnets"), 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("DefaultSubnetMaskIPv6"), _T("64"), _T("Default mask for synthetic IPv6 subnets"), 'I', true, false, false, false));
   CHK_EXEC(SetSchemaVersion(377));
   return TRUE;
}

/**
 * Upgrade from V375 to V376
 */
static BOOL H_UpgradeFromV375(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE nodes ADD last_agent_comm_time integer\n")
      _T("UPDATE nodes SET last_agent_comm_time=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetSchemaVersion(376));
   return TRUE;
}

/**
 * Upgrade from V374 to V375
 */
static BOOL H_UpgradeFromV374(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE object_tools_input_fields ADD sequence_num integer\n")
      _T("UPDATE object_tools_input_fields SET sequence_num=-1\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetSchemaVersion(375));
   return TRUE;
}

/**
 * Upgrade from V373 to V374
 */
static BOOL H_UpgradeFromV373(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE object_properties ADD maint_event_id $SQL:INT64\n")
      _T("UPDATE object_properties SET maint_mode='0',maint_event_id=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(CreateEventTemplate(EVENT_MAINTENANCE_MODE_ENTERED, _T("SYS_MAINTENANCE_MODE_ENTERED"), SEVERITY_NORMAL, EF_LOG, NULL,
         _T("Entered maintenance mode"),
         _T("Generated when node, cluster, or mobile device enters maintenance mode.")));

   CHK_EXEC(CreateEventTemplate(EVENT_MAINTENANCE_MODE_LEFT, _T("SYS_MAINTENANCE_MODE_LEFT"), SEVERITY_NORMAL, EF_LOG, NULL,
         _T("Left maintenance mode"),
         _T("Generated when node, cluster, or mobile device leaves maintenance mode.")));

   CHK_EXEC(SetSchemaVersion(374));
   return TRUE;
}

/**
 * Upgrade from V372 to V373
 */
static BOOL H_UpgradeFromV372(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE scheduled_tasks ADD object_id integer")));
   CHK_EXEC(SetSchemaVersion(373));
   return TRUE;
}

/**
 * Upgrade from V371 to V372
 */
static BOOL H_UpgradeFromV371(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE object_properties ADD maint_mode char(1)\n")
      _T("UPDATE object_properties SET maint_mode='0'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(372));
   return TRUE;
}

/**
 * Upgrade from V370 to V371
 */
static BOOL H_UpgradeFromV370(int currVersion, int newVersion)
{
   int defaultPollingInterval = ConfigReadInt(_T("DefaultDCIPollingInterval"), 60);
   int defaultRetentionTime = ConfigReadInt(_T("DefaultDCIRetentionTime"), 30);

   TCHAR query[256];
   _sntprintf(query, 256, _T("UPDATE items SET polling_interval=0 WHERE polling_interval=%d"), defaultPollingInterval);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 256, _T("UPDATE items SET retention_time=0 WHERE retention_time=%d"), defaultRetentionTime);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 256, _T("UPDATE dc_tables SET polling_interval=0 WHERE polling_interval=%d"), defaultPollingInterval);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 256, _T("UPDATE dc_tables SET retention_time=0 WHERE retention_time=%d"), defaultRetentionTime);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetSchemaVersion(371));
   return TRUE;
}

/**
 * Upgrade from V369 to V370
 */
static BOOL H_UpgradeFromV369(int currVersion, int newVersion)
{
    CHK_EXEC(CreateTable(
      _T("CREATE TABLE dashboard_associations (")
      _T("   object_id integer not null,")
      _T("   dashboard_id integer not null,")
      _T("   PRIMARY KEY(object_id,dashboard_id))")));
   CHK_EXEC(SetSchemaVersion(370));
   return TRUE;
}

/**
 * Upgrade from V368 to V369
 */
static BOOL H_UpgradeFromV368(int currVersion, int newVersion)
{
    CHK_EXEC(CreateTable(
      _T("CREATE TABLE scheduled_tasks (")
      _T("	 id integer not null,")
      _T("   taskId varchar(255) null,")
      _T("   schedule varchar(127) null,")
      _T("   params varchar(1023) null,")
      _T("   execution_time integer not null,")
      _T("   last_execution_time integer not null,")
      _T("   flags integer not null,")
      _T("   owner integer not null,")
      _T("   PRIMARY KEY(id))")));
   CHK_EXEC(SetSchemaVersion(369));
   return TRUE;
}

/**
 * Upgrade from V367 to V368
 */
static BOOL H_UpgradeFromV367(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE nodes ADD rack_height integer\n")
      _T("UPDATE nodes SET rack_height=1\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(368));
   return TRUE;
}

/**
 * Upgrade from V366 to V367
 */
static BOOL H_UpgradeFromV366(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("TrapSourcesInAllZones"), _T("0"), _T("Search all zones to match trap/syslog source address to node"), 'B', true, true, false, false));

   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD snmp_sys_contact varchar(127)")));
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD snmp_sys_location varchar(127)")));

   CHK_EXEC(SetSchemaVersion(367));
   return TRUE;
}

/**
 * Upgrade from V365 to V366
 */
static BOOL H_UpgradeFromV365(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("ServerCommandOutputTimeout"), _T("60"), true, false));

   CHK_EXEC(SetSchemaVersion(366));
   return TRUE;
}

/**
 * Upgrade from V364 to V365
 */
static BOOL H_UpgradeFromV364(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("SNMPPorts"), _T("161"), true, false));
   CHK_EXEC(SetSchemaVersion(365));
   return TRUE;
}

/**
 * Upgrade from V363 to V364
 */
static BOOL H_UpgradeFromV363(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE interfaces ADD speed $SQL:INT64\n")
      _T("ALTER TABLE interfaces ADD iftable_suffix varchar(127)\n")
      _T("UPDATE interfaces SET speed=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetSchemaVersion(364));
   return TRUE;
}

/**
 * Upgrade from V362 to V363
 */
static BOOL H_UpgradeFromV362(int currVersion, int newVersion)
{
   DBResizeColumn(g_hCoreDB, _T("config"), _T("var_value"), 2000, true);
   CHK_EXEC(SetSchemaVersion(363));
   return TRUE;
}

/**
 * Upgrade from V361 to V362
 */
static BOOL H_UpgradeFromV361(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("CaseInsensitiveLoginNames"), _T("0"), _T("Enable/disable case insensitive login names"), 'B', true, true, false));
   CHK_EXEC(SetSchemaVersion(362));
   return TRUE;
}

/**
 * Upgrade from V360 to V361
 */
static BOOL H_UpgradeFromV360(int currVersion, int newVersion)
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE object_tools_input_fields (")
      _T("	tool_id integer not null,")
      _T("   name varchar(31) not null,")
      _T("   input_type char(1) not null,")
      _T("   display_name varchar(127) null,")
      _T("   config $SQL:TEXT null,")
      _T("   PRIMARY KEY(tool_id,name))")));

   CHK_EXEC(SetSchemaVersion(361));
   return TRUE;
}

/**
 * Upgrade from V359 to V360
 */
static BOOL H_UpgradeFromV359(int currVersion, int newVersion)
{
   DB_RESULT hResult = SQLSelect(_T("SELECT tool_id,tool_type,tool_data,confirmation_text FROM object_tools"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);

         TCHAR *text = DBGetField(hResult, i, 3, NULL, 0);
         if (text != NULL)
         {
            ConvertObjectToolMacros(id, text, _T("confirmation_text"));
            free(text);
         }

         int type = DBGetFieldLong(hResult, i, 1);
         if ((type == 1) || (type == 4) || (type == 5))
         {
            text = DBGetField(hResult, i, 2, NULL, 0);
            if (text != NULL)
            {
               ConvertObjectToolMacros(id, text, _T("tool_data"));
               free(text);
            }
         }
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return FALSE;
   }
   CHK_EXEC(SetSchemaVersion(360));
   return TRUE;
}

/**
 * Upgrade from V358 to V359
 */
static BOOL H_UpgradeFromV358(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE network_maps ADD object_display_mode integer\n")
      _T("UPDATE network_maps SET object_display_mode=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetSchemaVersion(359));
   return TRUE;
}

/**
 * Upgrade from V357 to V358
 */
static BOOL H_UpgradeFromV357(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE config ADD data_type char(1) not null default 'S'\n")
      _T("ALTER TABLE config ADD is_public char(1) not null default 'N'\n")
      _T("ALTER TABLE config ADD description varchar(255)\n")
      _T("ALTER TABLE config ADD possible_values $SQL:TEXT\n")
      _T("<END>");
   static TCHAR batchOracle[] =
      _T("ALTER TABLE config ADD data_type char(1) default 'S' not null\n")
      _T("ALTER TABLE config ADD is_public char(1) default 'N' not null\n")
      _T("ALTER TABLE config ADD description varchar(255)\n")
      _T("ALTER TABLE config ADD possible_values $SQL:TEXT\n")
      _T("<END>");
   CHK_EXEC(SQLBatch((g_dbSyntax == DB_SYNTAX_ORACLE) ? batchOracle : batch));

   CHK_EXEC(CreateConfigParam(_T("DashboardDataExportEnableInterpolation"), _T("1"), _T("Enable/disable data interpolation in dashboard data export"), 'B', true, false, true));

   CHK_EXEC(SetSchemaVersion(358));
   return TRUE;
}

/**
 * Upgrade from V356 to V357
 */
static BOOL H_UpgradeFromV356(int currVersion, int newVersion)
{
   TCHAR comunityString[256];
   comunityString[0] = 0;
   DB_RESULT hResult = SQLSelect(_T("SELECT var_value FROM config WHERE var_name='DefaultCommunityString'"));
   if (hResult != NULL)
   {
      if(DBGetNumRows(hResult) > 0)
      {
         DBGetField(hResult, 0, 0, comunityString, 256);
      }
      DBFreeResult(hResult);
   }

   if (comunityString[0] != 0)
   {
      DB_RESULT hResult = SQLSelect(_T("SELECT id, community FROM snmp_communities"));
      if (hResult != NULL)
      {
         CHK_EXEC(SQLQuery(_T("DELETE FROM snmp_communities")));

         TCHAR query[1024];
         _sntprintf(query, 1024, _T("INSERT INTO snmp_communities (id,community) VALUES(%d,'%s')"), 1, comunityString);
         CHK_EXEC(SQLQuery(query));

         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            _sntprintf(query, 1024, _T("INSERT INTO snmp_communities (id,community) VALUES(%d,'%s')"), i + 2, DBGetField(hResult, i, 1, comunityString, 256));
            CHK_EXEC(SQLQuery(query));
         }
      }
   }

   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='DefaultCommunityString'")));

   CHK_EXEC(SetSchemaVersion(357));
   return TRUE;
}

/**
 * Upgrade from V355 to V356
 */
static BOOL H_UpgradeFromV355(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("DELETE FROM config WHERE var_name='NumberOfBusinessServicePollers'\n")
      _T("DELETE FROM config WHERE var_name='NumberOfConditionPollers'\n")
      _T("DELETE FROM config WHERE var_name='NumberOfConfigurationPollers'\n")
      _T("DELETE FROM config WHERE var_name='NumberOfDiscoveryPollers'\n")
      _T("DELETE FROM config WHERE var_name='NumberOfInstancePollers'\n")
      _T("DELETE FROM config WHERE var_name='NumberOfRoutingTablePollers'\n")
      _T("DELETE FROM config WHERE var_name='NumberOfStatusPollers'\n")
      _T("DELETE FROM config WHERE var_name='NumberOfTopologyTablePollers'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(CreateConfigParam(_T("PollerThreadPoolBaseSize"), _T("10"), true, true));
   CHK_EXEC(CreateConfigParam(_T("PollerThreadPoolMaxSize"), _T("250"), true, true));

   CHK_EXEC(SetSchemaVersion(356));
   return TRUE;
}

/**
 * Upgrade from V354 to V355
 */
static BOOL H_UpgradeFromV354(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE nodes ADD agent_cache_mode char(1)\n")
      _T("UPDATE nodes SET agent_cache_mode='0'\n")
      _T("DELETE FROM config WHERE var_name='ServerID'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(CreateConfigParam(_T("DefaultAgentCacheMode"), _T("2"), true, true));

   CHK_EXEC(SetSchemaVersion(355));
   return TRUE;
}

/**
 * Upgrade from V353 to V354
 */
static BOOL H_UpgradeFromV353(int currVersion, int newVersion)
{
   CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("users"), _T("password"), 127, false));
   CHK_EXEC(SetSchemaVersion(354));
   return TRUE;
}

/**
 * Upgrade from V352 to V353
 */
static BOOL H_UpgradeFromV352(int currVersion, int newVersion)
{
	CHK_EXEC(SQLQuery(_T("ALTER TABLE dci_summary_tables ADD guid varchar(36)")));
   CHK_EXEC(GenerateGUID(_T("dci_summary_tables"), _T("id"), _T("guid"), NULL));
   CHK_EXEC(SetSchemaVersion(353));
   return TRUE;
}

/**
 * Upgrade from V351 to V352
 */
static BOOL H_UpgradeFromV351(int currVersion, int newVersion)
{
	CHK_EXEC(SQLQuery(_T("ALTER TABLE object_tools ADD guid varchar(36)")));
   CHK_EXEC(GenerateGUID(_T("object_tools"), _T("tool_id"), _T("guid"), NULL));
   CHK_EXEC(SetSchemaVersion(352));
   return TRUE;
}

/**
 * Upgrade from V350 to V351
 */
static BOOL H_UpgradeFromV350(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE access_points ADD ap_index integer\n")
      _T("UPDATE access_points SET ap_index=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetSchemaVersion(351));
   return TRUE;
}

/**
 * Upgrade from V349 to V350
 */
static BOOL H_UpgradeFromV349(int currVersion, int newVersion)
{
   switch(g_dbSyntax)
	{
      case DB_SYNTAX_ORACLE:
         CHK_EXEC(SQLQuery(_T("UPDATE object_properties SET comments = comments || chr(13) || chr(10) || (SELECT description FROM ap_common WHERE ap_common.id = object_properties.object_id) WHERE EXISTS (SELECT description FROM ap_common WHERE ap_common.id = object_properties.object_id AND description IS NOT NULL)")));
			break;
		case DB_SYNTAX_DB2:
         CHK_EXEC(SQLQuery(_T("UPDATE object_properties SET comments = comments || chr(13) || chr(10) || (SELECT description FROM ap_common WHERE ap_common.id = object_properties.object_id) WHERE EXISTS (SELECT description FROM ap_common WHERE ap_common.id = object_properties.object_id AND description IS NOT NULL AND description <> '')")));
         break;
      case DB_SYNTAX_MSSQL:
         CHK_EXEC(SQLQuery(_T("UPDATE object_properties SET comments = CAST(comments AS varchar(4000)) + char(13) + char(10) + CAST((SELECT description FROM ap_common WHERE ap_common.id = object_properties.object_id) AS varchar(4000)) WHERE EXISTS (SELECT description FROM ap_common WHERE ap_common.id = object_properties.object_id AND description IS NOT NULL AND datalength(description) <> 0)")));
			break;
		case DB_SYNTAX_PGSQL:
         CHK_EXEC(SQLQuery(_T("UPDATE object_properties SET comments = comments || '\\015\\012' || (SELECT description FROM ap_common WHERE ap_common.id = object_properties.object_id) WHERE EXISTS (SELECT description FROM ap_common WHERE ap_common.id = object_properties.object_id AND description IS NOT NULL AND description <> '')")));
			break;
		case DB_SYNTAX_SQLITE:
         CHK_EXEC(SQLQuery(_T("UPDATE object_properties SET comments = comments || char(13,10) || (SELECT description FROM ap_common WHERE ap_common.id = object_properties.object_id) WHERE EXISTS (SELECT description FROM ap_common WHERE ap_common.id = object_properties.object_id AND description IS NOT NULL AND description <> '')")));
			break;
		case DB_SYNTAX_MYSQL:
			CHK_EXEC(SQLQuery(_T("UPDATE object_properties, ap_common SET object_properties.comments=CONCAT(object_properties.comments, '\\r\\n', ap_common.description) WHERE object_properties.object_id=ap_common.id AND (ap_common.description!='' AND ap_common.description IS NOT NULL)")));
			break;
		default:
         break;
	}

   CHK_EXEC(DBDropColumn(g_hCoreDB, _T("ap_common"), _T("description")));
   CHK_EXEC(SetSchemaVersion(350));
   return TRUE;
}

/**
 * Upgrade from V348 to V349
 */
static BOOL H_UpgradeFromV348(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='HouseKeepingInterval'")));
   CHK_EXEC(CreateConfigParam(_T("HousekeeperStartTime"), _T("02:00"), true, true));
   CHK_EXEC(SetSchemaVersion(349));
   return TRUE;
}

/**
 * Upgrade from V347 to V348
 */
static BOOL H_UpgradeFromV347(int currVersion, int newVersion)
{
   CHK_EXEC(CreateEventTemplate(EVENT_IF_IPADDR_ADDED, _T("SYS_IF_IPADDR_ADDED"), SEVERITY_NORMAL, EF_LOG, NULL,
         _T("IP address %3/%4 added to interface \"%2\""),
         _T("Generated when IP address added to interface.\r\n")
         _T("Parameters:\r\n")
         _T("    1) Interface object ID\r\n")
         _T("    2) Interface name\r\n")
         _T("    3) IP address\r\n")
         _T("    4) Network mask\r\n")
         _T("    5) Interface index")));

   CHK_EXEC(CreateEventTemplate(EVENT_IF_IPADDR_DELETED, _T("SYS_IF_IPADDR_DELETED"), SEVERITY_NORMAL, EF_LOG, NULL,
         _T("IP address %3/%4 deleted from interface \"%2\""),
         _T("Generated when IP address deleted from interface.\r\n")
         _T("Parameters:\r\n")
         _T("    1) Interface object ID\r\n")
         _T("    2) Interface name\r\n")
         _T("    3) IP address\r\n")
         _T("    4) Network mask\r\n")
         _T("    5) Interface index")));

   CHK_EXEC(SetSchemaVersion(348));
   return TRUE;
}

/**
 * Upgrade from V346 to V347
 */
static BOOL H_UpgradeFromV346(int currVersion, int newVersion)
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE interface_address_list (")
      _T("   iface_id integer not null,")
      _T("	 ip_addr varchar(48) not null,")
      _T("	 ip_netmask integer not null,")
      _T("   PRIMARY KEY(iface_id,ip_addr))")));

   DB_RESULT hResult = SQLSelect(_T("SELECT id,ip_addr,ip_netmask FROM interfaces WHERE ip_addr<>'0.0.0.0'"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         TCHAR query[256], addr[64];
         _sntprintf(query, 256, _T("INSERT INTO interface_address_list (iface_id,ip_addr,ip_netmask) VALUES (%d,'%s',%d)"),
            DBGetFieldLong(hResult, i, 0), DBGetField(hResult, i, 1, addr, 64), DBGetFieldLong(hResult, i, 2));
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return FALSE;
   }

   static TCHAR batch[] =
      _T("ALTER TABLE interfaces DROP COLUMN ip_addr\n")
      _T("ALTER TABLE interfaces DROP COLUMN ip_netmask\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetSchemaVersion(347));
   return TRUE;
}

/**
 * Upgrade from V345 to V346
 */
static BOOL H_UpgradeFromV345(int currVersion, int newVersion)
{
   if (g_dbSyntax == DB_SYNTAX_MSSQL)
   {
      CHK_EXEC(DBDropPrimaryKey(g_hCoreDB, _T("cluster_sync_subnets")));
      CHK_EXEC(DBDropPrimaryKey(g_hCoreDB, _T("address_lists")));
      CHK_EXEC(DBDropPrimaryKey(g_hCoreDB, _T("vpn_connector_networks")));
   }

   CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("cluster_sync_subnets"), _T("subnet_addr"), 48, false));
   CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("cluster_resources"), _T("ip_addr"), 48, false));
   CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("subnets"), _T("ip_addr"), 48, false));
   CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("interfaces"), _T("ip_addr"), 48, false));
   CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("network_services"), _T("ip_bind_addr"), 48, false));
   CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("vpn_connector_networks"), _T("ip_addr"), 48, false));
   CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("snmp_trap_log"), _T("ip_addr"), 48, false));
   CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("address_lists"), _T("addr1"), 48, false));
   CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("address_lists"), _T("addr2"), 48, false));
   CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("nodes"), _T("primary_ip"), 48, false));

   CHK_EXEC(ConvertNetMasks(_T("cluster_sync_subnets"), _T("subnet_mask"), _T("cluster_id")));
   CHK_EXEC(ConvertNetMasks(_T("subnets"), _T("ip_netmask"), _T("id")));
   CHK_EXEC(ConvertNetMasks(_T("interfaces"), _T("ip_netmask"), _T("id")));
   CHK_EXEC(ConvertNetMasks(_T("vpn_connector_networks"), _T("ip_netmask"), _T("vpn_id"), _T("ip_addr")));

   DB_RESULT hResult = SQLSelect(_T("SELECT community_id,list_type,addr1,addr2 FROM address_lists WHERE addr_type=0"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      if (count > 0)
      {
         CHK_EXEC(SQLQuery(_T("DELETE FROM address_lists WHERE addr_type=0")));

         for(int i = 0; i < count; i++)
         {
            TCHAR query[256], addr[64];
            _sntprintf(query, 256, _T("INSERT INTO address_lists (addr_type,community_id,list_type,addr1,addr2) VALUES (0,%d,%d,'%s','%d')"),
               DBGetFieldLong(hResult, i, 0), DBGetFieldLong(hResult, i, 1), DBGetField(hResult, i, 2, addr, 64),
               BitsInMask(DBGetFieldIPAddr(hResult, i, 3)));
            CHK_EXEC(SQLQuery(query));
         }
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return FALSE;
   }

   if (g_dbSyntax == DB_SYNTAX_MSSQL)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE cluster_sync_subnets ADD CONSTRAINT pk_cluster_sync_subnets PRIMARY KEY (cluster_id,subnet_addr)")));
      CHK_EXEC(SQLQuery(_T("ALTER TABLE address_lists ADD CONSTRAINT pk_address_lists PRIMARY KEY (list_type,community_id,addr_type,addr1,addr2)")));
      CHK_EXEC(SQLQuery(_T("ALTER TABLE vpn_connector_networks ADD CONSTRAINT pk_vpn_connector_networks PRIMARY KEY (vpn_id,ip_addr)")));
   }

   CHK_EXEC(SetSchemaVersion(346));
   return TRUE;
}

/**
 * Upgrade from V344 to V345
 */
static BOOL H_UpgradeFromV344(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("NumberOfInstancePollers"), _T("10"), 1, 1));
   CHK_EXEC(CreateConfigParam(_T("InstancePollingInterval"), _T("600"), 1, 1));
   CHK_EXEC(SetSchemaVersion(345));
   return TRUE;
}

/**
 * Upgrade from V343 to V344
 */
static BOOL H_UpgradeFromV343(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE interfaces ADD mtu integer\n")
      _T("ALTER TABLE interfaces ADD alias varchar(255)\n")
      _T("UPDATE interfaces SET mtu=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(344));
   return TRUE;
}

/**
 * Upgrade from V342 to V343
 */
static BOOL H_UpgradeFromV342(int currVersion, int newVersion)
{
   if (g_dbSyntax != DB_SYNTAX_MSSQL)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='CREATE INDEX idx_idata_%d_id_timestamp ON idata_%d(item_id,idata_timestamp DESC)' WHERE var_name='IDataIndexCreationCommand_0'")));
      CHK_EXEC(DBCommit(g_hCoreDB));   // do reindexing outside current transaction
      ReindexIData();
      CHK_EXEC(DBBegin(g_hCoreDB));
   }
   CHK_EXEC(SetSchemaVersion(343));
   return TRUE;
}

/**
 * Upgrade from V341 to V342
 */
static BOOL H_UpgradeFromV341(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE object_tools ADD tool_filter $SQL:TEXT")));
   DB_RESULT hResult = SQLSelect(_T("SELECT tool_id, matching_oid FROM object_tools"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         TCHAR *oid = DBGetField(hResult, i, 1, NULL, 0);
         if ((oid != NULL) && (*oid != 0))
         {
            TCHAR *newConfig = (TCHAR *)malloc((_tcslen(oid) + 512) * sizeof(TCHAR));
            _tcscpy(newConfig, _T("<objectToolFilter>"));
            _tcscat(newConfig, _T("<snmpOid>"));
            _tcscat(newConfig, oid);
            _tcscat(newConfig, _T("</snmpOid>"));
            _tcscat(newConfig, _T("</objectToolFilter>"));

            DB_STATEMENT statment = DBPrepare(g_hCoreDB, _T("UPDATE object_tools SET tool_filter=? WHERE tool_id=?"));
            if (statment != NULL)
            {
               DBBind(statment, 1, DB_SQLTYPE_TEXT, newConfig, DB_BIND_STATIC);
               DBBind(statment, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 0));
               CHK_EXEC(DBExecute(statment));
               DBFreeStatement(statment);
            }
            else
            {
               if (!g_bIgnoreErrors)
                  return FALSE;
            }
         }
         free(oid);
      }
   }
   CHK_EXEC(DBDropColumn(g_hCoreDB, _T("object_tools"), _T("matching_oid"))); //delete old column
   CHK_EXEC(SetSchemaVersion(342));
   return TRUE;
}

/**
 * Upgrade from V340 to V341
 */
static BOOL H_UpgradeFromV340(int currVersion, int newVersion)
{
    static TCHAR batch[] =
      _T("ALTER TABLE object_properties ADD country varchar(63)\n")
      _T("ALTER TABLE object_properties ADD city varchar(63)\n")
      _T("ALTER TABLE object_properties ADD street_address varchar(255)\n")
      _T("ALTER TABLE object_properties ADD postcode varchar(31)\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(341));
   return TRUE;
}

/**
 * Upgrade from V339 to V340
 */
static BOOL H_UpgradeFromV339(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("LdapPageSize"), _T("1000"), 1, 0));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_value='1' WHERE var_name='LdapUserDeleteAction'")));
   CHK_EXEC(SetSchemaVersion(340));
   return TRUE;
}

/**
 * Upgrade from V338 to V339
 */
static BOOL H_UpgradeFromV338(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("EscapeLocalCommands"), _T("0"), 1, 0));
   CHK_EXEC(SetSchemaVersion(339));
   return TRUE;
}

/**
 * Upgrade from V337 to V338
 */
static BOOL H_UpgradeFromV337(int currVersion, int newVersion)
{
    static TCHAR batch[] =
      _T("ALTER TABLE nodes ADD icmp_proxy integer\n")
      _T("UPDATE nodes SET icmp_proxy=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(338));
   return TRUE;
}

/**
 * Upgrade from V336 to V337
 */
static BOOL H_UpgradeFromV336(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("SyslogNodeMatchingPolicy"), _T("0"), 1, 1));
   CHK_EXEC(SetSchemaVersion(337));
   return TRUE;
}

/**
 * Upgrade from V335 to V336
 */
static BOOL H_UpgradeFromV335(int currVersion, int newVersion)
{
   CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("network_map_links"), _T("connector_name1"), 255, true));
   CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("network_map_links"), _T("connector_name2"), 255, true));
   CHK_EXEC(SetSchemaVersion(336));
   return TRUE;
}

/**
 * Upgrade from V334 to V335
 */
static BOOL H_UpgradeFromV334(int currVersion, int newVersion)
{
   CHK_EXEC(CreateEventTemplate(EVENT_IF_MASK_CHANGED, _T("SYS_IF_MASK_CHANGED"), SEVERITY_NORMAL, EF_LOG, NULL,
         _T("Interface \"%2\" changed mask from %6 to %4 (IP Addr: %3/%4, IfIndex: %5)"),
         _T("Generated when when network mask on interface is changed.\r\n")
         _T("Parameters:\r\n")
         _T("    1) Interface object ID\r\n")
         _T("    2) Interface name\r\n")
         _T("    3) IP address\r\n")
         _T("    4) New network mask\r\n")
         _T("    5) Interface index\r\n")
         _T("    6) Old network mask")));
   CHK_EXEC(SetSchemaVersion(335));
   return TRUE;
}

/**
 * Upgrade from V333 to V334
 */
static BOOL H_UpgradeFromV333(int currVersion, int newVersion)
{
   CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("user_groups"), _T("description")));
   CHK_EXEC(SetSchemaVersion(334));
   return TRUE;
}

/**
 * Upgrade from V332 to V333
 */
static BOOL H_UpgradeFromV332(int currVersion, int newVersion)
{
    static TCHAR batch[] =
      _T("INSERT INTO metadata (var_name,var_value)")
      _T("   VALUES ('LocationHistory','CREATE TABLE gps_history_%d (latitude varchar(20), longitude varchar(20), accuracy integer not null, start_timestamp integer not null, end_timestamp integer not null, PRIMARY KEY(start_timestamp))')\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(333));
   return TRUE;
}

/**
 * Upgrade from V331 to V332
 */
static BOOL H_UpgradeFromV331(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("UPDATE items SET instd_data=instance WHERE node_id=template_id AND instd_method=0")));
   CHK_EXEC(SetSchemaVersion(332));
   return TRUE;
}

/**
 * Upgrade from V330 to V331
 */
static BOOL H_UpgradeFromV330(int currVersion, int newVersion)
{
   if (g_dbSyntax == DB_SYNTAX_ORACLE)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE audit_log ADD session_id integer DEFAULT 0 NOT NULL")));
   }
   else
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE audit_log ADD session_id integer NOT NULL DEFAULT 0")));
   }
   CHK_EXEC(SetSchemaVersion(331));
   return TRUE;
}

/**
 * Upgrade from V329 to V330
 */
static BOOL H_UpgradeFromV329(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("AlarmListDisplayLimit"), _T("4096"), 1, 0));
   CHK_EXEC(SetSchemaVersion(330));
   return TRUE;
}

/**
 * Upgrade from V328 to V329
 */
static BOOL H_UpgradeFromV328(int currVersion, int newVersion)
{
	CHK_EXEC(SQLQuery(_T("ALTER TABLE items ADD comments $SQL:TEXT")));
	CHK_EXEC(SQLQuery(_T("ALTER TABLE dc_tables ADD comments $SQL:TEXT")));
   CHK_EXEC(SetSchemaVersion(329));
   return TRUE;
}

/**
 * Upgrade from V327 to V328
 */
static BOOL H_UpgradeFromV327(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("ResolveDNSToIPOnStatusPoll"), _T("0"), 1, 1));
   CHK_EXEC(SetSchemaVersion(328));
   return TRUE;
}

/**
 * Upgrade from V326 to V327
 */
static BOOL H_UpgradeFromV326(int currVersion, int newVersion)
{
   CHK_EXEC(DBDropPrimaryKey(g_hCoreDB, _T("network_map_links")));
   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_network_map_links_map_id ON network_map_links(map_id)")));
   CHK_EXEC(SetSchemaVersion(327));
   return TRUE;
}

/**
 * Upgrade from V325 to V326
 */
static BOOL H_UpgradeFromV325(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE network_map_links DROP COLUMN color\n")
      _T("ALTER TABLE network_map_links DROP COLUMN status_object\n")
      _T("ALTER TABLE network_map_links DROP COLUMN routing\n")
      _T("ALTER TABLE network_map_links DROP COLUMN bend_points\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   if (g_dbSyntax == DB_SYNTAX_DB2)
   {
      CHK_EXEC(SQLQuery(_T("CALL Sysproc.admin_cmd('REORG TABLE network_map_links')")));
   }

   CHK_EXEC(SetSchemaVersion(326));
   return TRUE;
}

/**
 * Upgrade from V324 to V325
 */
static BOOL H_UpgradeFromV324(int currVersion, int newVersion)
{
   //move map link configuration to xml

   DB_RESULT hResult = SQLSelect(_T("SELECT map_id, element1, element2, element_data, color, status_object, routing, bend_points FROM network_map_links"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         TCHAR *config = DBGetField(hResult, i, 3, NULL, 0);
         if (config == NULL)
            config = _tcsdup(_T(""));
         UINT32 color = DBGetFieldULong(hResult, i, 4);
         UINT32 statusObject = DBGetFieldULong(hResult, i, 5);
         UINT32 routing = DBGetFieldULong(hResult, i, 6);
         TCHAR bendPoints[1024];
         DBGetField(hResult, i, 7, bendPoints, 1024);

         TCHAR *newConfig = (TCHAR *)malloc((_tcslen(config) + 4096) * sizeof(TCHAR));
         _tcscpy(newConfig, _T("<config>"));
         TCHAR* c1 = _tcsstr(config, _T("<dciList"));
         TCHAR* c2 = _tcsstr(config, _T("</dciList>"));
         if(c1 != NULL && c2!= NULL)
         {
            *c2 = 0;
            _tcscat(newConfig, c1);
            _tcscat(newConfig, _T("</dciList>"));
         }

         TCHAR tmp[2048];
         _sntprintf(tmp, 2048, _T("<color>%d</color>"), color),
         _tcscat(newConfig, tmp);

         if (statusObject != 0)
         {
            _sntprintf(tmp, 2048, _T("<objectStatusList length=\"1\"><long>%d</long></objectStatusList>"), statusObject);
            _tcscat(newConfig, tmp);
         }

         _sntprintf(tmp, 2048, _T("<routing>%d</routing>"), routing);
         _tcscat(newConfig, tmp);

         if (routing == 3 && bendPoints[0] != 0)
         {
            count = 1;
            for(size_t j = 0; j < _tcslen(bendPoints); j++)
            {
               if (bendPoints[j] == _T(','))
                  count++;
            }
            _sntprintf(tmp, 2048, _T("<bendPoints length=\"%d\">%s</bendPoints>"), count, bendPoints);
            _tcscat(newConfig, tmp);
         }
         _tcscat(newConfig, _T("</config>"));

         free(config);
         DB_STATEMENT statment = DBPrepare(g_hCoreDB, _T("UPDATE network_map_links SET element_data=? WHERE map_id=? AND element1=? AND element2=?"));
         if (statment != NULL)
         {
            DBBind(statment, 1, DB_SQLTYPE_TEXT, newConfig, DB_BIND_STATIC);
            DBBind(statment, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 0));
            DBBind(statment, 3, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 1));
            DBBind(statment, 4, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 2));
            CHK_EXEC(DBExecute(statment));
            DBFreeStatement(statment);
         }
         else
         {
            if (!g_bIgnoreErrors)
               return false;
         }
         free(newConfig);
      }
      DBFreeResult(hResult);
   }

   CHK_EXEC(SetSchemaVersion(325));
   return TRUE;
}

/**
 * Upgrade from V323 to V324
 */
static BOOL H_UpgradeFromV323(int currVersion, int newVersion)
{
   if (!MetaDataReadInt(_T("ValidTDataIndex"), 0))  // check if schema is already correct
   {
      TCHAR query[1024];
      _sntprintf(query, 1024,
         _T("UPDATE metadata SET var_value='CREATE TABLE tdata_records_%%d (record_id %s not null,row_id %s not null,instance varchar(255) null,PRIMARY KEY(row_id),FOREIGN KEY (record_id) REFERENCES tdata_%%d(record_id) ON DELETE CASCADE)' WHERE var_name='TDataTableCreationCommand_1'"),
         g_pszSqlType[g_dbSyntax][SQL_TYPE_INT64], g_pszSqlType[g_dbSyntax][SQL_TYPE_INT64]);
      CHK_EXEC(SQLQuery(query));

      RecreateTData(_T("nodes"), true, true);
      RecreateTData(_T("clusters"), true, true);
      RecreateTData(_T("mobile_devices"), true, true);
   }

   CHK_EXEC(SQLQuery(_T("DELETE FROM metadata WHERE var_name='ValidTDataIndex'")));
   CHK_EXEC(SetSchemaVersion(324));
   return TRUE;
}

/**
 * Upgrade from V322 to V323
 */
static BOOL H_UpgradeFromV322(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("ProcessTrapsFromUnmanagedNodes"), _T("0"), 1, 1));
   CHK_EXEC(SetSchemaVersion(323));
   return TRUE;
}

/**
 * Upgrade from V321 to V322
 */
static BOOL H_UpgradeFromV321(int currVersion, int newVersion)
{
   switch(g_dbSyntax)
	{
		case DB_SYNTAX_DB2:
         CHK_EXEC(SQLBatch(
			   _T("ALTER TABLE users ALTER COLUMN system_access SET DATA TYPE $SQL:INT64\n")
			   _T("ALTER TABLE user_groups ALTER COLUMN system_access SET DATA TYPE $SQL:INT64\n")
			   _T("<END>")));
			break;
		case DB_SYNTAX_MSSQL:
         CHK_EXEC(SQLBatch(
			   _T("ALTER TABLE users ALTER COLUMN system_access $SQL:INT64\n")
			   _T("ALTER TABLE user_groups ALTER COLUMN system_access $SQL:INT64\n")
			   _T("<END>")));
			break;
		case DB_SYNTAX_PGSQL:
         CHK_EXEC(SQLBatch(
			   _T("ALTER TABLE users ALTER COLUMN system_access TYPE $SQL:INT64\n")
			   _T("ALTER TABLE user_groups ALTER COLUMN system_access TYPE $SQL:INT64\n")
            _T("<END>")));
			break;
		case DB_SYNTAX_SQLITE:
         CHK_EXEC(SQLBatch(
            _T("CREATE TABLE temp_users AS SELECT * FROM users\n")
            _T("DROP TABLE users\n")
            _T("CREATE TABLE users (id integer not null, guid varchar(36) not null, name varchar(63) not null, password varchar(48) not null, system_access $SQL:INT64 not null, flags integer not null,")
            _T("   full_name varchar(127) null, description varchar(255) null, grace_logins integer not null, auth_method integer not null, cert_mapping_method integer not null, cert_mapping_data $SQL:TEXT null,")
            _T("   auth_failures integer not null, last_passwd_change integer not null, min_passwd_length integer not null, disabled_until integer not null, last_login integer not null, password_history $SQL:TEXT null,")
            _T("   xmpp_id varchar(127) null, ldap_dn $SQL:TEXT null, PRIMARY KEY(id))\n")
            _T("INSERT INTO users SELECT * FROM temp_users\n")
            _T("DROP TABLE temp_users\n")
            _T("CREATE TABLE temp_user_groups AS SELECT * FROM user_groups\n")
            _T("DROP TABLE user_groups\n")
            _T("CREATE TABLE user_groups (id integer not null, guid varchar(36) not null, name varchar(63) not null, system_access $SQL:INT64 not null, flags integer not null,")
            _T("   description varchar(255) not null, ldap_dn $SQL:TEXT null, PRIMARY KEY(id))\n")
            _T("INSERT INTO user_groups SELECT * FROM temp_user_groups\n")
            _T("DROP TABLE temp_user_groups\n")
            _T("<END>")));
			break;
		case DB_SYNTAX_ORACLE:
         // no changes needed
         break;
		default:
         CHK_EXEC(SQLBatch(
			   _T("ALTER TABLE users MODIFY system_access $SQL:INT64\n")
			   _T("ALTER TABLE user_groups MODIFY system_access $SQL:INT64\n")
			   _T("<END>")));
			break;
	}

   CHK_EXEC(SetSchemaVersion(322));
   return TRUE;
}

/**
 * Upgrade from V320 to V321
 */
static BOOL H_UpgradeFromV320(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE object_tools ADD command_short_name varchar(31)\n")
      _T("UPDATE object_tools SET command_short_name='Shutdown' WHERE tool_id=1\n")
      _T("UPDATE object_tools SET command_short_name='Restart' WHERE tool_id=2\n")
      _T("UPDATE object_tools SET command_short_name='Wakeup' WHERE tool_id=3\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(321));
   return TRUE;
}

/**
 * Upgrade from V319 to V320
 */
static BOOL H_UpgradeFromV319(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("LdapConnectionString"), _T("ldap://localhost:389"), 1, 0));
   CHK_EXEC(CreateConfigParam(_T("LdapSyncUser"), _T(""), 1, 0));
   CHK_EXEC(CreateConfigParam(_T("LdapSyncUserPassword"), _T(""), 1, 0));
   CHK_EXEC(CreateConfigParam(_T("LdapSearchBase"), _T(""), 1, 0));
   CHK_EXEC(CreateConfigParam(_T("LdapSearchFilter"), _T(""), 1, 0));
   CHK_EXEC(CreateConfigParam(_T("LdapUserDeleteAction"), _T("1"), 1, 0));
   CHK_EXEC(CreateConfigParam(_T("LdapMappingName"), _T(""), 1, 0));
   CHK_EXEC(CreateConfigParam(_T("LdapMappingFullName"), _T("displayName"), 1, 0));
   CHK_EXEC(CreateConfigParam(_T("LdapMappingDescription"), _T(""), 1, 0));
   CHK_EXEC(CreateConfigParam(_T("LdapGroupClass"), _T(""), 1, 0));
   CHK_EXEC(CreateConfigParam(_T("LdapUserClass"), _T(""), 1, 0));
   CHK_EXEC(CreateConfigParam(_T("LdapSyncInterval"), _T("0"), 1, 0));

   static TCHAR batch[] =
      _T("ALTER TABLE users ADD ldap_dn $SQL:TEXT\n")
      _T("ALTER TABLE user_groups ADD ldap_dn $SQL:TEXT\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetSchemaVersion(320));
   return TRUE;
}

/*
 * Upgrade from V318 to V319
 */
static BOOL H_UpgradeFromV318(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE object_tools ADD icon $SQL:TEXT\n")
      _T("ALTER TABLE object_tools ADD command_name varchar(255)\n")
      _T("UPDATE object_tools SET flags=74,command_name='Shutdown system',icon='89504e470d0a1a0a0000000d49484452000000100000001008060000001ff3ff610000000473424954080808087c086488000002bf49444154388d95933f689c7518c73fbf3f7779efaede995c137369d54123ba4843070b1204c10ed24db182939b90c1c1e2eee05871e8d0d2b98a8ba0c52ee2d0cb622acd99222dd1084d9a18cd3597f7bd6bdebbdffbfbe7701d82e8e0071ebec303dfe7e1e1fb883fea7542aba54c080dac153c261ed1a30540b51a8f359bd9c9e565afedd4943ab6b4f4d56873f38d5014ff6af04f95950aaad5fa7e7d7dfdbcee1d1c3cd95d59395b5a5c7c82ffc1fd4ee7acc9f349fd683090b1db15499ae2013b3f8f1c0e296f6f539c380140796707333747a856296d6ca081d1e1a138cc73a95d8cc28f468834459f3ecd7367cee0b38ccd7bf7787e711180dfaf5ee599850544a3c1760898d5556c51e06314d2c5288be150186b995d58404bc9eef5ebb87e86140229257690b17be33b4a4a3173ea14236b71d60a17a3901684b59652b34952ab31dcda6470f76794c9b0b6c0160665320eefae317ab04552ad529e9ec6c78003292dc861bf2f4408e369fb7b948a8cb2cd7085c115868998936887eb75514a617a3db66eb68505211d30f86b97dde536420844a341b17e8bf8db0a21ed12d23ddcda0ff46f7e4dac24482939b8b386b3060f4207206a457afb16be9f519f7f91f22baf52f9e91bfca7ef00829a4fb1af9fa3fed2cbf8419f6c75054a0a0fc800a025f151cafdcb17514af3ecc79f939fbf40d69c259d9ca1ffd687cc7d7411a5145b573e230e52d0120f68ffd8400ad8b97685c9934f31f9ee07b4de5e227ff37d8c311c4f12aad50afb5f5c62e7da65a400519204408f37108408de471e5cfa04fbe3b74c9d7b8ff2d32f1042805f7e25bdf1257fdeee103c8408528d53afa356c85a42b107d6812920bdd3c16f7448cae3d81a0b837cdc2b1c380f724203445d8ff161767cb66df1afe5380a0d3d05ca8d0f148110c02bb035b013109b1a17747b06baa20d3c84897dc93420feeb0b8f22203603dd19307f037f0665861328b32e0000000049454e44ae426082' WHERE tool_id=1\n")
      _T("UPDATE object_tools SET flags=74,command_name='Restart system',icon='89504e470d0a1a0a0000000d49484452000000100000001008060000001ff3ff610000000473424954080808087c0864880000029849444154388d85934d8b1c5514869f73eeadaaae6ea77bda388999882241fc5c0e09ae4484c4857b4177fa0b5474256edcfa075cbacbc23f6074a12332a0a2200a2189e8c468a23d93e99e9eaeaaae7beb1e1733f9902c7ce06c0ebc2f2fe7f0ca5fe311fdd71e77d93332922e08b749dcc5fe3bc9f72d5d1fcf861f7dd6f9f295353778ebdd0b71ff977374ad60f7888e1003bbb3379ceb930f4e7fbe7be5a7573da7f65697db17cf2ff2e757fe6e06a00e1141040aab59ebb5dc47076efbdb739cacc63edc9aabee4f64796248efb10dbcf738e750556268e97eff14b937ce6d1607126e55eae33c4956d5f4f72f21ae40d6cfe0bc4755c9f39cd4ccc0d27d7a695ae23c896fe7a645d5482c26340f0d19e639beb9811463c85788610a29c1d11d4284cc416a82848589861a49754bab390fac3f4ba69174f963ba7040d745249f4136033f63efd859769f78933a792c244265ea436d9a9a99e86895bc28b0e90fc03632bd88e463acb787580696f3e0fa299ade09e275a5feed2b09b5898f4ba35bdc40bb6b0034f5357cda4277bec354400d0b0d5d75406a5e42caa751d90596c425e22d00aa48771933a3c99e6230a8d1b241dcd1eb03a4a2c4563600f07615bc622da80510149aefa1b982ef3dc24d7d071b7afc71f0c781d58c83d107e48347d1f62a1a7f44f4d0c0130c4c4196d89fefa1273f215f7d9d4b379fa4dfdf22cb32bc7f99b5f533c45893edbc4fe75a8c0116c05b008b408434fd9a327f031b7d08c73670ee2c65595296259afe20fbe76de2fc9ba39e1cd6c6a7e4b0ae87d5200a4cbea4acce3318bd8865cfe1a283dd9fe9a65f901615a982d400c96360be9eda4ebbfdf0a6ec752f741ac11f1a89db02d9ba5bc8d483ae877587e2f0abdfac2b26b209488fa218b07627d7ff636dc524d52cff0513e53f37235ac3190000000049454e44ae426082' WHERE tool_id=2\n")
      _T("UPDATE object_tools SET flags=64,command_name='Wakeup node using Wake-On-LAN',icon='89504e470d0a1a0a0000000d49484452000000100000001008060000001ff3ff610000000473424954080808087c086488000000097048597300000dd700000dd70142289b780000001974455874536f667477617265007777772e696e6b73636170652e6f72679bee3c1a0000023649444154388d8d924f48545114c67ff7bd37ff7cf9071bc70a4d47271ca15c848d448185b40a89204890362d8568eb2270d7a2762d5cb7711504d1ae10c195218895a488528e4e06a653d338de7bdfbcf75ac84ce38c901f9ccdb9e7fbce39f77c627ce6872df6dd71f01f781e1d9c00866003215efaf99de7d6763afb1078721262053a800908ed5a5aa9b1e3bb0802a600c0717d3cdf3fae6cccd24a25abb302a80b990c265a009859d941299763249296d6b2a6732468d25a1f24156f00e0cbd62e9b5a71a0dd9a490cad14a570b4266c780cf546797cab1b1317139747435ddcec69266c78385a53c9b1b45265b548d022d51563f45a9c778b69ce35850058de928c0cb4933fd04c7ffece812e9639e5158480865098ebc9181fbfeef07a6e9dc68805c0af8243f45480ab174e33bb9426e7484a9b942710020c3b40e24c236f3facb1bd9b634d3a00d8e100ab992cb7af7421bc225aa9b280a195a414524972054d5f679488e5a394442949d8f4b8d4d14caea09115f55a490cad155a2b9452ecfdcef37e619ddef6287706ba89c76ce2319be1fe4e926d51663e6d90cdeda3d42147ebaa4fcc161da6a61739df52cfe88d8b0ca712f8be871d0e31bb94666a7a916c2e8feb7aff3cd33ef2f4c8612dd3a0a5d1a6bfa78d544f1bbeef33bf9a617e65939fb902c50a328068bd3bb10c1c71a3210401cb24143cbc82d2459c62ad8980154b2b3909bca87e91c09fea642d26ad67f7fb32afe6bebd5958dd1c2c48ddf45f8a10d87591bdcb89b3b3f7063a337f01f30f1c1c580292640000000049454e44ae426082' WHERE tool_id=3\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(319));
   return TRUE;
}

/**
 * Upgrade from V317 to V318
 */
static BOOL H_UpgradeFromV317(int currVersion, int newVersion)
{
   CHK_EXEC(CreateEventTemplate(EVENT_AP_DOWN, _T("SYS_AP_DOWN"), SEVERITY_CRITICAL, EF_LOG, NULL,
      _T("Access point %2 changed state to DOWN"),
		_T("Generated when access point state changes to DOWN.\r\n")
		_T("Parameters:\r\n")
		_T("    1) Access point object ID\r\n")
		_T("    2) Access point name\r\n")
		_T("    3) Access point MAC address\r\n")
		_T("    4) Access point IP address\r\n")
		_T("    5) Access point vendor name\r\n")
		_T("    6) Access point model\r\n")
		_T("    7) Access point serial number")));

   CHK_EXEC(SetSchemaVersion(318));
   return TRUE;
}

/**
 * Upgrade from V316 to V317
 */
static BOOL H_UpgradeFromV316(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("MinViewRefreshInterval"), _T("1000"), 1, 0));
   CHK_EXEC(SetSchemaVersion(317));
   return TRUE;
}

/**
 * Upgrade from V315 to V316
 */
static BOOL H_UpgradeFromV315(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE access_points ADD ap_state integer\n")
      _T("UPDATE access_points SET ap_state=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(CreateEventTemplate(EVENT_AP_ADOPTED, _T("SYS_AP_ADOPTED"), SEVERITY_NORMAL, EF_LOG, NULL,
      _T("Access point %2 changed state to ADOPTED"),
		_T("Generated when access point state changes to ADOPTED.\r\n")
		_T("Parameters:\r\n")
		_T("    1) Access point object ID\r\n")
		_T("    2) Access point name\r\n")
		_T("    3) Access point MAC address\r\n")
		_T("    4) Access point IP address\r\n")
		_T("    5) Access point vendor name\r\n")
		_T("    6) Access point model\r\n")
		_T("    7) Access point serial number")));

   CHK_EXEC(CreateEventTemplate(EVENT_AP_UNADOPTED, _T("SYS_AP_UNADOPTED"), SEVERITY_MAJOR, EF_LOG, NULL,
      _T("Access point %2 changed state to UNADOPTED"),
		_T("Generated when access point state changes to UNADOPTED.\r\n")
		_T("Parameters:\r\n")
		_T("    1) Access point object ID\r\n")
		_T("    2) Access point name\r\n")
		_T("    3) Access point MAC address\r\n")
		_T("    4) Access point IP address\r\n")
		_T("    5) Access point vendor name\r\n")
		_T("    6) Access point model\r\n")
		_T("    7) Access point serial number")));

   CHK_EXEC(SetSchemaVersion(316));
   return TRUE;
}

/**
 * Upgrade from V314 to V315
 */
static BOOL H_UpgradeFromV314(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE thresholds ADD match_count integer\n")
      _T("UPDATE thresholds SET match_count=0 WHERE current_state=0\n")
      _T("UPDATE thresholds SET match_count=1 WHERE current_state<>0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(315));
   return TRUE;
}

/**
 * Upgrade from V313 to V314
 */
static BOOL H_UpgradeFromV313(int currVersion, int newVersion)
{
   // Replace double backslash with single backslash in all "file download" (code 7) object tools
   DB_RESULT hResult = SQLSelect(_T("SELECT tool_id, tool_data FROM object_tools WHERE tool_type=7"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         TCHAR* toolData = DBGetField(hResult, i, 1, NULL, 0);
         TranslateStr(toolData, _T("\\\\"), _T("\\"));

         DB_STATEMENT statment = DBPrepare(g_hCoreDB, _T("UPDATE object_tools SET tool_data=?  WHERE tool_id=?"));
         if (statment == NULL)
            return FALSE;
         DBBind(statment, 1, DB_SQLTYPE_TEXT, toolData, DB_BIND_DYNAMIC);
         DBBind(statment, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 0));
         if(!DBExecute(statment))
            return FALSE;
         DBFreeStatement(statment);
      }
      DBFreeResult(hResult);
   }

   CHK_EXEC(SetSchemaVersion(314));
   return TRUE;
}

/**
 * Upgrade from V312 to V313
 */
static BOOL H_UpgradeFromV312(int currVersion, int newVersion)
{
   CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("object_tools"), _T("tool_name")));
   CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("object_tools"), _T("tool_data")));
   CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("object_tools"), _T("description")));
   CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("object_tools"), _T("confirmation_text")));
   CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("object_tools"), _T("matching_oid")));
   CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("object_tools_table_columns"), _T("col_name")));
   CHK_EXEC(ConvertStrings(_T("object_tools"), _T("tool_id"), _T("tool_name")));
   CHK_EXEC(ConvertStrings(_T("object_tools"), _T("tool_id"), _T("tool_data")));
   CHK_EXEC(ConvertStrings(_T("object_tools"), _T("tool_id"), _T("description")));
   CHK_EXEC(ConvertStrings(_T("object_tools"), _T("tool_id"), _T("confirmation_text")));
   CHK_EXEC(ConvertStrings(_T("object_tools"), _T("tool_id"), _T("matching_oid")));
   CHK_EXEC(ConvertStrings(_T("object_tools_table_columns"), _T("tool_id"), _T("col_number"), _T("col_name"), false));
   CHK_EXEC(SetSchemaVersion(313));
   return TRUE;
}

/**
 * Upgrade from V311 to V312
 */
static BOOL H_UpgradeFromV311(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("EnableReportingServer"), _T("0"), 1, 1));
   CHK_EXEC(CreateConfigParam(_T("ReportingServerHostname"), _T("localhost"), 1, 1));
   CHK_EXEC(CreateConfigParam(_T("ReportingServerPort"), _T("4710"), 1, 1));
   CHK_EXEC(SetSchemaVersion(312));
   return TRUE;
}

/**
 * Upgrade from V310 to V311
 */
static BOOL H_UpgradeFromV310(int currVersion, int newVersion)
{
   IntegerArray<UINT32> deleteList;

   // reports
   DB_RESULT hResult = SQLSelect(_T("SELECT id FROM reports"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
         deleteList.add(DBGetFieldULong(hResult, i, 0));
      DBFreeResult(hResult);
   }

   // report groups
   hResult = SQLSelect(_T("SELECT id FROM containers WHERE object_class=25"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
         deleteList.add(DBGetFieldULong(hResult, i, 0));
      DBFreeResult(hResult);
   }

   for(int i = 0; i < deleteList.size(); i++)
   {
      TCHAR query[256];

      _sntprintf(query, 256, _T("DELETE FROM object_properties WHERE object_id=%d"), deleteList.get(i));
      CHK_EXEC(SQLQuery(query));

      _sntprintf(query, 256, _T("DELETE FROM object_custom_attributes WHERE object_id=%d"), deleteList.get(i));
      CHK_EXEC(SQLQuery(query));

      _sntprintf(query, 256, _T("DELETE FROM acl WHERE object_id=%d"), deleteList.get(i));
      CHK_EXEC(SQLQuery(query));

      _sntprintf(query, 256, _T("DELETE FROM container_members WHERE object_id=%d OR container_id=%d"), deleteList.get(i), deleteList.get(i));
      CHK_EXEC(SQLQuery(query));
   }

   static TCHAR batch[] =
      _T("DROP TABLE reports\n")
      _T("DROP TABLE report_results\n")
      _T("DELETE FROM containers WHERE object_class=25\n")
      _T("DELETE FROM object_properties WHERE object_id=8\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(311));
   return TRUE;
}

/**
 * Upgrade from V309 to V310
 */
static BOOL H_UpgradeFromV309(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE interfaces ADD peer_proto integer\n")
      _T("UPDATE interfaces SET peer_proto=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetSchemaVersion(310));
   return TRUE;
}

/**
 * Upgrade from V308 to V309
 */
static BOOL H_UpgradeFromV308(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("HelpDeskLink"), _T("none"), 1, 1));
   CHK_EXEC(SetSchemaVersion(309));
   return TRUE;
}

/**
 * Upgrade from V307 to V308
 */
static BOOL H_UpgradeFromV307(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE network_map_elements ADD flags integer\n")
      _T("UPDATE network_map_elements SET flags=0\n")  //set all elements like manually generated
      _T("ALTER TABLE network_map_links ADD element_data $SQL:TEXT\n")
      _T("ALTER TABLE network_map_links ADD flags integer\n")
      _T("UPDATE network_map_links SET flags=0\n")  //set all elements like manually generated
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   // it is assumed that now all autogenerated maps contain only autogenerated objects and links
   // get elements from autogenerated maps and set their flags to AUTO_GENERATED for map elements and map links
   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT id FROM network_maps WHERE map_type=%d OR map_type=%d"),
   MAP_TYPE_LAYER2_TOPOLOGY, MAP_TYPE_IP_TOPOLOGY);
   DB_RESULT hResult = SQLSelect(query);
	if (hResult != NULL)
	{
		int count = DBGetNumRows(hResult);
		for(int i = 0; i < count; i++)
		{
			_sntprintf(query, 256, _T("UPDATE network_map_elements SET flags='1' WHERE map_id=%d"),
			           DBGetFieldULong(hResult, i, 0));
			CHK_EXEC(SQLQuery(query));
			_sntprintf(query, 256, _T("UPDATE network_map_links SET flags='1' WHERE map_id=%d"),
			           DBGetFieldULong(hResult, i, 0));
			CHK_EXEC(SQLQuery(query));
		}
		DBFreeResult(hResult);
	}

   CHK_EXEC(SetSchemaVersion(308));
   return TRUE;
}

/**
 * Upgrade from V306 to V307
 */
static BOOL H_UpgradeFromV306(int currVersion, int newVersion)
{
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("config_clob"), _T("var_value")));
	CHK_EXEC(ConvertStrings(_T("config_clob"), _T("var_name"), NULL, _T("var_value"), true));
   CHK_EXEC(SetSchemaVersion(307));
   return TRUE;
}

/**
 * Upgrade from V305 to V306
 */
static BOOL H_UpgradeFromV305(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("ExtendedLogQueryAccessControl"), _T("0"), 1, 0));
   CHK_EXEC(CreateConfigParam(_T("EnableTimedAlarmAck"), _T("1"), 1, 1));
   CHK_EXEC(CreateConfigParam(_T("EnableCheckPointSNMP"), _T("0"), 1, 0));
   CHK_EXEC(SetSchemaVersion(306));
   return TRUE;
}

/**
 * Upgrade from V304 to V305
 */
static BOOL H_UpgradeFromV304(int currVersion, int newVersion)
{
   CHK_EXEC(CreateEventTemplate(EVENT_IF_PEER_CHANGED, _T("SYS_IF_PEER_CHANGED"), SEVERITY_NORMAL, EF_LOG, NULL,
      _T("New peer for interface %3 is %7 interface %10 (%12)"),
		_T("Generated when peer information for interface changes.\r\n")
		_T("Parameters:\r\n")
		_T("    1) Local interface object ID\r\n")
		_T("    2) Local interface index\r\n")
		_T("    3) Local interface name\r\n")
		_T("    4) Local interface IP address\r\n")
		_T("    5) Local interface MAC address\r\n")
		_T("    6) Peer node object ID\r\n")
		_T("    7) Peer node name\r\n")
		_T("    8) Peer interface object ID\r\n")
		_T("    9) Peer interface index\r\n")
		_T("   10) Peer interface name\r\n")
		_T("   11) Peer interface IP address\r\n")
		_T("   12) Peer interface MAC address")));
   CHK_EXEC(SetSchemaVersion(305));
   return TRUE;
}

/**
 * Upgrade from V303 to V304
 */
static BOOL H_UpgradeFromV303(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("StrictAlarmStatusFlow"), _T("0"), 1, 0));
   CHK_EXEC(SetSchemaVersion(304));
   return TRUE;
}

/**
 * Upgrade from V302 to V303
 */
static BOOL H_UpgradeFromV302(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE alarms ADD ack_timeout integer\n")
      _T("UPDATE alarms SET ack_timeout='0'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetSchemaVersion(303));
   return TRUE;
}

/**
 * Upgrade from V301 to V302
 */
static BOOL H_UpgradeFromV301(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("DELETE FROM config WHERE var_name='DisableVacuum'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetSchemaVersion(302));
   return TRUE;
}

/**
 * Upgrade from V300 to V301
 */
static BOOL H_UpgradeFromV300(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE thresholds ADD script $SQL:TEXT\n")
      _T("ALTER TABLE thresholds ADD sample_count integer\n")
      _T("UPDATE thresholds SET sample_count=parameter_1\n")
      _T("ALTER TABLE thresholds DROP COLUMN parameter_1\n")
      _T("ALTER TABLE thresholds DROP COLUMN parameter_2\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetSchemaVersion(301));
   return TRUE;
}

/**
 * Upgrade from V299 to V300
 */
static BOOL H_UpgradeFromV299(int currVersion, int newVersion)
{
	CHK_EXEC(CreateConfigParam(_T("EnableXMPPConnector"), _T("0"), 1, 1));
	CHK_EXEC(CreateConfigParam(_T("XMPPLogin"), _T("netxms@localhost"), 1, 1));
	CHK_EXEC(CreateConfigParam(_T("XMPPPassword"), _T("netxms"), 1, 1));
	CHK_EXEC(CreateConfigParam(_T("XMPPServer"), _T("localhost"), 1, 1));
	CHK_EXEC(CreateConfigParam(_T("XMPPPort"), _T("5222"), 1, 1));

   DBRemoveNotNullConstraint(g_hCoreDB, _T("users"), _T("full_name"));
   DBRemoveNotNullConstraint(g_hCoreDB, _T("users"), _T("description"));
   DBRemoveNotNullConstraint(g_hCoreDB, _T("users"), _T("cert_mapping_data"));
   DBRemoveNotNullConstraint(g_hCoreDB, _T("user_groups"), _T("description"));
   DBRemoveNotNullConstraint(g_hCoreDB, _T("userdb_custom_attributes"), _T("attr_value"));

   ConvertStrings(_T("users"), _T("id"), _T("full_name"));
   ConvertStrings(_T("users"), _T("id"), _T("description"));
   ConvertStrings(_T("users"), _T("id"), _T("cert_mapping_data"));
   ConvertStrings(_T("user_groups"), _T("id"), _T("description"));
   ConvertStrings(_T("userdb_custom_attributes"), _T("object_id"), _T("attr_name"), _T("attr_name"), true);
   ConvertStrings(_T("userdb_custom_attributes"), _T("object_id"), _T("attr_name"), _T("attr_value"), true);

   CHK_EXEC(SQLQuery(_T("ALTER TABLE users ADD xmpp_id varchar(127)")));

   CHK_EXEC(SetSchemaVersion(300));
   return TRUE;
}

/**
 * Upgrade from V298 to V299
 */
static BOOL H_UpgradeFromV298(int currVersion, int newVersion)
{
	CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET message='Subnet %2 added',description='")
      _T("Generated when subnet object added to the database.\r\n")
		_T("Parameters:\r\n")
      _T("   1) Subnet object ID\r\n")
      _T("   2) Subnet name\r\n")
      _T("   3) IP address\r\n")
      _T("   4) Network mask")
      _T("' WHERE event_code=2")));
	CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET message='Subnet %2 deleted',description='")
      _T("Generated when subnet object deleted from the database.\r\n")
		_T("Parameters:\r\n")
      _T("   1) Subnet object ID\r\n")
      _T("   2) Subnet name\r\n")
      _T("   3) IP address\r\n")
      _T("   4) Network mask")
      _T("' WHERE event_code=19")));
	CHK_EXEC(SetSchemaVersion(299));
   return TRUE;
}

/**
 * Upgrade from V297 to V298
 */
static BOOL H_UpgradeFromV297(int currVersion, int newVersion)
{
	CHK_EXEC(CreateConfigParam(_T("AgentDefaultSharedSecret"), _T("netxms"), 1, 0));
	CHK_EXEC(SetSchemaVersion(298));
   return TRUE;
}

/**
 * Upgrade from V296 to V297
 */
static BOOL H_UpgradeFromV296(int currVersion, int newVersion)
{
	CHK_EXEC(CreateConfigParam(_T("UseSNMPTrapsForDiscovery"), _T("0"), 1, 1));
	CHK_EXEC(SetSchemaVersion(297));
   return TRUE;
}

/**
 * Upgrade from V295 to V296
 */
static BOOL H_UpgradeFromV295(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE nodes ADD boot_time integer\n")
      _T("UPDATE nodes SET boot_time=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetSchemaVersion(296));
   return TRUE;
}

/**
 * Upgrade from V294 to V295
 */
static BOOL H_UpgradeFromV294(int currVersion, int newVersion)
{
	CHK_EXEC(CreateConfigParam(_T("IcmpPingTimeout"), _T("1500"), 1, 1));
	CHK_EXEC(SetSchemaVersion(295));
   return TRUE;
}

/**
 * Upgrade from V293 to V294
 */
static BOOL H_UpgradeFromV293(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("DELETE FROM metadata WHERE var_name LIKE 'TDataTableCreationCommand_%'\n")
      _T("INSERT INTO metadata (var_name,var_value)")
      _T("   VALUES ('TDataTableCreationCommand_0','CREATE TABLE tdata_%d (item_id integer not null,tdata_timestamp integer not null,record_id $SQL:INT64 not null,UNIQUE(record_id))')\n")
      _T("INSERT INTO metadata (var_name,var_value)")
	   _T("   VALUES ('TDataTableCreationCommand_1','CREATE TABLE tdata_records_%d (record_id $SQL:INT64 not null,row_id $SQL:INT64 not null,instance varchar(255) null,PRIMARY KEY(row_id),FOREIGN KEY (record_id) REFERENCES tdata_%d(record_id) ON DELETE CASCADE)')\n")
      _T("INSERT INTO metadata (var_name,var_value)")
	   _T("   VALUES ('TDataTableCreationCommand_2','CREATE TABLE tdata_rows_%d (row_id $SQL:INT64 not null,column_id integer not null,value varchar(255) null,PRIMARY KEY(row_id,column_id),FOREIGN KEY (row_id) REFERENCES tdata_records_%d(row_id) ON DELETE CASCADE)')\n")
      _T("INSERT INTO metadata (var_name,var_value)")
      _T("   VALUES ('TDataIndexCreationCommand_2','CREATE INDEX idx_tdata_rec_%d_id ON tdata_records_%d(record_id)')\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   RecreateTData(_T("nodes"), true, false);
   RecreateTData(_T("clusters"), true, false);
   RecreateTData(_T("mobile_devices"), true, false);

   CHK_EXEC(SQLQuery(_T("INSERT INTO metadata (var_name,var_value) VALUES ('ValidTDataIndex','1')")));
   CHK_EXEC(SetSchemaVersion(294));
   return TRUE;
}

/**
 * Upgrade from V292 to V293
 */
static BOOL H_UpgradeFromV292(int currVersion, int newVersion)
{
	CHK_EXEC(CreateConfigParam(_T("DefaultConsoleShortTimeFormat"), _T("HH:mm"), 1, 0));
	CHK_EXEC(SetSchemaVersion(293));
   return TRUE;
}

/**
 * Upgrade from V291 to V292
 */
static BOOL H_UpgradeFromV291(int currVersion, int newVersion)
{
	CHK_EXEC(SQLQuery(_T("ALTER TABLE event_policy ADD rule_guid varchar(36)")));
   CHK_EXEC(GenerateGUID(_T("event_policy"), _T("rule_id"), _T("rule_guid"), NULL));
   CHK_EXEC(SetSchemaVersion(292));
   return TRUE;
}

/**
 * Upgrade from V290 to V291
 */
static BOOL H_UpgradeFromV290(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("UPDATE network_services SET service_type=7 WHERE service_type=6")));
   CHK_EXEC(SetSchemaVersion(291));
   return TRUE;
}

/**
 * Upgrade from V289 to V290
 */
static BOOL H_UpgradeFromV289(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE network_maps ADD filter $SQL:TEXT")));
   CHK_EXEC(SetSchemaVersion(290));
   return TRUE;
}

/**
 * Upgrade from V288 to V289
 */
static BOOL H_UpgradeFromV288(int currVersion, int newVersion)
{
   CHK_EXEC(DBDropColumn(g_hCoreDB, _T("dct_thresholds"), _T("current_state")));
   CHK_EXEC(SetSchemaVersion(289));
   return TRUE;
}

/**
 * Upgrade from V287 to V288
 */
static BOOL H_UpgradeFromV287(int currVersion, int newVersion)
{
   CHK_EXEC(CreateEventTemplate(EVENT_TABLE_THRESHOLD_ACTIVATED, _T("SYS_TABLE_THRESHOLD_ACTIVATED"), EVENT_SEVERITY_MINOR, EF_LOG, NULL,
      _T("Threshold activated on table \"%2\" row %4 (%5)"),
      _T("Generated when table threshold is activated.\r\n")
      _T("Parameters:\r\n")
      _T("   1) Table DCI name\r\n")
      _T("   2) Table DCI description\r\n")
      _T("   3) Table DCI ID\r\n")
      _T("   4) Table row\r\n")
      _T("   5) Instance")));

   CHK_EXEC(CreateEventTemplate(EVENT_TABLE_THRESHOLD_DEACTIVATED, _T("SYS_TABLE_THRESHOLD_DEACTIVATED"), EVENT_SEVERITY_NORMAL, EF_LOG, NULL,
      _T("Threshold deactivated on table \"%2\" row %4 (%5)"),
      _T("Generated when table threshold is deactivated.\r\n")
      _T("Parameters:\r\n")
      _T("   1) Table DCI name\r\n")
      _T("   2) Table DCI description\r\n")
      _T("   3) Table DCI ID\r\n")
      _T("   4) Table row\r\n")
      _T("   5) Instance")));

   CHK_EXEC(SetSchemaVersion(288));
   return TRUE;
}

/**
 * Upgrade from V286 to V287
 */
static BOOL H_UpgradeFromV286(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE dc_table_columns ADD sequence_number integer\n")
      _T("UPDATE dc_table_columns SET sequence_number=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetSchemaVersion(287));
   return TRUE;
}

/**
 * Upgrade from V285 to V286
 */
static BOOL H_UpgradeFromV285(int currVersion, int newVersion)
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE dct_thresholds (")
      _T("id integer not null,")
      _T("table_id integer not null,")
      _T("sequence_number integer not null,")
      _T("current_state char(1) not null,")
      _T("activation_event integer not null,")
      _T("deactivation_event integer not null,")
      _T("PRIMARY KEY(id))")));

   CHK_EXEC(CreateTable(
      _T("CREATE TABLE dct_threshold_conditions (")
      _T("threshold_id integer not null,")
      _T("group_id integer not null,")
      _T("sequence_number integer not null,")
      _T("column_name varchar(63) null,")
      _T("check_operation integer not null,")
      _T("check_value varchar(255) null,")
      _T("PRIMARY KEY(threshold_id,group_id,sequence_number))")));

   CHK_EXEC(SetSchemaVersion(286));
   return TRUE;
}

/**
 * Upgrade from V284 to V285
 */
static BOOL H_UpgradeFromV284(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_items_node_id ON items(node_id)")));
   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_dc_tables_node_id ON dc_tables(node_id)")));

   CHK_EXEC(SetSchemaVersion(285));
   return TRUE;
}

/**
 * Upgrade from V283 to V284
 */
static BOOL H_UpgradeFromV283(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("SNMPTrapPort"), _T("162"), 1, 1));

   CHK_EXEC(SetSchemaVersion(284));
   return TRUE;
}

/**
 * Upgrade from V282 to V283
 */
static BOOL H_UpgradeFromV282(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE dc_table_columns ADD display_name varchar(255)\n")
      _T("UPDATE dc_table_columns SET display_name=column_name\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetSchemaVersion(283));
   return TRUE;
}

/**
 * Upgrade from V281 to V282
 */
static BOOL H_UpgradeFromV281(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='WindowsConsoleUpgradeURL'")));
   CHK_EXEC(CreateConfigParam(_T("EnableObjectTransactions"), _T("0"), 1, 1));

   CHK_EXEC(SetSchemaVersion(282));
   return TRUE;
}

/**
 * Upgrade from V280 to V281
 */
static BOOL H_UpgradeFromV280(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("DELETE FROM metadata WHERE var_name='TDataTableCreationCommand'\n")
      _T("INSERT INTO metadata (var_name,var_value)")
      _T("   VALUES ('TDataTableCreationCommand_0','CREATE TABLE tdata_%d (item_id integer not null,tdata_timestamp integer not null,record_id $SQL:INT64 not null)')\n")
      _T("INSERT INTO metadata (var_name,var_value)")
	   _T("   VALUES ('TDataTableCreationCommand_1','CREATE TABLE tdata_records_%d (record_id $SQL:INT64 not null,row_id $SQL:INT64 not null,instance varchar(255) null,PRIMARY KEY(record_id,row_id))')\n")
      _T("INSERT INTO metadata (var_name,var_value)")
	   _T("   VALUES ('TDataTableCreationCommand_2','CREATE TABLE tdata_rows_%d (row_id $SQL:INT64 not null,column_id integer not null,value varchar(255) null,PRIMARY KEY(row_id,column_id))')\n")
      _T("INSERT INTO metadata (var_name,var_value)")
	   _T("   VALUES ('TDataIndexCreationCommand_1','CREATE INDEX idx_tdata_rec_%d_instance ON tdata_records_%d(instance)')\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   RecreateTData(_T("nodes"), false, false);
   RecreateTData(_T("clusters"), false, false);
   RecreateTData(_T("mobile_devices"), false, false);

   CHK_EXEC(SetSchemaVersion(281));
   return TRUE;
}

/**
 * Upgrade from V279 to V280
 */
static BOOL H_UpgradeFromV279(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE dc_table_columns ADD flags integer\n")
      _T("UPDATE dc_table_columns SET flags=data_type\n")
      _T("ALTER TABLE dc_table_columns DROP COLUMN data_type\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   DB_RESULT hResult = SQLSelect(_T("SELECT item_id,instance_column FROM dc_tables"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         TCHAR columnName[MAX_COLUMN_NAME] = _T("");
         DBGetField(hResult, i, 1, columnName, MAX_COLUMN_NAME);
         if (columnName[0] != 0)
         {
            TCHAR query[256];
            _sntprintf(query, 256, _T("UPDATE dc_table_columns SET flags=flags+256 WHERE table_id=%d AND column_name=%s"),
               DBGetFieldLong(hResult, i, 0), (const TCHAR *)DBPrepareString(g_hCoreDB, columnName));
            CHK_EXEC(SQLQuery(query));
         }
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return FALSE;
   }

   CHK_EXEC(DBDropColumn(g_hCoreDB, _T("dc_tables"), _T("instance_column")));

   CHK_EXEC(SetSchemaVersion(280));
   return TRUE;
}

/**
 * Upgrade from V278 to V279
 */
static BOOL H_UpgradeFromV278(int currVersion, int newVersion)
{
   CHK_EXEC(CreateConfigParam(_T("DeleteEventsOfDeletedObject"), _T("1"), 1, 0));
   CHK_EXEC(CreateConfigParam(_T("DeleteAlarmsOfDeletedObject"), _T("1"), 1, 0));

   CHK_EXEC(SetSchemaVersion(279));
   return TRUE;
}

/**
 * Upgrade from V277 to V278
 */
static BOOL H_UpgradeFromV277(int currVersion, int newVersion)
{
   DB_RESULT hResult = SQLSelect(_T("SELECT id FROM clusters"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         DWORD id = DBGetFieldULong(hResult, i, 0);
         if (!CreateIDataTable(id))
         {
            if (!g_bIgnoreErrors)
            {
               DBFreeResult(hResult);
               return FALSE;
            }
         }
         if (!CreateTDataTable_preV281(id))
         {
            if (!g_bIgnoreErrors)
            {
               DBFreeResult(hResult);
               return FALSE;
            }
         }
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return FALSE;
   }

	CHK_EXEC(SetSchemaVersion(278));
   return TRUE;
}

/**
 * Upgrade from V276 to V277
 */
static BOOL H_UpgradeFromV276(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE dci_summary_tables (")
	                     _T("id integer not null,")
	                     _T("menu_path varchar(255) not null,")
	                     _T("title varchar(127) null,")
                        _T("node_filter $SQL:TEXT null,")
	                     _T("flags integer not null,")
                        _T("columns $SQL:TEXT null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateConfigParam(_T("DefaultMapBackgroundColor"), _T("0xffffff"), 1, 0));

	CHK_EXEC(SetSchemaVersion(277));
   return TRUE;
}

/**
 * Upgrade from V275 to V276
 */
static BOOL H_UpgradeFromV275(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE dc_table_columns DROP COLUMN transformation_script\n")
      _T("ALTER TABLE dc_tables ADD transformation_script $SQL:TEXT\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(276));
   return TRUE;
}

/**
 * Upgrade from V274 to V275
 */
static BOOL H_UpgradeFromV274(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE nodes ADD rack_image varchar(36)\n")
      _T("ALTER TABLE nodes ADD rack_position integer\n")
      _T("ALTER TABLE nodes ADD rack_id integer\n")
      _T("UPDATE nodes SET rack_image='00000000-0000-0000-0000-000000000000',rack_position=0,rack_id=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(CreateTable(_T("CREATE TABLE access_points (")
	                     _T("id integer not null,")
								_T("node_id integer not null,")
	                     _T("mac_address varchar(12) null,")
	                     _T("vendor varchar(64) null,")
	                     _T("model varchar(128) null,")
	                     _T("serial_number varchar(64) null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE racks (")
	                     _T("id integer not null,")
								_T("height integer not null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(SetSchemaVersion(275));
   return TRUE;
}

/**
 * Upgrade from V273 to V274
 */
static BOOL H_UpgradeFromV273(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE items ADD samples integer\n")
      _T("UPDATE items SET samples=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(274));
   return TRUE;
}

/**
 * Upgrade from V272 to V273
 */
static BOOL H_UpgradeFromV272(int currVersion, int newVersion)
{
	CHK_EXEC(CreateConfigParam(_T("DefaultDCIRetentionTime"), _T("30"), 1, 0));
	CHK_EXEC(CreateConfigParam(_T("DefaultDCIPollingInterval"), _T("60"), 1, 0));
   CHK_EXEC(SetSchemaVersion(273));
   return TRUE;
}

/**
 * Upgrade from V271 to V272
 */
static BOOL H_UpgradeFromV271(int currVersion, int newVersion)
{
	CHK_EXEC(CreateConfigParam(_T("SNMPTrapLogRetentionTime"), _T("90"), 1, 0));
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD driver_name varchar(32)\n")));
   CHK_EXEC(SetSchemaVersion(272));
   return TRUE;
}

/**
 * Upgrade from V270 to V271
 */
static BOOL H_UpgradeFromV270(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE object_properties ADD location_accuracy integer\n")
      _T("ALTER TABLE object_properties ADD location_timestamp integer\n")
      _T("UPDATE object_properties SET location_accuracy=0,location_timestamp=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetSchemaVersion(271));
   return TRUE;
}

/**
 * Upgrade from V269 to V270
 */
static BOOL H_UpgradeFromV269(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE items ADD instd_method integer\n")
		_T("ALTER TABLE items ADD instd_data varchar(255)\n")
		_T("ALTER TABLE items ADD instd_filter $SQL:TEXT\n")
		_T("UPDATE items SET instd_method=0\n")
		_T("<END>");
	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(270));
	return TRUE;
}

/**
 * Upgrade from V268 to V269
 */
static BOOL H_UpgradeFromV268(int currVersion, int newVersion)
{
	CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("alarms"), _T("message"), 2000, true));
	CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("alarm_events"), _T("message"), 2000, true));
	CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("event_log"), _T("event_message"), 2000, true));
	CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("event_cfg"), _T("message"), 2000, true));
	CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("event_policy"), _T("alarm_message"), 2000, true));
	CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("items"), _T("name"), 1024, true));
	CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("dc_tables"), _T("name"), 1024, true));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("event_policy"), _T("alarm_key")));
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("event_policy"), _T("alarm_message")));
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("event_policy"), _T("comments")));
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("event_policy"), _T("situation_instance")));
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("event_policy"), _T("script")));
	CHK_EXEC(ConvertStrings(_T("event_policy"), _T("rule_id"), _T("alarm_key")));
	CHK_EXEC(ConvertStrings(_T("event_policy"), _T("rule_id"), _T("alarm_message")));
	CHK_EXEC(ConvertStrings(_T("event_policy"), _T("rule_id"), _T("comments")));
	CHK_EXEC(ConvertStrings(_T("event_policy"), _T("rule_id"), _T("situation_instance")));
	CHK_EXEC(ConvertStrings(_T("event_policy"), _T("rule_id"), _T("script")));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("policy_situation_attr_list"), _T("attr_value")));
	// convert strings in policy_situation_attr_list
	DB_RESULT hResult = SQLSelect(_T("SELECT rule_id,situation_id,attr_name,attr_value FROM policy_situation_attr_list"));
	if (hResult != NULL)
	{
		if (SQLQuery(_T("DELETE FROM policy_situation_attr_list")))
		{
			TCHAR name[MAX_DB_STRING], value[MAX_DB_STRING], query[1024];
			int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
			{
				LONG ruleId = DBGetFieldLong(hResult, i, 0);
				LONG situationId = DBGetFieldLong(hResult, i, 1);
				DBGetField(hResult, i, 2, name, MAX_DB_STRING);
				DBGetField(hResult, i, 3, value, MAX_DB_STRING);

				DecodeSQLString(name);
				DecodeSQLString(value);

				if (name[0] == 0)
					_tcscpy(name, _T("noname"));

				_sntprintf(query, 1024, _T("INSERT INTO policy_situation_attr_list (rule_id,situation_id,attr_name,attr_value) VALUES (%d,%d,%s,%s)"),
				           ruleId, situationId, (const TCHAR *)DBPrepareString(g_hCoreDB, name), (const TCHAR *)DBPrepareString(g_hCoreDB, value));
				if (!SQLQuery(query))
				{
					if (!g_bIgnoreErrors)
					{
						DBFreeResult(hResult);
						return FALSE;
					}
				}
			}
		}
		else
		{
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

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("event_cfg"), _T("description")));
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("event_cfg"), _T("message")));
	CHK_EXEC(ConvertStrings(_T("event_cfg"), _T("event_code"), _T("description")));
	CHK_EXEC(ConvertStrings(_T("event_cfg"), _T("event_code"), _T("message")));

	CHK_EXEC(SetSchemaVersion(269));
	return TRUE;
}

/**
 * Upgrade from V267 to V268
 */
static BOOL H_UpgradeFromV267(int currVersion, int newVersion)
{
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("network_services"), _T("check_request")));
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("network_services"), _T("check_responce")));
	CHK_EXEC(ConvertStrings(_T("network_services"), _T("id"), _T("check_request")));
	CHK_EXEC(ConvertStrings(_T("network_services"), _T("id"), _T("check_responce")));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("config"), _T("var_value")));
	CHK_EXEC(ConvertStrings(_T("config"), _T("var_name"), NULL, _T("var_value"), true));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("dci_schedules"), _T("schedule")));
	CHK_EXEC(ConvertStrings(_T("dci_schedules"), _T("schedule_id"), _T("item_id"), _T("schedule"), false));

	CHK_EXEC(SetSchemaVersion(268));
	return TRUE;
}

/**
 * Upgrade from V266 to V267
 */
static BOOL H_UpgradeFromV266(int currVersion, int newVersion)
{
	CHK_EXEC(CreateEventTemplate(EVENT_NODE_UNREACHABLE, _T("SYS_NODE_UNREACHABLE"), EVENT_SEVERITY_CRITICAL,
	                             EF_LOG, NULL, _T("Node unreachable because of network failure"),
										  _T("Generated when node is unreachable by management server because of network failure.\r\nParameters:\r\n   No event-specific parameters")));
	CHK_EXEC(SetSchemaVersion(267));
	return TRUE;
}

/**
 * Upgrade from V265 to V266
 */
static BOOL H_UpgradeFromV265(int currVersion, int newVersion)
{
	// create index on root event ID in event log
	switch(g_dbSyntax)
	{
		case DB_SYNTAX_MSSQL:
		case DB_SYNTAX_PGSQL:
			CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_event_log_root_id ON event_log(root_event_id) WHERE root_event_id > 0")));
			break;
		case DB_SYNTAX_ORACLE:
			CHK_EXEC(SQLQuery(_T("CREATE OR REPLACE FUNCTION zero_to_null(id NUMBER) ")
			                  _T("RETURN NUMBER ")
			                  _T("DETERMINISTIC ")
			                  _T("AS BEGIN")
			                  _T("   IF id > 0 THEN")
			                  _T("      RETURN id;")
			                  _T("   ELSE")
			                  _T("      RETURN NULL;")
			                  _T("   END IF;")
			                  _T("END;")));
			CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_event_log_root_id ON event_log(zero_to_null(root_event_id))")));
			break;
		default:
			CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_event_log_root_id ON event_log(root_event_id)")));
			break;
	}

	CHK_EXEC(CreateTable(_T("CREATE TABLE mapping_tables (")
	                     _T("id integer not null,")
	                     _T("name varchar(63) not null,")
	                     _T("flags integer not null,")
								_T("description $SQL:TXT4K null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE mapping_data (")
	                     _T("table_id integer not null,")
	                     _T("md_key varchar(63) not null,")
	                     _T("md_value varchar(255) null,")
	                     _T("description $SQL:TXT4K null,")
	                     _T("PRIMARY KEY(table_id,md_key))")));

	CHK_EXEC(SQLQuery(_T("DROP TABLE deleted_objects")));

	CHK_EXEC(CreateConfigParam(_T("FirstFreeObjectId"), _T("100"), 0, 1, FALSE));

	CHK_EXEC(SetSchemaVersion(266));
	return TRUE;
}

/**
 * Upgrade from V264 to V265
 */
static BOOL H_UpgradeFromV264(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE alarm_events (")
	                     _T("alarm_id integer not null,")
								_T("event_id $SQL:INT64 not null,")
	                     _T("event_code integer not null,")
	                     _T("event_name varchar(63) null,")
	                     _T("severity integer not null,")
	                     _T("source_object_id integer not null,")
	                     _T("event_timestamp integer not null,")
	                     _T("message varchar(255) null,")
	                     _T("PRIMARY KEY(alarm_id,event_id))")));
	CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_alarm_events_alarm_id ON alarm_events(alarm_id)")));
	CHK_EXEC(SetSchemaVersion(265));
	return TRUE;
}

/**
 * Upgrade from V263 to V264
 */
static BOOL H_UpgradeFromV263(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE mobile_devices (")
	                     _T("id integer not null,")
								_T("device_id varchar(64) not null,")
								_T("vendor varchar(64) null,")
								_T("model varchar(128) null,")
								_T("serial_number varchar(64) null,")
								_T("os_name varchar(32) null,")
								_T("os_version varchar(64) null,")
								_T("user_id varchar(64) null,")
								_T("battery_level integer not null,")
	                     _T("PRIMARY KEY(id))")));
	CHK_EXEC(CreateConfigParam(_T("MobileDeviceListenerPort"), _T("4747"), 1, 1));
	CHK_EXEC(SetSchemaVersion(264));
	return TRUE;
}

/**
 * Upgrade from V262 to V263
 */
static BOOL H_UpgradeFromV262(int currVersion, int newVersion)
{
	CHK_EXEC(SQLQuery(_T("ALTER TABLE network_maps ADD radius integer")));
	CHK_EXEC(SQLQuery(_T("UPDATE network_maps SET radius=-1")));
	CHK_EXEC(SetSchemaVersion(263));
	return TRUE;
}

/**
 * Upgrade from V261 to V262
 */
static BOOL H_UpgradeFromV261(int currVersion, int newVersion)
{
	CHK_EXEC(CreateConfigParam(_T("ApplyDCIFromTemplateToDisabledDCI"), _T("0"), 1, 1));
	CHK_EXEC(SetSchemaVersion(262));
	return TRUE;
}

/**
 * Upgrade from V260 to V261
 */
static BOOL H_UpgradeFromV260(int currVersion, int newVersion)
{
	CHK_EXEC(CreateConfigParam(_T("NumberOfBusinessServicePollers"), _T("10"), 1, 1));
	CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='NumberOfEventProcessors'")));
	CHK_EXEC(SetSchemaVersion(261));
	return TRUE;
}

/**
 * Upgrade from V259 to V260
 */
static BOOL H_UpgradeFromV259(int currVersion, int newVersion)
{
	CHK_EXEC(CreateConfigParam(_T("UseFQDNForNodeNames"), _T("1"), 1, 1));
	CHK_EXEC(SetSchemaVersion(260));
	return TRUE;
}

/**
 * Upgrade from V258 to V259
 */
static BOOL H_UpgradeFromV258(int currVersion, int newVersion)
{
	// have to made these columns nullable again because
	// because they was forgotten as NOT NULL in schema.in
	// and so some databases can still have them as NOT NULL
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("templates"), _T("apply_filter")));
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("containers"), _T("auto_bind_filter")));

	CHK_EXEC(SetSchemaVersion(259));
	return TRUE;
}

/**
 * Upgrade from V257 to V258
 */
static BOOL H_UpgradeFromV257(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE nodes ADD down_since integer\n")
		_T("UPDATE nodes SET down_since=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(CreateConfigParam(_T("DeleteUnreachableNodesPeriod"), _T("0"), 1, 1));

	CHK_EXEC(SetSchemaVersion(258));
	return TRUE;
}

/**
 * Upgrade from V256 to V257
 */
static BOOL H_UpgradeFromV256(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE network_maps ADD bg_color integer\n")
		_T("ALTER TABLE network_maps ADD link_routing integer\n")
		_T("UPDATE network_maps SET bg_color=16777215,link_routing=1\n")
		_T("ALTER TABLE network_map_links ADD routing integer\n")
		_T("ALTER TABLE network_map_links ADD bend_points $SQL:TXT4K\n")
		_T("UPDATE network_map_links SET routing=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(257));
   return TRUE;
}

/**
 * Upgrade from V255 to V256
 */
static BOOL H_UpgradeFromV255(int currVersion, int newVersion)
{
	CHK_EXEC(CreateConfigParam(_T("DefaultConsoleDateFormat"), _T("dd.MM.yyyy"), 1, 0));
	CHK_EXEC(CreateConfigParam(_T("DefaultConsoleTimeFormat"), _T("HH:mm:ss"), 1, 0));

	CHK_EXEC(SetSchemaVersion(256));
   return TRUE;
}

/**
 * Upgrade from V254 to V255
 */
static BOOL H_UpgradeFromV254(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE alarms ADD resolved_by integer\n")
		_T("UPDATE alarms SET resolved_by=0\n")
		_T("UPDATE alarms SET alarm_state=3 WHERE alarm_state=2\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(255));
   return TRUE;
}

/**
 * Upgrade from V253 to V254
 */
static BOOL H_UpgradeFromV253(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE network_maps ADD flags integer\n")
		_T("ALTER TABLE network_maps ADD link_color integer\n")
		_T("UPDATE network_maps SET flags=1,link_color=-1\n")
		_T("ALTER TABLE network_map_links ADD color integer\n")
		_T("ALTER TABLE network_map_links ADD status_object integer\n")
		_T("UPDATE network_map_links SET color=-1,status_object=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(254));
   return TRUE;
}

/**
 * Upgrade from V252 to V253
 */
static BOOL H_UpgradeFromV252(int currVersion, int newVersion)
{
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("templates"), _T("apply_filter")));
	CHK_EXEC(ConvertStrings(_T("templates"), _T("id"), _T("apply_filter")));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("containers"), _T("auto_bind_filter")));
	CHK_EXEC(ConvertStrings(_T("containers"), _T("id"), _T("auto_bind_filter")));

	static TCHAR batch[] =
		_T("ALTER TABLE templates ADD flags integer\n")
		_T("UPDATE templates SET flags=0 WHERE enable_auto_apply=0\n")
		_T("UPDATE templates SET flags=3 WHERE enable_auto_apply<>0\n")
		_T("ALTER TABLE templates DROP COLUMN enable_auto_apply\n")
		_T("ALTER TABLE containers ADD flags integer\n")
		_T("UPDATE containers SET flags=0 WHERE enable_auto_bind=0\n")
		_T("UPDATE containers SET flags=3 WHERE enable_auto_bind<>0\n")
		_T("ALTER TABLE containers DROP COLUMN enable_auto_bind\n")
		_T("<END>");
	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(CreateEventTemplate(EVENT_CONTAINER_AUTOBIND, _T("SYS_CONTAINER_AUTOBIND"), EVENT_SEVERITY_NORMAL, EF_LOG, NULL,
	                             _T("Node %2 automatically bound to container %4"),
										  _T("Generated when node bound to container object by autobind rule.\r\n")
	                             _T("Parameters:#\r\n")
										  _T("   1) Node ID\r\n")
										  _T("   2) Node name\r\n")
										  _T("   3) Container ID\r\n")
										  _T("   4) Container name")
										  ));

	CHK_EXEC(CreateEventTemplate(EVENT_CONTAINER_AUTOUNBIND, _T("SYS_CONTAINER_AUTOUNBIND"), EVENT_SEVERITY_NORMAL, EF_LOG, NULL,
	                             _T("Node %2 automatically unbound from container %4"),
										  _T("Generated when node unbound from container object by autobind rule.\r\n")
	                             _T("Parameters:#\r\n")
										  _T("   1) Node ID\r\n")
										  _T("   2) Node name\r\n")
										  _T("   3) Container ID\r\n")
										  _T("   4) Container name")
										  ));

	CHK_EXEC(CreateEventTemplate(EVENT_TEMPLATE_AUTOAPPLY, _T("SYS_TEMPLATE_AUTOAPPLY"), EVENT_SEVERITY_NORMAL, EF_LOG, NULL,
	                             _T("Template %4 automatically applied to node %2"),
										  _T("Generated when template applied to node by autoapply rule.\r\n")
	                             _T("Parameters:#\r\n")
										  _T("   1) Node ID\r\n")
										  _T("   2) Node name\r\n")
										  _T("   3) Template ID\r\n")
										  _T("   4) Template name")
										  ));

	CHK_EXEC(CreateEventTemplate(EVENT_TEMPLATE_AUTOREMOVE, _T("SYS_TEMPLATE_AUTOREMOVE"), EVENT_SEVERITY_NORMAL, EF_LOG, NULL,
	                             _T("Template %4 automatically removed from node %2"),
										  _T("Generated when template removed from node by autoapply rule.\r\n")
	                             _T("Parameters:#\r\n")
										  _T("   1) Node ID\r\n")
										  _T("   2) Node name\r\n")
										  _T("   3) Template ID\r\n")
										  _T("   4) Template name")
										  ));

	TCHAR buffer[64];
	_sntprintf(buffer, 64, _T("%d"), ConfigReadInt(_T("AllowedCiphers"), 15) + 16);
	CreateConfigParam(_T("AllowedCiphers"), buffer, 1, 1, TRUE);

	CHK_EXEC(SetSchemaVersion(253));
   return TRUE;
}

/**
 * Upgrade from V251 to V252
 */
static BOOL H_UpgradeFromV251(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE interfaces ADD admin_state integer\n")
		_T("ALTER TABLE interfaces ADD oper_state integer\n")
		_T("UPDATE interfaces SET admin_state=0,oper_state=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(CreateEventTemplate(EVENT_INTERFACE_UNEXPECTED_UP, _T("SYS_IF_UNEXPECTED_UP"), EVENT_SEVERITY_MAJOR, EF_LOG, NULL,
	                             _T("Interface \"%2\" unexpectedly changed state to UP (IP Addr: %3/%4, IfIndex: %5)"),
										  _T("Generated when interface goes up but it's expected state set to DOWN.\r\n")
										  _T("Please note that source of event is node, not an interface itself.\r\n")
	                             _T("Parameters:#\r\n")
										  _T("   1) Interface object ID\r\n")
										  _T("   2) Interface name\r\n")
										  _T("   3) Interface IP address\r\n")
										  _T("   4) Interface netmask\r\n")
										  _T("   5) Interface index")
										  ));

	CHK_EXEC(CreateEventTemplate(EVENT_INTERFACE_EXPECTED_DOWN, _T("SYS_IF_EXPECTED_DOWN"), EVENT_SEVERITY_NORMAL, EF_LOG, NULL,
	                             _T("Interface \"%2\" with expected state DOWN changed state to DOWN (IP Addr: %3/%4, IfIndex: %5)"),
										  _T("Generated when interface goes down and it's expected state is DOWN.\r\n")
										  _T("Please note that source of event is node, not an interface itself.\r\n")
	                             _T("Parameters:#\r\n")
										  _T("   1) Interface object ID\r\n")
										  _T("   2) Interface name\r\n")
										  _T("   3) Interface IP address\r\n")
										  _T("   4) Interface netmask\r\n")
										  _T("   5) Interface index")
										  ));

	// Create rule pair in event processing policy
	int ruleId = 0;
	DB_RESULT hResult = SQLSelect(_T("SELECT max(rule_id) FROM event_policy"));
	if (hResult != NULL)
	{
		ruleId = DBGetFieldLong(hResult, 0, 0) + 1;
		DBFreeResult(hResult);
	}

	TCHAR query[1024];
	_sntprintf(query, 1024,
		_T("INSERT INTO event_policy (rule_id,flags,comments,alarm_message,alarm_severity,alarm_key,")
		_T("script,alarm_timeout,alarm_timeout_event,situation_id,situation_instance)	VALUES ")
		_T("(%d,7944,'Show alarm when interface is unexpectedly up','%%m',5,'IF_UNEXP_UP_%%i_%%1',")
		_T("'#00',0,%d,0,'#00')"), ruleId, EVENT_ALARM_TIMEOUT);
	CHK_EXEC(SQLQuery(query));
	_sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_INTERFACE_UNEXPECTED_UP);
	CHK_EXEC(SQLQuery(query));
	ruleId++;

	_sntprintf(query, 1024,
		_T("INSERT INTO event_policy (rule_id,flags,comments,alarm_message,alarm_severity,alarm_key,")
		_T("script,alarm_timeout,alarm_timeout_event,situation_id,situation_instance)	VALUES ")
		_T("(%d,7944,'Acknowlege interface unexpectedly up alarms when interface goes down','%%m',")
		_T("6,'IF_UNEXP_UP_%%i_%%1','#00',0,%d,0,'#00')"), ruleId, EVENT_ALARM_TIMEOUT);
	CHK_EXEC(SQLQuery(query));
	_sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_INTERFACE_EXPECTED_DOWN);
	CHK_EXEC(SQLQuery(query));

	CHK_EXEC(SetSchemaVersion(252));
   return TRUE;
}

/**
 * Upgrade from V250 to V251
 */
static BOOL H_UpgradeFromV250(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE thresholds ADD current_severity integer\n")
		_T("ALTER TABLE thresholds ADD last_event_timestamp integer\n")
		_T("UPDATE thresholds SET current_severity=0,last_event_timestamp=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("thresholds"), _T("fire_value")));
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("thresholds"), _T("rearm_value")));
	CHK_EXEC(ConvertStrings(_T("thresholds"), _T("threshold_id"), _T("fire_value")));
	CHK_EXEC(ConvertStrings(_T("thresholds"), _T("threshold_id"), _T("rearm_value")));

	CHK_EXEC(CreateConfigParam(_T("EnableNXSLContainerFunctions"), _T("1"), 1, 1));
	CHK_EXEC(CreateConfigParam(_T("UseDNSNameForDiscoveredNodes"), _T("0"), 1, 0));
	CHK_EXEC(CreateConfigParam(_T("AllowTrapVarbindsConversion"), _T("1"), 1, 1));

	CHK_EXEC(SetSchemaVersion(251));
   return TRUE;
}

/**
 * Upgrade from V249 to V250
 */
static BOOL H_UpgradeFromV249(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE licenses (")
	                     _T("id integer not null,")
								_T("content $SQL:TEXT null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(SetSchemaVersion(250));
   return TRUE;
}

/**
 * Upgrade from V248 to V249
 */
#define TDATA_CREATE_QUERY  _T("CREATE TABLE tdata_%d (item_id integer not null,tdata_timestamp integer not null,tdata_row integer not null,tdata_column integer not null,tdata_value varchar(255) null)")
#define TDATA_INDEX_MSSQL   _T("CREATE CLUSTERED INDEX idx_tdata_%d_id_timestamp ON tdata_%d(item_id,tdata_timestamp)")
#define TDATA_INDEX_PGSQL   _T("CREATE INDEX idx_tdata_%d_timestamp_id ON tdata_%d(tdata_timestamp,item_id)")
#define TDATA_INDEX_DEFAULT _T("CREATE INDEX idx_tdata_%d_id_timestamp ON tdata_%d(item_id,tdata_timestamp)")

static BOOL CreateTData(DWORD nodeId)
{
	TCHAR query[256];

	_sntprintf(query, 256, TDATA_CREATE_QUERY, (int)nodeId);
	CHK_EXEC(SQLQuery(query));

	switch(g_dbSyntax)
	{
		case DB_SYNTAX_MSSQL:
			_sntprintf(query, 256, TDATA_INDEX_MSSQL, (int)nodeId, (int)nodeId);
			break;
		case DB_SYNTAX_PGSQL:
			_sntprintf(query, 256, TDATA_INDEX_PGSQL, (int)nodeId, (int)nodeId);
			break;
		default:
			_sntprintf(query, 256, TDATA_INDEX_DEFAULT, (int)nodeId, (int)nodeId);
			break;
	}
	CHK_EXEC(SQLQuery(query));

	return TRUE;
}

static BOOL H_UpgradeFromV248(int currVersion, int newVersion)
{
	CHK_EXEC(SQLQuery(_T("INSERT INTO metadata (var_name,var_value) VALUES ('TDataTableCreationCommand','") TDATA_CREATE_QUERY _T("')")));

	switch(g_dbSyntax)
	{
		case DB_SYNTAX_MSSQL:
			CHK_EXEC(SQLQuery(_T("INSERT INTO metadata (var_name,var_value) VALUES ('TDataIndexCreationCommand_0','") TDATA_INDEX_MSSQL _T("')")));
			break;
		case DB_SYNTAX_PGSQL:
			CHK_EXEC(SQLQuery(_T("INSERT INTO metadata (var_name,var_value) VALUES ('TDataIndexCreationCommand_0','") TDATA_INDEX_PGSQL _T("')")));
			break;
		default:
			CHK_EXEC(SQLQuery(_T("INSERT INTO metadata (var_name,var_value) VALUES ('TDataIndexCreationCommand_0','") TDATA_INDEX_DEFAULT _T("')")));
			break;
	}

	CHK_EXEC(CreateTable(_T("CREATE TABLE dct_column_names (")
								_T("column_id integer not null,")
								_T("column_name varchar(63) not null,")
	                     _T("PRIMARY KEY(column_id))")));

	DB_RESULT hResult = SQLSelect(_T("SELECT id FROM nodes"));
	if (hResult != NULL)
	{
		int count = DBGetNumRows(hResult);
		for(int i = 0 ; i < count; i++)
		{
			if (!CreateTData(DBGetFieldULong(hResult, i, 0)))
			{
				if (!g_bIgnoreErrors)
				{
					DBFreeResult(hResult);
					return FALSE;
				}
			}
		}
		DBFreeResult(hResult);
	}
	else
	{
		if (!g_bIgnoreErrors)
			return FALSE;
	}

	CHK_EXEC(SetSchemaVersion(249));
   return TRUE;
}

/**
 * Upgrade from V247 to V248
 */
static BOOL H_UpgradeFromV247(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE dc_tables (")
	                     _T("item_id integer not null,")
								_T("node_id integer not null,")
								_T("template_id integer not null,")
								_T("template_item_id integer not null,")
								_T("name varchar(255) null,")
								_T("instance_column varchar(63) null,")
								_T("description varchar(255) null,")
								_T("flags integer not null,")
								_T("source integer not null,")
								_T("snmp_port integer not null,")
								_T("polling_interval integer not null,")
								_T("retention_time integer not null,")
								_T("status integer not null,")
								_T("system_tag varchar(255) null,")
								_T("resource_id integer not null,")
								_T("proxy_node integer not null,")
								_T("perftab_settings $SQL:TEXT null,")
	                     _T("PRIMARY KEY(item_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE dc_table_columns (")
	                     _T("table_id integer not null,")
	                     _T("column_name varchar(63) not null,")
								_T("snmp_oid varchar(1023) null,")
								_T("data_type integer not null,")
								_T("transformation_script $SQL:TEXT null,")
	                     _T("PRIMARY KEY(table_id,column_name))")));

	CHK_EXEC(SetSchemaVersion(248));
   return TRUE;
}

/**
 * Upgrade from V246 to V247
 */
static BOOL H_UpgradeFromV246(int currVersion, int newVersion)
{
	static TCHAR insertQuery[] = _T("INSERT INTO object_custom_attributes (object_id,attr_name,attr_value) VALUES (?,?,?)");

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("object_custom_attributes"), _T("attr_value")));

	// Convert strings in object_custom_attributes table
	DB_RESULT hResult = SQLSelect(_T("SELECT object_id,attr_name,attr_value FROM object_custom_attributes"));
	if (hResult != NULL)
	{
		if (SQLQuery(_T("DELETE FROM object_custom_attributes")))
		{
			TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
			DB_STATEMENT hStmt = DBPrepareEx(g_hCoreDB, insertQuery, errorText);
			if (hStmt != NULL)
			{
				TCHAR name[128], *value;
				int count = DBGetNumRows(hResult);
				for(int i = 0; i < count; i++)
				{
					UINT32 id = DBGetFieldULong(hResult, i, 0);
					DBGetField(hResult, i, 1, name, 128);
					DecodeSQLString(name);
					value = DBGetField(hResult, i, 2, NULL, 0);
					DecodeSQLString(value);

					DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
					DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
					DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, value, DB_BIND_DYNAMIC);
					if (g_bTrace)
						ShowQuery(insertQuery);
					if (!DBExecuteEx(hStmt, errorText))
					{
						WriteToTerminalEx(_T("SQL query failed (%s):\n\x1b[33;1m%s\x1b[0m\n"), errorText, insertQuery);
						if (!g_bIgnoreErrors)
						{
							DBFreeStatement(hStmt);
							DBFreeResult(hResult);
							return FALSE;
						}
					}
				}
				DBFreeStatement(hStmt);
			}
			else
			{
				WriteToTerminalEx(_T("SQL query failed (%s):\n\x1b[33;1m%s\x1b[0m\n"), errorText, insertQuery);
				if (!g_bIgnoreErrors)
				{
					DBFreeResult(hResult);
					return FALSE;
				}
			}
		}
		else
		{
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

	CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_ocattr_oid ON object_custom_attributes(object_id)")));
	CHK_EXEC(CreateConfigParam(_T("AlarmHistoryRetentionTime"), _T("180"), 1, 0));

	CHK_EXEC(SetSchemaVersion(247));
   return TRUE;
}

/**
 * Upgrade from V245 to V246
 */
static BOOL H_UpgradeFromV245(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE snmp_trap_pmap ADD flags integer\n")
		_T("UPDATE snmp_trap_pmap SET flags=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("snmp_trap_pmap"), _T("description")));
	CHK_EXEC(ConvertStrings(_T("snmp_trap_pmap"), _T("trap_id"), _T("parameter"), _T("description"), false));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("cluster_resources"), _T("resource_name")));
	CHK_EXEC(ConvertStrings(_T("cluster_resources"), _T("cluster_id"), _T("resource_id"), _T("resource_name"), false));

	CHK_EXEC(SetSchemaVersion(246));
   return TRUE;
}

/**
 * Upgrade from V244 to V245
 */
static BOOL H_UpgradeFromV244(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE nodes ADD runtime_flags integer\n")
		_T("UPDATE nodes SET runtime_flags=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("actions"), _T("rcpt_addr")));
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("actions"), _T("email_subject")));
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("actions"), _T("action_data")));

	CHK_EXEC(ConvertStrings(_T("actions"), _T("action_id"), _T("rcpt_addr")));
	CHK_EXEC(ConvertStrings(_T("actions"), _T("action_id"), _T("email_subject")));
	CHK_EXEC(ConvertStrings(_T("actions"), _T("action_id"), _T("action_data")));

	CHK_EXEC(SetSchemaVersion(245));
   return TRUE;
}


//
// Upgrade from V243 to V244
//

static BOOL H_UpgradeFromV243(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE interfaces ADD dot1x_pae_state integer\n")
		_T("ALTER TABLE interfaces ADD dot1x_backend_state integer\n")
		_T("UPDATE interfaces SET dot1x_pae_state=0,dot1x_backend_state=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(CreateEventTemplate(EVENT_8021X_PAE_STATE_CHANGED, _T("SYS_8021X_PAE_STATE_CHANGED"),
		EVENT_SEVERITY_NORMAL, 1, NULL, _T("Port %6 PAE state changed from %4 to %2"),
		_T("Generated when switch port PAE state changed.\r\nParameters:\r\n")
		_T("   1) New PAE state code\r\n")
		_T("   2) New PAE state as text\r\n")
		_T("   3) Old PAE state code\r\n")
		_T("   4) Old PAE state as text\r\n")
		_T("   5) Interface index\r\n")
		_T("   6) Interface name")));

	CHK_EXEC(CreateEventTemplate(EVENT_8021X_BACKEND_STATE_CHANGED, _T("SYS_8021X_BACKEND_STATE_CHANGED"),
		EVENT_SEVERITY_NORMAL, 1, NULL, _T("Port %6 backend authentication state changed from %4 to %2"),
		_T("Generated when switch port backend authentication state changed.\r\nParameters:\r\n")
		_T("   1) New backend state code\r\n")
		_T("   2) New backend state as text\r\n")
		_T("   3) Old backend state code\r\n")
		_T("   4) Old backend state as text\r\n")
		_T("   5) Interface index\r\n")
		_T("   6) Interface name")));

	CHK_EXEC(CreateEventTemplate(EVENT_8021X_PAE_FORCE_UNAUTH, _T("SYS_8021X_PAE_FORCE_UNAUTH"),
		EVENT_SEVERITY_MAJOR, 1, NULL, _T("Port %2 switched to force unauthorize state"),
		_T("Generated when switch port PAE state changed to FORCE UNAUTHORIZE.\r\nParameters:\r\n")
		_T("   1) Interface index\r\n")
		_T("   2) Interface name")));

	CHK_EXEC(CreateEventTemplate(EVENT_8021X_AUTH_FAILED, _T("SYS_8021X_AUTH_FAILED"),
		EVENT_SEVERITY_MAJOR, 1, NULL, _T("802.1x authentication failed on port %2"),
		_T("Generated when switch port backend authentication state changed to FAIL.\r\nParameters:\r\n")
		_T("   1) Interface index\r\n")
		_T("   2) Interface name")));

	CHK_EXEC(CreateEventTemplate(EVENT_8021X_AUTH_TIMEOUT, _T("SYS_8021X_AUTH_TIMEOUT"),
		EVENT_SEVERITY_MAJOR, 1, NULL, _T("802.1x authentication time out on port %2"),
		_T("Generated when switch port backend authentication state changed to TIMEOUT.\r\nParameters:\r\n")
		_T("   1) Interface index\r\n")
		_T("   2) Interface name")));

	CHK_EXEC(SetSchemaVersion(244));
   return TRUE;
}


//
// Upgrade from V242 to V243
//

static BOOL H_UpgradeFromV242(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE items ADD snmp_raw_value_type integer\n")
		_T("UPDATE items SET snmp_raw_value_type=0\n")
		_T("ALTER TABLE items ADD flags integer\n")
		_T("UPDATE items SET flags=adv_schedule+(all_thresholds*2)\n")
		_T("ALTER TABLE items DROP COLUMN adv_schedule\n")
		_T("ALTER TABLE items DROP COLUMN all_thresholds\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));
	CHK_EXEC(SetSchemaVersion(243));
   return TRUE;
}


//
// Upgrade from V241 to V242
//

static BOOL H_UpgradeFromV241(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("DROP TABLE business_service_templates\n")
		_T("ALTER TABLE dashboards ADD options integer\n")
		_T("UPDATE dashboards SET options=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));
	CHK_EXEC(SetSchemaVersion(242));
   return TRUE;
}


//
// Upgrade from V240 to V241
//

static BOOL H_UpgradeFromV240(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE slm_checks ADD template_id integer\n")
		_T("ALTER TABLE slm_checks ADD current_ticket integer\n")
		_T("UPDATE slm_checks SET template_id=0,current_ticket=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));
	CHK_EXEC(SetSchemaVersion(241));
   return TRUE;
}


//
// Upgrade from V239 to V240
//

static BOOL H_UpgradeFromV239(int currVersion, int newVersion)
{
	CHK_EXEC(SQLQuery(_T("ALTER TABLE raw_dci_values ADD transformed_value varchar(255)")));
	CHK_EXEC(SetSchemaVersion(240));
   return TRUE;
}


//
// Upgrade from V238 to V239
//

static BOOL H_UpgradeFromV238(int currVersion, int newVersion)
{
	CHK_EXEC(SQLQuery(
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES ")
		_T("(56,'SYS_IP_ADDRESS_CHANGED',1,1,'Primary IP address changed from %2 to %1',")
		_T("'Generated when primary IP address changed (usually because of primary name change or DNS change).#0D#0A")
		_T("Parameters:#0D#0A   1) New IP address#0D#0A   2) Old IP address#0D#0A   3) Primary host name')")));

	CHK_EXEC(SetSchemaVersion(239));
   return TRUE;
}


//
// Upgrade from V232 to V238
//

static BOOL H_UpgradeFromV232toV238(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_checks (")
	                     _T("id integer not null,")
	                     _T("type integer not null,")
								_T("content $SQL:TEXT null,")
	                     _T("threshold_id integer not null,")
	                     _T("reason varchar(255) null,")
	                     _T("is_template integer not null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_tickets (")
	                     _T("ticket_id integer not null,")
	                     _T("service_id integer not null,")
	                     _T("check_id integer not null,")
	                     _T("create_timestamp integer not null,")
	                     _T("close_timestamp integer not null,")
	                     _T("reason varchar(255) null,")
	                     _T("PRIMARY KEY(ticket_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_service_history (")
	                     _T("record_id integer not null,")
	                     _T("service_id integer not null,")
	                     _T("change_timestamp integer not null,")
	                     _T("new_status integer not null,")
	                     _T("PRIMARY KEY(record_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE report_results (")
	                     _T("report_id integer not null,")
	                     _T("generated integer not null,")
	                     _T("job_id integer not null,")
	                     _T("PRIMARY KEY(report_id,job_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE reports (")
	                     _T("id integer not null,")
								_T("definition $SQL:TEXT null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE job_history (")
	                     _T("id integer not null,")
	                     _T("time_created integer not null,")
	                     _T("time_started integer not null,")
	                     _T("time_finished integer not null,")
	                     _T("job_type varchar(127) null,")
	                     _T("description varchar(255) null,")
								_T("additional_info varchar(255) null,")
	                     _T("node_id integer not null,")
	                     _T("user_id integer not null,")
	                     _T("status integer not null,")
	                     _T("failure_message varchar(255) null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE business_services (")
	                     _T("service_id integer not null,")
	                     _T("PRIMARY KEY(service_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE business_service_templates (")
	                     _T("service_id integer not null,")
	                     _T("template_id integer not null,")
	                     _T("PRIMARY KEY(service_id,template_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE node_links (")
	                     _T("nodelink_id integer not null,")
	                     _T("node_id integer not null,")
	                     _T("PRIMARY KEY(nodelink_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_agreements (")
	                     _T("agreement_id integer not null,")
	                     _T("service_id integer not null,")
	                     _T("org_id integer not null,")
	                     _T("uptime varchar(63) not null,")
	                     _T("period integer not null,")
	                     _T("start_date integer not null,")
	                     _T("notes varchar(255),")
	                     _T("PRIMARY KEY(agreement_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE organizations (")
	                     _T("id integer not null,")
	                     _T("parent_id integer not null,")
	                     _T("org_type integer not null,")
	                     _T("name varchar(63) not null,")
	                     _T("description varchar(255),")
	                     _T("manager integer not null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE persons (")
	                     _T("id integer not null,")
	                     _T("org_id integer not null,")
	                     _T("first_name varchar(63),")
	                     _T("last_name varchar(63),")
	                     _T("title varchar(255),")
	                     _T("status integer not null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateConfigParam(_T("JobHistoryRetentionTime"), _T("90"), 1, 0));

	CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET description=")
	                  _T("'Generated when threshold check is rearmed for specific data collection item.#0D#0A")
	                  _T("Parameters:#0D#0A")
	                  _T("   1) Parameter name#0D#0A")
	                  _T("   2) Item description#0D#0A")
	                  _T("   3) Data collection item ID#0D#0A")
	                  _T("   4) Instance#0D#0A")
	                  _T("   5) Threshold value#0D#0A")
	                  _T("   6) Actual value' WHERE event_code=18")));

	CHK_EXEC(SetSchemaVersion(238));
   return TRUE;
}


//
// Upgrade from V237 to V238
//

static BOOL H_UpgradeFromV237(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("DROP TABLE slm_check_templates\n")
		_T("DROP TABLE node_link_checks\n")
		_T("DROP TABLE slm_checks\n")
		_T("DROP TABLE slm_tickets\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_checks (")
	                     _T("id integer not null,")
	                     _T("type integer not null,")
								_T("content $SQL:TEXT null,")
	                     _T("threshold_id integer not null,")
	                     _T("reason varchar(255) null,")
	                     _T("is_template integer not null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_tickets (")
	                     _T("ticket_id integer not null,")
	                     _T("service_id integer not null,")
	                     _T("check_id integer not null,")
	                     _T("create_timestamp integer not null,")
	                     _T("close_timestamp integer not null,")
	                     _T("reason varchar(255) null,")
	                     _T("PRIMARY KEY(ticket_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_service_history (")
	                     _T("record_id integer not null,")
	                     _T("service_id integer not null,")
	                     _T("change_timestamp integer not null,")
	                     _T("new_status integer not null,")
	                     _T("PRIMARY KEY(record_id))")));

	CHK_EXEC(SetSchemaVersion(238));
   return TRUE;
}

/**
 * Upgrade from V236 to V237
 */
static BOOL H_UpgradeFromV236(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE business_services DROP COLUMN name\n")
		_T("ALTER TABLE business_services DROP COLUMN parent_id\n")
		_T("ALTER TABLE business_services DROP COLUMN status\n")
		_T("ALTER TABLE slm_checks DROP COLUMN name\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(237));
   return TRUE;
}


//
// Upgrade from V235 to V236
//

static BOOL H_UpgradeFromV235(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE report_results (")
	                     _T("report_id integer not null,")
	                     _T("generated integer not null,")
	                     _T("job_id integer not null,")
	                     _T("PRIMARY KEY(report_id,job_id))")));

	CHK_EXEC(SetSchemaVersion(236));
   return TRUE;
}


//
// Upgrade from V234 to V235
//

static BOOL H_UpgradeFromV234(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE reports (")
	                     _T("id integer not null,")
								_T("definition $SQL:TEXT null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(SetSchemaVersion(235));
   return TRUE;
}


//
// Upgrade from V233 to V234
//

static BOOL H_UpgradeFromV233(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE job_history (")
	                     _T("id integer not null,")
	                     _T("time_created integer not null,")
	                     _T("time_started integer not null,")
	                     _T("time_finished integer not null,")
	                     _T("job_type varchar(127) null,")
	                     _T("description varchar(255) null,")
								_T("additional_info varchar(255) null,")
	                     _T("node_id integer not null,")
	                     _T("user_id integer not null,")
	                     _T("status integer not null,")
	                     _T("failure_message varchar(255) null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateConfigParam(_T("JobHistoryRetentionTime"), _T("90"), 1, 0));

	CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET description=")
	                  _T("'Generated when threshold check is rearmed for specific data collection item.#0D#0A")
	                  _T("Parameters:#0D#0A")
	                  _T("   1) Parameter name#0D#0A")
	                  _T("   2) Item description#0D#0A")
	                  _T("   3) Data collection item ID#0D#0A")
	                  _T("   4) Instance#0D#0A")
	                  _T("   5) Threshold value#0D#0A")
	                  _T("   6) Actual value' WHERE event_code=18")));

	CHK_EXEC(SetSchemaVersion(234));
   return TRUE;
}

/**
 * Upgrade from V231 to V232
 */
static BOOL H_UpgradeFromV231(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE object_properties ADD submap_id integer\n")
		_T("UPDATE object_properties SET submap_id=0\n")
		_T("DROP TABLE maps\n")
		_T("DROP TABLE map_access_lists\n")
		_T("DROP TABLE submaps\n")
		_T("DROP TABLE submap_object_positions\n")
		_T("DROP TABLE submap_links\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(232));
   return TRUE;
}

/**
 * Upgrade from V230 to V231
 */
static BOOL H_UpgradeFromV230(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE nodes ADD bridge_base_addr varchar(15)\n")
		_T("UPDATE nodes SET bridge_base_addr='000000000000'\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(231));
   return TRUE;
}


//
// Upgrade from V229 to V230
//

static BOOL H_UpgradeFromV229(int currVersion, int newVersion)
{
	static TCHAR batch1[] =
		_T("ALTER TABLE network_maps ADD bg_latitude varchar(20)\n")
		_T("ALTER TABLE network_maps ADD bg_longitude varchar(20)\n")
		_T("ALTER TABLE network_maps ADD bg_zoom integer\n")
		_T("ALTER TABLE dashboard_elements ADD layout_data $SQL:TEXT\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch1));

	DB_RESULT hResult = SQLSelect(_T("SELECT dashboard_id,element_id,horizontal_span,vertical_span,horizontal_alignment,vertical_alignment FROM dashboard_elements"));
	if (hResult != NULL)
	{
		TCHAR query[1024], xml[1024];

		int count = DBGetNumRows(hResult);
		for(int i = 0; i < count; i++)
		{
			_sntprintf(xml, 1024, _T("<layout><horizontalSpan>%d</horizontalSpan><verticalSpan>%d</verticalSpan><horizontalAlignment>%d</horizontalAlignment><verticalAlignment>%d</verticalAlignment></layout>"),
			           (int)DBGetFieldLong(hResult, i, 2), (int)DBGetFieldLong(hResult, i, 3),
						  (int)DBGetFieldLong(hResult, i, 4), (int)DBGetFieldLong(hResult, i, 5));
			_sntprintf(query, 1024, _T("UPDATE dashboard_elements SET layout_data=%s WHERE dashboard_id=%d AND element_id=%d"),
			           (const TCHAR *)DBPrepareString(g_hCoreDB, xml), (int)DBGetFieldLong(hResult, i, 0), (int)DBGetFieldLong(hResult, i, 1));
			CHK_EXEC(SQLQuery(query));
		}
		DBFreeResult(hResult);
	}
	else
	{
		if (!g_bIgnoreErrors)
			return FALSE;
	}

	static TCHAR batch2[] =
		_T("ALTER TABLE dashboard_elements DROP COLUMN horizontal_span\n")
		_T("ALTER TABLE dashboard_elements DROP COLUMN vertical_span\n")
		_T("ALTER TABLE dashboard_elements DROP COLUMN horizontal_alignment\n")
		_T("ALTER TABLE dashboard_elements DROP COLUMN vertical_alignment\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch2));

	CreateConfigParam(_T("TileServerURL"), _T("http://tile.openstreetmap.org/"), 1, 0);

	CHK_EXEC(SetSchemaVersion(230));
   return TRUE;
}


//
// Upgrade from V228 to V229
//

static BOOL H_UpgradeFromV228(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE dashboards (")
				_T("   id integer not null,")
				_T("   num_columns integer not null,")
				_T("   PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE dashboard_elements (")
				_T("   dashboard_id integer not null,")
				_T("   element_id integer not null,")
				_T("   element_type integer not null,")
				_T("   element_data $SQL:TEXT null,")
				_T("   horizontal_span integer not null,")
				_T("   vertical_span integer not null,")
				_T("   horizontal_alignment integer not null,")
				_T("   vertical_alignment integer not null,")
				_T("   PRIMARY KEY(dashboard_id,element_id))")));

	CHK_EXEC(SetSchemaVersion(229));
   return TRUE;
}

/**
 * Upgrade from V227 to V228
 */
static BOOL H_UpgradeFromV227(int currVersion, int newVersion)
{
	CHK_EXEC(SQLQuery(_T("DROP TABLE web_maps")));
	CHK_EXEC(MigrateMaps());
	CHK_EXEC(SetSchemaVersion(228));
   return TRUE;
}

/**
 * Upgrade from V226 to V227
 */
static BOOL H_UpgradeFromV226(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE clusters ADD zone_guid integer\n")
		_T("UPDATE clusters SET zone_guid=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(227));
   return TRUE;
}


//
// Upgrade from V225 to V226
//

static BOOL H_UpgradeFromV225(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE interfaces ADD flags integer\n")
		_T("UPDATE interfaces SET flags=0\n")
		_T("UPDATE interfaces SET flags=1 WHERE synthetic_mask<>0\n")
		_T("ALTER TABLE interfaces DROP COLUMN synthetic_mask\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(226));
   return TRUE;
}


//
// Upgrade from V224 to V225
//

static BOOL H_UpgradeFromV224(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE interfaces ADD description varchar(255)\n")
		_T("UPDATE interfaces SET description=''\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(225));
   return TRUE;
}

/**
 * Upgrade from V223 to V224
 */
static BOOL H_UpgradeFromV223(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("DROP TABLE zone_ip_addr_list\n")
		_T("ALTER TABLE zones DROP COLUMN zone_type\n")
		_T("ALTER TABLE zones DROP COLUMN controller_ip\n")
		_T("ALTER TABLE zones ADD agent_proxy integer\n")
		_T("ALTER TABLE zones ADD snmp_proxy integer\n")
		_T("ALTER TABLE zones ADD icmp_proxy integer\n")
		_T("UPDATE zones SET agent_proxy=0,snmp_proxy=0,icmp_proxy=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(224));
   return TRUE;
}

/**
 * Upgrade from V222 to V223
 */
static BOOL H_UpgradeFromV222(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("DROP TABLE oid_to_type\n")
		_T("ALTER TABLE nodes DROP COLUMN node_type\n")
		_T("ALTER TABLE nodes ADD primary_name varchar(255)\n")
		_T("UPDATE nodes SET primary_name=primary_ip\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(223));
   return TRUE;
}


//
// Upgrade from V221 to V222
//

static BOOL H_UpgradeFromV221(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE object_properties ADD image varchar(36)\n")
		_T("UPDATE object_properties SET image='00000000-0000-0000-0000-000000000000'\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(222));
   return TRUE;
}


//
// Upgrade from V220 to V221
//

static BOOL H_UpgradeFromV220(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE network_maps DROP COLUMN background\n")
		_T("ALTER TABLE network_maps ADD background varchar(36)\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(221));
   return TRUE;
}


//
// Upgrade from V219 to V220
//

static BOOL H_UpgradeFromV219(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE interfaces ADD bridge_port integer\n")
		_T("ALTER TABLE interfaces ADD phy_slot integer\n")
		_T("ALTER TABLE interfaces ADD phy_port integer\n")
		_T("ALTER TABLE interfaces ADD peer_node_id integer\n")
		_T("ALTER TABLE interfaces ADD peer_if_id integer\n")
		_T("UPDATE interfaces SET bridge_port=0,phy_slot=0,phy_port=0,peer_node_id=0,peer_if_id=0\n")
		_T("ALTER TABLE nodes ADD snmp_sys_name varchar(127)\n")
		_T("UPDATE nodes SET snmp_sys_name=''\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(220));
   return TRUE;
}


//
// Upgrade from V218 to V219
//

static BOOL H_UpgradeFromV218(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE images (")
				_T("   guid varchar(36) not null,")
				_T("   mimetype varchar(64) not null,")
				_T("   name varchar(255) not null,")
				_T("   category varchar(255) not null,")
				_T("   protected integer default 0,")
				_T("   ")
				_T("   PRIMARY KEY(guid),")
				_T("   UNIQUE(name, category))")));

   static TCHAR batch[] =
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('1ddb76a3-a05f-4a42-acda-22021768feaf', 'image/png', 'ATM', 'Network Objects', 1)\n")
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('b314cf44-b2aa-478e-b23a-73bc5bb9a624', 'image/png', 'HSM', 'Network Objects', 1)\n")
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('904e7291-ee3f-41b7-8132-2bd29288ecc8', 'image/png', 'Node', 'Network Objects', 1)\n")
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('f5214d16-1ab1-4577-bb21-063cfd45d7af', 'image/png', 'Printer', 'Network Objects', 1)\n")
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('bacde727-b183-4e6c-8dca-ab024c88b999', 'image/png', 'Router', 'Network Objects', 1)\n")
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('ba6ab507-f62d-4b8f-824c-ca9d46f22375', 'image/png', 'Server', 'Network Objects', 1)\n")
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('092e4b35-4e7c-42df-b9b7-d5805bfac64e', 'image/png', 'Service', 'Network Objects', 1)\n")
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('f9105c54-8dcf-483a-b387-b4587dfd3cba', 'image/png', 'Switch', 'Network Objects', 1)\n")
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('7cd999e9-fbe0-45c3-a695-f84523b3a50c', 'image/png', 'Unknown', 'Network Objects', 1)\n")
      _T("<END>");

   CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetSchemaVersion(219));
   return TRUE;
}

/**
 * Upgrade from V217 to V218
 */
static BOOL H_UpgradeFromV217(int currVersion, int newVersion)
{
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("snmp_communities"), _T("community")));
	CHK_EXEC(ConvertStrings(_T("snmp_communities"), _T("id"), _T("community")));

	CHK_EXEC(SetSchemaVersion(218));
   return TRUE;
}

/**
 * Upgrade from V216 to V217
 */
static BOOL H_UpgradeFromV216(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE nodes ADD snmp_port integer\n")
		_T("UPDATE nodes SET snmp_port=161\n")
		_T("ALTER TABLE items ADD snmp_port integer\n")
		_T("UPDATE items SET snmp_port=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("nodes"), _T("community")));
	CHK_EXEC(ConvertStrings(_T("nodes"), _T("id"), _T("community")));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("nodes"), _T("usm_auth_password")));
	CHK_EXEC(ConvertStrings(_T("nodes"), _T("id"), _T("usm_auth_password")));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("nodes"), _T("usm_priv_password")));
	CHK_EXEC(ConvertStrings(_T("nodes"), _T("id"), _T("usm_priv_password")));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("nodes"), _T("snmp_oid")));
	CHK_EXEC(ConvertStrings(_T("nodes"), _T("id"), _T("snmp_oid")));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("nodes"), _T("secret")));
	CHK_EXEC(ConvertStrings(_T("nodes"), _T("id"), _T("secret")));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("nodes"), _T("agent_version")));
	CHK_EXEC(ConvertStrings(_T("nodes"), _T("id"), _T("agent_version")));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("nodes"), _T("platform_name")));
	CHK_EXEC(ConvertStrings(_T("nodes"), _T("id"), _T("platform_name")));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("nodes"), _T("uname")));
	CHK_EXEC(ConvertStrings(_T("nodes"), _T("id"), _T("uname")));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("items"), _T("name")));
	CHK_EXEC(ConvertStrings(_T("items"), _T("item_id"), _T("name")));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("items"), _T("description")));
	CHK_EXEC(ConvertStrings(_T("items"), _T("item_id"), _T("description")));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("items"), _T("transformation")));
	CHK_EXEC(ConvertStrings(_T("items"), _T("item_id"), _T("transformation")));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("items"), _T("instance")));
	CHK_EXEC(ConvertStrings(_T("items"), _T("item_id"), _T("instance")));

	CHK_EXEC(SetSchemaVersion(217));
   return TRUE;
}

/**
 * Upgrade from V215 to V216
 */
static BOOL H_UpgradeFromV215(int currVersion, int newVersion)
{
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("ap_common"), _T("description")));
	CHK_EXEC(ConvertStrings(_T("ap_common"), _T("id"), _T("description")));

	if (g_dbSyntax != DB_SYNTAX_SQLITE)
		CHK_EXEC(SQLQuery(_T("ALTER TABLE ap_config_files DROP COLUMN file_name")));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("ap_config_files"), _T("file_content")));
	CHK_EXEC(ConvertStrings(_T("ap_config_files"), _T("policy_id"), _T("file_content")));

	CHK_EXEC(SQLQuery(_T("ALTER TABLE object_properties ADD guid varchar(36)")));

	// Generate GUIDs for all objects
	DB_RESULT hResult = SQLSelect(_T("SELECT object_id FROM object_properties"));
	if (hResult != NULL)
	{
		int count = DBGetNumRows(hResult);
		for(int i = 0; i < count; i++)
		{
			uuid_t guid;
			TCHAR query[256], buffer[64];

			_uuid_generate(guid);
			_sntprintf(query, 256, _T("UPDATE object_properties SET guid='%s' WHERE object_id=%d"),
			           _uuid_to_string(guid, buffer), DBGetFieldULong(hResult, i, 0));
			CHK_EXEC(SQLQuery(query));
		}
		DBFreeResult(hResult);
	}

	CHK_EXEC(SetSchemaVersion(216));
   return TRUE;
}


//
// Upgrade from V214 to V215
//

static BOOL H_UpgradeFromV214(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE network_maps (")
		                  _T("id integer not null,")
						      _T("map_type integer not null,")
						      _T("layout integer not null,")
						      _T("seed integer not null,")
						      _T("background integer not null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE network_map_elements (")
		                  _T("map_id integer not null,")
		                  _T("element_id integer not null,")
						      _T("element_type integer not null,")
								_T("element_data $SQL:TEXT not null,")
	                     _T("PRIMARY KEY(map_id,element_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE network_map_links (")
		                  _T("map_id integer not null,")
		                  _T("element1 integer not null,")
		                  _T("element2 integer not null,")
						      _T("link_type integer not null,")
								_T("link_name varchar(255) null,")
								_T("connector_name1 varchar(255) null,")
								_T("connector_name2 varchar(255) null,")
	                     _T("PRIMARY KEY(map_id,element1,element2))")));

	CHK_EXEC(SetSchemaVersion(215));
   return TRUE;
}

/**
 * Upgrade from V213 to V214
 */
static BOOL H_UpgradeFromV213(int currVersion, int newVersion)
{
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("script_library"), _T("script_code")));
	CHK_EXEC(ConvertStrings(_T("script_library"), _T("script_id"), _T("script_code")));

	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("raw_dci_values"), _T("raw_value")));
	CHK_EXEC(ConvertStrings(_T("raw_dci_values"), _T("item_id"), _T("raw_value")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='CREATE TABLE idata_%d (item_id integer not null,idata_timestamp integer not null,idata_value varchar(255) null)' WHERE var_name='IDataTableCreationCommand'")));

	DB_RESULT hResult = SQLSelect(_T("SELECT id FROM nodes"));
	if (hResult != NULL)
	{
		int count = DBGetNumRows(hResult);
		for(int i = 0; i < count; i++)
		{
			TCHAR table[32];

			DWORD nodeId = DBGetFieldULong(hResult, i, 0);
			_sntprintf(table, 32, _T("idata_%d"), nodeId);
			CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, table, _T("idata_value")));
		}
		DBFreeResult(hResult);
	}

	// Convert values for string DCIs from # encoded to normal form
	hResult = SQLSelect(_T("SELECT node_id,item_id FROM items WHERE datatype=4"));
	if (hResult != NULL)
	{
		int count = DBGetNumRows(hResult);
		for(int i = 0; i < count; i++)
		{
			TCHAR query[512];

			DWORD nodeId = DBGetFieldULong(hResult, i, 0);
			DWORD dciId = DBGetFieldULong(hResult, i, 1);

			if (IsDatabaseRecordExist(_T("nodes"), _T("id"), nodeId))
			{
				_sntprintf(query, 512, _T("SELECT idata_timestamp,idata_value FROM idata_%d WHERE item_id=%d AND idata_value LIKE '%%#%%'"), nodeId, dciId);
				DB_RESULT hData = SQLSelect(query);
				if (hData != NULL)
				{
					int valueCount = DBGetNumRows(hData);
					for(int j = 0; j < valueCount; j++)
					{
						TCHAR buffer[MAX_DB_STRING];

						LONG ts = DBGetFieldLong(hData, j, 0);
						DBGetField(hData, j, 1, buffer, MAX_DB_STRING);
						DecodeSQLString(buffer);

						_sntprintf(query, 512, _T("UPDATE idata_%d SET idata_value=%s WHERE item_id=%d AND idata_timestamp=%ld"),
									  nodeId, (const TCHAR *)DBPrepareString(g_hCoreDB, buffer), dciId, (long)ts);
						CHK_EXEC(SQLQuery(query));
					}
					DBFreeResult(hData);
				}
			}
		}
		DBFreeResult(hResult);
	}

	CHK_EXEC(SetSchemaVersion(214));
   return TRUE;
}

/**
 * Upgrade from V212 to V213
 */
static BOOL H_UpgradeFromV212(int currVersion, int newVersion)
{
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("items"), _T("custom_units_name")));
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("items"), _T("perftab_settings")));

	CHK_EXEC(ConvertStrings(_T("items"), _T("item_id"), _T("custom_units_name")));
	CHK_EXEC(ConvertStrings(_T("items"), _T("item_id"), _T("perftab_settings")));

	CHK_EXEC(SetSchemaVersion(213));

   return TRUE;
}

/**
 * Upgrade from V211 to V212
 */
static BOOL H_UpgradeFromV211(int currVersion, int newVersion)
{
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("snmp_trap_cfg"), _T("snmp_oid")));
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("snmp_trap_cfg"), _T("user_tag")));
	CHK_EXEC(DBRemoveNotNullConstraint(g_hCoreDB, _T("snmp_trap_cfg"), _T("description")));

	CHK_EXEC(ConvertStrings(_T("snmp_trap_cfg"), _T("trap_id"), _T("user_tag")));
	CHK_EXEC(ConvertStrings(_T("snmp_trap_cfg"), _T("trap_id"), _T("description")));

	CHK_EXEC(SetSchemaVersion(212));

   return TRUE;
}

/**
 * Upgrade from V210 to V211
 */
static BOOL H_UpgradeFromV210(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES")
			_T(" (53,'SYS_DCI_UNSUPPORTED',2,1,'Status of DCI %1 (%5: %2) changed to UNSUPPORTED',")
			_T("'Generated when DCI status changed to UNSUPPORTED.#0D#0AParameters:#0D#0A")
			_T("   1) DCI ID#0D#0A   2) DCI Name#0D#0A   3) DCI Description#0D#0A   4) DCI Origin code#0D#0A   5) DCI Origin name')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES")
			_T(" (54,'SYS_DCI_DISABLED',1,1,'Status of DCI %1 (%5: %2) changed to DISABLED',")
			_T("'Generated when DCI status changed to DISABLED.#0D#0AParameters:#0D#0A")
			_T("   1) DCI ID#0D#0A   2) DCI Name#0D#0A   3) DCI Description#0D#0A   4) DCI Origin code#0D#0A   5) DCI Origin name')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES")
			_T(" (55,'SYS_DCI_ACTIVE',0,1,'Status of DCI %1 (%5: %2) changed to ACTIVE',")
			_T("'Generated when DCI status changed to ACTIVE.#0D#0AParameters:#0D#0A")
			_T("   1) DCI ID#0D#0A   2) DCI Name#0D#0A   3) DCI Description#0D#0A   4) DCI Origin code#0D#0A   5) DCI Origin name')\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));
	CHK_EXEC(SetSchemaVersion(211));

   return TRUE;
}

/**
 * Upgrade from V209 to V210
 */
static BOOL H_UpgradeFromV209(int currVersion, int newVersion)
{
	if (!SQLQuery(_T("DELETE FROM metadata WHERE var_name like 'IDataIndexCreationCommand_%'")))
		if (!g_bIgnoreErrors)
			return FALSE;

	const TCHAR *query;
	switch(g_dbSyntax)
	{
		case DB_SYNTAX_PGSQL:
			query = _T("INSERT INTO metadata (var_name,var_value)	VALUES ('IDataIndexCreationCommand_0','CREATE INDEX idx_idata_%d_timestamp_id ON idata_%d(idata_timestamp,item_id)')");
			break;
		case DB_SYNTAX_MSSQL:
			query = _T("INSERT INTO metadata (var_name,var_value)	VALUES ('IDataIndexCreationCommand_0','CREATE CLUSTERED INDEX idx_idata_%d_id_timestamp ON idata_%d(item_id,idata_timestamp)')");
			break;
		default:
			query = _T("INSERT INTO metadata (var_name,var_value)	VALUES ('IDataIndexCreationCommand_0','CREATE INDEX idx_idata_%d_id_timestamp ON idata_%d(item_id,idata_timestamp)')");
			break;
	}

	if (!SQLQuery(query))
		if (!g_bIgnoreErrors)
			return FALSE;

	ReindexIData();

	if (!SQLQuery(_T("UPDATE metadata SET var_value='210' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}

/**
 * Upgrade from V208 to V209
 */
static BOOL H_UpgradeFromV208(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE users ADD auth_failures integer\n")
		_T("ALTER TABLE users ADD last_passwd_change integer\n")
		_T("ALTER TABLE users ADD min_passwd_length integer\n")
		_T("ALTER TABLE users ADD disabled_until integer\n")
		_T("ALTER TABLE users ADD last_login integer\n")
		_T("ALTER TABLE users ADD password_history $SQL:TEXT\n")
		_T("UPDATE users SET auth_failures=0,last_passwd_change=0,min_passwd_length=-1,disabled_until=0,last_login=0\n")
		_T("<END>");

	if (!SQLBatch(batch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("PasswordHistoryLength"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("IntruderLockoutThreshold"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("IntruderLockoutTime"), _T("30"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("MinPasswordLength"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("PasswordComplexity"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("PasswordExpiration"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("BlockInactiveUserAccounts"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='209' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V207 to V208
//

static BOOL H_UpgradeFromV207(int currVersion, int newVersion)
{
	if (!SQLQuery(_T("ALTER TABLE items ADD system_tag varchar(255)")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='208' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V206 to V207
//

static BOOL H_UpgradeFromV206(int currVersion, int newVersion)
{
	if (!CreateConfigParam(_T("RADIUSSecondaryServer"), _T("none"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("RADIUSSecondarySecret"), _T("netxms"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("RADIUSSecondaryPort"), _T("1645"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ExternalAuditServer"), _T("none"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ExternalAuditPort"), _T("514"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ExternalAuditFacility"), _T("13"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ExternalAuditSeverity"), _T("5"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ExternalAuditTag"), _T("netxmsd-audit"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='207' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V205 to V206
//

static BOOL H_UpgradeFromV205(int currVersion, int newVersion)
{
	if (g_dbSyntax == DB_SYNTAX_ORACLE)
	{
		static TCHAR oraBatch[] =
			_T("ALTER TABLE audit_log MODIFY message null\n")
			_T("ALTER TABLE event_log MODIFY event_message null\n")
			_T("ALTER TABLE event_log MODIFY user_tag null\n")
			_T("ALTER TABLE syslog MODIFY hostname null\n")
			_T("ALTER TABLE syslog MODIFY msg_tag null\n")
			_T("ALTER TABLE syslog MODIFY msg_text null\n")
			_T("ALTER TABLE snmp_trap_log MODIFY trap_varlist null\n")
			_T("<END>");

		if (!SQLBatch(oraBatch))
			if (!g_bIgnoreErrors)
				return FALSE;
	}

	bool clearLogs = GetYesNo(_T("This database upgrade requires log conversion. This can take significant amount of time ")
	                          _T("(up to few hours for large databases). If preserving all log records is not very important, it is ")
	                          _T("recommended to clear logs befor conversion. Clear logs?"));

	if (clearLogs)
	{
		if (!SQLQuery(_T("DELETE FROM audit_log")))
			if (!g_bIgnoreErrors)
				return FALSE;

		if (!SQLQuery(_T("DELETE FROM event_log")))
			if (!g_bIgnoreErrors)
				return FALSE;

		if (!SQLQuery(_T("DELETE FROM syslog")))
			if (!g_bIgnoreErrors)
				return FALSE;

		if (!SQLQuery(_T("DELETE FROM snmp_trap_log")))
			if (!g_bIgnoreErrors)
				return FALSE;
	}
	else
	{
		// Convert event log
		if (!ConvertStrings(_T("event_log"), _T("event_id"), _T("event_message")))
			if (!g_bIgnoreErrors)
				return FALSE;
		if (!ConvertStrings(_T("event_log"), _T("event_id"), _T("user_tag")))
			if (!g_bIgnoreErrors)
				return FALSE;

		// Convert audit log
		if (!ConvertStrings(_T("audit_log"), _T("record_id"), _T("message")))
			if (!g_bIgnoreErrors)
				return FALSE;

		// Convert syslog
		if (!ConvertStrings(_T("syslog"), _T("msg_id"), _T("msg_text")))
			if (!g_bIgnoreErrors)
				return FALSE;

		// Convert SNMP trap log
		if (!ConvertStrings(_T("snmp_trap_log"), _T("trap_id"), _T("trap_varlist")))
			if (!g_bIgnoreErrors)
				return FALSE;
	}

	if (!SQLQuery(_T("UPDATE metadata SET var_value='206' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V204 to V205
//

static BOOL H_UpgradeFromV204(int currVersion, int newVersion)
{
	if (!CreateTable(_T("CREATE TABLE usm_credentials (")
		              _T("id integer not null,")
	                 _T("user_name varchar(255) not null,")
						  _T("auth_method integer not null,")
						  _T("priv_method integer not null,")
						  _T("auth_password varchar(255),")
						  _T("priv_password varchar(255),")
						  _T("PRIMARY KEY(id))")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='205' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V203 to V204
//

static BOOL H_UpgradeFromV203(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE object_properties ADD location_type integer\n")
		_T("ALTER TABLE object_properties ADD latitude varchar(20)\n")
		_T("ALTER TABLE object_properties ADD longitude varchar(20)\n")
		_T("UPDATE object_properties SET location_type=0\n")
		_T("ALTER TABLE object_properties DROP COLUMN image_id\n")
		_T("<END>");

	if (!SQLBatch(batch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ConnectionPoolBaseSize"), _T("5"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ConnectionPoolMaxSize"), _T("20"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ConnectionPoolCooldownTime"), _T("300"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (g_dbSyntax == DB_SYNTAX_ORACLE)
	{
		if (!SQLQuery(_T("ALTER TABLE object_properties MODIFY comments null\n")))
			if (!g_bIgnoreErrors)
				return FALSE;
	}

	if (!ConvertStrings(_T("object_properties"), _T("object_id"), _T("comments")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='204' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V202 to V203
//

static BOOL H_UpgradeFromV202(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description,confirmation_text)")
			_T(" VALUES (20,'&Info->Topology table (LLDP)',2,'Topology Table',1,' ','Show topology table (LLDP)','#00')\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr)")
			_T(" VALUES (20,0,'Chassis ID','.1.0.8802.1.1.2.1.4.1.1.5',0,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr)")
			_T(" VALUES (20,1,'Local port','.1.0.8802.1.1.2.1.4.1.1.2',5,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr)")
			_T(" VALUES (20,2,'System name','.1.0.8802.1.1.2.1.4.1.1.9',0,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr)")
			_T(" VALUES (20,3,'System description','.1.0.8802.1.1.2.1.4.1.1.10',0,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr)")
			_T(" VALUES (20,4,'Remote port ID','.1.0.8802.1.1.2.1.4.1.1.7',4,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr)")
			_T(" VALUES (20,5,'Remote port description','.1.0.8802.1.1.2.1.4.1.1.8',0,0)\n")
		_T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (20,-2147483648)\n")
		_T("<END>");

	if (!SQLBatch(batch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='203' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V201 to V202
//

static BOOL H_UpgradeFromV201(int currVersion, int newVersion)
{
	if (g_dbSyntax == DB_SYNTAX_ORACLE)
	{
		static TCHAR oraBatch[] =
			_T("ALTER TABLE alarms MODIFY message null\n")
			_T("ALTER TABLE alarms MODIFY alarm_key null\n")
			_T("ALTER TABLE alarms MODIFY hd_ref null\n")
			_T("<END>");

		if (!SQLBatch(oraBatch))
			if (!g_bIgnoreErrors)
				return FALSE;
	}

	if (!ConvertStrings(_T("alarms"), _T("alarm_id"), _T("message")))
      if (!g_bIgnoreErrors)
         return FALSE;
	if (!ConvertStrings(_T("alarms"), _T("alarm_id"), _T("alarm_key")))
      if (!g_bIgnoreErrors)
         return FALSE;
	if (!ConvertStrings(_T("alarms"), _T("alarm_id"), _T("hd_ref")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='202' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V200 to V201
//

static BOOL H_UpgradeFromV200(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE nodes ADD usm_auth_password varchar(127)\n")
		_T("ALTER TABLE nodes ADD usm_priv_password varchar(127)\n")
		_T("ALTER TABLE nodes ADD usm_methods integer\n")
		_T("UPDATE nodes SET usm_auth_password='#00',usm_priv_password='#00',usm_methods=0\n")
		_T("<END>");

	if (!SQLBatch(batch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='201' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V92 to V200
//      or from V93 to V201
//      or from V94 to V202
//      or from V95 to V203
//      or from V96 to V204
//      or from V97 to V205
//      or from V98 to V206
//      or from V99 to V207
//

static BOOL H_UpgradeFromV9x(int currVersion, int newVersion)
{
	if (!CreateTable(_T("CREATE TABLE ap_common (")
		              _T("id integer not null,")
		              _T("policy_type integer not null,")
		              _T("version integer not null,")
						  _T("description $SQL:TEXT not null,")
						  _T("PRIMARY KEY(id))")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateTable(_T("CREATE TABLE ap_bindings (")
		              _T("policy_id integer not null,")
		              _T("node_id integer not null,")
						  _T("PRIMARY KEY(policy_id,node_id))")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateTable(_T("CREATE TABLE ap_config_files (")
		              _T("policy_id integer not null,")
		              _T("file_name varchar(63) not null,")
						  _T("file_content $SQL:TEXT not null,")
						  _T("PRIMARY KEY(policy_id))")))
		if (!g_bIgnoreErrors)
			return FALSE;

	CHK_EXEC(SetSchemaVersion(newVersion));
   return TRUE;
}

/**
 * Upgrade from V100 to V214
 *      or from V101 to V214
 *      or from V102 to V214
 *      or from V103 to V214
 *      or from V104 to V214
 */
static BOOL H_UpgradeFromV10x(int currVersion, int newVersion)
{
	if (!H_UpgradeFromV9x(currVersion, 207))
		return FALSE;

	// Now database at V207 level
	// V100 already has changes V209 -> V210, but missing V207 -> V209 and V210 -> V214 changes
	// V101 already has changes V209 -> V211, but missing V207 -> V209 and V211 -> V214 changes
	// V102 already has changes V209 -> V212, but missing V207 -> V209 and V212 -> V214 changes
	// V103 already has changes V209 -> V213, but missing V207 -> V209 and V213 -> V214 changes
	// V104 already has changes V209 -> V214, but missing V207 -> V209 changes

	if (!H_UpgradeFromV207(207, 208))
		return FALSE;

	if (!H_UpgradeFromV208(208, 209))
		return FALSE;

	if (currVersion == 100)
		if (!H_UpgradeFromV210(210, 211))
			return FALSE;

	if (currVersion < 102)
		if (!H_UpgradeFromV211(211, 212))
			return FALSE;

	if (currVersion < 103)
		if (!H_UpgradeFromV212(212, 213))
			return FALSE;

	if (currVersion < 104)
		if (!H_UpgradeFromV213(213, 214))
			return FALSE;

	CHK_EXEC(SetSchemaVersion(214));
   return TRUE;
}

/**
 * Upgrade from V105 to V217
 */
static BOOL H_UpgradeFromV105(int currVersion, int newVersion)
{
	if (!H_UpgradeFromV10x(currVersion, 214))
		return FALSE;

	// V105 already have V216 -> V217 changes, but missing V207 -> V209 and V214 -> V216 changes
	if (!H_UpgradeFromV214(214, 215))
		return FALSE;

	if (!H_UpgradeFromV215(215, 216))
		return FALSE;

	CHK_EXEC(SetSchemaVersion(217));
   return TRUE;
}

/**
 * Upgrade from V91 to V92
 */
static BOOL H_UpgradeFromV91(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("DROP TABLE images\n")
		_T("DROP TABLE default_images\n")
		_T("<END>");

	if (!SQLBatch(batch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='92' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V90 to V91
//

static BOOL H_UpgradeFromV90(int currVersion, int newVersion)
{
	if (!CreateTable(_T("CREATE TABLE userdb_custom_attributes (")
		              _T("object_id integer not null,")
	                 _T("attr_name varchar(255) not null,")
						  _T("attr_value $SQL:TEXT not null,")
						  _T("PRIMARY KEY(object_id,attr_name))")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='91' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V89 to V90
//

static BOOL H_UpgradeFromV89(int currVersion, int newVersion)
{
	static TCHAR m_szBatch[] =
		_T("ALTER TABLE items ADD base_units integer\n")
		_T("ALTER TABLE items ADD unit_multiplier integer\n")
		_T("ALTER TABLE items ADD custom_units_name varchar(63)\n")
		_T("ALTER TABLE items ADD perftab_settings $SQL:TEXT\n")
		_T("UPDATE items SET base_units=0,unit_multiplier=1,custom_units_name='#00',perftab_settings='#00'\n")
		_T("<END>");

	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='90' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V88 to V89
//

static BOOL H_UpgradeFromV88(int currVersion, int newVersion)
{
	static TCHAR m_szBatch[] =
		_T("ALTER TABLE containers ADD enable_auto_bind integer\n")
		_T("ALTER TABLE containers ADD auto_bind_filter $SQL:TEXT\n")
		_T("UPDATE containers SET enable_auto_bind=0,auto_bind_filter='#00'\n")
		_T("ALTER TABLE cluster_resources ADD current_owner integer\n")
		_T("UPDATE cluster_resources SET current_owner=0\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES (")
			_T("52,'SYS_DB_QUERY_FAILED',4,1,'Database query failed (Query: %1; Error: %2)',")
			_T("'Generated when SQL query to backend database failed.#0D#0A")
			_T("Parameters:#0D#0A   1) Query#0D#0A   2) Error message')\n")
		_T("<END>");

	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='89' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V87 to V88
//

static BOOL H_UpgradeFromV87(int currVersion, int newVersion)
{
	static TCHAR m_szBatch[] =
		_T("ALTER TABLE templates ADD enable_auto_apply integer\n")
		_T("ALTER TABLE templates ADD apply_filter $SQL:TEXT\n")
		_T("UPDATE templates SET enable_auto_apply=0,apply_filter='#00'\n")
		_T("<END>");

	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='88' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V86 to V87
//

static BOOL MoveConfigToMetadata(const TCHAR *cfgVar, const TCHAR *mdVar)
{
	TCHAR query[1024], buffer[256];
	DB_RESULT hResult;
	BOOL success;

	_sntprintf(query, 1024, _T("SELECT var_value FROM config WHERE var_name='%s'"), cfgVar);
	hResult = SQLSelect(query);
	if (hResult != NULL)
	{
		if (DBGetNumRows(hResult) > 0)
		{
			DBGetField(hResult, 0, 0, buffer, 256);
			DecodeSQLString(buffer);
			_sntprintf(query, 1024, _T("INSERT INTO metadata (var_name,var_value) VALUES ('%s','%s')"),
						  mdVar, buffer);
			DBFreeResult(hResult);
			success = SQLQuery(query);
			if (success)
			{
				_sntprintf(query, 1024, _T("DELETE FROM config WHERE var_name='%s'"), cfgVar);
				success = SQLQuery(query);
			}
		}
		else
		{
			success = TRUE;	// Variable missing in 'config' table, nothing to move
		}
	}
	else
	{
		success = FALSE;
	}
	return success;
}

static BOOL H_UpgradeFromV86(int currVersion, int newVersion)
{
	if (!CreateTable(_T("CREATE TABLE metadata (")
		              _T("var_name varchar(63) not null,")
						  _T("var_value varchar(255) not null,")
						  _T("PRIMARY KEY(var_name))")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!MoveConfigToMetadata(_T("DBFormatVersion"), _T("SchemaVersion")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!MoveConfigToMetadata(_T("DBSyntax"), _T("Syntax")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!MoveConfigToMetadata(_T("IDataTableCreationCommand"), _T("IDataTableCreationCommand")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!MoveConfigToMetadata(_T("IDataIndexCreationCommand_0"), _T("IDataIndexCreationCommand_0")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!MoveConfigToMetadata(_T("IDataIndexCreationCommand_1"), _T("IDataIndexCreationCommand_1")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!MoveConfigToMetadata(_T("IDataIndexCreationCommand_2"), _T("IDataIndexCreationCommand_2")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!MoveConfigToMetadata(_T("IDataIndexCreationCommand_3"), _T("IDataIndexCreationCommand_3")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='87' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V85 to V86
//

static BOOL H_UpgradeFromV85(int currVersion, int newVersion)
{
	static TCHAR m_szBatch[] =
		_T("DROP TABLE alarm_grops\n")
		_T("DROP TABLE alarm_group_map\n")
		_T("DROP TABLE alarm_change_log\n")
		_T("DROP TABLE lpp\n")
		_T("DROP TABLE lpp_associations\n")
		_T("DROP TABLE lpp_rulesets\n")
		_T("DROP TABLE lpp_rules\n")
		_T("DROP TABLE lpp_groups\n")
		_T("<END>");

	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='86' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V84 to V85
//

static BOOL H_UpgradeFromV84(int currVersion, int newVersion)
{
	static TCHAR m_szBatch[] =
		_T("ALTER TABLE nodes ADD use_ifxtable integer\n")
		_T("UPDATE nodes SET use_ifxtable=0\n")
		_T("<END>");

	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("UseIfXTable"), _T("1"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("SMTPRetryCount"), _T("1"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='85' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V83 to V84
//

static BOOL H_UpgradeFromV83(int currVersion, int newVersion)
{
	if (!CreateConfigParam(_T("EnableAgentRegistration"), _T("1"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("AnonymousFileAccess"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("EnableISCListener"), _T("0"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ReceiveForwardedEvents"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='84' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V82 to V83
//

static BOOL H_UpgradeFromV82(int currVersion, int newVersion)
{
	// Fix incorrect alarm timeouts
	if (!SQLQuery(_T("UPDATE alarms SET timeout=0,timeout_event=43")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='83' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V81 to V82
//

static BOOL H_UpgradeFromV81(int currVersion, int newVersion)
{
	if (!CreateTable(_T("CREATE TABLE config_clob (")
	                 _T("var_name varchar(63) not null,")
	                 _T("var_value $SQL:TEXT not null,")
	                 _T("PRIMARY KEY(var_name))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='82' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V80 to V81
//

static BOOL H_UpgradeFromV80(int currVersion, int newVersion)
{
	DB_RESULT hResult;
	TCHAR query[1024], buffer[1024];
	int i;

	// Update dci_schedules table
	hResult = SQLSelect(_T("SELECT item_id,schedule FROM dci_schedules"));
	if (hResult != NULL)
	{
		if (!SQLQuery(_T("DROP TABLE dci_schedules")))
		   if (!g_bIgnoreErrors)
		      return FALSE;

		if (!CreateTable(_T("CREATE TABLE dci_schedules (")
							  _T("schedule_id integer not null,")
							  _T("item_id integer not null,")
							  _T("schedule varchar(255) not null,")
							  _T("PRIMARY KEY(item_id,schedule_id))")))
			if (!g_bIgnoreErrors)
				return FALSE;

		for(i = 0; i < DBGetNumRows(hResult); i++)
		{
			_sntprintf(query, 1024, _T("INSERT INTO dci_schedules (item_id,schedule_id,schedule) VALUES(%d,%d,'%s')"),
			           DBGetFieldULong(hResult, i, 0), i + 1, DBGetField(hResult, i, 1, buffer, 1024));
			if (!SQLQuery(query))
				if (!g_bIgnoreErrors)
				   return FALSE;
		}
		DBFreeResult(hResult);
	}
	else
	{
      if (!g_bIgnoreErrors)
         return FALSE;
	}

	// Update address_lists table
	hResult = SQLSelect(_T("SELECT list_type,community_id,addr_type,addr1,addr2 FROM address_lists"));
	if (hResult != NULL)
	{
		if (!SQLQuery(_T("DROP TABLE address_lists")))
		   if (!g_bIgnoreErrors)
		      return FALSE;

		if (!CreateTable(_T("CREATE TABLE address_lists (")
							  _T("list_type integer not null,")
							  _T("community_id integer not null,")
							  _T("addr_type integer not null,")
							  _T("addr1 varchar(15) not null,")
							  _T("addr2 varchar(15) not null,")
							  _T("PRIMARY KEY(list_type,community_id,addr_type,addr1,addr2))")))
			if (!g_bIgnoreErrors)
				return FALSE;

		for(i = 0; i < DBGetNumRows(hResult); i++)
		{
			_sntprintf(query, 1024, _T("INSERT INTO address_lists (list_type,community_id,addr_type,addr1,addr2) VALUES(%d,%d,%d,'%s','%s')"),
			           DBGetFieldULong(hResult, i, 0), DBGetFieldULong(hResult, i, 1),
						  DBGetFieldULong(hResult, i, 2), DBGetField(hResult, i, 3, buffer, 64),
						  DBGetField(hResult, i, 4, &buffer[128], 64));
			if (!SQLQuery(query))
				if (!g_bIgnoreErrors)
				   return FALSE;
		}

		DBFreeResult(hResult);
	}
	else
	{
      if (!g_bIgnoreErrors)
         return FALSE;
	}

	// Create new tables
	if (!CreateTable(_T("CREATE TABLE object_custom_attributes (")
	                 _T("object_id integer not null,")
	                 _T("attr_name varchar(127) not null,")
	                 _T("attr_value $SQL:TEXT not null,")
	                 _T("PRIMARY KEY(object_id,attr_name))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE web_maps (")
	                 _T("id integer not null,")
	                 _T("title varchar(63) not null,")
	                 _T("properties $SQL:TEXT not null,")
	                 _T("data $SQL:TEXT not null,")
	                 _T("PRIMARY KEY(id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='81' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V79 to V80
//

static BOOL H_UpgradeFromV79(int currVersion, int newVersion)
{
	static TCHAR m_szBatch[] =
		_T("ALTER TABLE nodes ADD uname varchar(255)\n")
		_T("UPDATE nodes SET uname='#00'\n")
		_T("<END>");

	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='80' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V78 to V79
//

static BOOL H_UpgradeFromV78(int currVersion, int newVersion)
{
	static TCHAR m_szBatch[] =
		_T("DELETE FROM config WHERE var_name='RetainCustomInterfaceNames'\n")
		_T("DROP TABLE modules\n")
		_T("<END>");
	static TCHAR m_szMySQLBatch[] =
		_T("ALTER TABLE users MODIFY COLUMN cert_mapping_data text not null\n")
		_T("ALTER TABLE user_profiles MODIFY COLUMN var_value text not null\n")
		_T("ALTER TABLE object_properties MODIFY COLUMN comments text not null\n")
		_T("ALTER TABLE network_services MODIFY COLUMN check_request text not null\n")
		_T("ALTER TABLE network_services MODIFY COLUMN check_responce text not null\n")
		_T("ALTER TABLE conditions MODIFY COLUMN script text not null\n")
		_T("ALTER TABLE container_categories MODIFY COLUMN description text not null\n")
		_T("ALTER TABLE items MODIFY COLUMN transformation text not null\n")
		_T("ALTER TABLE event_cfg MODIFY COLUMN description text not null\n")
		_T("ALTER TABLE actions MODIFY COLUMN action_data text not null\n")
		_T("ALTER TABLE event_policy MODIFY COLUMN comments text not null\n")
		_T("ALTER TABLE event_policy MODIFY COLUMN script text not null\n")
		_T("ALTER TABLE alarm_change_log MODIFY COLUMN info_text text not null\n")
		_T("ALTER TABLE alarm_notes MODIFY COLUMN note_text text not null\n")
		_T("ALTER TABLE object_tools MODIFY COLUMN tool_data text not null\n")
		_T("ALTER TABLE syslog MODIFY COLUMN msg_text text not null\n")
		_T("ALTER TABLE script_library MODIFY COLUMN script_code text not null\n")
		_T("ALTER TABLE snmp_trap_log MODIFY COLUMN trap_varlist text not null\n")
		_T("ALTER TABLE maps MODIFY COLUMN description text not null\n")
		_T("ALTER TABLE agent_configs MODIFY COLUMN config_file text not null\n")
		_T("ALTER TABLE agent_configs MODIFY COLUMN config_filter text not null\n")
		_T("ALTER TABLE graphs MODIFY COLUMN config text not null\n")
		_T("ALTER TABLE certificates MODIFY COLUMN cert_data text not null\n")
		_T("ALTER TABLE certificates MODIFY COLUMN subject text not null\n")
		_T("ALTER TABLE certificates MODIFY COLUMN comments text not null\n")
		_T("ALTER TABLE audit_log MODIFY COLUMN message text not null\n")
		_T("ALTER TABLE situations MODIFY COLUMN comments text not null\n")
		_T("<END>");

	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (g_dbSyntax == DB_SYNTAX_MYSQL)
	{
		if (!SQLBatch(m_szMySQLBatch))
			if (!g_bIgnoreErrors)
				return FALSE;
	}

	if (!SQLQuery(_T("UPDATE config SET var_value='79' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}

/**
 * Upgrade from V77 to V78
 */
static BOOL H_UpgradeFromV77(int currVersion, int newVersion)
{
	if (!CreateTable(_T("CREATE TABLE trusted_nodes (")
	                 _T("source_object_id integer not null,")
	                 _T("target_node_id integer not null,")
	                 _T("PRIMARY KEY(source_object_id,target_node_id))")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("CheckTrustedNodes"), _T("1"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='78' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V76 to V77
//

static BOOL H_UpgradeFromV76(int currVersion, int newVersion)
{
	DB_RESULT hResult;
	int i, count, seq;
	DWORD id, lastId;
	TCHAR query[1024];

	hResult = SQLSelect(_T("SELECT condition_id,dci_id,node_id,dci_func,num_polls FROM cond_dci_map ORDER BY condition_id"));
	if (hResult == NULL)
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("DROP TABLE cond_dci_map")))
		if (!g_bIgnoreErrors)
			goto error;

	if (!CreateTable(_T("CREATE TABLE cond_dci_map (")
	                 _T("condition_id integer not null,")
	                 _T("sequence_number integer not null,")
	                 _T("dci_id integer not null,")
	                 _T("node_id integer not null,")
	                 _T("dci_func integer not null,")
	                 _T("num_polls integer not null,")
	                 _T("PRIMARY KEY(condition_id,sequence_number))")))
		if (!g_bIgnoreErrors)
			goto error;

	count = DBGetNumRows(hResult);
	for(i = 0, seq = 0, lastId = 0; i < count; i++, seq++)
	{
		id = DBGetFieldULong(hResult, i, 0);
		if (id != lastId)
		{
			seq = 0;
			lastId = id;
		}
		_sntprintf(query, 1024, _T("INSERT INTO cond_dci_map (condition_id,sequence_number,dci_id,node_id,dci_func,num_polls) VALUES (%d,%d,%d,%d,%d,%d)"),
		           id, seq, DBGetFieldULong(hResult, i, 1), DBGetFieldULong(hResult, i, 2),
					  DBGetFieldULong(hResult, i, 3), DBGetFieldULong(hResult, i, 4));
		if (!SQLQuery(query))
			if (!g_bIgnoreErrors)
				goto error;
	}

	DBFreeResult(hResult);

	if (!SQLQuery(_T("UPDATE config SET var_value='77' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;

error:
	DBFreeResult(hResult);
	return FALSE;
}


//
// Upgrade from V75 to V76
//

static BOOL H_UpgradeFromV75(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES (")
			_T("50,'SYS_NETWORK_CONN_LOST',4,1,'NetXMS server network connectivity lost',")
			_T("'Generated when system detects loss of network connectivity based on beacon ")
			_T("probing.#0D#0AParameters:#0D#0A   1) Number of beacons')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES (")
			_T("51,'SYS_NETWORK_CONN_RESTORED',0,1,'NetXMS server network connectivity restored',")
			_T("'Generated when system detects restoration of network connectivity based on ")
			_T("beacon probing.#0D#0AParameters:#0D#0A   1) Number of beacons')\n")
      _T("<END>");

	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("AgentCommandTimeout"), _T("2000"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("BeaconHosts"), _T(""), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("BeaconTimeout"), _T("1000"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("BeaconPollingInterval"), _T("1000"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='76' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}

/**
 * Upgrade from V74 to V75
 */
static BOOL H_UpgradeFromV74(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE address_lists ADD community_id integer\n")
		_T("UPDATE address_lists SET community_id=0\n")
      _T("<END>");

	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateTable(_T("CREATE TABLE snmp_communities (")
	                 _T("id integer not null,")
	                 _T("community varchar(255) not null,")
	                 _T("PRIMARY KEY(id))")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("UseInterfaceAliases"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("SyncNodeNamesWithDNS"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='75' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}

/**
 * Upgrade from V73 to V74
 */
static BOOL H_UpgradeFromV73(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
			_T("VALUES (48,'SYS_EVENT_STORM_DETECTED',3,1,'Event storm detected (Events per second: %1)',")
			_T("'Generated when system detects an event storm.#0D#0AParameters:#0D#0A")
			_T("   1) Events per second#0D#0A   2) Duration#0D#0A   3) Threshold')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
			_T("VALUES (49,'SYS_EVENT_STORM_ENDED',0,1,'Event storm ended',")
			_T("'Generated when system clears event storm condition.#0D#0AParameters:#0D#0A")
			_T("   1) Events per second#0D#0A   2) Duration#0D#0A   3) Threshold')\n")
		_T("DELETE FROM config WHERE var_name='NumberOfEventProcessors'\n")
		_T("DELETE FROM config WHERE var_name='EventStormThreshold'\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("EnableEventStormDetection"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateConfigParam(_T("EventStormEventsPerSecond"), _T("100"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateConfigParam(_T("EventStormDuration"), _T("15"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='74' WHERE var_name='DBFormatVersion'")))
		if (!g_bIgnoreErrors)
			return FALSE;

   return TRUE;
}


//
// Upgrade from V72 to V73
//

static BOOL H_UpgradeFromV72(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE event_policy ADD situation_id integer\n")
		_T("ALTER TABLE event_policy ADD situation_instance varchar(255)\n")
		_T("UPDATE event_policy SET situation_id=0,situation_instance='#00'\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE policy_situation_attr_list (")
	                 _T("rule_id integer not null,")
	                 _T("situation_id integer not null,")
	                 _T("attr_name varchar(255) not null,")
	                 _T("attr_value varchar(255) not null,")
	                 _T("PRIMARY KEY(rule_id,situation_id,attr_name))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE situations (")
	                 _T("id integer not null,")
	                 _T("name varchar(127) not null,")
	                 _T("comments $SQL:TEXT not null,")
	                 _T("PRIMARY KEY(id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("RetainCustomInterfaceNames"), _T("0"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("AllowDirectSMS"), _T("0"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("EventStormThreshold"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='73' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V71 to V72
//

static BOOL H_UpgradeFromV71(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE items ADD proxy_node integer\n")
		_T("UPDATE items SET proxy_node=0\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='72' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V70 to V71
//

static BOOL H_UpgradeFromV70(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE nodes ADD required_polls integer\n")
		_T("UPDATE nodes SET required_polls=0\n")
		_T("ALTER TABLE interfaces ADD required_polls integer\n")
		_T("UPDATE interfaces SET required_polls=0\n")
		_T("ALTER TABLE network_services ADD required_polls integer\n")
		_T("UPDATE network_services SET required_polls=0\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("PollCountForStatusChange"), _T("1"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='71' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V69 to V70
//

static BOOL H_UpgradeFromV69(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE snmp_trap_cfg ADD user_tag varchar(63)\n")
		_T("UPDATE snmp_trap_cfg SET user_tag='#00'\n")
		_T("ALTER TABLE event_log ADD user_tag varchar(63)\n")
		_T("UPDATE event_log SET user_tag='#00'\n")
      _T("<END>");
	int n;
	TCHAR buffer[64];

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	// Convert event log retention time from seconds to days
	n = ConfigReadInt(_T("EventLogRetentionTime"), 5184000) / 86400;
	_sntprintf(buffer, 64, _T("%d"), std::max(n, 1));
   if (!CreateConfigParam(_T("EventLogRetentionTime"), buffer, 1, 0, TRUE))
      if (!g_bIgnoreErrors)
         return FALSE;

	// Convert event log retention time from seconds to days
	n = ConfigReadInt(_T("SyslogRetentionTime"), 5184000) / 86400;
	_sntprintf(buffer, 64, _T("%d"), std::max(n, 1));
   if (!CreateConfigParam(_T("SyslogRetentionTime"), buffer, 1, 0, TRUE))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='70' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}

/**
 * Upgrade from V68 to V69
 */
static BOOL H_UpgradeFromV68(int currVersion, int newVersion)
{
	if (!CreateTable(_T("CREATE TABLE audit_log (")
	                 _T("record_id integer not null,")
	                 _T("timestamp integer not null,")
	                 _T("subsystem varchar(32) not null,")
	                 _T("success integer not null,")
	                 _T("user_id integer not null,")
	                 _T("workstation varchar(63) not null,")
	                 _T("object_id integer not null,")
	                 _T("message $SQL:TEXT not null,")
	                 _T("PRIMARY KEY(record_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("EnableAuditLog"), _T("1"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("AuditLogRetentionTime"), _T("90"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='69' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V67 to V68
//

static BOOL H_UpgradeFromV67(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE thresholds ADD repeat_interval integer\n")
		_T("UPDATE thresholds SET repeat_interval=-1\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("ThresholdRepeatInterval"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='68' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V66 to V67
//

static BOOL H_UpgradeFromV66(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE subnets ADD synthetic_mask integer\n")
		_T("UPDATE subnets SET synthetic_mask=0\n")
		_T("ALTER TABLE interfaces ADD synthetic_mask integer\n")
		_T("UPDATE interfaces SET synthetic_mask=0\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='67' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V65 to V66
//

static BOOL H_UpgradeFromV65(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE submap_links ADD port1 varchar(255)\n")
		_T("ALTER TABLE submap_links ADD port2 varchar(255)\n")
		_T("UPDATE submap_links SET port1='#00',port2='#00'\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='66' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}

/**
 * Upgrade from V64 to V65
 */
static BOOL H_UpgradeFromV64(int currVersion, int newVersion)
{
   static TCHAR m_szPGSQLBatch[] =
		_T("ALTER TABLE nodes ADD new_community varchar(127)\n")
		_T("UPDATE nodes SET new_community=community\n")
		_T("ALTER TABLE nodes DROP COLUMN community\n")
		_T("ALTER TABLE nodes RENAME COLUMN new_community TO community\n")
		_T("ALTER TABLE nodes ALTER COLUMN community SET NOT NULL\n")
      _T("<END>");

	switch(g_dbSyntax)
	{
		case DB_SYNTAX_MYSQL:
		case DB_SYNTAX_ORACLE:
			if (!SQLQuery(_T("ALTER TABLE nodes MODIFY community varchar(127)")))
		      if (!g_bIgnoreErrors)
					return FALSE;
			break;
		case DB_SYNTAX_PGSQL:
			if (g_bTrace)
				ShowQuery(_T("ALTER TABLE nodes ALTER COLUMN community TYPE varchar(127)"));

			if (!DBQuery(g_hCoreDB, _T("ALTER TABLE nodes ALTER COLUMN community TYPE varchar(127)")))
			{
				// Assume that we are using PostgreSQL oldest than 8.x
				if (!SQLBatch(m_szPGSQLBatch))
					if (!g_bIgnoreErrors)
						return FALSE;
			}
			break;
		case DB_SYNTAX_MSSQL:
			if (!SQLQuery(_T("ALTER TABLE nodes ALTER COLUMN community varchar(127)")))
		      if (!g_bIgnoreErrors)
					return FALSE;
			break;
		case DB_SYNTAX_SQLITE:
			_tprintf(_T("WARNING: Due to limitations of SQLite requested operation cannot be completed\nYou system will still be limited to use SNMP community strings not longer than 32 characters.\n"));
			break;
		default:
			_tprintf(_T("INTERNAL ERROR: Unknown database syntax %d\n"), g_dbSyntax);
			break;
	}

	if (!SQLQuery(_T("UPDATE config SET var_value='65' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V63 to V64
//

static BOOL H_UpgradeFromV63(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (15,'.1.3.6.1.4.1.45.3.29.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (16,'.1.3.6.1.4.1.45.3.41.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (17,'.1.3.6.1.4.1.45.3.45.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (18,'.1.3.6.1.4.1.45.3.43.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (19,'.1.3.6.1.4.1.45.3.57.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (20,'.1.3.6.1.4.1.45.3.49.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (21,'.1.3.6.1.4.1.45.3.54.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (22,'.1.3.6.1.4.1.45.3.63.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (23,'.1.3.6.1.4.1.45.3.64.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (24,'.1.3.6.1.4.1.45.3.53.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (25,'.1.3.6.1.4.1.45.3.59.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (26,'.1.3.6.1.4.1.45.3.39.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (27,'.1.3.6.1.4.1.45.3.65.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (28,'.1.3.6.1.4.1.45.3.66.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (29,'.1.3.6.1.4.1.45.3.44.*',4,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (30,'.1.3.6.1.4.1.45.3.47.*',4,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (31,'.1.3.6.1.4.1.45.3.48.*',4,0)\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='64' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V62 to V63
//

static BOOL H_UpgradeFromV62(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
			_T("VALUES (45,'SYS_IF_UNKNOWN',1,1,")
			_T("'Interface \"%2\" changed state to UNKNOWN (IP Addr: %3/%4, IfIndex: %5)',")
			_T("'Generated when interface goes to unknown state.#0D#0A")
			_T("Please note that source of event is node, not an interface itself.#0D#0A")
			_T("Parameters:#0D#0A   1) Interface object ID#0D#0A   2) Interface name#0D#0A")
			_T("   3) Interface IP address#0D#0A   4) Interface netmask#0D#0A")
			_T("   5) Interface index')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
			_T("VALUES (46,'SYS_IF_DISABLED',0,1,")
			_T("'Interface \"%2\" disabled (IP Addr: %3/%4, IfIndex: %5)',")
			_T("'Generated when interface administratively disabled.#0D#0A")
			_T("Please note that source of event is node, not an interface itself.#0D#0A")
			_T("Parameters:#0D#0A   1) Interface object ID#0D#0A   2) Interface name#0D#0A")
			_T("   3) Interface IP address#0D#0A   4) Interface netmask#0D#0A")
			_T("   5) Interface index')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
			_T("VALUES (47,'SYS_IF_TESTING',0,1,")
			_T("'Interface \"%2\" is testing (IP Addr: %3/%4, IfIndex: %5)',")
			_T("'Generated when interface goes to testing state.#0D#0A")
			_T("Please note that source of event is node, not an interface itself.#0D#0A")
			_T("Parameters:#0D#0A   1) Interface object ID#0D#0A   2) Interface name#0D#0A")
			_T("   3) Interface IP address#0D#0A   4) Interface netmask#0D#0A")
			_T("   5) Interface index')\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='63' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V61 to V62
//

static BOOL H_UpgradeFromV61(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("UPDATE event_policy SET alarm_key=alarm_ack_key WHERE alarm_severity=6\n")
		_T("ALTER TABLE event_policy DROP COLUMN alarm_ack_key\n")
		_T("ALTER TABLE event_policy ADD alarm_timeout integer\n")
		_T("ALTER TABLE event_policy ADD alarm_timeout_event integer\n")
		_T("UPDATE event_policy SET alarm_timeout=0,alarm_timeout_event=43\n")
		_T("ALTER TABLE alarms ADD timeout integer\n")
		_T("ALTER TABLE alarms ADD timeout_event integer\n")
		_T("UPDATE alarms SET timeout=0,timeout_event=43\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
			_T("VALUES (43,'SYS_ALARM_TIMEOUT',1,1,'Alarm timeout expired (ID: %1; Text: %2)',")
			_T("'Generated when alarm timeout expires.#0D#0AParameters:#0D#0A")
			_T("   1) Alarm ID#0D#0A   2) Alarm message#0D#0A   3) Alarm key#0D#0A   4) Event code')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
			_T("VALUES (44,'SYS_LOG_RECORD_MATCHED',1,1,")
			_T("'Log record matched (Policy: %1; File: %2; Record: %4)',")
			_T("'Default event for log record match.#0D#0AParameters:#0D#0A")
			_T("   1) Policy name#0D#0A   2) Log file name#0D#0A   3) Matching regular expression#0D#0A")
			_T("   4) Matched record#0D#0A   5 .. 9) Reserved#0D#0A")
			_T("   10 .. 99) Substrings extracted by regular expression')\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE lpp_groups (")
	                 _T("lpp_group_id integer not null,")
	                 _T("lpp_group_name varchar(63) not null,")
	                 _T("parent_group integer not null,")
	                 _T("PRIMARY KEY(lpp_group_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE lpp (")
	                 _T("lpp_id integer not null,")
	                 _T("lpp_group_id integer not null,")
	                 _T("lpp_name varchar(63) not null,")
	                 _T("lpp_version integer not null,")
	                 _T("lpp_flags integer not null,")
	                 _T("PRIMARY KEY(lpp_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE lpp_associations (")
	                 _T("lpp_id integer not null,")
	                 _T("node_id integer not null,")
	                 _T("log_file varchar(255) not null)")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE lpp_rulesets (")
	                 _T("ruleset_id integer not null,")
	                 _T("ruleset_name varchar(63),")
	                 _T("PRIMARY KEY(ruleset_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE lpp_rules (")
	                 _T("lpp_id integer not null,")
	                 _T("rule_number integer not null,")
	                 _T("ruleset_id integer not null,")
	                 _T("msg_id_start integer not null,")
	                 _T("msg_id_end integer not null,")
	                 _T("severity integer not null,")
	                 _T("source_name varchar(255) not null,")
	                 _T("msg_text_regexp varchar(255) not null,")
	                 _T("event_code integer not null,")
	                 _T("PRIMARY KEY(lpp_id,rule_number))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='62' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V60 to V61
//

static BOOL H_UpgradeFromV60(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("DELETE FROM object_tools WHERE tool_id=14\n")
		_T("DELETE FROM object_tools_table_columns WHERE tool_id=14\n")
		_T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description,confirmation_text) ")
			_T("VALUES (14,'&Info->Topology table (Nortel)',2,'Topology table',1,' ','Show topology table (Nortel protocol)','#00')\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (14,0,'Peer IP','.1.3.6.1.4.1.45.1.6.13.2.1.1.3',3,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (14,1,'Peer MAC','.1.3.6.1.4.1.45.1.6.13.2.1.1.5',4,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (14,2,'Slot','.1.3.6.1.4.1.45.1.6.13.2.1.1.1',1,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (14,3,'Port','.1.3.6.1.4.1.45.1.6.13.2.1.1.2',1,0)\n")
		_T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description,confirmation_text) ")
			_T("VALUES (17,'&Info->AR&P cache (SNMP)',2,'ARP Cache',1,' ','Show ARP cache','#00')\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (17,0,'IP Address','.1.3.6.1.2.1.4.22.1.3',3,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (17,1,'MAC Address','.1.3.6.1.2.1.4.22.1.2',4,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (17,2,'Interface','.1.3.6.1.2.1.4.22.1.1',5,0)\n")
		_T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description,confirmation_text) ")
			_T("VALUES (18,'&Info->AR&P cache (Agent)',3,")
			_T("'ARP Cache#7FNet.ArpCache#7F(.*) (.*) (.*)',2,' ','Show ARP cache','#00')\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (18,0,'IP Address','.1.3.6.1.2.1.4.22.1.3',0,2)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (18,1,'MAC Address','.1.3.6.1.2.1.4.22.1.2',0,1)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (18,2,'Interface','.1.3.6.1.2.1.4.22.1.1',5,3)\n")
		_T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description,confirmation_text) ")
			_T("VALUES (19,'&Info->&Routing table (SNMP)',2,'Routing Table',1,' ','Show IP routing table','#00')\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (19,0,'Destination','.1.3.6.1.2.1.4.21.1.1',3,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (19,1,'Mask','.1.3.6.1.2.1.4.21.1.11',3,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (19,2,'Next hop','.1.3.6.1.2.1.4.21.1.7',3,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (19,3,'Metric','.1.3.6.1.2.1.4.21.1.3',1,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (19,4,'Interface','.1.3.6.1.2.1.4.21.1.2',5,0)\n")
		_T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (17,-2147483648)\n")
		_T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (18,-2147483648)\n")
		_T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (19,-2147483648)\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("TopologyExpirationTime"), _T("900"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("TopologyDiscoveryRadius"), _T("3"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='61' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V59 to V60
//

static BOOL H_UpgradeFromV59(int currVersion, int newVersion)
{
	if (!CreateTable(_T("CREATE TABLE certificates (")
	                 _T("cert_id integer not null,")
	                 _T("cert_type integer not null,")
	                 _T("cert_data $SQL:TEXT not null,")
	                 _T("subject $SQL:TEXT not null,")
	                 _T("comments $SQL:TEXT not null,")
	                 _T("PRIMARY KEY(cert_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("SNMPRequestTimeout"), _T("2000"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='60' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V58 to V59
//

static BOOL H_UpgradeFromV58(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE users ADD cert_mapping_method integer\n")
		_T("ALTER TABLE users ADD cert_mapping_data $SQL:TEXT\n")
		_T("UPDATE users SET cert_mapping_method=0\n")
		_T("UPDATE users SET cert_mapping_data='#00'\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("InternalCA"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='59' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V57 to V58
//

static BOOL H_UpgradeFromV57(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE object_properties ADD is_system integer\n")
		_T("UPDATE object_properties SET is_system=0\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE graphs (")
	                 _T("graph_id integer not null,")
	                 _T("owner_id integer not null,")
	                 _T("name varchar(255) not null,")
	                 _T("config $SQL:TEXT not null,")
	                 _T("PRIMARY KEY(graph_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE graph_acl (")
	                 _T("graph_id integer not null,")
	                 _T("user_id integer not null,")
	                 _T("user_rights integer not null,")
	                 _T("PRIMARY KEY(graph_id,user_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='58' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V56 to V57
//

static BOOL H_UpgradeFromV56(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE items ADD resource_id integer\n")
		_T("UPDATE items SET resource_id=0\n")
		_T("ALTER TABLE nodes ADD snmp_proxy integer\n")
		_T("UPDATE nodes SET snmp_proxy=0\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='57' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V55 to V56
//

static BOOL H_UpgradeFromV55(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES")
			_T(" (38,'SYS_CLUSTER_RESOURCE_MOVED',1,1,")
			_T("'Cluster resource \"%2\" moved from node %4 to node %6',")
			_T("'Generated when cluster resource moved between nodes.#0D#0A")
			_T("Parameters:#0D#0A   1) Resource ID#0D#0A")
			_T("   2) Resource name#0D#0A   3) Previous owner node ID#0D#0A")
			_T("   4) Previous owner node name#0D#0A   5) New owner node ID#0D#0A")
			_T("   6) New owner node name')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES")
			_T(" (39,'SYS_CLUSTER_RESOURCE_DOWN',3,1,")
			_T("'Cluster resource \"%2\" is down (last owner was %4)',")
			_T("'Generated when cluster resource goes down.#0D#0A")
			_T("Parameters:#0D#0A   1) Resource ID#0D#0A   2) Resource name#0D#0A")
			_T("   3) Last owner node ID#0D#0A   4) Last owner node name')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES")
			_T(" (40,'SYS_CLUSTER_RESOURCE_UP',0,1,")
			_T("'Cluster resource \"%2\" is up (new owner is %4)',")
			_T("'Generated when cluster resource goes up.#0D#0A")
			_T("Parameters:#0D#0A   1) Resource ID#0D#0A   2) Resource name#0D#0A")
			_T("   3) New owner node ID#0D#0A   4) New owner node name')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES")
			_T(" (41,'SYS_CLUSTER_DOWN',4,1,'Cluster is down',")
			_T("'Generated when cluster goes down.#0D#0AParameters:#0D#0A   No message-specific parameters')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES")
			_T(" (42,'SYS_CLUSTER_UP',0,1,'Cluster is up',")
			_T("'Generated when cluster goes up.#0D#0AParameters:#0D#0A   No message-specific parameters')\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE cluster_resources (")
	                 _T("cluster_id integer not null,")
	                 _T("resource_id integer not null,")
	                 _T("resource_name varchar(255) not null,")
	                 _T("ip_addr varchar(15) not null,")
	                 _T("PRIMARY KEY(cluster_id,resource_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='56' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V54 to V55
//

static BOOL H_UpgradeFromV54(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE containers DROP COLUMN description\n")
		_T("ALTER TABLE nodes DROP COLUMN description\n")
		_T("ALTER TABLE templates DROP COLUMN description\n")
		_T("ALTER TABLE zones DROP COLUMN description\n")
		_T("INSERT INTO images (image_id,name,file_name_png,file_hash_png,")
			_T("file_name_ico,file_hash_ico) VALUES (16,'Obj.Cluster',")
			_T("'cluster.png','<invalid_hash>','cluster.ico','<invalid_hash>')\n")
		_T("INSERT INTO default_images (object_class,image_id) VALUES (14,16)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
			_T("VALUES (12,'.1.3.6.1.4.1.45.3.46.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
			_T("VALUES (13,'.1.3.6.1.4.1.45.3.52.*',3,0)\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE clusters (")
	                 _T("id integer not null,")
	                 _T("cluster_type integer not null,")
	                 _T("PRIMARY KEY(id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE cluster_members (")
	                 _T("cluster_id integer not null,")
	                 _T("node_id integer not null,")
	                 _T("PRIMARY KEY(cluster_id,node_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE cluster_sync_subnets (")
	                 _T("cluster_id integer not null,")
	                 _T("subnet_addr varchar(15) not null,")
	                 _T("subnet_mask varchar(15) not null,")
	                 _T("PRIMARY KEY(cluster_id,subnet_addr))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("WindowsConsoleUpgradeURL"), _T("http://www.netxms.org/download/netxms-%version%.exe"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='55' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}

/**
 * Upgrade from V53 to V54
 */
static BOOL H_UpgradeFromV53(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("CREATE INDEX idx_address_lists_list_type ON address_lists(list_type)\n")
      _T("DELETE FROM config WHERE var_name='EnableAccessControl'\n")
      _T("DELETE FROM config WHERE var_name='EnableEventAccessControl'\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE address_lists (")
                    _T("list_type integer not null,")
	                 _T("addr_type integer not null,")
	                 _T("addr1 varchar(15) not null,")
	                 _T("addr2 varchar(15) not null)")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("ActiveNetworkDiscovery"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("ActiveDiscoveryInterval"), _T("7200"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("DiscoveryFilterFlags"), _T("0"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='54' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V52 to V53
//

static BOOL H_UpgradeFromV52(int currVersion, int newVersion)
{
   DB_RESULT hResult;
   int i, nCount;
   DWORD dwId;
   TCHAR szQuery[1024];
   static const TCHAR *pszNewIdx[] =
   {
      _T("CREATE INDEX idx_idata_%d_id_timestamp ON idata_%d(item_id,idata_timestamp)"),  // MySQL
      _T("CREATE INDEX idx_idata_%d_timestamp_id ON idata_%d(idata_timestamp,item_id)"),  // POstgreSQL
      _T("CREATE CLUSTERED INDEX idx_idata_%d_id_timestamp ON idata_%d(item_id,idata_timestamp)"), // MS SQL
      _T("CREATE INDEX idx_idata_%d_timestamp_id ON idata_%d(idata_timestamp,item_id)"),  // Oracle
      _T("CREATE INDEX idx_idata_%d_timestamp_id ON idata_%d(idata_timestamp,item_id)")   // SQLite
   };

   hResult = SQLSelect(_T("SELECT id FROM nodes"));
   if (hResult == NULL)
      return FALSE;

   _tprintf(_T("Reindexing database:\n"));
   nCount = DBGetNumRows(hResult);
   for(i = 0; i < nCount; i++)
   {
      dwId = DBGetFieldULong(hResult, i, 0);
      _tprintf(_T("   * idata_%d\n"), dwId);

      // Drop old indexes
      _sntprintf(szQuery, 1024, _T("DROP INDEX idx_idata_%d_timestamp"), dwId);
      DBQuery(g_hCoreDB, szQuery);

      // Create new index
      _sntprintf(szQuery, 1024, pszNewIdx[g_dbSyntax], dwId, dwId);
      SQLQuery(szQuery);
   }

   DBFreeResult(hResult);

   // Update index creation command
   DBQuery(g_hCoreDB, _T("DELETE FROM config WHERE var_name='IDataIndexCreationCommand_1'"));
   if (!CreateConfigParam(_T("IDataIndexCreationCommand_1"), pszNewIdx[g_dbSyntax], 0, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='53' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V51 to V52
//

static BOOL H_UpgradeFromV51(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
	   _T("UPDATE object_tools SET tool_data='Configured ICMP targets#7FICMP.TargetList#7F^(.*) (.*) (.*) (.*) (.*)' WHERE tool_id=12\n")
	   _T("UPDATE object_tools_table_columns SET col_number=4 WHERE col_number=3 AND tool_id=12\n")
	   _T("UPDATE object_tools_table_columns SET col_number=3 WHERE col_number=2 AND tool_id=12\n")
   	_T("UPDATE object_tools_table_columns SET col_substr=5 WHERE col_number=1 AND tool_id=12\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (12,2,'Packet size','',0,4)\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description,confirmation_text) ")
	      _T("VALUES (16,'&Info->Active &user sessions',3,")
            _T("'Active User Sessions#7FSystem.ActiveUserSessions#7F^\"(.*)\" \"(.*)\" \"(.*)\"',")
            _T("2,'','Show list of active user sessions','#00')\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (16,0,'User','',0,1)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (16,1,'Terminal','',0,2)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (16,2,'From','',0,3)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (16,-2147483648)\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("MailEncoding"), _T("iso-8859-1"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='52' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V50 to V51
//

static BOOL H_UpgradeFromV50(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE event_groups ADD range_start integer\n")
      _T("ALTER TABLE event_groups ADD range_END integer\n")
      _T("UPDATE event_groups SET range_start=0,range_end=0\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='51' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V49 to V50
//

static BOOL H_UpgradeFromV49(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE object_tools ADD confirmation_text varchar(255)\n")
      _T("UPDATE object_tools SET confirmation_text='#00'\n")
      _T("UPDATE object_tools SET flags=10 WHERE tool_id=1 OR tool_id=2 OR tool_id=4\n")
      _T("UPDATE object_tools SET confirmation_text='Host #25OBJECT_NAME#25 (#25OBJECT_IP_ADDR#25) will be shut down. Are you sure?' WHERE tool_id=1\n")
      _T("UPDATE object_tools SET confirmation_text='Host #25OBJECT_NAME#25 (#25OBJECT_IP_ADDR#25) will be restarted. Are you sure?' WHERE tool_id=2\n")
      _T("UPDATE object_tools SET confirmation_text='NetXMS agent on host #25OBJECT_NAME#25 (#25OBJECT_IP_ADDR#25) will be restarted. Are you sure?' WHERE tool_id=4\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='50' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V48 to V49
//

static BOOL H_UpgradeFromV48(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE items ADD all_thresholds integer\n")
      _T("UPDATE items SET all_thresholds=0\n")
      _T("ALTER TABLE thresholds ADD rearm_event_code integer\n")
      _T("UPDATE thresholds SET rearm_event_code=18\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='49' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V47 to V48
//

static BOOL H_UpgradeFromV47(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE event_policy ADD script $SQL:TEXT\n")
      _T("UPDATE event_policy SET script='#00'\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE policy_time_range_list (")
		              _T("rule_id integer not null,")
		              _T("time_range_id integer not null,")
		              _T("PRIMARY KEY(rule_id,time_range_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE time_ranges (")
		              _T("time_range_id integer not null,")
		              _T("wday_mask integer not null,")
		              _T("mday_mask integer not null,")
		              _T("month_mask integer not null,")
		              _T("time_range varchar(255) not null,")
		              _T("PRIMARY KEY(time_range_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='48' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V46 to V47
//

static BOOL H_UpgradeFromV46(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE object_properties ADD comments $SQL:TEXT\n")
      _T("UPDATE object_properties SET comments='#00'\n")
      _T("ALTER TABLE nodes DROP COLUMN discovery_flags\n")
      _T("DROP TABLE alarm_notes\n")
      _T("ALTER TABLE alarms ADD alarm_state integer\n")
      _T("ALTER TABLE alarms ADD hd_state integer\n")
      _T("ALTER TABLE alarms ADD hd_ref varchar(63)\n")
      _T("ALTER TABLE alarms ADD creation_time integer\n")
      _T("ALTER TABLE alarms ADD last_change_time integer\n")
      _T("ALTER TABLE alarms ADD original_severity integer\n")
      _T("ALTER TABLE alarms ADD current_severity integer\n")
      _T("ALTER TABLE alarms ADD repeat_count integer\n")
      _T("ALTER TABLE alarms ADD term_by integer\n")
      _T("UPDATE alarms SET hd_state=0,hd_ref='#00',creation_time=alarm_timestamp,")
         _T("last_change_time=alarm_timestamp,original_severity=severity,")
         _T("current_severity=severity,repeat_count=1,term_by=ack_by\n")
      _T("UPDATE alarms SET alarm_state=0 WHERE is_ack=0\n")
      _T("UPDATE alarms SET alarm_state=2 WHERE is_ack<>0\n")
      _T("ALTER TABLE alarms DROP COLUMN severity\n")
      _T("ALTER TABLE alarms DROP COLUMN alarm_timestamp\n")
      _T("ALTER TABLE alarms DROP COLUMN is_ack\n")
      _T("ALTER TABLE thresholds ADD current_state integer\n")
      _T("UPDATE thresholds SET current_state=0\n")
      _T("<END>");
   static TCHAR m_szBatch2[] =
      _T("CREATE INDEX idx_alarm_notes_alarm_id ON alarm_notes(alarm_id)\n")
      _T("CREATE INDEX idx_alarm_change_log_alarm_id ON alarm_change_log(alarm_id)\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE alarm_notes (")
		                 _T("note_id integer not null,")
		                 _T("alarm_id integer not null,")
		                 _T("change_time integer not null,")
		                 _T("user_id integer not null,")
		                 _T("note_text $SQL:TEXT not null,")
		                 _T("PRIMARY KEY(note_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE alarm_change_log	(")
                  	  _T("change_id $SQL:INT64 not null,")
		                 _T("change_time integer not null,")
		                 _T("alarm_id integer not null,")
		                 _T("opcode integer not null,")
		                 _T("user_id integer not null,")
		                 _T("info_text $SQL:TEXT not null,")
		                 _T("PRIMARY KEY(change_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE alarm_grops (")
		                 _T("alarm_group_id integer not null,")
		                 _T("group_name varchar(255) not null,")
		                 _T("PRIMARY KEY(alarm_group_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE alarm_group_map (")
		                 _T("alarm_group_id integer not null,")
                       _T("alarm_id integer not null,")
		                 _T("PRIMARY KEY(alarm_group_id,alarm_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch2))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='47' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V45 to V46
//

static BOOL H_UpgradeFromV45(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("UPDATE object_tools_table_columns SET col_format=5 WHERE tool_id=5 AND col_number=1\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (2,'.1.3.6.1.4.1.45.3.26.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (3,'.1.3.6.1.4.1.45.3.30.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (4,'.1.3.6.1.4.1.45.3.31.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (5,'.1.3.6.1.4.1.45.3.32.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (6,'.1.3.6.1.4.1.45.3.33.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (7,'.1.3.6.1.4.1.45.3.34.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (8,'.1.3.6.1.4.1.45.3.35.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (9,'.1.3.6.1.4.1.45.3.36.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (10,'.1.3.6.1.4.1.45.3.40.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (11,'.1.3.6.1.4.1.45.3.61.*',3,0)\n")
	   _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description) ")
		   _T("VALUES (14,'&Info->Topology table (Nortel)',2 ,'Topology table',1,'','Show topology table (Nortel protocol)')\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (14,0,'Peer IP','.1.3.6.1.4.1.45.1.6.13.2.1.1.3',3 ,0)\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (14,1,'Peer MAC','.1.3.6.1.4.1.45.1.6.13.2.1.1.5',4 ,0)\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (14,2,'Port','.1.3.6.1.4.1.45.1.6.13.2.1.1.2',5 ,0)\n")
	   _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (14,-2147483648)\n")
	   _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description) ")
		   _T("VALUES (15,'&Info->Topology table (CDP)',2 ,'Topology table',1,'','Show topology table (CDP)')\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (15,0,'Device ID','.1.3.6.1.4.1.9.9.23.1.2.1.1.6',0 ,0)\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (15,1,'IP Address','.1.3.6.1.4.1.9.9.23.1.2.1.1.4',3 ,0)\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (15,2,'Platform','.1.3.6.1.4.1.9.9.23.1.2.1.1.8',0 ,0)\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (15,3,'Version','.1.3.6.1.4.1.9.9.23.1.2.1.1.5',0 ,0)\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (15,4,'Port','.1.3.6.1.4.1.9.9.23.1.2.1.1.7',0 ,0)\n")
	   _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (15,-2147483648)\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='46' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V44 to V45
//

static BOOL H_UpgradeFromV44(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
	   _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
         _T("VALUES (36,'SYS_DB_CONN_LOST',4,1,")
			_T("'Lost connection with backend database engine',")
			_T("'Generated if connection with backend database engine is lost.#0D#0A")
			_T("Parameters:#0D#0A   No message-specific parameters')\n")
	   _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
         _T("VALUES (37,'SYS_DB_CONN_RESTORED',0,1,")
			_T("'Connection with backend database engine restored',")
			_T("'Generated when connection with backend database engine restored.#0D#0A")
			_T("Parameters:#0D#0A   No message-specific parameters')\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='45' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V43 to V44
//

static BOOL H_UpgradeFromV43(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("DELETE FROM object_tools WHERE tool_id=8000\n")
      _T("DELETE FROM object_tools_table_columns WHERE tool_id=8000\n")
      _T("DELETE FROM object_tools_acl WHERE tool_id=8000\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,")
         _T("matching_oid,description) VALUES (13,'&Info->&Process list',3,")
         _T("'Process List#7FSystem.ProcessList#7F^([0-9]+) (.*)',2,'',")
         _T("'Show list of currently running processes')\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,")
         _T("col_oid,col_format,col_substr) VALUES (13,0,'PID','',0,1)\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,")
         _T("col_oid,col_format,col_substr) VALUES (13,1,'Name','',0,2)\n")
	   _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (13,-2147483648)\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE agent_configs (")
		                 _T("config_id integer not null,")
		                 _T("config_name varchar(255) not null,")
		                 _T("config_file $SQL:TEXT not null,")
		                 _T("config_filter $SQL:TEXT not null,")
		                 _T("sequence_number integer not null,")
		                 _T("PRIMARY KEY(config_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("DBLockPID"), _T("0"), 0, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='44' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V42 to V43
//

static BOOL H_UpgradeFromV42(int currVersion, int newVersion)
{
   if (!CreateConfigParam(_T("RADIUSPort"), _T("1645"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='43' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V41 to V42
//

static BOOL H_UpgradeFromV41(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
	   _T("INSERT INTO images (image_id,name,file_name_png,file_hash_png,")
         _T("file_name_ico,file_hash_ico) VALUES (15,'Obj.Condition',")
         _T("'condition.png','<invalid_hash>','condition.ico','<invalid_hash>')\n")
	   _T("INSERT INTO default_images (object_class,image_id) VALUES (13, 15)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (1,'.1.3.6.1.4.1.3224.1.*',2,0)\n")
	   _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
         _T("VALUES (34,'SYS_CONDITION_ACTIVATED',2,1,'Condition \"%2\" activated',")
			_T("'Default event for condition activation.#0D#0AParameters:#0D#0A")
			_T("   1) Condition object ID#0D#0A   2) Condition object name#0D#0A")
			_T("   3) Previous condition status#0D#0A   4) Current condition status')\n")
	   _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
         _T("VALUES (35,'SYS_CONDITION_DEACTIVATED',0,1,'Condition \"%2\" deactivated',")
			_T("'Default event for condition deactivation.#0D#0AParameters:#0D#0A")
			_T("   1) Condition object ID#0D#0A   2) Condition object name#0D#0A")
			_T("   3) Previous condition status#0D#0A   4) Current condition status')\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE conditions	(")
		                 _T("id integer not null,")
		                 _T("activation_event integer not null,")
		                 _T("deactivation_event integer not null,")
		                 _T("source_object integer not null,")
		                 _T("active_status integer not null,")
		                 _T("inactive_status integer not null,")
		                 _T("script $SQL:TEXT not null,")
		                 _T("PRIMARY KEY(id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE cond_dci_map (")
		                 _T("condition_id integer not null,")
		                 _T("dci_id integer not null,")
		                 _T("node_id integer not null,")
		                 _T("dci_func integer not null,")
		                 _T("num_polls integer not null,")
		                 _T("PRIMARY KEY(condition_id,dci_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("NumberOfConditionPollers"), _T("10"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("ConditionPollingInterval"), _T("60"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='42' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V40 to V41
//

static BOOL H_UpgradeFromV40(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE users ADD guid varchar(36)\n")
      _T("ALTER TABLE users ADD auth_method integer\n")
      _T("ALTER TABLE user_groups ADD guid varchar(36)\n")
      _T("UPDATE users SET auth_method=0\n")
      _T("<END>");
   DB_RESULT hResult;
   int i, nCount;
   DWORD dwId;
   uuid_t guid;
   TCHAR szQuery[256], szGUID[64];

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   // Generate GUIDs for users and groups
   _tprintf(_T("Generating GUIDs...\n"));

   hResult = SQLSelect(_T("SELECT id FROM users"));
   if (hResult != NULL)
   {
      nCount = DBGetNumRows(hResult);
      for(i = 0; i < nCount; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         _uuid_generate(guid);
         _sntprintf(szQuery, 256, _T("UPDATE users SET guid='%s' WHERE id=%d"),
                    _uuid_to_string(guid, szGUID), dwId);
         if (!SQLQuery(szQuery))
            if (!g_bIgnoreErrors)
            {
               DBFreeResult(hResult);
               return FALSE;
            }
      }
      DBFreeResult(hResult);
   }

   hResult = SQLSelect(_T("SELECT id FROM user_groups"));
   if (hResult != NULL)
   {
      nCount = DBGetNumRows(hResult);
      for(i = 0; i < nCount; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         _uuid_generate(guid);
         _sntprintf(szQuery, 256, _T("UPDATE user_groups SET guid='%s' WHERE id=%d"),
                    _uuid_to_string(guid, szGUID), dwId);
         if (!SQLQuery(szQuery))
            if (!g_bIgnoreErrors)
            {
               DBFreeResult(hResult);
               return FALSE;
            }
      }
      DBFreeResult(hResult);
   }

   if (!CreateConfigParam(_T("RADIUSServer"), _T("localhost"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("RADIUSSecret"), _T("netxms"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("RADIUSNumRetries"), _T("5"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("RADIUSTimeout"), _T("3"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='41' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V39 to V40
//

static BOOL H_UpgradeFromV39(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE users ADD grace_logins integer\n")
      _T("UPDATE users SET grace_logins=5\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='40' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V38 to V39
//

static BOOL H_UpgradeFromV38(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
	   _T("INSERT INTO maps (map_id,map_name,description,root_object_id) ")
		   _T("VALUES (1,'Default','Default network map',1)\n")
	   _T("INSERT INTO map_access_lists (map_id,user_id,access_rights) VALUES (1,-2147483648,1)\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
         _T("VALUES (33,'SYS_SCRIPT_ERROR',2,1,")
		   _T("'Script (%1) execution error: %2',")
		   _T("'Generated when server encounters NXSL script execution error.#0D#0A")
		   _T("Parameters:#0D#0A")
		   _T("   1) Script name#0D#0A")
		   _T("   2) Error text#0D#0A")
		   _T("   3) DCI ID if script is DCI transformation script, or 0 otherwise')\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE maps	(")
		                 _T("map_id integer not null,")
		                 _T("map_name varchar(255) not null,")
		                 _T("description $SQL:TEXT not null,")
		                 _T("root_object_id integer not null,")
		                 _T("PRIMARY KEY(map_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE map_access_lists (")
		                 _T("map_id integer not null,")
		                 _T("user_id integer not null,")
		                 _T("access_rights integer not null,")
		                 _T("PRIMARY KEY(map_id,user_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE submaps (")
		                 _T("map_id integer not null,")
		                 _T("submap_id integer not null,")
		                 _T("attributes integer not null,")
		                 _T("PRIMARY KEY(map_id,submap_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE submap_object_positions (")
		                 _T("map_id integer not null,")
		                 _T("submap_id integer not null,")
		                 _T("object_id integer not null,")
		                 _T("x integer not null,")
		                 _T("y integer not null,")
		                 _T("PRIMARY KEY(map_id,submap_id,object_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE submap_links (")
		                 _T("map_id integer not null,")
		                 _T("submap_id integer not null,")
		                 _T("object_id1 integer not null,")
		                 _T("object_id2 integer not null,")
		                 _T("link_type integer not null,")
		                 _T("PRIMARY KEY(map_id,submap_id,object_id1,object_id2))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("LockTimeout"), _T("60000"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("DisableVacuum"), _T("0"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='39' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V37 to V38
//

static BOOL H_UpgradeFromV37(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
	   _T("CREATE INDEX idx_event_log_event_timestamp ON event_log(event_timestamp)\n")
	   _T("CREATE INDEX idx_syslog_msg_timestamp ON syslog(msg_timestamp)\n")
      _T("CREATE INDEX idx_snmp_trap_log_trap_timestamp ON snmp_trap_log(trap_timestamp)\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE snmp_trap_log (")
		                 _T("trap_id $SQL:INT64 not null,")
		                 _T("trap_timestamp integer not null,")
		                 _T("ip_addr varchar(15) not null,")
		                 _T("object_id integer not null,")
		                 _T("trap_oid varchar(255) not null,")
		                 _T("trap_varlist $SQL:TEXT not null,")
		                 _T("PRIMARY KEY(trap_id))")))

      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("LogAllSNMPTraps"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='38' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V36 to V37
//

static BOOL H_UpgradeFromV36(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("DROP TABLE new_nodes\n")
      _T("DELETE FROM config WHERE var_name='NewNodePollingInterval'\n")
      _T("INSERT INTO script_library (script_id,script_name,script_code) ")
	      _T("VALUES (1,'Filter::SNMP','sub main()#0D#0A{#0D#0A   return $1->isSNMP;#0D#0A}#0D#0A')\n")
      _T("INSERT INTO script_library (script_id,script_name,script_code) ")
	      _T("VALUES (2,'Filter::Agent','sub main()#0D#0A{#0D#0A   return $1->isAgent;#0D#0A}#0D#0A')\n")
      _T("INSERT INTO script_library (script_id,script_name,script_code) ")
	      _T("VALUES (3,'Filter::AgentOrSNMP','sub main()#0D#0A{#0D#0A   return $1->isAgent || $1->isSNMP;#0D#0A}#0D#0A')\n")
      _T("INSERT INTO script_library (script_id,script_name,script_code) ")
	      _T("VALUES (4,'DCI::SampleTransform','sub dci_transform()#0D#0A{#0D#0A   return $1 + 1;#0D#0A}#0D#0A')\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE script_library (")
		              _T("script_id integer not null,")
		              _T("script_name varchar(63) not null,")
		              _T("script_code $SQL:TEXT not null,")
		              _T("PRIMARY KEY(script_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("DefaultCommunityString"), _T("public"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("DiscoveryFilter"), _T("none"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='37' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V35 to V36
//

static BOOL H_UpgradeFromV35(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE nodes ADD proxy_node integer\n")
      _T("UPDATE nodes SET proxy_node=0\n")
      _T("ALTER TABLE object_tools ADD matching_oid varchar(255)\n")
      _T("UPDATE object_tools SET matching_oid='#00'\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("CapabilityExpirationTime"), _T("604800"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='36' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V34 to V35
//

static BOOL H_UpgradeFromV34(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE object_properties DROP COLUMN status_alg\n")
      _T("ALTER TABLE object_properties ADD status_calc_alg integer\n")
	   _T("ALTER TABLE object_properties ADD status_prop_alg integer\n")
	   _T("ALTER TABLE object_properties ADD status_fixed_val integer\n")
	   _T("ALTER TABLE object_properties ADD status_shift integer\n")
	   _T("ALTER TABLE object_properties ADD status_translation varchar(8)\n")
	   _T("ALTER TABLE object_properties ADD status_single_threshold integer\n")
	   _T("ALTER TABLE object_properties ADD status_thresholds varchar(8)\n")
      _T("UPDATE object_properties SET status_calc_alg=0,status_prop_alg=0,")
         _T("status_fixed_val=0,status_shift=0,status_translation='01020304',")
         _T("status_single_threshold=75,status_thresholds='503C2814'\n")
      _T("DELETE FROM config WHERE var_name='StatusCalculationAlgorithm'\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("StatusCalculationAlgorithm"), _T("1"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("StatusPropagationAlgorithm"), _T("1"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("FixedStatusValue"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("StatusShift"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("StatusTranslation"), _T("01020304"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("StatusSingleThreshold"), _T("75"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("StatusThresholds"), _T("503C2814"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='35' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V33 to V34
//

static BOOL H_UpgradeFromV33(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE items ADD adv_schedule integer\n")
      _T("UPDATE items SET adv_schedule=0\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE dci_schedules (")
		                 _T("item_id integer not null,")
                       _T("schedule varchar(255) not null)")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE syslog (")
                       _T("msg_id $SQL:INT64 not null,")
		                 _T("msg_timestamp integer not null,")
		                 _T("facility integer not null,")
		                 _T("severity integer not null,")
		                 _T("source_object_id integer not null,")
		                 _T("hostname varchar(127) not null,")
		                 _T("msg_tag varchar(32) not null,")
		                 _T("msg_text $SQL:TEXT not null,")
		                 _T("PRIMARY KEY(msg_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("IcmpPingSize"), _T("46"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("SMSDrvConfig"), _T(""), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("EnableSyslogDaemon"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("SyslogListenPort"), _T("514"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("SyslogRetentionTime"), _T("5184000"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='34' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V32 to V33
//

static BOOL H_UpgradeFromV32(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (5,'&Info->&Switch forwarding database (FDB)',2 ,'Forwarding database',0,'Show switch forwarding database')\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (5,0,'MAC Address','.1.3.6.1.2.1.17.4.3.1.1',4 ,0)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (5,1,'Port','.1.3.6.1.2.1.17.4.3.1.2',1 ,0)\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (6,'&Connect->Open &web browser',4 ,'http://%OBJECT_IP_ADDR%',0,'Open embedded web browser to node')\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (7,'&Connect->Open &web browser (HTTPS)',4 ,'https://%OBJECT_IP_ADDR%',0,'Open embedded web browser to node using HTTPS')\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (8,'&Info->&Agent->&Subagent list',3 ,'Subagent List#7FAgent.SubAgentList#7F^(.*) (.*) (.*) (.*)',0,'Show list of loaded subagents')\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (8,0,'Name','',0 ,1)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (8,1,'Version','',0 ,2)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (8,2,'File','',0 ,4)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (8,3,'Module handle','',0 ,3)\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (9,'&Info->&Agent->Supported &parameters',3 ,'Supported parameters#7FAgent.SupportedParameters#7F^(.*)',0,'Show list of parameters supported by agent')\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (9,0,'Parameter','',0 ,1)\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (10,'&Info->&Agent->Supported &enums',3 ,'Supported enums#7FAgent.SupportedEnums#7F^(.*)',0,'Show list of enums supported by agent')\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (10,0,'Parameter','',0 ,1)\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (11,'&Info->&Agent->Supported &actions',3 ,'Supported actions#7FAgent.ActionList#7F^(.*) (.*) #22(.*)#22.*',0,'Show list of actions supported by agent')\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (11,0,'Name','',0 ,1)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (11,1,'Type','',0 ,2)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (11,2,'Data','',0 ,3)\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (12,'&Info->&Agent->Configured &ICMP targets',3 ,'Configured ICMP targets#7FICMP.TargetList#7F^(.*) (.*) (.*) (.*)',0,'Show list of actions supported by agent')\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (12,0,'IP Address','',0 ,1)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (12,1,'Name','',0 ,4)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (12,2,'Last RTT','',0 ,2)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (12,4,'Average RTT','',0 ,3)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (5,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (6,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (7,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (8,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (9,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (10,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (11,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (12,-2147483648)\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE object_tools_table_columns (")
                  	  _T("tool_id integer not null,")
		                 _T("col_number integer not null,")
		                 _T("col_name varchar(255),")
		                 _T("col_oid varchar(255),")
		                 _T("col_format integer,")
		                 _T("col_substr integer,")
		                 _T("PRIMARY KEY(tool_id,col_number))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='33' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V31 to V32
//

static BOOL H_UpgradeFromV31(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (1,'&Shutdown system',1 ,'System.Shutdown',0,'Shutdown target node via NetXMS agent')\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (2,'&Restart system',1 ,'System.Restart',0,'Restart target node via NetXMS agent')\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (3,'&Wakeup node',0 ,'wakeup',0,'Wakeup node using Wake-On-LAN magic packet')\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (4,'Restart &agent',1 ,'Agent.Restart',0,'Restart NetXMS agent on target node')\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (1,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (2,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (3,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (4,-2147483648)\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE object_tools (")
	                    _T("tool_id integer not null,")
	                    _T("tool_name varchar(255) not null,")
	                    _T("tool_type integer not null,")
	                    _T("tool_data $SQL:TEXT,")
	                    _T("description varchar(255),")
	                    _T("flags integer not null,")
	                    _T("PRIMARY KEY(tool_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE object_tools_acl (")
	                    _T("tool_id integer not null,")
	                    _T("user_id integer not null,")
	                    _T("PRIMARY KEY(tool_id,user_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='32' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V30 to V31
//

static BOOL H_UpgradeFromV30(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
	   _T("INSERT INTO default_images (object_class,image_id) ")
		   _T("VALUES (12, 14)\n")
	   _T("INSERT INTO images (image_id,name,file_name_png,file_hash_png,")
         _T("file_name_ico,file_hash_ico) VALUES (14,'Obj.VPNConnector',")
         _T("'vpnc.png','<invalid_hash>','vpnc.ico','<invalid_hash>')\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE vpn_connectors (")
		                 _T("id integer not null,")
		                 _T("node_id integer not null,")
		                 _T("peer_gateway integer not null,")
		                 _T("PRIMARY KEY(id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE vpn_connector_networks (")
		                 _T("vpn_id integer not null,")
		                 _T("network_type integer not null,")
		                 _T("ip_addr varchar(15) not null,")
		                 _T("ip_netmask varchar(15) not null,")
		                 _T("PRIMARY KEY(vpn_id,ip_addr))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("NumberOfRoutingTablePollers"), _T("5"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("RoutingTableUpdateInterval"), _T("300"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='31' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V29 to V30
//

static BOOL H_UpgradeFromV29(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE object_properties ADD status_alg integer\n")
      _T("UPDATE object_properties SET status_alg=-1\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("StatusCalculationAlgorithm"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("EnableMultipleDBConnections"), _T("1"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("NumberOfDatabaseWriters"), _T("1"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("DefaultEncryptionPolicy"), _T("1"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("AllowedCiphers"), _T("15"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("KeepAliveInterval"), _T("60"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='30' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V28 to V29
//

static BOOL H_UpgradeFromV28(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE nodes ADD zone_guid integer\n")
      _T("ALTER TABLE subnets ADD zone_guid integer\n")
      _T("UPDATE nodes SET zone_guid=0\n")
      _T("UPDATE subnets SET zone_guid=0\n")
      _T("INSERT INTO default_images (object_class,image_id) VALUES (6,13)\n")
      _T("INSERT INTO images (image_id,name,file_name_png,file_hash_png,")
         _T("file_name_ico,file_hash_ico) VALUES (13,'Obj.Zone','zone.png',")
         _T("'<invalid_hash>','zone.ico','<invalid_hash>')\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE zones (")
	                    _T("id integer not null,")
	                    _T("zone_guid integer not null,")
	                    _T("zone_type integer not null,")
	                    _T("controller_ip varchar(15) not null,")
	                    _T("description $SQL:TEXT,")
	                    _T("PRIMARY KEY(id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE zone_ip_addr_list (")
	                    _T("zone_id integer not null,")
	                    _T("ip_addr varchar(15) not null,")
	                    _T("PRIMARY KEY(zone_id,ip_addr))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("EnableZoning"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='29' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V27 to V28
//

static BOOL H_UpgradeFromV27(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE users ADD system_access integer\n")
      _T("UPDATE users SET system_access=access\n")
      _T("ALTER TABLE users DROP COLUMN access\n")
      _T("ALTER TABLE user_groups ADD system_access integer\n")
      _T("UPDATE user_groups SET system_access=access\n")
      _T("ALTER TABLE user_groups DROP COLUMN access\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='28' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Move object data from class-specific tables to object_prioperties table
//

static BOOL MoveObjectData(DWORD dwId, BOOL bInheritRights)
{
   DB_RESULT hResult;
   TCHAR szQuery[1024] ,szName[MAX_OBJECT_NAME];
   BOOL bRead = FALSE, bIsDeleted, bIsTemplate;
   DWORD i, dwStatus, dwImageId;
   static const TCHAR *m_pszTableNames[] = { _T("nodes"), _T("interfaces"), _T("subnets"),
                                             _T("templates"), _T("network_services"),
                                             _T("containers"), NULL };

   // Try to read information from nodes table
   for(i = 0; (!bRead) && (m_pszTableNames[i] != NULL); i++)
   {
      bIsTemplate = !_tcscmp(m_pszTableNames[i], _T("templates"));
      _sntprintf(szQuery, 1024, _T("SELECT name,is_deleted,image_id%s FROM %s WHERE id=%d"),
                 bIsTemplate ? _T("") : _T(",status"),
                 m_pszTableNames[i], dwId);
      hResult = SQLSelect(szQuery);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            DBGetField(hResult, 0, 0, szName, MAX_OBJECT_NAME);
            bIsDeleted = DBGetFieldLong(hResult, 0, 1) ? TRUE : FALSE;
            dwImageId = DBGetFieldULong(hResult, 0, 2);
            dwStatus = bIsTemplate ? STATUS_UNKNOWN : DBGetFieldULong(hResult, 0, 3);
            bRead = TRUE;
         }
         DBFreeResult(hResult);
      }
      else
      {
         if (!g_bIgnoreErrors)
            return FALSE;
      }
   }

   if (bRead)
   {
      _sntprintf(szQuery, 1024, _T("INSERT INTO object_properties (object_id,name,")
                                   _T("status,is_deleted,image_id,inherit_access_rights,")
                                   _T("last_modified) VALUES (%d,'%s',%d,%d,%d,%d,") INT64_FMT _T(")"),
                 dwId, szName, dwStatus, bIsDeleted, dwImageId, bInheritRights, (INT64)time(NULL));

      if (!SQLQuery(szQuery))
         if (!g_bIgnoreErrors)
            return FALSE;
   }
   else
   {
      _tprintf(_T("WARNING: object with ID %d presented in access control tables but cannot be found in data tables\n"), dwId);
   }

   return TRUE;
}

/**
 * Upgrade from V26 to V27
 */
static BOOL H_UpgradeFromV26(int currVersion, int newVersion)
{
   DB_RESULT hResult;
   DWORD i, dwNumObjects, dwId;
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE nodes DROP COLUMN name\n")
      _T("ALTER TABLE nodes DROP COLUMN status\n")
      _T("ALTER TABLE nodes DROP COLUMN is_deleted\n")
      _T("ALTER TABLE nodes DROP COLUMN image_id\n")
      _T("ALTER TABLE interfaces DROP COLUMN name\n")
      _T("ALTER TABLE interfaces DROP COLUMN status\n")
      _T("ALTER TABLE interfaces DROP COLUMN is_deleted\n")
      _T("ALTER TABLE interfaces DROP COLUMN image_id\n")
      _T("ALTER TABLE subnets DROP COLUMN name\n")
      _T("ALTER TABLE subnets DROP COLUMN status\n")
      _T("ALTER TABLE subnets DROP COLUMN is_deleted\n")
      _T("ALTER TABLE subnets DROP COLUMN image_id\n")
      _T("ALTER TABLE network_services DROP COLUMN name\n")
      _T("ALTER TABLE network_services DROP COLUMN status\n")
      _T("ALTER TABLE network_services DROP COLUMN is_deleted\n")
      _T("ALTER TABLE network_services DROP COLUMN image_id\n")
      _T("ALTER TABLE containers DROP COLUMN name\n")
      _T("ALTER TABLE containers DROP COLUMN status\n")
      _T("ALTER TABLE containers DROP COLUMN is_deleted\n")
      _T("ALTER TABLE containers DROP COLUMN image_id\n")
      _T("ALTER TABLE templates DROP COLUMN name\n")
      _T("ALTER TABLE templates DROP COLUMN is_deleted\n")
      _T("ALTER TABLE templates DROP COLUMN image_id\n")
      _T("DROP TABLE access_options\n")
      _T("DELETE FROM config WHERE var_name='TopologyRootObjectName'\n")
      _T("DELETE FROM config WHERE var_name='TopologyRootImageId'\n")
      _T("DELETE FROM config WHERE var_name='ServiceRootObjectName'\n")
      _T("DELETE FROM config WHERE var_name='ServiceRootImageId'\n")
      _T("DELETE FROM config WHERE var_name='TemplateRootObjectName'\n")
      _T("DELETE FROM config WHERE var_name='TemplateRootImageId'\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
         _T("VALUES (31,'SYS_SNMP_OK',0,1,'Connectivity with SNMP agent restored',")
		   _T("'Generated when connectivity with node#27s SNMP agent restored.#0D#0A")
		   _T("Parameters:#0D#0A   No message-specific parameters')\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
		   _T("VALUES (32,'SYS_AGENT_OK',0,1,'Connectivity with native agent restored',")
		   _T("'Generated when connectivity with node#27s native agent restored.#0D#0A")
		   _T("Parameters:#0D#0A   No message-specific parameters')\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE object_properties (")
	                    _T("object_id integer not null,")
	                    _T("name varchar(63) not null,")
	                    _T("status integer not null,")
	                    _T("is_deleted integer not null,")
	                    _T("image_id integer,")
	                    _T("last_modified integer not null,")
	                    _T("inherit_access_rights integer not null,")
	                    _T("PRIMARY KEY(object_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE user_profiles (")
	                    _T("user_id integer not null,")
	                    _T("var_name varchar(255) not null,")
	                    _T("var_value $SQL:TEXT,")
	                    _T("PRIMARY KEY(user_id,var_name))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   // Move data from access_options and class-specific tables to object_properties
   hResult = SQLSelect(_T("SELECT object_id,inherit_rights FROM access_options"));
   if (hResult != NULL)
   {
      dwNumObjects = DBGetNumRows(hResult);
      for(i = 0; i < dwNumObjects; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         if (dwId >= 10)   // Id below 10 reserved for built-in objects
         {
            if (!MoveObjectData(dwId, DBGetFieldLong(hResult, i, 1) ? TRUE : FALSE))
            {
               DBFreeResult(hResult);
               return FALSE;
            }
         }
         else
         {
            TCHAR szName[MAX_OBJECT_NAME], szQuery[1024];
            DWORD dwImageId;
            BOOL bValidObject = TRUE;

            switch(dwId)
            {
               case 1:     // Topology Root
                  ConfigReadStr(_T("TopologyRootObjectName"), szName,
                                MAX_OBJECT_NAME, _T("Entire Network"));
                  dwImageId = ConfigReadULong(_T("TopologyRootImageId"), 0);
                  break;
               case 2:     // Service Root
                  ConfigReadStr(_T("ServiceRootObjectName"), szName,
                                MAX_OBJECT_NAME, _T("All Services"));
                  dwImageId = ConfigReadULong(_T("ServiceRootImageId"), 0);
                  break;
               case 3:     // Template Root
                  ConfigReadStr(_T("TemplateRootObjectName"), szName,
                                MAX_OBJECT_NAME, _T("All Services"));
                  dwImageId = ConfigReadULong(_T("TemplateRootImageId"), 0);
                  break;
               default:
                  bValidObject = FALSE;
                  break;
            }

            if (bValidObject)
            {
               _sntprintf(szQuery, 1024, _T("INSERT INTO object_properties (object_id,name,")
                                            _T("status,is_deleted,image_id,inherit_access_rights,")
                                            _T("last_modified) VALUES (%d,'%s',5,0,%d,%d,") INT64_FMT _T(")"),
                          dwId, szName, dwImageId,
                          DBGetFieldLong(hResult, i, 1) ? TRUE : FALSE,
                          (INT64)time(NULL));

               if (!SQLQuery(szQuery))
                  if (!g_bIgnoreErrors)
                     return FALSE;
            }
            else
            {
               _tprintf(_T("WARNING: Invalid built-in object ID %d\n"), dwId);
            }
         }
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return FALSE;
   }

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='27' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V25 to V26
//

static BOOL H_UpgradeFromV25(int currVersion, int newVersion)
{
   DB_RESULT hResult;
   TCHAR szTemp[512];

   hResult = SQLSelect(_T("SELECT var_value FROM config WHERE var_name='IDataIndexCreationCommand'"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         if (!CreateConfigParam(_T("IDataIndexCreationCommand_0"),
                                DBGetField(hResult, 0, 0, szTemp, 512), 0, 1))
         {
            if (!g_bIgnoreErrors)
            {
               DBFreeResult(hResult);
               return FALSE;
            }
         }
      }
      DBFreeResult(hResult);

      if (!SQLQuery(_T("DELETE FROM config WHERE var_name='IDataIndexCreationCommand'")))
         if (!g_bIgnoreErrors)
            return FALSE;
   }

   if (!CreateConfigParam(_T("IDataIndexCreationCommand_1"),
                          _T("CREATE INDEX idx_timestamp ON idata_%d(idata_timestamp)"), 0, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='26' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V24 to V25
//

static BOOL H_UpgradeFromV24(int currVersion, int newVersion)
{
   DB_RESULT hResult;
   int i, iNumRows;
   DWORD dwNodeId;
   TCHAR szQuery[256];

   if (GetYesNo(_T("Create indexes on existing IDATA tables?")))
   {
      hResult = SQLSelect(_T("SELECT id FROM nodes WHERE is_deleted=0"));
      if (hResult != NULL)
      {
         iNumRows = DBGetNumRows(hResult);
         for(i = 0; i < iNumRows; i++)
         {
            dwNodeId = DBGetFieldULong(hResult, i, 0);
            _tprintf(_T("Creating indexes for table \"idata_%d\"...\n"), dwNodeId);
            _sntprintf(szQuery, 256, _T("CREATE INDEX idx_timestamp ON idata_%d(idata_timestamp)"), dwNodeId);
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
   }

   if (!SQLQuery(_T("UPDATE config SET var_value='25' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V23 to V24
//

static BOOL H_UpgradeFromV23(int currVersion, int newVersion)
{
   DB_RESULT hResult;
   TCHAR szQuery[256];
   int i, iNumRows;

   if (!CreateTable(_T("CREATE TABLE raw_dci_values (")
                       _T("	item_id integer not null,")
	                    _T("   raw_value varchar(255),")
	                    _T("   last_poll_time integer,")
	                    _T("   PRIMARY KEY(item_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("CREATE INDEX idx_item_id ON raw_dci_values(item_id)")))
      if (!g_bIgnoreErrors)
         return FALSE;


   // Create empty records in raw_dci_values for all existing DCIs
   hResult = SQLSelect(_T("SELECT item_id FROM items"));
   if (hResult != NULL)
   {
      iNumRows = DBGetNumRows(hResult);
      for(i = 0; i < iNumRows; i++)
      {
         _sntprintf(szQuery, 256, _T("INSERT INTO raw_dci_values (item_id,raw_value,last_poll_time) VALUES (%d,'#00',1)"),
                   DBGetFieldULong(hResult, i, 0));
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

   if (!SQLQuery(_T("UPDATE config SET var_value='24' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V22 to V23
//

static BOOL H_UpgradeFromV22(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE items ADD template_item_id integer\n")
      _T("UPDATE items SET template_item_id=0\n")
      _T("CREATE INDEX idx_sequence ON thresholds(sequence_number)\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='23' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V21 to V22
//

static BOOL H_UpgradeFromV21(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
         _T("VALUES (4009,'DC_HDD_TEMP_WARNING',1,1,")
		   _T("'Temperature of hard disk %6 is above warning level of %3 (current: %4)',")
		   _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
      	_T("VALUES (4010,'DC_HDD_TEMP_MAJOR',3,1,")
		   _T("'Temperature of hard disk %6 is above %3 (current: %4)',")
		   _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
	      _T("VALUES (4011,'DC_HDD_TEMP_CRITICAL',4,1,")
		   _T("'Temperature of hard disk %6 is above critical level of %3 (current: %4)',")
		   _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("SMSDriver"), _T("<none>"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("AgentUpgradeWaitTime"), _T("600"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("NumberOfUpgradeThreads"), _T("10"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='22' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V20 to V21
//

static BOOL H_UpgradeFromV20(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)")
         _T(" VALUES (30,'SYS_SMS_FAILURE',1,1,'Unable to send SMS to phone %1',")
		   _T("'Generated when server is unable to send SMS.#0D#0A")
		   _T("   Parameters:#0D#0A   1) Phone number')\n")
      _T("ALTER TABLE nodes ADD node_flags integer\n")
      _T("<END>");
   static TCHAR m_szBatch2[] =
      _T("ALTER TABLE nodes DROP COLUMN is_snmp\n")
      _T("ALTER TABLE nodes DROP COLUMN is_agent\n")
      _T("ALTER TABLE nodes DROP COLUMN is_bridge\n")
      _T("ALTER TABLE nodes DROP COLUMN is_router\n")
      _T("ALTER TABLE nodes DROP COLUMN is_local_mgmt\n")
      _T("ALTER TABLE nodes DROP COLUMN is_ospf\n")
      _T("CREATE INDEX idx_item_id ON thresholds(item_id)\n")
      _T("<END>");
   static DWORD m_dwFlag[] = { 0x00000001, 0x00000002, 0x00000004,
                                 0x00000008, 0x00000010, 0x00000040 };
   DB_RESULT hResult;
   int i, j, iNumRows;
   DWORD dwFlags, dwNodeId;
   TCHAR szQuery[256];

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   // Convert "is_xxx" fields into one _T("node_flags") field
   hResult = SQLSelect(_T("SELECT id,is_snmp,is_agent,is_bridge,is_router,")
                          _T("is_local_mgmt,is_ospf FROM nodes"));
   if (hResult != NULL)
   {
      iNumRows = DBGetNumRows(hResult);
      for(i = 0; i < iNumRows; i++)
      {
         dwFlags = 0;
         for(j = 1; j <= 6; j++)
            if (DBGetFieldLong(hResult, i, j))
               dwFlags |= m_dwFlag[j - 1];
         _sntprintf(szQuery, 256, _T("UPDATE nodes SET node_flags=%d WHERE id=%d"),
                    dwFlags, DBGetFieldULong(hResult, i, 0));
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

   if (!SQLBatch(m_szBatch2))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (GetYesNo(_T("Create indexes on existing IDATA tables?")))
   {
      hResult = SQLSelect(_T("SELECT id FROM nodes WHERE is_deleted=0"));
      if (hResult != NULL)
      {
         iNumRows = DBGetNumRows(hResult);
         for(i = 0; i < iNumRows; i++)
         {
            dwNodeId = DBGetFieldULong(hResult, i, 0);
            _tprintf(_T("Creating indexes for table \"idata_%d\"...\n"), dwNodeId);
            _sntprintf(szQuery, 256, _T("CREATE INDEX idx_item_id ON idata_%d(item_id)"), dwNodeId);
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
   }

   if (!CreateConfigParam(_T("NumberOfStatusPollers"), _T("10"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;
   if (!CreateConfigParam(_T("NumberOfConfigurationPollers"), _T("4"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;
   if (!CreateConfigParam(_T("IDataIndexCreationCommand"), _T("CREATE INDEX idx_item_id ON idata_%d(item_id)"), 0, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='21' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V19 to V20
//

static BOOL H_UpgradeFromV19(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE nodes ADD poller_node_id integer\n")
      _T("ALTER TABLE nodes ADD is_ospf integer\n")
      _T("UPDATE nodes SET poller_node_id=0,is_ospf=0\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)")
         _T(" VALUES (28,'SYS_NODE_DOWN',4,1,'Node down',")
		   _T("'Generated when node is not responding to management server.#0D#0A")
		   _T("Parameters:#0D#0A   No event-specific parameters')\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)")
         _T(" VALUES (29,'SYS_NODE_UP',0,1,'Node up',")
		   _T("'Generated when communication with the node re-established.#0D#0A")
		   _T("Parameters:#0D#0A   No event-specific parameters')\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)")
         _T(" VALUES (25,'SYS_SERVICE_DOWN',3,1,'Network service #22%1#22 is not responding',")
		   _T("'Generated when network service is not responding to management server as ")
         _T("expected.#0D#0AParameters:#0D#0A   1) Service name#0D0A")
		   _T("   2) Service object ID#0D0A   3) Service type')\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)")
         _T(" VALUES (26,'SYS_SERVICE_UP',0,1,")
         _T("'Network service #22%1#22 returned to operational state',")
		   _T("'Generated when network service responds as expected after failure.#0D#0A")
         _T("Parameters:#0D#0A   1) Service name#0D0A")
		   _T("   2) Service object ID#0D0A   3) Service type')\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)")
         _T(" VALUES (27,'SYS_SERVICE_UNKNOWN',1,1,")
		   _T("'Status of network service #22%1#22 is unknown',")
		   _T("'Generated when management server is unable to determine state of the network ")
         _T("service due to agent or server-to-agent communication failure.#0D#0A")
         _T("Parameters:#0D#0A   1) Service name#0D0A")
		   _T("   2) Service object ID#0D0A   3) Service type')\n")
      _T("INSERT INTO images (image_id,name,file_name_png,file_hash_png,file_name_ico,")
         _T("file_hash_ico) VALUES (12,'Obj.NetworkService','network_service.png',")
         _T("'<invalid_hash>','network_service.ico','<invalid_hash>')\n")
      _T("INSERT INTO default_images (object_class,image_id) VALUES (11,12)\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE network_services (")
	                    _T("id integer not null,")
	                    _T("name varchar(63),")
	                    _T("status integer,")
                       _T("is_deleted integer,")
	                    _T("node_id integer not null,")
	                    _T("service_type integer,")
	                    _T("ip_bind_addr varchar(15),")
	                    _T("ip_proto integer,")
	                    _T("ip_port integer,")
	                    _T("check_request $SQL:TEXT,")
	                    _T("check_responce $SQL:TEXT,")
	                    _T("poller_node_id integer not null,")
	                    _T("image_id integer not null,")
	                    _T("PRIMARY KEY(id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='20' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V18 to V19
//

static BOOL H_UpgradeFromV18(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE nodes ADD platform_name varchar(63)\n")
      _T("UPDATE nodes SET platform_name=''\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE agent_pkg (")
	                       _T("pkg_id integer not null,")
	                       _T("pkg_name varchar(63),")
	                       _T("version varchar(31),")
	                       _T("platform varchar(63),")
	                       _T("pkg_file varchar(255),")
	                       _T("description varchar(255),")
	                       _T("PRIMARY KEY(pkg_id))")))
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

static BOOL H_UpgradeFromV17(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE nodes DROP COLUMN inherit_access_rights\n")
      _T("ALTER TABLE nodes ADD agent_version varchar(63)\n")
      _T("UPDATE nodes SET agent_version='' WHERE is_agent=0\n")
      _T("UPDATE nodes SET agent_version='<unknown>' WHERE is_agent=1\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,")
         _T("message,description) SELECT event_id,name,severity,flags,")
         _T("message,description FROM events\n")
      _T("DROP TABLE events\n")
      _T("DROP TABLE event_group_members\n")
      _T("CREATE TABLE event_group_members (group_id integer not null,")
	      _T("event_code integer not null,	PRIMARY KEY(group_id,event_code))\n")
      _T("ALTER TABLE alarms ADD source_event_code integer\n")
      _T("UPDATE alarms SET source_event_code=source_event_id\n")
      _T("ALTER TABLE alarms DROP COLUMN source_event_id\n")
      _T("ALTER TABLE alarms ADD source_event_id bigint\n")
      _T("UPDATE alarms SET source_event_id=0\n")
      _T("ALTER TABLE snmp_trap_cfg ADD event_code integer not null default 0\n")
      _T("UPDATE snmp_trap_cfg SET event_code=event_id\n")
      _T("ALTER TABLE snmp_trap_cfg DROP COLUMN event_id\n")
      _T("DROP TABLE event_log\n")
      _T("CREATE TABLE event_log (event_id bigint not null,event_code integer,")
	      _T("event_timestamp integer,event_source integer,event_severity integer,")
	      _T("event_message varchar(255),root_event_id bigint default 0,")
	      _T("PRIMARY KEY(event_id))\n")
      _T("<END>");
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

      if (!SQLQuery(_T("CREATE TABLE policy_event_list (")
                       _T("rule_id integer not null,")
                       _T("event_code integer not null,")
                       _T("PRIMARY KEY(rule_id,event_code))")))
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
         _sntprintf(szQuery, 4096, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"),
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
      _T("CREATE TABLE event_cfg (event_code integer not null,")
	      _T("event_name varchar(63) not null,severity integer,flags integer,")
	      _T("message varchar(255),description %s,PRIMARY KEY(event_code))"),
              g_pszSqlType[g_dbSyntax][SQL_TYPE_TEXT]);
   if (!SQLQuery(szQuery))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   _sntprintf(szQuery, 4096,
      _T("CREATE TABLE modules (module_id integer not null,")
	      _T("module_name varchar(63),exec_name varchar(255),")
	      _T("module_flags integer not null default 0,description %s,")
	      _T("license_key varchar(255),PRIMARY KEY(module_id))"),
              g_pszSqlType[g_dbSyntax][SQL_TYPE_TEXT]);
   if (!SQLQuery(szQuery))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='18' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}

/**
 * Upgrade from V16 to V17
 */
static BOOL H_UpgradeFromV16(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("DROP TABLE locks\n")
      _T("CREATE TABLE snmp_trap_cfg (trap_id integer not null,snmp_oid varchar(255) not null,")
	      _T("event_id integer not null,description varchar(255),PRIMARY KEY(trap_id))\n")
      _T("CREATE TABLE snmp_trap_pmap (trap_id integer not null,parameter integer not null,")
         _T("snmp_oid varchar(255),description varchar(255),PRIMARY KEY(trap_id,parameter))\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(500, 'SNMP_UNMATCHED_TRAP',	0, 1,	'SNMP trap received: %1 (Parameters: %2)',")
		   _T("'Generated when system receives an SNMP trap without match in trap ")
         _T("configuration table#0D#0AParameters:#0D#0A   1) SNMP trap OID#0D#0A")
		   _T("   2) Trap parameters')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(501, 'SNMP_COLD_START', 0, 1, 'System was cold-started',")
		   _T("'Generated when system receives a coldStart SNMP trap#0D#0AParameters:#0D#0A")
		   _T("   1) SNMP trap OID')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(502, 'SNMP_WARM_START', 0, 1, 'System was warm-started',")
		   _T("'Generated when system receives a warmStart SNMP trap#0D#0A")
		   _T("Parameters:#0D#0A   1) SNMP trap OID')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(503, 'SNMP_LINK_DOWN', 3, 1, 'Link is down',")
		   _T("'Generated when system receives a linkDown SNMP trap#0D#0A")
		   _T("Parameters:#0D#0A   1) SNMP trap OID#0D#0A   2) Interface index')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(504, 'SNMP_LINK_UP', 0, 1, 'Link is up',")
		   _T("'Generated when system receives a linkUp SNMP trap#0D#0AParameters:#0D#0A")
		   _T("   1) SNMP trap OID#0D#0A   2) Interface index')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(505, 'SNMP_AUTH_FAILURE', 1, 1, 'SNMP authentication failure',")
		   _T("'Generated when system receives an authenticationFailure SNMP trap#0D#0A")
		   _T("Parameters:#0D#0A   1) SNMP trap OID')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(506, 'SNMP_EGP_NEIGHBOR_LOSS',	1, 1,	'EGP neighbor loss',")
		   _T("'Generated when system receives an egpNeighborLoss SNMP trap#0D#0A")
		   _T("Parameters:#0D#0A   1) SNMP trap OID')\n")
      _T("INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) ")
         _T("VALUES (1,'.1.3.6.1.6.3.1.1.5.1',501,'Generic coldStart trap')\n")
      _T("INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) ")
         _T("VALUES (2,'.1.3.6.1.6.3.1.1.5.2',502,'Generic warmStart trap')\n")
      _T("INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) ")
         _T("VALUES (3,'.1.3.6.1.6.3.1.1.5.3',503,'Generic linkDown trap')\n")
      _T("INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) ")
         _T("VALUES (4,'.1.3.6.1.6.3.1.1.5.4',504,'Generic linkUp trap')\n")
      _T("INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) ")
         _T("VALUES (5,'.1.3.6.1.6.3.1.1.5.5',505,'Generic authenticationFailure trap')\n")
      _T("INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) ")
         _T("VALUES (6,'.1.3.6.1.6.3.1.1.5.6',506,'Generic egpNeighborLoss trap')\n")
      _T("INSERT INTO snmp_trap_pmap (trap_id,parameter,snmp_oid,description) ")
         _T("VALUES (3,1,'.1.3.6.1.2.1.2.2.1.1','Interface index')\n")
      _T("INSERT INTO snmp_trap_pmap (trap_id,parameter,snmp_oid,description) ")
         _T("VALUES (4,1,'.1.3.6.1.2.1.2.2.1.1','Interface index')\n")
      _T("<END>");

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

/**
 * Upgrade from V15 to V16
 */
static BOOL H_UpgradeFromV15(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(4005, 'DC_MAILBOX_TOO_LARGE', 1, 1,")
		   _T("'Mailbox #22%6#22 exceeds size limit (allowed size: %3; actual size: %4)',")
		   _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(4006, 'DC_AGENT_VERSION_CHANGE',	0, 1,")
         _T("'NetXMS agent version was changed from %3 to %4',")
         _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(4007, 'DC_HOSTNAME_CHANGE',	1, 1,")
         _T("'Host name was changed from %3 to %4',")
         _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(4008, 'DC_FILE_CHANGE',	1, 1,")
         _T("'File #22%6#22 was changed',")
         _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;
   if (!SQLQuery(_T("UPDATE config SET var_value='16' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;
   return TRUE;
}

/**
 * Upgrade from V14 to V15
 */
static BOOL H_UpgradeFromV14(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE items ADD instance varchar(255)\n")
      _T("UPDATE items SET instance=''\n")
      _T("INSERT INTO config (var_name,var_value,is_visible,need_server_restart) VALUES ")
         _T("('SMTPServer','localhost',1,0)\n")
      _T("INSERT INTO config (var_name,var_value,is_visible,need_server_restart) VALUES ")
         _T("('SMTPFromAddr','netxms@localhost',1,0)\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(4003, 'DC_AGENT_RESTARTED',	0, 1,")
         _T("'NetXMS agent was restarted within last 5 minutes',")
         _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(4004, 'DC_SERVICE_NOT_RUNNING', 3, 1,")
		   _T("'Service #22%6#22 is not running',")
		   _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("UPDATE events SET ")
         _T("description='Generated when threshold value reached ")
         _T("for specific data collection item.#0D#0A")
         _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance' WHERE ")
         _T("event_id=17 OR (event_id>=4000 AND event_id<5000)\n")
      _T("UPDATE events SET ")
         _T("description='Generated when threshold check is rearmed ")
         _T("for specific data collection item.#0D#0A")
		   _T("Parameters:#0D#0A   1) Parameter name#0D#0A")
		   _T("   2) Item description#0D#0A   3) Data collection item ID' ")
         _T("WHERE event_id=18\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;
   if (!SQLQuery(_T("UPDATE config SET var_value='15' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;
   return TRUE;
}

/**
 * Upgrade map
 */
static struct
{
   int version;
   int newVersion;
   BOOL (* fpProc)(int, int);
} m_dbUpgradeMap[] =
{
   { 14, 15, H_UpgradeFromV14 },
   { 15, 16, H_UpgradeFromV15 },
   { 16, 17, H_UpgradeFromV16 },
   { 17, 18, H_UpgradeFromV17 },
   { 18, 19, H_UpgradeFromV18 },
   { 19, 20, H_UpgradeFromV19 },
   { 20, 21, H_UpgradeFromV20 },
   { 21, 22, H_UpgradeFromV21 },
   { 22, 23, H_UpgradeFromV22 },
   { 23, 24, H_UpgradeFromV23 },
   { 24, 25, H_UpgradeFromV24 },
   { 25, 26, H_UpgradeFromV25 },
   { 26, 27, H_UpgradeFromV26 },
   { 27, 28, H_UpgradeFromV27 },
   { 28, 29, H_UpgradeFromV28 },
   { 29, 30, H_UpgradeFromV29 },
   { 30, 31, H_UpgradeFromV30 },
   { 31, 32, H_UpgradeFromV31 },
   { 32, 33, H_UpgradeFromV32 },
   { 33, 34, H_UpgradeFromV33 },
   { 34, 35, H_UpgradeFromV34 },
   { 35, 36, H_UpgradeFromV35 },
   { 36, 37, H_UpgradeFromV36 },
   { 37, 38, H_UpgradeFromV37 },
   { 38, 39, H_UpgradeFromV38 },
   { 39, 40, H_UpgradeFromV39 },
   { 40, 41, H_UpgradeFromV40 },
   { 41, 42, H_UpgradeFromV41 },
   { 42, 43, H_UpgradeFromV42 },
   { 43, 44, H_UpgradeFromV43 },
   { 44, 45, H_UpgradeFromV44 },
   { 45, 46, H_UpgradeFromV45 },
   { 46, 47, H_UpgradeFromV46 },
   { 47, 48, H_UpgradeFromV47 },
   { 48, 49, H_UpgradeFromV48 },
   { 49, 50, H_UpgradeFromV49 },
   { 50, 51, H_UpgradeFromV50 },
   { 51, 52, H_UpgradeFromV51 },
   { 52, 53, H_UpgradeFromV52 },
   { 53, 54, H_UpgradeFromV53 },
   { 54, 55, H_UpgradeFromV54 },
   { 55, 56, H_UpgradeFromV55 },
   { 56, 57, H_UpgradeFromV56 },
   { 57, 58, H_UpgradeFromV57 },
   { 58, 59, H_UpgradeFromV58 },
   { 59, 60, H_UpgradeFromV59 },
   { 60, 61, H_UpgradeFromV60 },
   { 61, 62, H_UpgradeFromV61 },
   { 62, 63, H_UpgradeFromV62 },
   { 63, 64, H_UpgradeFromV63 },
   { 64, 65, H_UpgradeFromV64 },
   { 65, 66, H_UpgradeFromV65 },
   { 66, 67, H_UpgradeFromV66 },
   { 67, 68, H_UpgradeFromV67 },
   { 68, 69, H_UpgradeFromV68 },
   { 69, 70, H_UpgradeFromV69 },
   { 70, 71, H_UpgradeFromV70 },
   { 71, 72, H_UpgradeFromV71 },
   { 72, 73, H_UpgradeFromV72 },
   { 73, 74, H_UpgradeFromV73 },
   { 74, 75, H_UpgradeFromV74 },
   { 75, 76, H_UpgradeFromV75 },
   { 76, 77, H_UpgradeFromV76 },
   { 77, 78, H_UpgradeFromV77 },
   { 78, 79, H_UpgradeFromV78 },
   { 79, 80, H_UpgradeFromV79 },
   { 80, 81, H_UpgradeFromV80 },
   { 81, 82, H_UpgradeFromV81 },
   { 82, 83, H_UpgradeFromV82 },
   { 83, 84, H_UpgradeFromV83 },
   { 84, 85, H_UpgradeFromV84 },
	{ 85, 86, H_UpgradeFromV85 },
	{ 86, 87, H_UpgradeFromV86 },
	{ 87, 88, H_UpgradeFromV87 },
	{ 88, 89, H_UpgradeFromV88 },
	{ 89, 90, H_UpgradeFromV89 },
	{ 90, 91, H_UpgradeFromV90 },
	{ 91, 92, H_UpgradeFromV91 },
	{ 92, 200, H_UpgradeFromV9x },
	{ 93, 201, H_UpgradeFromV9x },
	{ 94, 202, H_UpgradeFromV9x },
	{ 95, 203, H_UpgradeFromV9x },
	{ 96, 204, H_UpgradeFromV9x },
	{ 97, 205, H_UpgradeFromV9x },
	{ 98, 206, H_UpgradeFromV9x },
	{ 99, 207, H_UpgradeFromV9x },
	{ 100, 214, H_UpgradeFromV10x },
	{ 101, 214, H_UpgradeFromV10x },
	{ 102, 214, H_UpgradeFromV10x },
	{ 103, 214, H_UpgradeFromV10x },
	{ 104, 214, H_UpgradeFromV10x },
	{ 105, 217, H_UpgradeFromV105 },
	{ 200, 201, H_UpgradeFromV200 },
	{ 201, 202, H_UpgradeFromV201 },
	{ 202, 203, H_UpgradeFromV202 },
	{ 203, 204, H_UpgradeFromV203 },
	{ 204, 205, H_UpgradeFromV204 },
	{ 205, 206, H_UpgradeFromV205 },
	{ 206, 207, H_UpgradeFromV206 },
	{ 207, 208, H_UpgradeFromV207 },
	{ 208, 209, H_UpgradeFromV208 },
	{ 209, 210, H_UpgradeFromV209 },
	{ 210, 211, H_UpgradeFromV210 },
	{ 211, 212, H_UpgradeFromV211 },
	{ 212, 213, H_UpgradeFromV212 },
	{ 213, 214, H_UpgradeFromV213 },
	{ 214, 215, H_UpgradeFromV214 },
	{ 215, 216, H_UpgradeFromV215 },
	{ 216, 217, H_UpgradeFromV216 },
	{ 217, 218, H_UpgradeFromV217 },
	{ 218, 219, H_UpgradeFromV218 },
	{ 219, 220, H_UpgradeFromV219 },
	{ 220, 221, H_UpgradeFromV220 },
	{ 221, 222, H_UpgradeFromV221 },
	{ 222, 223, H_UpgradeFromV222 },
	{ 223, 224, H_UpgradeFromV223 },
	{ 224, 225, H_UpgradeFromV224 },
	{ 225, 226, H_UpgradeFromV225 },
	{ 226, 227, H_UpgradeFromV226 },
	{ 227, 228, H_UpgradeFromV227 },
	{ 228, 229, H_UpgradeFromV228 },
	{ 229, 230, H_UpgradeFromV229 },
	{ 230, 231, H_UpgradeFromV230 },
	{ 231, 232, H_UpgradeFromV231 },
	{ 232, 238, H_UpgradeFromV232toV238 },
	{ 233, 234, H_UpgradeFromV233 },
	{ 234, 235, H_UpgradeFromV234 },
	{ 235, 236, H_UpgradeFromV235 },
	{ 236, 237, H_UpgradeFromV236 },
	{ 237, 238, H_UpgradeFromV237 },
	{ 238, 239, H_UpgradeFromV238 },
	{ 239, 240, H_UpgradeFromV239 },
	{ 240, 241, H_UpgradeFromV240 },
	{ 241, 242, H_UpgradeFromV241 },
	{ 242, 243, H_UpgradeFromV242 },
	{ 243, 244, H_UpgradeFromV243 },
	{ 244, 245, H_UpgradeFromV244 },
	{ 245, 246, H_UpgradeFromV245 },
	{ 246, 247, H_UpgradeFromV246 },
	{ 247, 248, H_UpgradeFromV247 },
	{ 248, 249, H_UpgradeFromV248 },
	{ 249, 250, H_UpgradeFromV249 },
	{ 250, 251, H_UpgradeFromV250 },
	{ 251, 252, H_UpgradeFromV251 },
	{ 252, 253, H_UpgradeFromV252 },
	{ 253, 254, H_UpgradeFromV253 },
	{ 254, 255, H_UpgradeFromV254 },
	{ 255, 256, H_UpgradeFromV255 },
	{ 256, 257, H_UpgradeFromV256 },
	{ 257, 258, H_UpgradeFromV257 },
	{ 258, 259, H_UpgradeFromV258 },
	{ 259, 260, H_UpgradeFromV259 },
	{ 260, 261, H_UpgradeFromV260 },
	{ 261, 262, H_UpgradeFromV261 },
	{ 262, 263, H_UpgradeFromV262 },
	{ 263, 264, H_UpgradeFromV263 },
	{ 264, 265, H_UpgradeFromV264 },
	{ 265, 266, H_UpgradeFromV265 },
	{ 266, 267, H_UpgradeFromV266 },
	{ 267, 268, H_UpgradeFromV267 },
	{ 268, 269, H_UpgradeFromV268 },
	{ 269, 270, H_UpgradeFromV269 },
   { 270, 271, H_UpgradeFromV270 },
   { 271, 272, H_UpgradeFromV271 },
   { 272, 273, H_UpgradeFromV272 },
   { 273, 274, H_UpgradeFromV273 },
   { 274, 275, H_UpgradeFromV274 },
   { 275, 276, H_UpgradeFromV275 },
   { 276, 277, H_UpgradeFromV276 },
   { 277, 278, H_UpgradeFromV277 },
   { 278, 279, H_UpgradeFromV278 },
   { 279, 280, H_UpgradeFromV279 },
   { 280, 281, H_UpgradeFromV280 },
   { 281, 282, H_UpgradeFromV281 },
   { 282, 283, H_UpgradeFromV282 },
   { 283, 284, H_UpgradeFromV283 },
   { 284, 285, H_UpgradeFromV284 },
   { 285, 286, H_UpgradeFromV285 },
   { 286, 287, H_UpgradeFromV286 },
   { 287, 288, H_UpgradeFromV287 },
   { 288, 289, H_UpgradeFromV288 },
   { 289, 290, H_UpgradeFromV289 },
   { 290, 291, H_UpgradeFromV290 },
   { 291, 292, H_UpgradeFromV291 },
   { 292, 293, H_UpgradeFromV292 },
   { 293, 294, H_UpgradeFromV293 },
   { 294, 295, H_UpgradeFromV294 },
   { 295, 296, H_UpgradeFromV295 },
   { 296, 297, H_UpgradeFromV296 },
   { 297, 298, H_UpgradeFromV297 },
   { 298, 299, H_UpgradeFromV298 },
   { 299, 300, H_UpgradeFromV299 },
   { 300, 301, H_UpgradeFromV300 },
   { 301, 302, H_UpgradeFromV301 },
   { 302, 303, H_UpgradeFromV302 },
   { 303, 304, H_UpgradeFromV303 },
   { 304, 305, H_UpgradeFromV304 },
   { 305, 306, H_UpgradeFromV305 },
   { 306, 307, H_UpgradeFromV306 },
   { 307, 308, H_UpgradeFromV307 },
   { 308, 309, H_UpgradeFromV308 },
   { 309, 310, H_UpgradeFromV309 },
   { 310, 311, H_UpgradeFromV310 },
   { 311, 312, H_UpgradeFromV311 },
   { 312, 313, H_UpgradeFromV312 },
   { 313, 314, H_UpgradeFromV313 },
   { 314, 315, H_UpgradeFromV314 },
   { 315, 316, H_UpgradeFromV315 },
   { 316, 317, H_UpgradeFromV316 },
   { 317, 318, H_UpgradeFromV317 },
   { 318, 319, H_UpgradeFromV318 },
   { 319, 320, H_UpgradeFromV319 },
   { 320, 321, H_UpgradeFromV320 },
   { 321, 322, H_UpgradeFromV321 },
   { 322, 323, H_UpgradeFromV322 },
   { 323, 324, H_UpgradeFromV323 },
   { 324, 325, H_UpgradeFromV324 },
   { 325, 326, H_UpgradeFromV325 },
   { 326, 327, H_UpgradeFromV326 },
   { 327, 328, H_UpgradeFromV327 },
   { 328, 329, H_UpgradeFromV328 },
   { 329, 330, H_UpgradeFromV329 },
   { 330, 331, H_UpgradeFromV330 },
   { 331, 332, H_UpgradeFromV331 },
   { 332, 333, H_UpgradeFromV332 },
   { 333, 334, H_UpgradeFromV333 },
   { 334, 335, H_UpgradeFromV334 },
   { 335, 336, H_UpgradeFromV335 },
   { 336, 337, H_UpgradeFromV336 },
   { 337, 338, H_UpgradeFromV337 },
   { 338, 339, H_UpgradeFromV338 },
   { 339, 340, H_UpgradeFromV339 },
   { 340, 341, H_UpgradeFromV340 },
   { 341, 342, H_UpgradeFromV341 },
   { 342, 343, H_UpgradeFromV342 },
   { 343, 344, H_UpgradeFromV343 },
   { 344, 345, H_UpgradeFromV344 },
   { 345, 346, H_UpgradeFromV345 },
   { 346, 347, H_UpgradeFromV346 },
   { 347, 348, H_UpgradeFromV347 },
   { 348, 349, H_UpgradeFromV348 },
   { 349, 350, H_UpgradeFromV349 },
   { 350, 351, H_UpgradeFromV350 },
   { 351, 352, H_UpgradeFromV351 },
   { 352, 353, H_UpgradeFromV352 },
   { 353, 354, H_UpgradeFromV353 },
   { 354, 355, H_UpgradeFromV354 },
   { 355, 356, H_UpgradeFromV355 },
   { 356, 357, H_UpgradeFromV356 },
   { 357, 358, H_UpgradeFromV357 },
   { 358, 359, H_UpgradeFromV358 },
   { 359, 360, H_UpgradeFromV359 },
   { 360, 361, H_UpgradeFromV360 },
   { 361, 362, H_UpgradeFromV361 },
   { 362, 363, H_UpgradeFromV362 },
   { 363, 364, H_UpgradeFromV363 },
   { 364, 365, H_UpgradeFromV364 },
   { 365, 366, H_UpgradeFromV365 },
   { 366, 367, H_UpgradeFromV366 },
   { 367, 368, H_UpgradeFromV367 },
   { 368, 369, H_UpgradeFromV368 },
   { 369, 370, H_UpgradeFromV369 },
   { 370, 371, H_UpgradeFromV370 },
   { 371, 372, H_UpgradeFromV371 },
   { 372, 373, H_UpgradeFromV372 },
   { 373, 374, H_UpgradeFromV373 },
   { 374, 375, H_UpgradeFromV374 },
   { 375, 376, H_UpgradeFromV375 },
   { 376, 377, H_UpgradeFromV376 },
   { 377, 378, H_UpgradeFromV377 },
   { 378, 379, H_UpgradeFromV378 },
   { 379, 380, H_UpgradeFromV379 },
   { 380, 381, H_UpgradeFromV380 },
   { 381, 382, H_UpgradeFromV381 },
   { 382, 383, H_UpgradeFromV382 },
   { 383, 384, H_UpgradeFromV383 },
   { 384, 385, H_UpgradeFromV384 },
   { 385, 386, H_UpgradeFromV385 },
   { 386, 387, H_UpgradeFromV386 },
   { 387, 388, H_UpgradeFromV387 },
   { 388, 389, H_UpgradeFromV388 },
   { 389, 390, H_UpgradeFromV389 },
   { 390, 391, H_UpgradeFromV390 },
   { 391, 392, H_UpgradeFromV391 },
   { 392, 393, H_UpgradeFromV392 },
   { 393, 394, H_UpgradeFromV393 },
   { 394, 395, H_UpgradeFromV394 },
   { 395, 396, H_UpgradeFromV395 },
   { 396, 397, H_UpgradeFromV396 },
   { 397, 398, H_UpgradeFromV397 },
   { 398, 399, H_UpgradeFromV398 },
   { 399, 400, H_UpgradeFromV399 },
   { 400, 401, H_UpgradeFromV400 },
   { 401, 402, H_UpgradeFromV401 },
   { 402, 403, H_UpgradeFromV402 },
   { 403, 404, H_UpgradeFromV403 },
   { 404, 405, H_UpgradeFromV404 },
   { 405, 406, H_UpgradeFromV405 },
   { 406, 407, H_UpgradeFromV406 },
   { 407, 408, H_UpgradeFromV407 },
   { 408, 409, H_UpgradeFromV408 },
   { 409, 410, H_UpgradeFromV409 },
   { 410, 411, H_UpgradeFromV410 },
   { 411, 412, H_UpgradeFromV411 },
   { 412, 413, H_UpgradeFromV412 },
   { 413, 414, H_UpgradeFromV413 },
   { 414, 415, H_UpgradeFromV414 },
   { 415, 416, H_UpgradeFromV415 },
   { 416, 417, H_UpgradeFromV416 },
   { 417, 418, H_UpgradeFromV417 },
   { 418, 419, H_UpgradeFromV418 },
   { 419, 420, H_UpgradeFromV419 },
   { 420, 421, H_UpgradeFromV420 },
   { 421, 422, H_UpgradeFromV421 },
   { 422, 423, H_UpgradeFromV422 },
   { 423, 424, H_UpgradeFromV423 },
   { 424, 425, H_UpgradeFromV424 },
   { 425, 426, H_UpgradeFromV425 },
   { 426, 427, H_UpgradeFromV426 },
   { 427, 428, H_UpgradeFromV427 },
   { 428, 429, H_UpgradeFromV428 },
   { 429, 430, H_UpgradeFromV429 },
   { 430, 431, H_UpgradeFromV430 },
   { 431, 432, H_UpgradeFromV431 },
   { 432, 433, H_UpgradeFromV432 },
   { 433, 434, H_UpgradeFromV433 },
   { 434, 435, H_UpgradeFromV434 },
   { 435, 436, H_UpgradeFromV435 },
   { 436, 437, H_UpgradeFromV436 },
   { 437, 438, H_UpgradeFromV437 },
   { 438, 439, H_UpgradeFromV438 },
   { 439, 440, H_UpgradeFromV439 },
   { 440, 441, H_UpgradeFromV440 },
   { 441, 442, H_UpgradeFromV441 },
   { 442, 443, H_UpgradeFromV442 },
   { 443, 444, H_UpgradeFromV443 },
   { 444, 445, H_UpgradeFromV444 },
   { 445, 446, H_UpgradeFromV445 },
   { 446, 447, H_UpgradeFromV446 },
   { 447, 448, H_UpgradeFromV447 },
   { 448, 449, H_UpgradeFromV448 },
   { 449, 450, H_UpgradeFromV449 },
   { 450, 451, H_UpgradeFromV450 },
   { 451, 452, H_UpgradeFromV451 },
   { 452, 453, H_UpgradeFromV452 },
   { 453, 454, H_UpgradeFromV453 },
   { 454, 455, H_UpgradeFromV454 },
   { 455, 456, H_UpgradeFromV455 },
   { 456, 457, H_UpgradeFromV456 },
   { 457, 458, H_UpgradeFromV457 },
   { 458, 459, H_UpgradeFromV458 },
   { 459, 700, H_UpgradeFromV459 },
   { 460, 700, H_UpgradeFromV460 },
   { 461, 700, H_UpgradeFromV461 },
   { 462, 700, H_UpgradeFromV462 },
   { 0, 0, NULL }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V0()
{
   INT32 major, version;
   if (!DBGetSchemaVersion(g_hCoreDB, &major, &version))
      return false;

   while((major == 0) && (version < DB_LEGACY_SCHEMA_VERSION))
   {
      // Find upgrade procedure
      int i;
      for(i = 0; m_dbUpgradeMap[i].fpProc != NULL; i++)
         if (m_dbUpgradeMap[i].version == version)
            break;
      if (m_dbUpgradeMap[i].fpProc == NULL)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 0.%d\n"), version);
         return false;
      }
      _tprintf(_T("Upgrading from version 0.%d to 0.%d\n"), version, m_dbUpgradeMap[i].newVersion);
      DBBegin(g_hCoreDB);
      if (m_dbUpgradeMap[i].fpProc(version, m_dbUpgradeMap[i].newVersion))
      {
         DBCommit(g_hCoreDB);
         if (!DBGetSchemaVersion(g_hCoreDB, &major, &version))
            return false;
      }
      else
      {
         _tprintf(_T("Rolling back last stage due to upgrade errors...\n"));
         DBRollback(g_hCoreDB);
         return false;
      }
   }
   return true;
}
