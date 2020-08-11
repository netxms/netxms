/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2020 Raden Solutions
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
** File: upgrade_v34.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 34.16 to 35.0
 */
static bool H_UpgradeFromV16()
{
   CHK_EXEC(SetMajorSchemaVersion(35, 0));
   return true;
}

/**
 * Upgrade from 34.15 to 34.16
 */
static bool H_UpgradeFromV15()
{
   CHK_EXEC(CreateConfigParam(_T("FirstFreeDCIId"), _T("1"), 0, 1, FALSE));

   CHK_EXEC(CreateTable(
         _T("CREATE TABLE dci_delete_list (")
         _T("node_id integer not null,")
         _T("dci_id integer not null,")
         _T("type char(1) not null,")
         _T("PRIMARY KEY (node_id,dci_id))")));

   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Upgrade from 34.14 to 34.15
 */
static bool H_UpgradeFromV14()
{
   CHK_EXEC(CreateConfigParam(_T("DBWriter.HouseKeeperInterlock"), _T("0"), _T("Controls if server should block background write of collected performance data while housekeeper deletes expired records."), nullptr, 'C', true, false, false, false));

   static const TCHAR *batch =
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('DBWriter.HouseKeeperInterlock','0','Auto')\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('DBWriter.HouseKeeperInterlock','1','Off')\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('DBWriter.HouseKeeperInterlock','2','On')\n")
         _T("UPDATE config_values SET var_name='LDAP.UserDeleteAction' WHERE var_name='LdapUserDeleteAction'\n")
         _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade from 34.13 to 34.14
 */
static bool H_UpgradeFromV13()
{
   CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("snmp_ports")));
   CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("snmp_ports"), _T("id,zone")));

   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Upgrade from 34.12 to 34.13
 */
static bool H_UpgradeFromV12()
{
   CHK_EXEC(SQLQuery(_T("INSERT INTO metadata (var_name,var_value) VALUES ('PruneCustomAttributes', '1')"))); //Set flag to deduplicate custom attributes

   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}


/**
 * Upgrade from 34.11 to 34.12
 */
static bool H_UpgradeFromV11()
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET description='Allows/disallows conversion of SNMP trap OCTET STRING varbinds into hex strings if they contain non-printable characters.' WHERE var_name='AllowTrapVarbindsConversion'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET description='Time when housekeeper starts. Housekeeper deletes expired log records and DCI data as well as cleans removed objects.' WHERE var_name='Housekeeper.StartTime'")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name IN ('AnonymousFileAccess','LockTimeout')")));
   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 34.10 to 34.11
 */
