/*
** NetXMS - Network Management System
** Driver for BDCOM switches
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: bdcom.cpp
**
**/

#include "bdcom.h"
#include <netxms-regex.h>

#define DEBUG_TAG_BDCOM _T("ndd.bdcom")

/**
 * Get driver name
 */
const TCHAR *BdcomDriver::getName()
{
   return _T("BDCOM");
}

/**
 * Get driver version
 */
const TCHAR *BdcomDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int BdcomDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 3320, 1 }) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param context device context
 * @param oid Device OID
 */
bool BdcomDriver::isDeviceSupported(DeviceContext *context, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Get list of interfaces for given node. BDCOM (and the GIGALINK OEM rebrand)
 * names gigabit ports "gigaethernet" rather than the more common
 * "gigabitethernet", so the regex below targets that family explicitly.
 *
 * @param context device context
 * @param node Node
 * @param driverData driver-specific data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *BdcomDriver::getInterfaces(DeviceContext *context, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(context, node, driverData, true);
   if (ifList == nullptr)
      return nullptr;

   const char *eptr;
   int eoffset;

   // Standalone switch port names: gi1/0, fa1/0, ...
   PCREHandle reBase(_pcre_compile_t(
         reinterpret_cast<const PCRE_TCHAR*>(_T("^(fastethernet|fa|gigaethernet|gi)[ ]*([0-9]+)/([0-9]+)$")),
         PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr));
   if (reBase == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_BDCOM, 5, _T("BdcomDriver::getInterfaces: cannot compile base regexp: %hs at offset %d"), eptr, eoffset);
      return ifList;
   }

   // Stacked switch port names: gi1/0/1, fa1/0/1, ...
   PCREHandle reStack(_pcre_compile_t(
         reinterpret_cast<const PCRE_TCHAR*>(_T("^(fastethernet|fa|gigaethernet|gi)[ ]*([0-9]+)/([0-9]+)/([0-9]+)$")),
         PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr));
   if (reStack == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_BDCOM, 5, _T("BdcomDriver::getInterfaces: cannot compile stack regexp: %hs at offset %d"), eptr, eoffset);
      return ifList;
   }

   int pmatch[30];
   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (_pcre_exec_t(reBase.get(), nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30) == 4)
      {
         iface->isPhysicalPort = true;
         iface->location.chassis = 1;
         iface->location.module = IntegerFromCGroup(iface->name, pmatch, 2);
         iface->location.port = IntegerFromCGroup(iface->name, pmatch, 3);
      }
      else if (_pcre_exec_t(reStack.get(), nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30) == 5)
      {
         iface->isPhysicalPort = true;
         iface->location.chassis = IntegerFromCGroup(iface->name, pmatch, 2);
         iface->location.module = IntegerFromCGroup(iface->name, pmatch, 3);
         iface->location.port = IntegerFromCGroup(iface->name, pmatch, 4);
      }
   }

   return ifList;
}

/**
 * Get hardware information from device.
 *
 * @param context device context
 * @param node Node
 * @param driverData driver data
 * @param hwInfo pointer to hardware information structure to fill
 * @return true if hardware information is available
 */
bool BdcomDriver::getHardwareInformation(DeviceContext *context, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   SNMP_Transport *snmp = context->getSNMPTransport();
   _tcscpy(hwInfo->vendor, _T("BDCOM"));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.3320.9.225.1.7.0")));  // Product code
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.2.1.1.1.0")));             // Product name (sysDescr)
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.3320.9.225.1.8.0")));  // Firmware version
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.3320.9.225.1.2.0")));  // Serial number

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      TCHAR buffer[256];

      const SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productCode, v->getValueAsString(buffer, 256), 32);
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);
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
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(BdcomDriver);

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
