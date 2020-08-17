/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2020 Raden Solutions
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
** File: upgrade_v40.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Update group ID in given table
 */
bool UpdateGroupId(const TCHAR *table, const TCHAR *column);

/**
 * Upgrade from 40.4 to 40.5
 */
static bool H_UpgradeFromV4()
{
   if (GetSchemaLevelForMajorVersion(35) < 10)
   {
      CHK_EXEC(UpdateGroupId(_T("user_groups"), _T("id")));
      CHK_EXEC(UpdateGroupId(_T("user_group_members"), _T("group_id")));
      CHK_EXEC(UpdateGroupId(_T("userdb_custom_attributes"), _T("object_id")));
      CHK_EXEC(UpdateGroupId(_T("acl"), _T("user_id")));
      CHK_EXEC(UpdateGroupId(_T("dci_access"), _T("user_id")));
      CHK_EXEC(UpdateGroupId(_T("alarm_category_acl"), _T("user_id")));
      CHK_EXEC(UpdateGroupId(_T("object_tools_acl"), _T("user_id")));
      CHK_EXEC(UpdateGroupId(_T("graph_acl"), _T("user_id")));
      CHK_EXEC(UpdateGroupId(_T("object_access_snapshot"), _T("user_id")));
      CHK_EXEC(UpdateGroupId(_T("responsible_users"), _T("user_id")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 9));
   }
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 40.3 to 40.4
 */
static bool H_UpgradeFromV3()
{
   if (GetSchemaLevelForMajorVersion(35) < 9)
   {
      CHK_EXEC(CreateConfigParam(_T("SNMP.Traps.RateLimit.Threshold"), _T("0"), _T("Threshold for number of SNMP traps per second that defines SNMP trap flood condition. Detection is disabled if 0 is set."), _T("seconds"), 'I', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("SNMP.Traps.RateLimit.Duration"), _T("15"), _T("Time period for SNMP traps per second to be above threshold that defines SNMP trap flood condition."), _T("seconds"), 'I', true, false, false, false));

      CHK_EXEC(CreateEventTemplate(EVENT_SNMP_TRAP_FLOOD_DETECTED, _T("SNMP_TRAP_FLOOD_DETECTED"),
               SEVERITY_MAJOR, EF_LOG, _T("6b2bb689-23b7-4e7c-9128-5102f658e450"),
               _T("SNMP trap flood detected (Traps per second: %1)"),
               _T("Generated when system detects an SNMP trap flood.\r\n")
               _T("Parameters:\r\n")
               _T("   1) SNMP traps per second\r\n")
               _T("   2) Duration\r\n")
               _T("   3) Threshold")
               ));
      CHK_EXEC(CreateEventTemplate(EVENT_SNMP_TRAP_FLOOD_ENDED, _T("SNMP_TRAP_FLOOD_DETECTED"),
               SEVERITY_NORMAL, EF_LOG, _T("f2c41199-9338-4c9a-9528-d65835c6c271"),
               _T("SNMP trap flood ended"),
               _T("Generated after SNMP trap flood state is cleared.\r\n")
               _T("Parameters:\r\n")
               _T("   1) SNMP traps per second\r\n")
               _T("   2) Duration\r\n")
               _T("   3) Threshold")
               ));

      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 9));
   }

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 40.2 to 40.3
 */