static bool H_UpgradeFromV10()
{
   CHK_EXEC(CreateEventTemplate(EVENT_LICENSE_PROBLEM, _T("SYS_LICENSE_PROBLEM"),
            SEVERITY_WARNING, EF_LOG, _T("ea282f6f-c636-414f-91ce-0aa871648144"),
            _T("License problem - %1"),
            _T("Generated when licensing problem is detected by extension module.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Problem description")
            ));
   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 34.9 to 34.10
 */
static bool H_UpgradeFromV9()
{
   CHK_EXEC(CreateConfigParam(_T("Scheduler.TaskRetentionTime"), _T("86400"), _T("Retention time for completed non-recurrent scheduled tasks. Such tasks will be deleted automatically after given number of seconds since completion time."), _T("seconds"), 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 34.8 to 34.9
 */
static bool H_UpgradeFromV8()
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='SMTP.FromAddr' WHERE var_name='SMTPFromAddr'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='SMTP.FromName' WHERE var_name='SMTPFromName'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='SMTP.Port' WHERE var_name='SMTPPort'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='SMTP.RetryCount' WHERE var_name='SMTPRetryCount'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='SMTP.Server' WHERE var_name='SMTPServer'")));
   CHK_EXEC(CreateConfigParam(_T("SMTP.LocalHostName"), _T(""), _T("Local host name used in HELO command. If empty then fully qualified name of local system will be used."), nullptr, 'S', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 34.7 to 34.8
 */
static bool H_UpgradeFromV7()
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='Objects.Nodes.CapabilityExpirationTime',description='Time before capability (NetXMS Agent, SNMP, EtherNet/IP) expires if node is not responding for requests via appropriate protocol.' WHERE var_name='CapabilityExpirationTime'")));
   CHK_EXEC(CreateConfigParam(_T("Objects.Nodes.CapabilityExpirationGracePeriod"), _T("3600"), _T("Grace period for capability expiration after node recovered from unreachable state."), _T("seconds"), 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 34.6 to 34.7
 */
static bool H_UpgradeFromV6()
{
   static const TCHAR *batch =
      _T("ALTER TABLE user_agent_notifications ADD creation_time integer\n")
      _T("ALTER TABLE user_agent_notifications ADD created_by integer\n")
      _T("UPDATE user_agent_notifications SET created_by=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   //update one time notifications to delete on housekeeper old notifications
   time_t now = time(nullptr);
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("UPDATE user_agent_notifications SET creation_time=%u WHERE end_time=0"), (uint32_t)now);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("user_agent_notifications"), _T("creation_time")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("user_agent_notifications"), _T("created_by")));

   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 34.5 to 34.6
 */
static bool H_UpgradeFromV5()
{
   CHK_EXEC(SQLQuery(
         _T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
         _T("VALUES ('818f0711-1b0e-42ec-8d28-1298d18be2e9',22,'Hook::AlarmStateChange','")
         _T("/* Available global variables:\r\n")
         _T(" *  $alarm - alarm being processed (object of ''Alarm'' class)\r\n")
         _T(" *\r\n")
         _T(" * Expected return value:\r\n")
         _T(" *  none - returned value is ignored\r\n */\r\n')")));
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 34.4 to 34.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(CreateConfigParam(_T("Objects.Nodes.FallbackToLocalResolver"), _T("0"), _T("Fallback to server's local resolver if node address cannot be resolved via zone proxy."), nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 34.3 to 34.4
 */
static bool H_UpgradeFromV3()
{
   // Recreate index for those who initialized database with an incorrect init script
   CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("snmp_communities")));
   CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("usm_credentials")));
   CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("shared_secrets")));

   CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("snmp_communities"), _T("id,zone")));
   CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("usm_credentials"), _T("id,zone")));
   CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("shared_secrets"),  _T("id,zone")));

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 34.2 to 34.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("audit_log"), _T("value_diff")));

   if (g_dbSyntax == DB_SYNTAX_ORACLE)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE audit_log ADD (value_type char(1), hmac varchar(64))")));
   }
   else
   {
      static const TCHAR *batch =
         _T("ALTER TABLE audit_log ADD value_type char(1)\n")
         _T("ALTER TABLE audit_log ADD hmac varchar(64)\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
   }

   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 34.1 to 34.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(ConvertStrings(_T("conditions"), _T("id"), _T("script")));
   CHK_EXEC(ConvertStrings(_T("agent_configs"), _T("config_id"), _T("config_name")));
   CHK_EXEC(ConvertStrings(_T("agent_configs"), _T("config_id"), _T("config_file")));
   CHK_EXEC(ConvertStrings(_T("agent_configs"), _T("config_id"), _T("config_filter")));
   CHK_EXEC(ConvertStrings(_T("certificates"), _T("cert_id"), _T("subject")));
   CHK_EXEC(ConvertStrings(_T("certificates"), _T("cert_id"), _T("comments")));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 34.0 to 34.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(ConvertStrings(_T("agent_pkg"), _T("pkg_id"), _T("description")));
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
   bool (*upgradeProc)();
} s_dbUpgradeMap[] =
{
   { 16, 35, 0,  H_UpgradeFromV16 },
   { 15, 34, 16, H_UpgradeFromV15 },
   { 14, 34, 15, H_UpgradeFromV14 },
   { 13, 34, 14, H_UpgradeFromV13 },
   { 12, 34, 13, H_UpgradeFromV12 },
   { 11, 34, 12, H_UpgradeFromV11 },
   { 10, 34, 11, H_UpgradeFromV10 },
   { 9,  34, 10, H_UpgradeFromV9  },
   { 8,  34, 9,  H_UpgradeFromV8  },
   { 7,  34, 8,  H_UpgradeFromV7  },
   { 6,  34, 7,  H_UpgradeFromV6  },
   { 5,  34, 6,  H_UpgradeFromV5  },
   { 4,  34, 5,  H_UpgradeFromV4  },
   { 3,  34, 4,  H_UpgradeFromV3  },
   { 2,  34, 3,  H_UpgradeFromV2  },
   { 1,  34, 2,  H_UpgradeFromV1  },
   { 0,  34, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr          }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V34()
{
   INT32 major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while(major == 34)
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 34.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 34.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
