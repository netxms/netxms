/*
** NetXMS - Network Management System
** Drivers for Fortinet devices
** Copyright (C) 2023-2024 Raden Solutions
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
** File: fortigayte.cpp
**
**/

#include "fortinet.h"

/**
 * Get driver name
 */
const TCHAR *FortiGateDriver::getName()
{
   return _T("FORTIGATE");
}

/**
 * Get driver version
 */
const TCHAR *FortiGateDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int FortiGateDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 12356, 101, 1 }) ? 250 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool FortiGateDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
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
bool FortiGateDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.2.1.47.1.1.1.1.13.1")));  // first entry in entPhysicalModelName
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.12356.100.1.1.1.0")));  // fnSysSerial
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.12356.101.4.1.1.0")));  // fgSysVersion

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return false;

   if (response->getNumVariables() != request.getNumVariables())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Malformed device response in FortiGateDriver::getHardwareInformation(%s [%u])"), node->getName(), node->getId());
      delete response;
      return false;
   }

   _tcscpy(hwInfo->vendor, _T("Fortinet"));

   response->getVariable(0)->getValueAsString(hwInfo->productName, sizeof(hwInfo->productName) / sizeof(TCHAR));
   response->getVariable(1)->getValueAsString(hwInfo->serialNumber, sizeof(hwInfo->serialNumber) / sizeof(TCHAR));
   response->getVariable(2)->getValueAsString(hwInfo->productVersion, sizeof(hwInfo->productVersion) / sizeof(TCHAR));

   for(int i = 0; hwInfo->productName[i] != 0; i++)
   {
      if (hwInfo->productName[i] == '_')
         hwInfo->productName[i] = '-';
   }

   delete response;
   return true;
}

/**
 * Get device virtualization type.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param vtype pointer to virtualization type enum to fill
 * @return true if virtualization type is known
 */
bool FortiGateDriver::getVirtualizationType(SNMP_Transport *snmp, NObject *node, DriverData *driverData, VirtualizationType *vtype)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 2, 1, 47, 1, 1, 1, 1, 13, 1 }));  // first entry in entPhysicalModelName

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return false;

   if (response->getNumVariables() != request.getNumVariables())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Malformed device response in FortiGateDriver::getVirtualizationType(%s [%u])"), node->getName(), node->getId());
      delete response;
      return false;
   }

   TCHAR model[128];
   response->getVariable(0)->getValueAsString(model, sizeof(model) / sizeof(TCHAR));
   _tcsupr(model);
   *vtype = ((_tcsstr(model, _T("_VM")) != nullptr) || (_tcsstr(model, _T("-VM")) != nullptr)) ? VTYPE_FULL : VTYPE_NONE;
   delete response;
   return true;
}

/**
 * Get interface state. Both states must be set to UNKNOWN if cannot be read from device.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver's data
 * @param ifIndex interface index
 * @param ifName interface name
 * @param ifType interface type
 * @param ifTableSuffixLen length of interface table suffix
 * @param ifTableSuffix interface table suffix
 * @param adminState OUT: interface administrative state
 * @param operState OUT: interface operational state
 * @param speed OUT: updated interface speed
 */
void FortiGateDriver::getInterfaceState(SNMP_Transport *snmp, NObject *node, DriverData *driverData, uint32_t ifIndex, const TCHAR *ifName,
         uint32_t ifType, int ifTableSuffixLen, const uint32_t *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState, uint64_t *speed)
{
   NetworkDeviceDriver::getInterfaceState(snmp, node, driverData, ifIndex, ifName, ifType, ifTableSuffixLen, ifTableSuffix, adminState, operState, speed);
   if ((ifType == IFTYPE_TUNNEL) && (*adminState == IF_ADMIN_STATE_UP))
   {
      // Find tunnel index by name
      uint32_t tunnelIndex[2] = { 0, 0 };
      SnmpWalk(snmp, _T(".1.3.6.1.4.1.12356.101.12.2.2.1.2"),
         [&tunnelIndex, ifName, node] (SNMP_Variable *v) -> uint32_t {
            TCHAR name[256];
            v->getValueAsString(name, 256);
            if (!_tcscmp(name, ifName))
            {
               memcpy(tunnelIndex, &v->getName().value()[v->getName().length() - 2], 2 * sizeof(uint32_t));
               nxlog_debug_tag(DEBUG_TAG, 6, _T("FortiGateDriver::getInterfaceState(%s [%u]): found tunnel entry for interface \"%s\""), node->getName(), node->getId(), ifName);
               return SNMP_ERR_ABORTED;
            }
            return SNMP_ERR_SUCCESS;
         });
      if (tunnelIndex[0] != 0)
      {
         uint32_t oid[] = { 1, 3, 6, 1, 4, 1, 12356, 101, 12, 2, 2, 1, 20, 0, 0 };
         memcpy(&oid[13], tunnelIndex, 2 * sizeof(uint32_t));

         uint32_t state;
         if (SnmpGetEx(snmp, nullptr, oid, sizeof(oid) / sizeof(uint32_t), &state, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS)
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("FortiGateDriver::getInterfaceState(%s [%u]): tunnel \"%s\" state %d"), node->getName(), node->getId(), ifName, state);
            switch(state)
            {
               case 1:
                  *operState = IF_OPER_STATE_DOWN;
                  break;
               case 2:
                  *operState = IF_OPER_STATE_UP;
                  break;
               default:
                  *operState = IF_OPER_STATE_UNKNOWN;
                  break;
            }
         }
      }
   }
}