static bool H_UpgradeFromV2()
{
   if (GetSchemaLevelForMajorVersion(35) < 8)
   {
      static const TCHAR *batch =
            _T("UPDATE config SET description='A bitmask for encryption algorithms allowed in the server(sum the values to allow multiple algorithms at once): 1 = AES256, 2 = Blowfish-256, 4 = IDEA, 8 = 3DES, 16 = AES128, 32 = Blowfish-128.' WHERE var_name='AllowedCiphers'\n")
            _T("UPDATE config SET description='Comma-separated list of hosts to be used as beacons for checking NetXMS server network connectivity. Either DNS names or IP addresses can be used. This list is pinged by NetXMS server and if none of the hosts have responded, server considers that connection with network is lost and generates specific event.' WHERE var_name='BeaconHosts'\n")
            _T("UPDATE config SET description='The LdapConnectionString configuration parameter may be a comma- or whitespace-separated list of URIs containing only the schema, the host, and the port fields. Format: schema://host:port.' WHERE var_name='LDAP.ConnectionString'\n")
            _T("UPDATE config SET units='minutes',description='The synchronization interval (in minutes) between the NetXMS server and the LDAP server. If the parameter is set to 0, no synchronization will take place.' WHERE var_name='LDAP.SyncInterval'\n")
            _T("UPDATE config SET var_name='Client.MinViewRefreshInterval',default_value='300',units='milliseconds',description='Minimal interval between view refresh in milliseconds (hint for client).' WHERE var_name='MinViewRefreshInterval'\n")
            _T("UPDATE config SET var_name='Beacon.Hosts' WHERE var_name='BeaconHosts'\n")
            _T("UPDATE config SET var_name='Beacon.PollingInterval' WHERE var_name='BeaconPollingInterval'\n")
            _T("UPDATE config SET var_name='Beacon.Timeout' WHERE var_name='BeaconTimeout'\n")
            _T("UPDATE config SET var_name='SNMP.Traps.Enable' WHERE var_name='EnableSNMPTraps'\n")
            _T("UPDATE config SET var_name='SNMP.Traps.ListenerPort' WHERE var_name='SNMPTrapPort'\n")
            _T("UPDATE config_values SET var_name='SNMP.Traps.ListenerPort' WHERE var_name='SNMPTrapPort'\n")
            _T("UPDATE config SET var_name='SNMP.Traps.ProcessUnmanagedNodes',default_value='0',data_type='B',description='Enable/disable processing of SNMP traps received from unmanaged nodes.' WHERE var_name='ProcessTrapsFromUnmanagedNodes'\n")
            _T("DELETE FROM config WHERE var_name='SNMPPorts'\n")
            _T("DELETE FROM config_values WHERE var_name='SNMPPorts'\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(CreateConfigParam(_T("SNMP.Traps.ProcessUnmanagedNodes"), _T("0"), _T("Enable/disable processing of SNMP traps received from unmanaged nodes."), nullptr, 'B', true, true, false, false));

      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 8));
   }
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 40.1 to 40.2
 */
static bool H_UpgradeFromV1()
{
   if (GetSchemaLevelForMajorVersion(35) < 7)
   {
      CHK_EXEC(SQLQuery(
            _T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
            _T("VALUES ('9c2dba59-493b-4645-9159-2ad7a28ea611',23,'Hook::OpenBoundTunnel','")
            _T("/* Available global variables:\r\n")
            _T(" *  $tunnel - incoming tunnel information (object of ''Tunnel'' class)\r\n")
            _T(" *\r\n")
            _T(" * Expected return value:\r\n")
            _T(" *  none - returned value is ignored\r\n */\r\n')")));
      CHK_EXEC(SQLQuery(
            _T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
            _T("VALUES ('64c90b92-27e9-4a96-98ea-d0e152d71262',24,'Hook::OpenUnboundTunnel','")
            _T("/* Available global variables:\r\n")
            _T(" *  $node - node this tunnel was bound to (object of ''Node'' class)\r\n")
            _T(" *  $tunnel - incoming tunnel information (object of ''Tunnel'' class)\r\n")
            _T(" *\r\n")
            _T(" * Expected return value:\r\n")
            _T(" *  none - returned value is ignored\r\n */\r\n')")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 7));
   }

   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}


/**
 * Upgrade from 40.0 to 40.1
 */
static bool H_UpgradeFromV0()
{
   if (GetSchemaLevelForMajorVersion(35) < 6)
   {
      CHK_EXEC(CreateConfigParam(_T("RoamingServer"), _T("0"), _T("Enable/disable roaming mode for server (when server can be disconnected from one network and connected to another or IP address of the server can change)."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 6));
   }
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
   { 4,  40, 5,  H_UpgradeFromV4  },
   { 3,  40, 4,  H_UpgradeFromV3  },
   { 2,  40, 3,  H_UpgradeFromV2  },
   { 1,  40, 2,  H_UpgradeFromV1  },
   { 0,  40, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr          }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V40()
{
   INT32 major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while((major == 40) && (minor < DB_SCHEMA_VERSION_V40_MINOR))
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 40.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 40.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
