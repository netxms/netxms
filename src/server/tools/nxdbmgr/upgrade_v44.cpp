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
** File: upgrade_v44.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 44.22 to 44.23
 */
static bool H_UpgradeFromV22()
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

   CHK_EXEC(SetMinorSchemaVersion(23));

   return true;
}

/**
 * Upgrade from 44.21 to 44.22
 */
static bool H_UpgradeFromV21()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD snmp_context_engine_id varchar(255)")));
   CHK_EXEC(SetMinorSchemaVersion(22));
   return true;
}

/**
 * Upgrade from 44.20 to 44.21
 */
static bool H_UpgradeFromV20()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE thresholds ADD last_event_message varchar(2000)")));
   CHK_EXEC(SetMinorSchemaVersion(21));
   return true;
}

/**
 * Upgrade from 44.19 to 44.20
 */
static bool H_UpgradeFromV19()
{
   static const TCHAR *batch =
      _T("ALTER TABLE interfaces ADD stp_port_state integer\n")
      _T("UPDATE interfaces SET stp_port_state=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("interfaces"), _T("stp_port_state")));

   CHK_EXEC(CreateEventTemplate(EVENT_IF_STP_STATE_CHANGED, _T("SYS_IF_STP_STATE_CHANGED"),
         EVENT_SEVERITY_NORMAL, EF_LOG, _T("977a5634-d0ec-44a0-a7a5-6e193ca57c38"),
         _T("Interface %<ifName> STP state changed from %<oldStateText> to %<newStateText>"),
         _T("Generated when system detects interface Spanning Tree state change.\r\n")
         _T("Parameters:\r\n")
         _T("   ifIndex - Interface index\r\n")
         _T("   ifName - Interface name\r\n")
         _T("   oldState - Old state\r\n")
         _T("   oldStateText - Old state as text\r\n")
         _T("   newState - New state\r\n")
         _T("   newStateText - New state as text")
      ));
   CHK_EXEC(SetMinorSchemaVersion(20));
   return true;
}

/**
 * Upgrade from 44.18 to 44.19
 */
static bool H_UpgradeFromV18()
{
   CHK_EXEC(CreateEventTemplate(EVENT_MODBUS_UNREACHABLE, _T("SYS_MODBUS_UNREACHABLE"),
         EVENT_SEVERITY_MAJOR, EF_LOG, _T("f8dc0d5f-0e46-4bbf-a91d-bb3c0412db2e"),
         _T("Modbus connectivity failed"),
         _T("Generated when node is not responding to Modbus requests.\r\n")
         _T("Parameters:\r\n")
         _T("   No event-specific parameters")
      ));
   CHK_EXEC(CreateEventTemplate(EVENT_MODBUS_OK, _T("SYS_MODBUS_OK"),
         EVENT_SEVERITY_NORMAL, EF_LOG, _T("47584648-fc4e-4757-b4e7-f6f16b146bac"),
         _T("Modbus connectivity restored"),
         _T("Generated when Modbus connectivity with the node is restored.\r\n")
         _T("Parameters:\r\n")
         _T("   No event-specific parameters")
      ));
   CHK_EXEC(SetMinorSchemaVersion(19));
   return true;
}

/**
 * Upgrade from 44.17 to 44.18
 */
static bool H_UpgradeFromV17()
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='InternalCA'")));
   CHK_EXEC(SetMinorSchemaVersion(18));
   return true;
}

/**
 * Upgrade from 44.16 to 44.17
 */
