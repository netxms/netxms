/*
** NetXMS - Network Management System
** Driver for HPE iLO controllers
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: ilo.cpp
**/

#include "hpe.h"

/**
 * Get driver name
 */
const TCHAR *ILODriver::getName()
{
   return _T("HPE-ILO");
}

/**
 * Get driver version
 */
const TCHAR *ILODriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int ILODriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 232, 9, 4 }) ? 255 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool ILODriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
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
bool ILODriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   _tcscpy(hwInfo->vendor, _T("Hewlett Packard Enterprise"));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.232.9.2.2.21.0"))); // Controller model
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.232.9.2.2.2.0")));  // Version

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      const SNMP_Variable *v = response->getVariable(0);
      if (v != nullptr)
      {
         switch(v->getValueAsInt())
         {
            case 2:
               _tcscpy(hwInfo->productName, _T("EISA Remote Insight"));
               break;
            case 3:
               _tcscpy(hwInfo->productName, _T("PCI Remote Insight"));
               break;
            case 4:
               _tcscpy(hwInfo->productName, _T("Remote Insight Lights-Out"));
               break;
            case 5:
               _tcscpy(hwInfo->productName, _T("Integrated Remote Insight Lights-Out"));
               break;
            case 6:
               _tcscpy(hwInfo->productName, _T("Remote Insight Lights-Out Version II"));
               break;
            case 7:
               _tcscpy(hwInfo->productName, _T("Integrated Lights-Out 2"));
               break;
            case 8:
               _tcscpy(hwInfo->productName, _T("Integrated Lights-Out 100"));
               break;
            case 9:
               _tcscpy(hwInfo->productName, _T("Integrated Lights-Out 3"));
               break;
            case 10:
               _tcscpy(hwInfo->productName, _T("Integrated Lights-Out 4"));
               break;
            case 11:
               _tcscpy(hwInfo->productName, _T("Integrated Lights-Out 5"));
               break;
         }
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         v->getValueAsString(hwInfo->productVersion, 16);
      }

      delete response;
   }
   return true;
}

/**
 * Handler for interface walk
 */
static uint32_t HandlerInterfaceList(SNMP_Variable *var, SNMP_Transport *snmp, InterfaceList *ifList)
{
   SNMP_ObjectId oid = var->getName();

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   oid.changeElement(12, 4); // cpqSm2NicMacAddress
   request.bindVariable(new SNMP_Variable(oid));
   oid.changeElement(12, 5); // cpqSm2NicIpAddress
   request.bindVariable(new SNMP_Variable(oid));
   oid.changeElement(12, 6); // cpqSm2NicIpSubnetMask
   request.bindVariable(new SNMP_Variable(oid));
   oid.changeElement(12, 12); // cpqSm2NicMtu
   request.bindVariable(new SNMP_Variable(oid));
   oid.changeElement(12, 9); // cpqSm2NicSpeed
   request.bindVariable(new SNMP_Variable(oid));
   oid.changeElement(12, 3); // cpqSm2NicType
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == request.getNumVariables())
      {
         InterfaceInfo *iface = new InterfaceInfo(oid.getLastElement());
         var->getValueAsString(iface->name, MAX_DB_STRING);
         if (iface->name[0] == 0)
         {
            _sntprintf(iface->name, MAX_DB_STRING, _T("eth%u"), oid.getLastElement());
         }

         response->getVariable(0)->getRawValue(iface->macAddr, sizeof(iface->macAddr));

         InetAddress addr(ntohl(response->getVariable(1)->getValueAsUInt()));
         InetAddress mask(ntohl(response->getVariable(2)->getValueAsUInt()));
         if (addr.isValid() && mask.isValid())
         {
            addr.setMaskBits(BitsInMask(mask.getAddressV4()));
            iface->ipAddrList.add(addr);
         }

         iface->mtu = response->getVariable(3)->getValueAsUInt();
         iface->speed = static_cast<uint64_t>(response->getVariable(4)->getValueAsUInt()) * _ULL(1000000);  // Mbps to bps

         int type = response->getVariable(5)->getValueAsInt();
         if (type == 2)
            iface->type = IFTYPE_ETHERNET_CSMACD;
         else if (type == 3)
            iface->type = IFTYPE_ISO88025_TOKENRING;

         ifList->add(iface);
      }
      delete response;
   }

   return SNMP_ERR_SUCCESS;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 */
InterfaceList *ILODriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = new InterfaceList(8);
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.232.9.2.5.1.1.2"), HandlerInterfaceList, ifList) != SNMP_ERR_SUCCESS)
      delete_and_null(ifList);
   return ifList;
}

/**
 * Base OID for iLO NICs
 */
static SNMP_ObjectId s_cpqSm2NicEnabledStatus(SNMP_ObjectId::parse(_T(".1.3.6.1.4.1.232.9.2.5.1.1.7")));

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
void ILODriver::getInterfaceState(SNMP_Transport *snmp, NObject *node, DriverData *driverData, uint32_t ifIndex, const TCHAR *ifName,
         uint32_t ifType, int ifTableSuffixLen, const uint32_t *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState, uint64_t *speed)
{
   SNMP_ObjectId oid(s_cpqSm2NicEnabledStatus, ifIndex);

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(oid));
   oid.changeElement(12, 11); // cpqSm2NicCondition
   request.bindVariable(new SNMP_Variable(oid));
   oid.changeElement(12, 9); // cpqSm2NicSpeed
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == request.getNumVariables())
      {
         switch(response->getVariable(0)->getValueAsInt())
         {
            case 2:
               *adminState = IF_ADMIN_STATE_UP;
               break;
            case 3:
               *adminState = IF_ADMIN_STATE_DOWN;
               break;
            default:
               *adminState = IF_ADMIN_STATE_UNKNOWN;
               break;
         }

         switch(response->getVariable(1)->getValueAsInt())
         {
            case 2:
            case 3:  // degraded - consider it still up
               *operState = IF_OPER_STATE_UP;
               break;
            case 4:
               *operState = IF_OPER_STATE_DOWN;
               break;
            default:
               *operState = IF_OPER_STATE_UNKNOWN;
               break;
         }

         *speed = static_cast<uint64_t>(response->getVariable(2)->getValueAsUInt()) * _ULL(1000000);  // Mbps to bps
      }
      else
      {
         *adminState = IF_ADMIN_STATE_UNKNOWN;
         *operState = IF_OPER_STATE_UNKNOWN;
      }
      delete response;
   }
   else
   {
      *adminState = IF_ADMIN_STATE_UNKNOWN;
      *operState = IF_OPER_STATE_UNKNOWN;
   }
}
