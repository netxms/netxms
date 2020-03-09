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
** File: upgrade_v32.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 32.12 to 33.0
 */
static bool H_UpgradeFromV12()
{
   CHK_EXEC(SetMajorSchemaVersion(33, 0));
   return true;
}

/**
 * Upgrade from 32.11 to 32.12
 */
static bool H_UpgradeFromV11()
{
   static const TCHAR *batch =
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Objects.Interfaces.UseAliases','0','Always use name')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Objects.Interfaces.UseAliases','1','Use alias when available')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Objects.Interfaces.UseAliases','2','Concatenate alias with name')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Objects.Interfaces.UseAliases','3','Concatenate name with alias')\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 32.10 to 32.11
 */
static bool H_UpgradeFromV10()
{
   CHK_EXEC(CreateConfigParam(_T("NetworkDiscovery.ActiveDiscovery.BlockSize"), _T("1024"), _T("Block size for active discovery."), _T("addresses"), 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("NetworkDiscovery.ActiveDiscovery.InterBlockDelay"), _T("0"), _T("Interval in milliseconds between scanning address blocks during active discovery."), _T("milliseconds"), 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 32.9 to 32.10
 */
static bool H_UpgradeFromV9()
{
   // Update CAS parameter names and default values if they are already entered manually
   static const TCHAR *batch =
            _T("UPDATE config SET var_name='CAS.Host',description='CAS server DNS name or IP address.',default_value='localhost' WHERE var_name='CASHost'\n")
            _T("UPDATE config SET var_name='CAS.Port',description='CAS server TCP port number.',default_value='8443',data_type='I' WHERE var_name='CASPort'\n")
            _T("UPDATE config SET var_name='CAS.Service',description='Service to validate (usually NetXMS web UI URL).',default_value='https://127.0.0.1/nxmc' WHERE var_name='CASService'\n")
            _T("UPDATE config SET var_name='CAS.TrustedCACert',description='File system path to CAS server trusted CA certificate.',default_value='' WHERE var_name='CASTrustedCACert'\n")
            _T("UPDATE config SET var_name='CAS.ValidateURL',description='URL for service validation on CAS server.',default_value='/cas/serviceValidate' WHERE var_name='CASValidateURL'\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   // Create all parameters if they does not exist
   CHK_EXEC(CreateConfigParam(_T("CAS.AllowedProxies"), _T(""), _T("Comma-separated list of allowed CAS proxies."), NULL, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("CAS.Host"), _T("localhost"), _T("CAS server DNS name or IP address."), NULL, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("CAS.Port"), _T("8443"), _T("CAS server TCP port number."), NULL, 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("CAS.Service"), _T("https://127.0.0.1/nxmc"), _T("Service to validate (usually NetXMS web UI URL)."), NULL, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("CAS.TrustedCACert"), _T(""), _T("File system path to CAS server trusted CA certificate."), NULL, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("CAS.ValidateURL"), _T("/cas/serviceValidate"), _T("URL for service validation on CAS server."), NULL, 'S', true, false, false, false));

   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 32.8 to 32.9
 */
static bool H_UpgradeFromV8()
{
   static const TCHAR *batch =
            _T("ALTER TABLE nodes ADD eip_port integer\n")
            _T("ALTER TABLE nodes ADD eip_proxy integer\n")
            _T("UPDATE nodes SET eip_port=44818,eip_proxy=0\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("eip_port")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("eip_proxy")));

   CHK_EXEC(CreateEventTemplate(EVENT_ETHERNET_IP_UNREACHABLE, _T("SYS_ETHERNET_IP_UNREACHABLE"),
            SEVERITY_MAJOR, EF_LOG, _T("3fbbe8da-70cf-449d-9c6c-64333c6ff60"),
            _T("EtherNet/IP connectivity failed"),
            _T("Generated when node is not responding to EtherNet/IP requests.\r\n")
            _T("Parameters:\r\n")
            _T("   No message-specific parameters")
            ));
   CHK_EXEC(CreateEventTemplate(EVENT_ETHERNET_IP_OK, _T("SYS_ETHERNET_IP_OK"),
            SEVERITY_NORMAL, EF_LOG, _T("25c7104e-168b-4845-8c96-28edf715785f"),
            _T("EtherNet/IP connectivity restored"),
            _T("Generated when EtherNet/IP connectivity with the node is restored.\r\n")
            _T("Parameters:\r\n")
            _T("   No message-specific parameters")
            ));

   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 32.7 to 32.8
 */
static bool H_UpgradeFromV7()
{
   static const TCHAR *batch =
            _T("ALTER TABLE websvc_definitions ADD description varchar(2000)\n")
            _T("ALTER TABLE nodes ADD vendor varchar(127)\n")
            _T("ALTER TABLE nodes ADD product_name varchar(127)\n")
            _T("ALTER TABLE nodes ADD product_version varchar(15)\n")
            _T("ALTER TABLE nodes ADD product_code varchar(31)\n")
            _T("ALTER TABLE nodes ADD serial_number varchar(31)\n")
            _T("ALTER TABLE nodes ADD cip_device_type integer\n")
            _T("ALTER TABLE nodes ADD cip_status integer\n")
            _T("ALTER TABLE nodes ADD cip_state integer\n")
            _T("UPDATE nodes SET cip_device_type=0,cip_status=0,cip_state=0\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("cip_device_type")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("cip_status")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("cip_state")));
   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 32.6 to 32.7
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE websvc_definitions (")
         _T("id integer not null,")
         _T("guid varchar(36) not null,")
         _T("name varchar(63) not null,")
         _T("url varchar(4000) null,")
         _T("auth_type integer not null,")
         _T("login varchar(255) null,")
         _T("password varchar(255) null,")
         _T("cache_retention_time integer not null,")
         _T("request_timeout integer not null,")
         _T("PRIMARY KEY(id))")));
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE websvc_headers (")
         _T("websvc_id integer not null,")
         _T("name varchar(63) not null,")
         _T("value varchar(2000) null,")
         _T("PRIMARY KEY(websvc_id,name))")));
   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 32.5 to 32.6
 */
static bool H_UpgradeFromV5()
{
   static const TCHAR *batch =
            _T("DELETE FROM config WHERE var_name='EnableCheckPointSNMP'\n")
            _T("ALTER TABLE items ADD snmp_version integer\n")
            _T("ALTER TABLE dc_tables ADD snmp_version integer\n")
            _T("UPDATE items SET snmp_version=127\n")
            _T("UPDATE dc_tables SET snmp_version=127\n")
            _T("UPDATE items SET source=2,snmp_version=0,snmp_port=260 WHERE source=3\n")
            _T("UPDATE dc_tables SET source=2,snmp_version=0,snmp_port=260 WHERE source=3\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("items"), _T("snmp_version")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dc_tables"), _T("snmp_version")));
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 32.4 to 32.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("nodes"), _T("chassis_id")));
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("nodes"), _T("rack_id"), _T("physical_container_id")));
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD chassis_placement_config varchar(2000)\n")));
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 32.3 to 32.4
 */
static bool H_UpgradeFromV3()
{
   static const TCHAR *batch =
            _T("ALTER TABLE object_properties ADD creation_time integer\n")
            _T("UPDATE object_properties SET creation_time=0\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("object_properties"), _T("creation_time")));
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 32.2 to 32.3 (also included in 31.10)
 */
static bool H_UpgradeFromV2()
{
   if (GetSchemaLevelForMajorVersion(31) < 10)
   {
      static const TCHAR *batch =
               _T("ALTER TABLE user_agent_notifications ADD on_startup char(1)\n")
               _T("UPDATE user_agent_notifications SET on_startup='0'\n")
               _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("user_agent_notifications"), _T("on_startup")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(31, 10));
   }
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 32.1 to 32.2
 */
static bool H_UpgradeFromV1()
{
   static const TCHAR *batch =
            _T("ALTER TABLE alarms ADD rca_script_name varchar(255)\n")
            _T("ALTER TABLE alarms ADD impact varchar(1000)\n")
            _T("ALTER TABLE event_policy ADD alarm_impact varchar(1000)\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 32.0 to 32.1
 */
static bool H_UpgradeFromV0()
{
   static const TCHAR *batch =
            _T("ALTER TABLE alarms ADD parent_alarm_id integer\n")
            _T("UPDATE alarms SET parent_alarm_id=0\n")
            _T("ALTER TABLE event_policy ADD rca_script_name varchar(255)\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("alarms"), _T("parent_alarm_id")));

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
   { 12, 33, 0,  H_UpgradeFromV12 },
   { 11, 32, 12, H_UpgradeFromV11 },
   { 10, 32, 11, H_UpgradeFromV10 },
   { 9,  32, 10, H_UpgradeFromV9  },
   { 8,  32, 9,  H_UpgradeFromV8  },
   { 7,  32, 8,  H_UpgradeFromV7  },
   { 6,  32, 7,  H_UpgradeFromV6  },
   { 5,  32, 6,  H_UpgradeFromV5  },
   { 4,  32, 5,  H_UpgradeFromV4  },
   { 3,  32, 4,  H_UpgradeFromV3  },
   { 2,  32, 3,  H_UpgradeFromV2  },
   { 1,  32, 2,  H_UpgradeFromV1  },
   { 0,  32, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr          }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V32()
{
   INT32 major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while(major == 32)
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 32.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 32.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