static bool H_UpgradeFromV16()
{
   static const TCHAR *batch =
      _T("ALTER TABLE nodes ADD modbus_proxy integer\n")
      _T("ALTER TABLE nodes ADD modbus_tcp_port integer\n")
      _T("ALTER TABLE nodes ADD modbus_unit_id integer\n")
      _T("UPDATE nodes SET modbus_proxy=0,modbus_tcp_port=502,modbus_unit_id=255\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("modbus_proxy")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("modbus_tcp_port")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("modbus_unit_id")));
   CHK_EXEC(SetMinorSchemaVersion(17));
   return true;
}

/**
 * Upgrade from 44.15 to 44.16
 */
static bool H_UpgradeFromV15()
{
   static const TCHAR *batch =
      _T("ALTER TABLE assets ADD last_update_timestamp integer\n")
      _T("ALTER TABLE assets ADD last_update_uid integer\n")
      _T("UPDATE assets SET last_update_timestamp=0,last_update_uid=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("assets"), _T("last_update_timestamp")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("assets"), _T("last_update_uid")));
   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Upgrade from 44.14 to 44.15
 */
static bool H_UpgradeFromV14()
{
   static const TCHAR *batch =
      _T("ALTER TABLE am_attributes ADD is_hidden char(1)\n")
      _T("UPDATE am_attributes SET is_hidden='0'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("am_attributes"), _T("is_hidden")));
   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade from 44.13 to 44.14
 */
static bool H_UpgradeFromV13()
{
   CHK_EXEC(CreateEventTemplate(EVENT_CONFIGURATION_ERROR, _T("SYS_CONFIGURATION_ERROR"),
         EVENT_SEVERITY_MINOR, EF_LOG, _T("762c581c-e9bf-11ed-a05b-0242ac120003"),
         _T("System configuration error (%<description>)"),
         _T("Generated on any system configuration error.\r\n")
         _T("Parameters:\r\n")
         _T("   subsystem - The subsystem which has the error\r\n")
         _T("   tag - Related tag for the error\r\n")
         _T("   descriptipon - Description of the error")
      ));
   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Upgrade from 44.12 to 44.13
 */
static bool H_UpgradeFromV12()
{
   CHK_EXEC(CreateConfigParam(_T("Objects.Assets.AllowDeleteIfLinked"),
         _T("0"),
         _T("Enable/disable deletion of linked assets."),
         nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

/**
 * Upgrade from 44.11 to 44.12
 */
static bool H_UpgradeFromV11()
{
   CHK_EXEC(SQLQuery( _T("UPDATE business_service_checks SET prototype_service_id=0,prototype_check_id=0 WHERE type=1 AND prototype_service_id=service_id")));
   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 44.10 to 44.11
 */
static bool H_UpgradeFromV10()
{
   CHK_EXEC(CreateEventTemplate(EVENT_ASSET_LINK, _T("SYS_ASSET_LINK"),
                                EVENT_SEVERITY_NORMAL, EF_LOG, _T("8dae7b06-b854-4d88-9eb7-721b6110c200"),
                                _T("Asset %<assetName> linked"),
                                _T("Generated when asset is linked with node.\r\n")
                                _T("Parameters:\r\n")
                                _T("   assetId - asset id\r\n")
                                _T("   assetName - asset name")
                                ));

   CHK_EXEC(CreateEventTemplate(EVENT_ASSET_UNLINK, _T("SYS_ASSET_UNLINK"),
                                EVENT_SEVERITY_NORMAL, EF_LOG, _T("f149433b-ea2f-4bfd-bf23-35e7778b1b55"),
                                _T("Asset %<assetName> unlinked"),
                                _T("Generated when asset is unlinked from node.\r\n")
                                _T("Parameters:\r\n")
                                _T("   assetId - asset id\r\n")
                                _T("   assetName - asset name")
                                ));

   CHK_EXEC(CreateEventTemplate(EVENT_ASSET_LINK_CONFLICT, _T("SYS_ASSET_LINK_CONFLICT"),
                                EVENT_SEVERITY_MINOR, EF_LOG, _T("2bfd6557-1b88-4cf0-801b-78cffb2afc3c"),
                                _T("Asset %<assetName> already linked with %<currentNodeId> node"),
                                _T("Generated when asset is linked with node.\r\n")
                                _T("Parameters:\r\n")
                                _T("   assetId - asset id found by auto linking\r\n")
                                _T("   assetName - asset name found by auto linking\r\n")
                                _T("   currentNodeId - currently linked node id\r\n")
                                _T("   currentNodeName - currently linked node name")
                                ));

   int ruleId = NextFreeEPPruleID();
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,filter_script,alarm_timeout,alarm_timeout_event) ")
                           _T("VALUES (%d,'acbf02d5-3ff1-4235-a8a8-f85755b9a06b',7944,'Generate alarm when asset linking conflict occurs','%%m',5,'ASSET_LINK_CONFLICT_%%i_%%<assetId>','',0,%d)"),
         ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_ASSET_LINK_CONFLICT);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 44.9 to 44.10
 */
static bool H_UpgradeFromV9()
{
   DB_RESULT result = SQLSelect(_T("SELECT id FROM clusters"));
   if (result != nullptr)
   {
      int count = DBGetNumRows(result);
      for (int i = 0; i < count; i++)
      {
         uint32_t clusterId = DBGetFieldULong(result, i, 0);

         TCHAR query[1024];
         _sntprintf(query, 1024,
               _T("UPDATE clusters SET zone_guid="
                     "coalesce((SELECT zone_guid FROM nodes WHERE id="
                     "(SELECT MIN(node_id) FROM cluster_members WHERE cluster_id=%d)), 0) WHERE id=%d"),
               clusterId, clusterId);
         if (!SQLQuery(query) && !g_ignoreErrors)
            return false;
      }
      DBFreeResult(result);
   }

   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade form 44.8 to 44.9
 */
static bool H_UpgradeFromV8()
{
   if (GetSchemaLevelForMajorVersion(43) < 9)
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         CHK_EXEC(SQLQuery(_T("ALTER TABLE maintenance_journal RENAME TO maintenance_journal_v43_9")));
         CHK_EXEC(CreateTable(
               _T("CREATE TABLE maintenance_journal (")
               _T("   record_id integer not null,")
               _T("   object_id integer not null,")
               _T("   author integer not null,")
               _T("   last_edited_by integer not null,")
               _T("   description $SQL:TEXT null,")
               _T("   creation_time timestamptz not null,")
               _T("   modification_time timestamptz not null,")
               _T("   PRIMARY KEY(record_id,creation_time))")));
         CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_maintjrn_creation_time ON maintenance_journal(creation_time)")));
         CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_maintjrn_object_id ON maintenance_journal(object_id)")));
         CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('maintenance_journal', 'creation_time', chunk_time_interval => interval '86400 seconds')")));

         CHK_EXEC(SQLQuery(_T("ALTER TABLE certificate_action_log RENAME TO certificate_action_log_v43_9")));
         CHK_EXEC(CreateTable(
               _T("CREATE TABLE certificate_action_log (")
               _T("   record_id integer not null,")
               _T("   operation_timestamp timestamptz not null,")
               _T("   operation integer not null,")
               _T("   user_id integer not null,")
               _T("   node_id integer not null,")
               _T("   node_guid varchar(36) null,")
               _T("   cert_type integer not null,")
               _T("   subject varchar(36) null,")
               _T("   serial integer null,")
               _T("   PRIMARY KEY(record_id,operation_timestamp))")));
         CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_cert_action_log_timestamp ON certificate_action_log(operation_timestamp)")));
         CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('certificate_action_log', 'operation_timestamp', chunk_time_interval => interval '86400 seconds')")));

         RegisterOnlineUpgrade(43, 9);
      }
      else
      {
         CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_maintjrn_creation_time ON maintenance_journal(creation_time)")));
         CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_maintjrn_object_id ON maintenance_journal(object_id)")));

         CHK_EXEC(DBRenameColumn(g_dbHandle, _T("certificate_action_log"), _T("timestamp"), _T("operation_timestamp")));
         CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_cert_action_log_timestamp ON certificate_action_log(operation_timestamp)")));
      }

      CHK_EXEC(CreateConfigParam(_T("CertificateActionLog.RetentionTime"),
            _T("370"),
            _T("Retention time in days for certificate action log. All records older than specified will be deleted by housekeeping process."),
            _T("days"),
            'I', true, false, false, false));

      CHK_EXEC(SetSchemaLevelForMajorVersion(43, 9));
   }
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade form 44.7 to 44.8
 */
static bool H_UpgradeFromV7()
{
   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      CHK_EXEC(CreateTable(
         _T("CREATE TABLE asset_change_log (")
         _T("   record_id $SQL:INT64 not null,")
         _T("   operation_timestamp timestamptz not null,")
         _T("   asset_id integer not null,")
         _T("   attribute_name varchar(63) null,")
         _T("   operation integer not null,")
         _T("   old_value varchar(2000) null,")
         _T("   new_value varchar(2000) null,")
         _T("   user_id integer not null,")
         _T("   linked_object_id integer not null,")
         _T("PRIMARY KEY(record_id,operation_timestamp))")));
      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('asset_change_log', 'operation_timestamp', chunk_time_interval => interval '86400 seconds')")));
   }
   else
   {
      CHK_EXEC(CreateTable(
         _T("CREATE TABLE asset_change_log (")
         _T("   record_id $SQL:INT64 not null,")
         _T("   operation_timestamp integer not null,")
         _T("   asset_id integer not null,")
         _T("   attribute_name varchar(63) null,")
         _T("   operation integer not null,")
         _T("   old_value varchar(2000) null,")
         _T("   new_value varchar(2000) null,")
         _T("   user_id integer not null,")
         _T("   linked_object_id integer not null,")
         _T("PRIMARY KEY(record_id))")));
   }

   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_srv_asset_log_timestamp ON asset_change_log(operation_timestamp)")));
   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_srv_asset_log_asset_id ON asset_change_log(asset_id)")));

   CHK_EXEC(CreateConfigParam(_T("AssetChangeLog.RetentionTime"),
         _T("90"),
         _T("Retention time in days for the records in asset change log. All records older than specified will be deleted by housekeeping process."),
         _T("days"),
         'I', true, false, false, false));

   // Update access rights for predefined "Admins" group
   DB_RESULT hResult = SQLSelect(_T("SELECT system_access FROM user_groups WHERE id=1073741825"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         uint64_t access = DBGetFieldUInt64(hResult, 0, 0);
         access |= SYSTEM_ACCESS_VIEW_ASSET_CHANGE_LOG;
         TCHAR query[256];
         _sntprintf(query, 256, _T("UPDATE user_groups SET system_access=") UINT64_FMT _T(" WHERE id=1073741825"), access);
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_ignoreErrors)
         return false;
   }

   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 44.6 to 44.7
 */
static bool H_UpgradeFromV6()
{
   static const TCHAR *batch =
      _T("INSERT INTO acl (object_id,user_id,access_rights) VALUES (5,1073741825,524287)\n")
      _T("ALTER TABLE object_properties ADD asset_id integer\n")
      _T("UPDATE object_properties SET asset_id=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("object_properties"), _T("asset_id")));

   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("am_enum_values"), _T("attr_value"), _T("value")));

   CHK_EXEC(CreateTable(
      _T("CREATE TABLE assets (")
      _T("   id integer not null,")
      _T("   linked_object_id integer not null,")
      _T("   PRIMARY KEY (id))")));

   CHK_EXEC(CreateTable(
      _T("CREATE TABLE asset_properties (")
      _T("   asset_id integer not null,")
      _T("   attr_name varchar(63) not null,")
      _T("   value varchar(2000) null,")
      _T("PRIMARY KEY(asset_id,attr_name))")));

   if (DBIsTableExist(g_dbHandle, _T("am_object_data")) == DBIsTableExist_Found)
   {
      CHK_EXEC(SQLQuery(_T("DROP TABLE am_object_data")));
   }

   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 44.5 to 44.6
 */
static bool H_UpgradeFromV5()
{
   CHK_EXEC(CreateConfigParam(_T("Objects.Nodes.Resolver.AddressFamilyHint"), _T("0"), _T("Address family hint for node DNS name resolver."), nullptr, 'C', true, false, false, false));

   static const TCHAR *batch =
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Objects.Nodes.Resolver.AddressFamilyHint','0','None')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Objects.Nodes.Resolver.AddressFamilyHint','1','IPv4')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Objects.Nodes.Resolver.AddressFamilyHint','2','IPv6')\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 44.4 to 44.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(CreateEventTemplate(EVENT_ASSET_AUTO_UPDATE_FAILED, _T("SYS_ASSET_AUTO_UPDATE_FAILED"),
                                EVENT_SEVERITY_MAJOR, EF_LOG, _T("f4ae5e79-9d39-41d0-b74c-b6c591930d08"),
                                _T("Automatic update of asset management attribute \"%<name>\" with value \"%<newValue>\" failed (%<reason>)"),
                                _T("Generated when automatic update of asset management attribute fails.\r\n")
                                _T("Parameters:\r\n")
                                _T("   name - attribute's name\r\n")
                                _T("   displayName - attribute's display name\r\n")
                                _T("   dataType - attribute's data type\r\n")
                                _T("   currValue - current attribute's value\r\n")
                                _T("   newValue - new attribute's value\r\n")
                                _T("   reason - failure reason")
                                ));

   int ruleId = NextFreeEPPruleID();
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,filter_script,alarm_timeout,alarm_timeout_event) ")
                           _T("VALUES (%d,'1499d2d3-2304-4bb1-823b-0c530cbb6224',7944,'Generate alarm when automatic update of asset management attribute fails','%%m',5,'ASSET_UPDATE_FAILED_%%i_%%<name>','',0,%d)"),
         ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_ASSET_AUTO_UPDATE_FAILED);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 44.3 to 44.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when threshold value reached for specific data collection item.\r\n")
      _T("Parameters are accessible via %<...> and can have \"m\" or \"multipliers\" and \"u\" or \"units\" format modifiers for value formatting (for example %<{m,u}currentValue>).\r\n\r\n")
      _T("Parameters:\r\n")
      _T("   dciName - Parameter name\r\n")
      _T("   dciDescription - Item description\r\n")
      _T("   thresholdValue - Threshold value\r\n")
      _T("   currentValue - Actual value which is compared to threshold value\r\n")
      _T("   dciId - Data collection item ID\r\n")
      _T("   instance - Instance\r\n")
      _T("   isRepeatedEvent - Repeat flag\r\n")
      _T("   dciValue - Last collected DCI value\r\n")
      _T("   operation - Threshold''s operation code\r\n")
      _T("   function - Threshold''s function code\r\n")
      _T("   pollCount - Threshold''s required poll count\r\n")
      _T("   thresholdDefinition - Threshold''s textual definition'")
      _T(" WHERE event_code=17")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when threshold check is rearmed for specific data collection item.\r\n")
      _T("Parameters are accessible via %<...> and can have \"m\" or \"multipliers\" and \"u\" or \"units\" format modifiers for value formatting (for example %<{m,u}currentValue>)\r\n\r\n")
      _T("Parameters:\r\n")
      _T("   dciName - Parameter name\r\n")
      _T("   dciDescription - Item description\r\n")
      _T("   dciId - Data collection item ID\r\n")
      _T("   instance - Instance\r\n")
      _T("   thresholdValue - Threshold value\r\n")
      _T("   currentValue - Actual value which is compared to threshold value\r\n")
      _T("   dciValue - Last collected DCI value\r\n")
      _T("   operation - Threshold''s operation code\r\n")
      _T("   function - Threshold''s function code\r\n")
      _T("   pollCount - Threshold''s required poll count\r\n")
      _T("   thresholdDefinition - Threshold''s textual definition'")
      _T( "WHERE event_code=18")));

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 44.2 to 44.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("object_properties"), _T("submap_id"), _T("drilldown_object_id")));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 44.1 to 44.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(CreateConfigParam(_T("Server.Security.2FA.TrustedDeviceTTL"), _T("0"), _T("TTL in seconds for 2FA trusted device."), _T("seconds"), 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 44.0 to 44.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE am_attributes (")
      _T("   attr_name varchar(63) not null,")
      _T("   display_name varchar(255) null,")
      _T("   data_type integer not null,")
      _T("   is_mandatory char(1) not null,")
      _T("   is_unique char(1) not null,")
      _T("   autofill_script $SQL:TEXT null,")
      _T("   range_min integer not null,")
      _T("   range_max integer not null,")
      _T("   sys_type integer not null,")
      _T("   PRIMARY KEY(attr_name))")));

   CHK_EXEC(CreateTable(
      _T("CREATE TABLE am_enum_values (")
      _T("   attr_name varchar(63) not null,")
      _T("   attr_value varchar(63) not null,")
      _T("   display_name varchar(255) not null,")
      _T("   PRIMARY KEY(attr_name,attr_value))")));

   // Update access rights for predefined "Admins" group
   DB_RESULT hResult = SQLSelect(_T("SELECT system_access FROM user_groups WHERE id=1073741825"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         uint64_t access = DBGetFieldUInt64(hResult, 0, 0);
         access |= SYSTEM_ACCESS_MANAGE_AM_SCHEMA;
         TCHAR query[256];
         _sntprintf(query, 256, _T("UPDATE user_groups SET system_access=") UINT64_FMT _T(" WHERE id=1073741825"), access);
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_ignoreErrors)
         return false;
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
} s_dbUpgradeMap[] = {
   { 22, 44, 23, H_UpgradeFromV22 },
   { 21, 44, 22, H_UpgradeFromV21 },
   { 20, 44, 21, H_UpgradeFromV20 },
   { 19, 44, 20, H_UpgradeFromV19 },
   { 18, 44, 19, H_UpgradeFromV18 },
   { 17, 44, 18, H_UpgradeFromV17 },
   { 16, 44, 17, H_UpgradeFromV16 },
   { 15, 44, 16, H_UpgradeFromV15 },
   { 14, 44, 15, H_UpgradeFromV14 },
   { 13, 44, 14, H_UpgradeFromV13 },
   { 12, 44, 13, H_UpgradeFromV12 },
   { 11, 44, 12, H_UpgradeFromV11 },
   { 10, 44, 11, H_UpgradeFromV10 },
   { 9,  44, 10, H_UpgradeFromV9  },
   { 8,  44, 9,  H_UpgradeFromV8  },
   { 7,  44, 8,  H_UpgradeFromV7  },
   { 6,  44, 7,  H_UpgradeFromV6  },
   { 5,  44, 6,  H_UpgradeFromV5  },
   { 4,  44, 5,  H_UpgradeFromV4  },
   { 3,  44, 4,  H_UpgradeFromV3  },
   { 2,  44, 3,  H_UpgradeFromV2  },
   { 1,  44, 2,  H_UpgradeFromV1  },
   { 0,  44, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V44()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 44) && (minor < DB_SCHEMA_VERSION_V44_MINOR))
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
      _tprintf(_T("Upgrading from version 44.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
