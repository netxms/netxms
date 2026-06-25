/* 
** NetXMS - Network Management System
** Driver for Huawei switch devices
** Copyright (C) 2003-2024 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: lansw.cpp
**/

#include "huawei.h"
#include <ieee8021x.h>

#define DEBUG_TAG _T("ndd.huawei.lansw")

/**
 * Get driver name
 */
const TCHAR *HuaweiSWDriver::getName()
{
   return _T("HUAWEI-SW");
}

/**
 * Get driver version
 */
const TCHAR *HuaweiSWDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int HuaweiSWDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	return oid.startsWith({ 1, 3, 6, 1, 4, 1, 2011, 2, 23 }) ? 255 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool HuaweiSWDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
	return true;
}

/**
 * Get hardware information from device.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param hwInfo pointer to hardware information structure to fill
 * @return true if hardware information is available
 */
bool HuaweiSWDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   // Only set vendor here, rest will be retrieved from ENTITY MIB
   _tcscpy(hwInfo->vendor, _T("Huawei"));
   return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *HuaweiSWDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, true);
   if (ifList == nullptr)
      return nullptr;
   
   // Physical interfaces can be mixed with virtual ones, so we just count ports, assuming single chassis and consequential port numbering.
   uint32_t port = 1;
   for (int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type == 6)
      {
         iface->location.chassis = 1;
         iface->isPhysicalPort = true;
         iface->location.module = 0;
         iface->location.port = port++;
      }
   }

   return ifList;
}

/**
 * Check if device supports IEEE 802.1x. Probes hwDot1xGlobal (.1.3.6.1.4.1.2011.5.25.40.4.1.2.0) from
 * HUAWEI-BRAS-SRVCFG-EAP-MIB and returns true if it equals enabled(1).
 */
bool HuaweiSWDriver::is8021xSupported(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return CheckSNMPIntegerValue(snmp, { 1, 3, 6, 1, 4, 1, 2011, 5, 25, 40, 4, 1, 2, 0 }, 1);
}

/**
 * Read 802.1x state for given interface from HUAWEI-BRAS-SRVCFG-EAP-MIB hwDot1xPortConfigTable
 * (.1.3.6.1.4.1.2011.5.25.40.4.1.14.1.<col>.<ifIndex>):
 *   col 2  hwDot1xPortSwitch   - 1=enabled, 2=disabled
 *   col 5  hwDot1xPortControl  - 1=auto, 2=authorizedForce, 3=unauthorizedForce
 *   col 11 hwDot1xAuthStatus   - TruthValue, 1=true (authenticated), 2=false
 *
 * PAE state is synthesized from these. Backend state has no per-port analog in this MIB and is left UNKNOWN.
 */
void HuaweiSWDriver::get8021xPortState(SNMP_Transport *snmp, NObject *node, DriverData *driverData, uint32_t ifIndex,
         int32_t *paeState, int32_t *backendState)
{
   uint32_t oid[16] = { 1, 3, 6, 1, 4, 1, 2011, 5, 25, 40, 4, 1, 14, 1, 2, ifIndex };

   int32_t portSwitch = 0;
   if ((SnmpGetEx(snmp, nullptr, oid, 16, &portSwitch, sizeof(int32_t), 0) != SNMP_ERR_SUCCESS) || (portSwitch != 1))
      return;  // 802.1x not enabled on this port (or unreadable) — leave PAE state UNKNOWN

   oid[14] = 5;  // hwDot1xPortControl
   int32_t portControl = 0;
   if (SnmpGetEx(snmp, nullptr, oid, 16, &portControl, sizeof(int32_t), 0) != SNMP_ERR_SUCCESS)
      return;

   switch (portControl)
   {
      case 2:  // authorizedForce
         *paeState = PAE_STATE_FORCE_AUTH;
         return;
      case 3:  // unauthorizedForce
         *paeState = PAE_STATE_FORCE_UNAUTH;
         return;
      case 1:  // auto - need to query runtime auth status
         break;
      default:
         return;
   }

   oid[14] = 11;  // hwDot1xAuthStatus
   int32_t authStatus = 0;
   if (SnmpGetEx(snmp, nullptr, oid, 16, &authStatus, sizeof(int32_t), 0) == SNMP_ERR_SUCCESS)
      *paeState = (authStatus == 1) ? PAE_STATE_AUTHENTICATED : PAE_STATE_DISCONNECTED;
}

/**
 * Get additional STP bridge ID. Huawei devices running M-LAG (V-STP) present the M-LAG system as a single
 * logical bridge with a shared virtual bridge ID that differs from each chassis' dot1dBaseBridgeAddress.
 * On every downstream M-LAG member port the device advertises that shared ID (which equals the peer chassis'
 * bridge MAC) as the segment's designated bridge. Reporting it here lets the STP topology code recognize
 * these as the node's own designated ports and avoid creating false inter-switch links toward the M-LAG peer.
 */
MacAddress HuaweiSWDriver::getStpBridgeId(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   // hwMstpiBridgeID (1.3.6.1.4.1.2011.5.25.42.4.1.19.1.2.0) - bridge identifier for the spanning tree
   // instance, encoded as 8 octets: 2-byte priority followed by the 6-byte bridge MAC address.
   BYTE bridgeId[16];
   uint32_t dataLen = 0;
   if ((SnmpGetEx(snmp, { 1, 3, 6, 1, 4, 1, 2011, 5, 25, 42, 4, 1, 19, 1, 2, 0 }, bridgeId, sizeof(bridgeId), SG_RAW_RESULT, &dataLen) != SNMP_ERR_SUCCESS) || (dataLen < 8))
      return MacAddress();
   return MacAddress(&bridgeId[2], 6);
}

/**
 * Get SSH driver hints for Huawei VRP devices
 */
void HuaweiSWDriver::getSSHDriverHints(SSHDriverHints *hints) const
{
   // Huawei VRP prompt patterns:
   // - User view: <hostname>
   // - System view: [hostname]
   // - Interface view: [hostname-GigabitEthernet0/0/1]
   // - Other config views: [hostname-xxx]
   hints->promptPattern = "^[<\\[][\\w.-]+([-][\\w/.-]+)?[>\\]]\\s*$";
   hints->enabledPromptPattern = "^\\[[\\w.-]+([-][\\w/.-]+)?\\]\\s*$";

   // Privilege escalation uses "super" command
   hints->enableCommand = "super";
   hints->enablePromptPattern = "[Pp]assword:\\s*$";

   // Pagination control
   hints->paginationDisableCmd = "screen-length 0 temporary";
   hints->paginationPrompt = "---- More ----|  ---- More ----|^--More--";
   hints->paginationContinue = " ";

   // Exit command
   hints->exitCommand = "quit";

   // Test command for verifying command mode support
   hints->testCommand = "display version | include VRP";
   hints->testCommandPattern = "VRP";

   // Timeouts
   hints->commandTimeout = 30000;
   hints->connectTimeout = 15000;
}
