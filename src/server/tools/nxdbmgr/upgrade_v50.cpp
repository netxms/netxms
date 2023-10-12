/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2022-2023 Raden Solutions
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
** File: upgrade_v50.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 50.7 to 50.8
 */
static bool H_UpgradeFromV7()
{
   if (GetSchemaLevelForMajorVersion(44) < 27)
   {
      static const TCHAR *batch =
         _T("ALTER TABLE ssh_keys ADD tmp_public_key $SQL:TEXT\n")
         _T("ALTER TABLE ssh_keys ADD tmp_private_key $SQL:TEXT\n")
         _T("UPDATE ssh_keys SET tmp_public_key=public_key, tmp_private_key=private_key\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(DBDropColumn(g_dbHandle, _T("ssh_keys"), _T("public_key")));
      CHK_EXEC(DBDropColumn(g_dbHandle, _T("ssh_keys"), _T("private_key")));

      CHK_EXEC(DBRenameColumn(g_dbHandle, _T("ssh_keys"), _T("tmp_public_key"), _T("public_key")));
      CHK_EXEC(DBRenameColumn(g_dbHandle, _T("ssh_keys"), _T("tmp_private_key"), _T("private_key")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(44, 27));
   }

   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 50.6 to 50.7
 */
static bool H_UpgradeFromV6()
{
   static int defaultWeight = 1300;
   static int defaultHeight = 850;

   CHK_EXEC(CreateConfigParam(_T("Objects.NetworkMaps.DefaultWidth"),
         _T("1300"),
         _T("Default network map width"),
         _T("pixels"),
         'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("Objects.NetworkMaps.DefaultHeight"),
         _T("850"),
         _T("Default network map height"),
         _T("pixels"),
         'I', true, false, false, false));

   static const TCHAR *batch =
      _T("ALTER TABLE network_maps ADD width integer\n")
      _T("ALTER TABLE network_maps ADD height integer\n")
      _T("UPDATE network_maps SET height=0,width=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("network_maps"), _T("width")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("network_maps"), _T("height")));

   DB_RESULT result = SQLSelect(_T("SELECT element_data,map_id FROM network_map_elements ORDER BY map_id"));
   if (result != nullptr)
   {
      int size = DBGetNumRows(result);
      uint32_t mpaId = size > 0 ? DBGetFieldLong(result, 0, 1) : 0;
      uint32_t maxWidth = defaultWeight;
      uint32_t maxHeight = defaultHeight;
      TCHAR query[256];
      for (int i = 0; i < size; i++)
      {
         if (mpaId != DBGetFieldLong(result, i, 1))
         {
            //Save map
            if ((maxWidth > defaultWeight) || (maxHeight > defaultHeight))
            {
               _sntprintf(query, 256, _T("UPDATE network_maps set width=%d,height=%d WHERE id=%d"), maxWidth, maxHeight, mpaId);
               CHK_EXEC(SQLQuery(query));
            }

            //Reset counters
            maxWidth = defaultWeight;
            maxHeight = defaultHeight;
            mpaId = DBGetFieldLong(result, i, 1);
         }

         Config config;
         TCHAR *data = DBGetField(result, i, 0, nullptr, 0);
         if (data != nullptr)
         {
#ifdef UNICODE
            char *utf8data = UTF8StringFromWideString(data);
            config.loadXmlConfigFromMemory(utf8data, (int)strlen(utf8data), _T("<database>"), "element");
            MemFree(utf8data);
#else
            config.loadXmlConfigFromMemory(data, (int)strlen(data), _T("<database>"), "element");
#endif
            MemFree(data);
            if (config.getValueAsInt(_T("type"), 1) == 2)
            {
               maxWidth = std::max(config.getValueAsUInt(_T("/posX"), 0) + config.getValueAsUInt(_T("/width"), 0), maxWidth);
               maxHeight = std::max(config.getValueAsUInt(_T("/posY"), 0) + config.getValueAsUInt(_T("/height"), 0), maxHeight);
            }
            else
            {
               maxWidth = std::max(config.getValueAsUInt(_T("/posX"), 0) + 50, maxWidth);
               maxHeight = std::max(config.getValueAsUInt(_T("/posY"), 0) + 50, maxHeight);
            }
         }
      }

      if ((maxWidth > defaultWeight) || (maxHeight > defaultHeight))
      {
         _sntprintf(query, 256, _T("UPDATE network_maps set width=%d,height=%d WHERE id=%d"), maxWidth, maxHeight, mpaId);
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(result);
   }
   else
   {
      if (!g_ignoreErrors)
         return false;
   }

   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 50.5 to 50.6
 */
static bool H_UpgradeFromV5()
{
   CHK_EXEC_NO_SP(SQLQuery(_T("DROP TABLE dct_column_names")));
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 50.4 to 50.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.AllowedVersions"),
         _T("7"),
         _T("A bitmask for SNMP versions allowed by built-in SNMP agent (sum the values to allow multiple versions at once): 1 = version 1, 2 = version 2c, 4 = version 3)."),
         nullptr, 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.CommunityString"),
         _T("public"),
         _T("Community string for SNMPv1/v2 requests to built-in SNMP agent."),
         nullptr, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.Enable"),
         _T("0"),
         _T("Enable/disable built-in SNMP agent."),
         nullptr, 'B', true, true, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.ListenerPort"),
         _T("161"),
         _T("Port used by built-in SNMP agent."),
         nullptr, 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.V3.AuthenticationMethod"),
         _T("0"),
         _T("Authentication method for SNMPv3 requests to built-in SNMP agent."),
         nullptr, 'C', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.V3.AuthenticationPassword"),
         _T(""),
         _T("Authentication password for SNMPv3 requests to built-in SNMP agent."),
         nullptr, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.V3.EncryptionMethod"),
         _T("0"),
         _T("Encryption method for SNMPv3 requests to built-in SNMP agent."),
         nullptr, 'C', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.V3.EncryptionPassword"),
         _T(""),
         _T("Encryption password for SNMPv3 requests to built-in SNMP agent."),
         nullptr, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.V3.UserName"),
         _T("netxms"),
         _T("User name for SNMPv3 requests to built-in SNMP agent."),
         nullptr, 'S', true, false, false, false));

   static const TCHAR *configBatch =
      _T("INSERT INTO config_values (var_name,var_value) VALUES ('SNMP.Agent.ListenerPort','65535')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.AuthenticationMethod','0','None')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.AuthenticationMethod','1','MD5')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.AuthenticationMethod','2','SHA-1')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.AuthenticationMethod','3','SHA-224')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.AuthenticationMethod','4','SHA-256')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.AuthenticationMethod','5','SHA-384')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.AuthenticationMethod','6','SHA-512')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.EncryptionMethod','0','None')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.EncryptionMethod','1','DES')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.EncryptionMethod','2','AES-128')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.EncryptionMethod','3','AES-192')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.EncryptionMethod','4','AES-256')\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(configBatch));

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 50.3 to 50.4
 */
static bool H_UpgradeFromV3()
{
   if (GetSchemaLevelForMajorVersion(44) < 26)
   {
      CHK_EXEC(CreateEventTemplate(EVENT_IF_SPEED_CHANGED, _T("SYS_IF_SPEED_CHANGED"),
            EVENT_SEVERITY_NORMAL, EF_LOG, _T("3967eab5-71c0-4ba7-aeb9-eb70bcf5caa9"),
            _T("Interface %<ifName> speed changed from %<oldSpeedText> to %<newSpeedText>"),
            _T("Generated when system detects interface speed change.\r\n")
            _T("Parameters:\r\n")
            _T("   1) ifIndex - Interface index\r\n")
            _T("   2) ifName - Interface name\r\n")
            _T("   3) oldSpeed - Old speed in bps\r\n")
            _T("   4) oldSpeedText - Old speed in bps with optional multiplier (kbps, Mbps, etc.)\r\n")
            _T("   5) newSpeed - New speed in bps\r\n")
            _T("   6) newSpeedText - New speed in bps with optional multiplier (kbps, Mbps, etc.)")
         ));
      CHK_EXEC(SetSchemaLevelForMajorVersion(44, 26));
   }

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 50.2 to 50.3
 */
static bool H_UpgradeFromV2()
{
   if (GetSchemaLevelForMajorVersion(44) < 25)
   {
      CHK_EXEC(CreateConfigParam(_T("Objects.Maintenance.PredefinedPeriods"),
            _T("1h,8h,1d"),
            _T("Predefined object maintenance periods. Use m for minutes, h for hours and d for days."),
            _T(""),
            'S', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(44, 25));
   }

   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 50.1 to 50.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE network_maps ADD link_styling_script $SQL:TEXT")));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 50.0 to 50.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when system detects loss of network connectivity based on beacon probing.\r\n")
      _T("Parameters:\r\n")
      _T("   1) numberOfBeacons - Number of beacons'")
      _T("WHERE event_code=50")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when system detects restoration of network connectivity based on beacon probing.\r\n")
      _T("Parameters:\r\n")
      _T("   1) numberOfBeacons - Number of beacons'")
      _T("WHERE event_code=51")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when cluster resource goes up.\r\n")
      _T("Parameters:\r\n")
      _T("   1) resourceId - Resource ID\r\n")
      _T("   2) resourceName - Resource name\r\n")
      _T("   3) newOwnerNodeId - New owner node ID\r\n")
      _T("   4) newOwnerNodeName - New owner node name'")
      _T("WHERE event_code=40")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when cluster resource moved between nodes.\r\n")
      _T("Parameters:\r\n")
      _T("   1) resourceId - Resource ID\r\n")
      _T("   2) resourceName - Resource name\r\n")
      _T("   3) previousOwnerNodeId - Previous owner node ID\r\n")
      _T("   4) previousOwnerNodeName - Previous owner node name\r\n")
      _T("   5) newOwnerNodeId - New owner node ID\r\n")
      _T("   6) newOwnerNodeName - New owner node name'")
      _T("WHERE event_code=38")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node, cluster, or mobile device entered maintenance mode.\r\n")
      _T("Parameters:\r\n")
      _T("   1) comments - Comments\r\n")
      _T("   2) userId - Initiating user ID\r\n")
      _T("   3) userName - Initiating user name'")
      _T("WHERE event_code=78")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node, cluster, or mobile device left maintenance mode.\r\n")
      _T("Parameters:\r\n")
      _T("   1) userId - Initiating user ID\r\n")
      _T("   2) userName - Initiating user name'")
      _T("WHERE event_code=79")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when template applied to node by autoapply rule.\r\n")
      _T("Parameters:\r\n")
      _T("   1) nodeId - Node ID\r\n")
      _T("   2) nodeName - Node name\r\n")
      _T("   3) templateId - Template ID\r\n")
      _T("   4) templateName - Template name'")
      _T("WHERE event_code=66")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node bound to container object by autobind rule.\r\n")
      _T("Parameters:\r\n")
      _T("   1) nodeId - Node ID\r\n")
      _T("   2) nodeName - Node name\r\n")
      _T("   3) containerId - Container ID\r\n")
      _T("   4) containerName - Container name'")
      _T("WHERE event_code=64")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when cluster resource goes down.\r\n")
      _T("Parameters:\r\n")
      _T("   1) resourceId - Resource ID\r\n")
      _T("   2) resourceName - Resource name\r\n")
      _T("   3) lastOwnerNodeId - Last owner node ID\r\n")
      _T("   4) lastOwnerNodeName - Last owner node name'")
      _T("WHERE event_code=39")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node added to cluster object by autoadd rule.\r\n")
      _T("Parameters:\r\n")
      _T("   1) nodeId - Node ID\r\n")
      _T("   2) nodeName - Node name\r\n")
      _T("   3) clusterId - Cluster ID\r\n")
      _T("   4) clusterName - Cluster name'")
      _T("WHERE event_code=114")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node removed from cluster object by autoadd rule.\r\n")
      _T("Parameters:\r\n")
      _T("   1) nodeId - Node ID\r\n")
      _T("   2) nodeName - Node name\r\n")
      _T("   3) clusterId - Cluster ID\r\n")
      _T("   4) clusterName - Cluster name'")
      _T("WHERE event_code=115")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node bound to container object by autobind rule.\r\n")
      _T("Parameters:\r\n")
      _T("   1) nodeId - Node ID\r\n")
      _T("   2) nodeName - Node name\r\n")
      _T("   3) containerId - Container ID\r\n")
      _T("   4) containerName - Container name'")
      _T("WHERE event_code=64")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node unbound from container object by autobind rule.\r\n")
      _T("Parameters:\r\n")
      _T("   1) nodeId - Node ID\r\n")
      _T("   2) nodeName - Node name\r\n")
      _T("   3) containerId - Container ID\r\n")
      _T("   4) containerName - Container name'")
      _T("WHERE event_code=65")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when template removed from node by autoapply rule.\r\n")
      _T("Parameters:\r\n")
      _T("   1) nodeId - Node ID\r\n")
      _T("   2) nodeName - Node name\r\n")
      _T("   3) templateId - Template ID\r\n")
      _T("   4) templateName - Template name'")
      _T("WHERE event_code=67")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when device geolocation change is detected.\r\n")
      _T("Parameters:\r\n")
      _T("   1) newLatitude - Latitude of new location in degrees\r\n")
      _T("   2) newLongitude - Longitude of new location in degrees\r\n")
      _T("   3) newLatitudeAsString- Latitude of new location in textual form\r\n")
      _T("   4) newLongitudeAsString - Longitude of new location in textual form\r\n")
      _T("   5) previousLatitude - Latitude of previous location in degrees\r\n")
      _T("   6) previousLongitude - Longitude of previous location in degrees\r\n")
      _T("   7) previousLatitudeAsString - Latitude of previous location in textual form\r\n")
      _T("   8) previoudLongitudeAsString - Longitude of previous location in textual form'")
      _T("WHERE event_code=111")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when new device geolocation is within restricted area.\r\n")
      _T("Parameters:\r\n")
      _T("   1) newLatitude - Latitude of new location in degrees\r\n")
      _T("   2) newLongitude - Longitude of new location in degrees\r\n")
      _T("   3) newLatitudeAsString- Latitude of new location in textual form\r\n")
      _T("   4) newLongitudeAsString - Longitude of new location in textual form\r\n")
      _T("   5) areaName - Area name\r\n")
      _T("   6) areaId - Area ID'")
      _T("WHERE event_code=112")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when new device geolocation is ouside allowed areas.\r\n")
      _T("Parameters:\r\n")
      _T("   1) newLatitude - Latitude of new location in degrees\r\n")
      _T("   2) newLongitude - Longitude of new location in degrees\r\n")
      _T("   3) newLatitudeAsString- Latitude of new location in textual form\r\n")
      _T("   4) newLongitudeAsString - Longitude of new location in textual form'")
      _T("WHERE event_code=113")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when duplicate MAC address found.\r\n")
      _T("Parameters:\r\n")
      _T("   1) macAddress - MAC address\r\n")
      _T("   2) listOfInterfaces - List of interfaces where MAC address was found'")
      _T("WHERE event_code=119")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when system detects an event storm.\r\n")
      _T("Parameters:\r\n")
      _T("   1) eps - Events per second\r\n")
      _T("   2) duration - Duration\r\n")
      _T("   3) threshold - Threshold'")
      _T("WHERE event_code=48")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when system clears event storm condition.\r\n")
      _T("Parameters:\r\n")
      _T("   1) eps - Events per second\r\n")
      _T("   2) duration - Duration\r\n")
      _T("   3) threshold - Threshold'")
      _T("WHERE event_code=49")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when internal housekeeper task completes.\r\n")
      _T("Parameters:\r\n")
      _T("   1) elapsedTime - Housekeeper execution time in seconds'")
      _T("WHERE event_code=107")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when possibly duplicate IP address is detected by network discovery process.\r\n")
      _T("Parameters:\r\n")
      _T("   1) ipAddress - IP address\r\n")
      _T("   2) knownNodeId - Known node object ID\r\n")
      _T("   3) knownNodeName - Known node name\r\n")
      _T("   4) knownInterfaceName - Known interface name\r\n")
      _T("   5) knownMacAddress - Known MAC address\r\n")
      _T("   6) discoveredMacAddress - Discovered MAC address\r\n")
      _T("   7) discoverySourceNodeId - Discovery source node object ID\r\n")
      _T("   8) discoverySourceNodeName - Discovery source node name\r\n")
      _T("   9) discoveryDataSource - Discovery data source'")
      _T("WHERE event_code=101")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when switch port PAE state changed to FORCE UNAUTHORIZE.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceIndex - Interface index\r\n")
      _T("   2) interfaceName - Interface name'")
      _T("WHERE event_code=59")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when switch port PAE state changed.\r\n")
      _T("Parameters:\r\n")
      _T("   1) newPaeStateCode - New PAE state code\r\n")
      _T("   2) newPaeStateText - New PAE state as text\r\n")
      _T("   3) oldPaeStateCode - Old PAE state code\r\n")
      _T("   4) oldPaeStateText - Old PAE state as text\r\n")
      _T("   5) interfaceIndex - Interface index\r\n")
      _T("   6) interfaceName - Interface name'")
      _T("WHERE event_code=57")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when LDAP synchronization error occurs.\r\n")
      _T("Parameters:\r\n")
      _T("   1) userId - User ID\r\n")
      _T("   2) userGuid - User GUID\r\n")
      _T("   3) userLdapDn - User LDAP DN\r\n")
      _T("   4) userName - User name\r\n")
      _T("   5) description - Problem description'")
      _T("WHERE event_code=80")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when switch port backend authentication state changed.\r\n")
      _T("Parameters:\r\n")
      _T("   1) newBackendStateCode - New backend state code\r\n")
      _T("   2) newBackendStateText - New backend state as text\r\n")
      _T("   3) oldBackendStateCode - Old backend state code\r\n")
      _T("   4) oldBackendStateText - Old backend state as text\r\n")
      _T("   5) interfaceIndex - Interface index\r\n")
      _T("   6) interfaceName - Interface name'")
      _T("WHERE event_code=58")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when licensing problem is detected by extension module.\r\n")
      _T("Parameters:\r\n")
      _T("   1) description - Problem description'")
      _T("WHERE event_code=108")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when switch port backend authentication state changed to FAIL.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceIndex - Interface index\r\n")
      _T("   2) interfaceName - Interface name'")
      _T("WHERE event_code=60")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when switch port backend authentication state changed to TIMEOUT.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceIndex - Interface index\r\n")
      _T("   2) interfaceName - Interface name'")
      _T("WHERE event_code=61")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node status changed to normal.\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousNodeStatus - Previous node status'")
      _T("WHERE event_code=6")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node status changed to \"Warning\".\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousNodeStatus - Previous node status'")
      _T("WHERE event_code=7")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node status changed to \"Minor Problem\" (informational).\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousNodeStatus - Previous node status'")
      _T("WHERE event_code=8")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node status changed to \"Major Problem\".\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousNodeStatus - Previous node status'")
      _T("WHERE event_code=9")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node status changed to critical.s\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousNodeStatus - Previous node status'")
      _T("WHERE event_code=10")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node status changed to unknown.\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousNodeStatus - Previous node status'")
      _T("WHERE event_code=11")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node status changed to unmanaged.\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousNodeStatus - Previous node status'")
      _T("WHERE event_code=12")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when new interface object added to the database.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetmask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=3")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface expected state set to UP.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceIndex - Interface index\r\n")
      _T("   2) intefaceName - Interface name'")
      _T("WHERE event_code=83")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface expected state set to DOWN.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceIndex - Interface index\r\n")
      _T("   2) intefaceName - Interface name'")
      _T("WHERE event_code=84")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface expected state set to IGNORE.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceIndex - Interface index\r\n")
      _T("   2) intefaceName - Interface name'")
      _T("WHERE event_code=85")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when communication with the node re-established.\r\n")
      _T("Parameters:\r\n")
      _T("   1) reason - Reason'")
      _T("WHERE event_code=29")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated on status poll if agent reports problem with log file.\r\n")
      _T("Parameters:\r\n")
      _T("   1) statusCode - Status code\r\n")
      _T("   2) description - Description'")
      _T("WHERE event_code=81")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when server is unable to send notification.\r\n")
      _T("Parameters:\r\n")
      _T("   1) notificationChannelName - Notification channel name\r\n")
      _T("   2) recipientAddress - Recipient address\r\n")
      _T("   3) notificationSubject - Notification subject\r\n")
      _T("   4) notificationMessage - Notification message'")
      _T("WHERE event_code=22")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when notification channel fails health check.\r\n")
      _T("Parameters:\r\n")
      _T("   1) channelName - Notification channel name\r\n")
      _T("   2) channelDriverName - Notification channel driver name'")
      _T("WHERE event_code=125")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when notification channel passes health check.\r\n")
      _T("Parameters:\r\n")
      _T("   1) channelName - Notification channel name\r\n")
      _T("   2) channelDriverName - Notification channel driver name'")
      _T("WHERE event_code=126")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when one of the system threads hangs or stops unexpectedly.\r\n")
      _T("Parameters:\r\n")
      _T("   1) threadName - Thread name'")
      _T("WHERE event_code=20")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when one of the system threads which previously hangs or stops unexpectedly was returned to running state.\r\n")
      _T("Parameters:\r\n")
      _T("   1) threadName - Thread name'")
      _T("WHERE event_code=21")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Default event for condition activation.\r\n")
      _T("Parameters:\r\n")
      _T("   1) conditionObjectId - Condition object ID\r\n")
      _T("   2) conditionObjectName - Condition object name\r\n")
      _T("   3) previousConditionStatus - Previous condition status\r\n")
      _T("   4) currentConditionStatus - Current condition status'")
      _T("WHERE event_code=34")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Default event for condition deactivation.\r\n")
      _T("Parameters:\r\n")
      _T("   1) conditionObjectId - Condition object ID\r\n")
      _T("   2) conditionObjectName - Condition object name\r\n")
      _T("   3) previousConditionStatus - Previous condition status\r\n")
      _T("   4) currentConditionStatus - Current condition status'")
      _T("WHERE event_code=35")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when new node object added to the database.\r\n")
      _T("Parameters:\r\n")
      _T("   1) nodeOrigin - Node origin (0 = created manually, 1 = created by network discovery, 2 = created by tunnel auto bind)'")
      _T("WHERE event_code=1")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when new subnet object added to the database.\r\n")
      _T("Parameters:\r\n")
      _T("   1) subnetObjectId - Subnet object ID\r\n")
      _T("   2) subnetName - Subnet name\r\n")
      _T("   3) ipAddress - IP address\r\n")
      _T("   4) networkMask - Network mask'")
      _T("WHERE event_code=2")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when subnet object deleted from the database.\r\n")
      _T("Parameters:\r\n")
      _T("   1) subnetObjectId - Subnet object ID\r\n")
      _T("   2) subnetName - Subnet name\r\n")
      _T("   3) ipAddress - IP address\r\n")
      _T("   4) networkMask - Network mask'")
      _T("WHERE event_code=19")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when alarm timeout expires.\r\n")
      _T("Parameters:\r\n")
      _T("   1) alarmId - Alarm ID\r\n")
      _T("   2) alarmMessage - Alarm message\r\n")
      _T("   3) alarmKey - Alarm key\r\n")
      _T("   4) originalEventCode - Original event code\r\n")
      _T("   5) originalEventName - Original event name'")
      _T("WHERE event_code=43")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated on status poll if agent reports local database problem.\r\n")
      _T("Parameters:\r\n")
      _T("   1) statusCode - Status code\r\n")
      _T("   2) description - Description'")
      _T("WHERE event_code=82")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node capabilities changed.\r\n")
      _T("Parameters:\r\n")
      _T("   1) oldCapabilities - Old capabilities\r\n")
      _T("   2) newCapabilities - New capabilities'")
      _T("WHERE event_code=13")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when IP address added to interface.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) ipAddress - IP address\r\n")
      _T("   4) networkMask - Network mask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=76")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when IP address deleted from interface.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) ipAddress - IP address\r\n")
      _T("   4) networkMask - Network mask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=77")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when primary IP address changed (usually because of primary name change or DNS change).\r\n")
      _T("Parameters:\r\n")
      _T("   1) newIpAddress - New IP address\r\n")
      _T("   2) oldIpAddress - Old IP address\r\n")
      _T("   3) primaryHostName - Primary host name'")
      _T("WHERE event_code=56")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node is deleted by network discovery de-duplication process.\r\n")
      _T("Parameters:\r\n")
      _T("   1) originalNodeObjectId - Original node object ID\r\n")
      _T("   2) originalNodeName - Original node name\r\n")
      _T("   3) originalNodePrimaryHostName - Original node primary host name\r\n")
      _T("   4) duplicateNodeObjectId - Duplicate node object ID\r\n")
      _T("   5) duplicateNodeName - Duplicate node name\r\n")
      _T("   6) duplicateNodePrimaryHostName - Duplicate node primary host name\r\n")
      _T("   7) reason - Reason'")
      _T("WHERE event_code=100")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when system detects an SNMP trap flood.\r\n")
      _T("Parameters:\r\n")
      _T("   1) snmpTrapsPerSecond - SNMP traps per second\r\n")
      _T("   2) duration - Duration\r\n")
      _T("   3) threshold - Threshold'")
      _T("WHERE event_code=109")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated after SNMP trap flood state is cleared.\r\n")
      _T("Parameters:\r\n")
      _T("   1) snmpTrapsPerSecond - SNMP traps per second\r\n")
      _T("   2) duration - Duration\r\n")
      _T("   3) threshold - Threshold'")
      _T("WHERE event_code=110")));
      
   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when agent ID change detected.\r\n")
      _T("Parameters:\r\n")
      _T("   1) oldAgentId - Old agent ID\r\n")
      _T("   2) newAgentId - New agent ID'")
      _T("WHERE event_code=93")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface object deleted from the database.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceIndex - Interface index\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress -Interface IP address\r\n")
      _T("   4) interfaceMask - Interface netmask'")
      _T("WHERE event_code=16")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when server detects change of interface''s MAC address.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceIndex - Interface index\r\n")
      _T("   3) interfaceName - Interface name\r\n")
      _T("   4) oldMacAddress - Old MAC address\r\n")
      _T("   5) newMacAddress - New MAC address'")
      _T("WHERE event_code=23")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when when network mask on interface is corrected.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetmask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index\r\n")
      _T("   6) interfaceOldMask - Interface old mask'")
      _T("WHERE event_code=75")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when server detects invalid network mask on an interface.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceIndex - Interface index\r\n")
      _T("   3) interfaceName - Interface name\r\n")
      _T("   4) actualNetworkMask - Actual network mask on interface\r\n")
      _T("   5) correctNetworkMask - Correct network mask'")
      _T("WHERE event_code=24")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface goes to unknown state.\r\n")
      _T("Please note that source of event is node, not an interface itself.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetMask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=45")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface administratively disabled.\r\n")
      _T("Please note that source of event is node, not an interface itself.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetMask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=46")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface goes to testing state.\r\n")
      _T("Please note that source of event is node, not an interface itself.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetMask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=47")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface goes up but it''s expected state set to DOWN.\r\n")
      _T("Please note that source of event is node, not an interface itself.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetMask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=62")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface goes down and it''s expected state is DOWN.\r\n")
      _T("Please note that source of event is node, not an interface itself.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetMask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=63")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface goes up.\r\n")
      _T("Please note that source of event is node, not an interface itself.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetMask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=4")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface goes down.\r\n")
      _T("Please note that source of event is node, not an interface itself.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetMask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=5")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when network service is not responding to management server as expected.\r\n")
      _T("Parameters:\r\n")
      _T("   1) serviceName - Service name\r\n")
      _T("   2) serviceObjectId - Service object ID\r\n")
      _T("   3) serviceType - Service type'")
      _T("WHERE event_code=25")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when network service responds as expected after failure.\r\n")
      _T("Parameters:\r\n")
      _T("   1) serviceName - Service name\r\n")
      _T("   2) serviceObjectId - Service object ID\r\n")
      _T("   3) serviceType - Service type'")
      _T("WHERE event_code=26")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when management server is unable to determine state of the network service due to agent or server-to-agent communication failure.\r\n")
      _T("Parameters:\r\n")
      _T("   1) serviceName - Service name\r\n")
      _T("   2) serviceObjectId - Service object ID\r\n")
      _T("   3) serviceType - Service type'")
      _T("WHERE event_code=27")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when agent policy within template fails validation.\r\n")
      _T("Parameters:\r\n")
      _T("   1) templateName - Template name\r\n")
      _T("   2) templateId - Template ID\r\n")
      _T("   3) policyName - Policy name\r\n")
      _T("   4) policyType - Policy type\r\n")
      _T("   5) policyId - Policy ID\r\n")
      _T("   6) additionalInfo - Additional info'")
      _T("WHERE event_code=117")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when peer information for interface changes.\r\n")
      _T("Parameters:\r\n")
      _T("   1) localIfId - Local interface object ID\r\n")
      _T("   2) localIfIndex - Local interface index\r\n")
      _T("   3) localIfName - Local interface name\r\n")
      _T("   4) localIfIP - Local interface IP address\r\n")
      _T("   5) localIfMAC - Local interface MAC address\r\n")
      _T("   6) remoteNodeId - Peer node object ID\r\n")
      _T("   7) remoteNodeName - Peer node name\r\n")
      _T("   8) remoteIfId - interface object ID\r\n")
      _T("   9) remoteIfIndex - Peer interface index\r\n")
      _T("   10) remoteIfName - Peer interface name\r\n")
      _T("   11) remoteIfIP - Peer interface IP address\r\n")
      _T("   12) remoteIfMAC - Peer interface MAC address\r\n")
      _T("   13) protocol - Discovery protocol'")
      _T("WHERE event_code=71")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node is unreachable by management server because of network failure.\r\n")
      _T("Parameters:\r\n")
      _T("   1) reasonCode - Reason Code\r\n")
      _T("   2) reason - Reason\r\n")
      _T("   3) rootCauseNodeId - Root Cause Node ID\r\n")
      _T("   4) rootCauseNodeName - Root Cause Node Name\r\n")
      _T("   5) rootCauseInterfaceId - Root Cause Interface ID\r\n")
      _T("   6) rootCauseInterfaceName - Root Cause Interface Name\r\n")
      _T("   7) description - Description'")
      _T("WHERE event_code=68")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when server detects routing loop during network path trace.\r\n")
      _T("Parameters:\r\n")
      _T("   1) protocol - Protovol (IPv4 or IPv6)\r\n")
      _T("   2) destNodeId - Path trace destination node ID\r\n")
      _T("   3) destAddress - Path trace destination address\r\n")
      _T("   4) sourceNodeId - Path trace source node ID\r\n")
      _T("   5) sourceAddress - Path trace source node address\r\n")
      _T("   6) prefix - Routing prefix (subnet address)\r\n")
      _T("   7) prefixLength - Routing prefix length (subnet mask length)\r\n")
      _T("   8) nextHopNodeId - Next hop node ID\r\n")
      _T("   9) nextHopAddress - Next hop address'")
      _T("WHERE event_code=86")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when new hardware component is found.\r\n")
      _T("Parameters:\r\n")
      _T("   1) category - Category\r\n")
      _T("   2) type - Type\r\n")
      _T("   3) vendor - Vendor\r\n")
      _T("   4) model - Model\r\n")
      _T("   5) location - Location\r\n")
      _T("   6) partNumber - Part number\r\n")
      _T("   7) serialNumber - Serial number\r\n")
      _T("   8) capacity - Capacity\r\n")
      _T("   9) description - Description'")
      _T("WHERE event_code=98")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when hardware component removal is detected.\r\n")
      _T("Parameters:\r\n")
      _T("   1) category - Category\r\n")
      _T("   2) type - Type\r\n")
      _T("   3) vendor - Vendor\r\n")
      _T("   4) model - Model\r\n")
      _T("   5) location - Location\r\n")
      _T("   6) partNumber - Part number\r\n")
      _T("   7) serialnumber - Serial number\r\n")
      _T("   8) capacity - Capacity\r\n")
      _T("   9) description - Description'")
      _T("WHERE event_code=99")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when new software package is found.\r\n")
      _T("Parameters:\r\n")
      _T("   1) name - Package name\r\n")
      _T("   2) version - Package version'")
      _T("WHERE event_code=87")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when software package version change is detected.\r\n")
      _T("Parameters:\r\n")
      _T("   1) name - Package name\r\n")
      _T("   2) version - New package version\r\n")
      _T("   3) previousVersion - Old package version'")
      _T("WHERE event_code=88")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when software package removal is detected.\r\n")
      _T("Parameters:\r\n")
      _T("   1) name - Package name\r\n")
      _T("   2) version - Last known package version'")
      _T("WHERE event_code=89")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when agent tunnel is open and bound.\r\n")
      _T("Parameters:\r\n")
      _T("   1) tunnelId - Tunnel ID\r\n")
      _T("   2) ipAddress - Remote system IP address\r\n")
      _T("   3) systemName - Remote system name\r\n")
      _T("   4) hostName - Remote system FQDN\r\n")
      _T("   5) platformName - Remote system platform\r\n")
      _T("   6) systemInfo - Remote system information\r\n")
      _T("   7) agentVersion - Agent version\r\n")
      _T("   8) agentId - Agent ID'")
      _T("WHERE event_code=94")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when agent tunnel is closed.\r\n")
      _T("Parameters:\r\n")
      _T("   1) tunnelId - Tunnel ID\r\n")
      _T("   2) ipAddress - Remote system IP address\r\n")
      _T("   3) systemName - Remote system name\r\n")
      _T("   4) hostName - Remote system FQDN\r\n")
      _T("   5) platformName - Remote system platform\r\n")
      _T("   6) systemInfo - Remote system information\r\n")
      _T("   7) agentVersion - Agent version\r\n")
      _T("   8) agentId - Agent ID'")
      _T("WHERE event_code=94")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when new tunnel is replacing existing one and host data mismatch is detected.\r\n")
      _T("Parameters:\r\n")
      _T("   1) tunnelId - Tunnel ID\r\n")
      _T("   2) oldIPAddress - Old remote system IP address\r\n")
      _T("   3) newIPAddress - New remote system IP address\r\n")
      _T("   4) oldSystemName - Old remote system name\r\n")
      _T("   5) newSystemName- New remote system name\r\n")
      _T("   6) oldHostName - Old remote system FQDN\r\n")
      _T("   7) newHostName - New remote system FQDN\r\n")
      _T("   8) oldHardwareId - Old hardware ID\r\n")
      _T("   9) newHardwareId - New hardware ID'")
      _T("WHERE event_code=116")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when agent ID mismatch detected during tunnel bind.\r\n")
      _T("Parameters:\r\n")
      _T("   1) tunnelId - Tunnel ID\r\n")
      _T("   2) ipAddress - Remote system IP address\r\n")
      _T("   3) systemName - Remote system name\r\n")
      _T("   4) hostName - Remote system FQDN\r\n")
      _T("   5) platformName - Remote system platform\r\n")
      _T("   6) systemInfo - Remote system information\r\n")
      _T("   7) agentVersion - Agent version\r\n")
      _T("   8) tunnelAgentId - Tunnel agent ID\r\n")
      _T("   9) nodeAgentId - Node agent ID'")
      _T("WHERE event_code=96")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when unbound agent tunnel is not bound or closed for more than configured threshold.\r\n")
      _T("Parameters:\r\n")
      _T("   1) tunnelId - Tunnel ID\r\n")
      _T("   2) ipAddress - Remote system IP address\r\n")
      _T("   3) systemName - Remote system name\r\n")
      _T("   4) hostName - Remote system FQDN\r\n")
      _T("   5) platformName - Remote system platform\r\n")
      _T("   6) systemInfo - Remote system information\r\n")
      _T("   7) agentVersion - Agent version\r\n")
      _T("   8) agentId - Agent ID\r\n")
      _T("   9) idleTimeout - Configured idle timeout'")
      _T("WHERE event_code=92")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when access point state changes to ADOPTED.\r\n")
      _T("Parameters:\r\n")
      _T("   1) id - Access point object ID\r\n")
      _T("   2) name - Access point name\r\n")
      _T("   3) macAddr - Access point MAC address\r\n")
      _T("   4) ipAddr - Access point IP address\r\n")
      _T("   5) vendor - Access point vendor name\r\n")
      _T("   6) model - Access point model\r\n")
      _T("   7) serialNumber - Access point serial number'")
      _T("WHERE event_code=72")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when access point state changes to UNADOPTED.\r\n")
      _T("Parameters:\r\n")
      _T("   1) id - Access point object ID\r\n")
      _T("   2) name - Access point name\r\n")
      _T("   3) macAddr - Access point MAC address\r\n")
      _T("   4) ipAddr - Access point IP address\r\n")
      _T("   5) vendor - Access point vendor name\r\n")
      _T("   6) model - Access point model\r\n")
      _T("   7) serialNumber - Access point serial number'")
      _T("WHERE event_code=73")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when access point state changes to DOWN.\r\n")
      _T("Parameters:\r\n")
      _T("   1) id - Access point object ID\r\n")
      _T("   2) name - Access point name\r\n")
      _T("   3) macAddr - Access point MAC address\r\n")
      _T("   4) ipAddr - Access point IP address\r\n")
      _T("   5) vendor - Access point vendor name\r\n")
      _T("   6) model - Access point model\r\n")
      _T("   7) serialNumber - Access point serial number'")
      _T("WHERE event_code=74")));

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
} s_dbUpgradeMap[] = {
   { 7,  50, 8,  H_UpgradeFromV7  },
   { 6,  50, 7,  H_UpgradeFromV6  },
   { 5,  50, 6,  H_UpgradeFromV5  },
   { 4,  50, 5,  H_UpgradeFromV4  },
   { 3,  50, 4,  H_UpgradeFromV3  },
   { 2,  50, 3,  H_UpgradeFromV2  },
   { 1,  50, 2,  H_UpgradeFromV1  },
   { 0,  50, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V50()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 50) && (minor < DB_SCHEMA_VERSION_V50_MINOR))
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 44.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 50.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
