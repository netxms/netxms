/* 
** NetXMS - Network Management System
** Driver for Edgecore enterprise switches
** Copyright (C) 2024 Raden Solutions
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
** File: esw.cpp
**/

#include "edgecore.h"
#include <netxms-regex.h>

/**
 * Get driver name
 */
const TCHAR *EdgecoreEnterpriseSwitchDriver::getName()
{
   return _T("EDGECORE-ESW");
}

/**
 * Get driver version
 */
const TCHAR *EdgecoreEnterpriseSwitchDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int EdgecoreEnterpriseSwitchDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 259, 10, 1 }) ? 200 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool EdgecoreEnterpriseSwitchDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   static uint32_t subid[] = { 1, 1, 5, 1, 0 };
   SNMP_ObjectId queryOID = oid;
   queryOID.truncate(queryOID.length() - 10);
   queryOID.extend(subid, 5);
   BYTE buffer[256];
	return SnmpGetEx(snmp, nullptr, queryOID.value(), queryOID.length(), buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS;
}

/**
 * Do additional checks on the device required by driver.
 * Driver can set device's custom attributes and driver's data from within this method.
 * If driver's data was set on previous call, same pointer will be passed on all subsequent calls.
 * It is driver's responsibility to destroy existing object if it is to be replaced. One data
 * object should not be used for multiple nodes. Data object may be destroyed by framework when no longer needed.
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 * @param node Node
 * @param driverData pointer to pointer to driver-specific data
 */
void EdgecoreEnterpriseSwitchDriver::analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData)
{
   if (*driverData == nullptr)
   {
      *driverData = new ESWDriverData(oid.getElement(9));
   }
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
bool EdgecoreEnterpriseSwitchDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   if (driverData == nullptr)
      return false;

   _tcscpy(hwInfo->vendor, _T("Edgecore"));

   uint32_t oid[] = { 1, 3, 6, 1, 4, 1, 259, 10, 1, static_cast<ESWDriverData*>(driverData)->getSubtree(), 1, 1, 5, 3, 0, 0 };

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(oid, sizeof(oid) / sizeof(uint32_t) - 1));  // Product name
   oid[13] = 1;
   request.bindVariable(new SNMP_Variable(oid, sizeof(oid) / sizeof(uint32_t) - 1));  // Product code
   oid[13] = 4;
   request.bindVariable(new SNMP_Variable(oid, sizeof(oid) / sizeof(uint32_t) - 1));  // Firmware version
   oid[12] = 3;
   oid[13] = 1;
   oid[14] = 10;
   oid[15] = 1;
   request.bindVariable(new SNMP_Variable(oid, sizeof(oid) / sizeof(uint32_t)));  // Serial number

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      TCHAR buffer[256];
      const SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productCode, v->getValueAsString(buffer, 256), 32);
      }

      v = response->getVariable(2);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
      }

      v = response->getVariable(3);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->serialNumber, v->getValueAsString(buffer, 256), 32);
      }

      delete response;
   }
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
InterfaceList *EdgecoreEnterpriseSwitchDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
	if (ifList == nullptr)
	   return nullptr;

   const char *eptr;
   int eoffset;
   PCRE *re = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("Ethernet Port on unit ([0-9]+), port ([0-9]+)$")), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
   if (re == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_EDGECORE, 5, _T("EdgecoreEnterpriseSwitchDriver::getInterfaces: cannot compile base regexp: %hs at offset %d"), eptr, eoffset);
      return ifList;
   }

   int pmatch[30];
	for(int i = 0; i < ifList->size(); i++)
	{
	   InterfaceInfo *iface = ifList->get(i);
	   if (iface->type != IFTYPE_ETHERNET_CSMACD)
	      continue;   // Logical interface

      if (_pcre_exec_t(re, nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->description), static_cast<int>(_tcslen(iface->description)), 0, 0, pmatch, 30) == 3)
      {
         iface->isPhysicalPort = true;
         iface->location.chassis = IntegerFromCGroup(iface->description, pmatch, 1);
         iface->location.port = IntegerFromCGroup(iface->description, pmatch, 2);
      }
	}

	return ifList;
}
