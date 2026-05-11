/*
 ** NetXMS - Network Management System
 ** Driver for Qtech switches
 **
 ** Licensed under MIT lisence, as stated by the original
 ** author: https://dev.raden.solutions/issues/779#note-4
 **
 ** Copyright (c) 2015 Procyshin Dmitriy
 ** Copyright (c) 2023 Raden Solutions
 ** Copyleft (l) 2023 Anatoly Rudnev
 **
 ** Permission is hereby granted, free of charge, to any person obtaining a copy of
 ** this software and associated documentation files (the �Software�), to deal in
 ** the Software without restriction, including without limitation the rights to
 ** use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 ** of the Software, and to permit persons to whom the Software is furnished to do
 ** so, subject to the following conditions:
 **
 ** The above copyright notice and this permission notice shall be included in all
 ** copies or substantial portions of the Software.
 **
 ** THE SOFTWARE IS PROVIDED �AS IS�, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 ** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 ** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 ** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 ** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 ** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 ** SOFTWARE.
 **
 ** File: sw.cpp
 **/

#include "qtech.h"
#include <netxms-regex.h>
#include <netxms-version.h>

/**
 * Get driver name
 */
const TCHAR* QtechSWDriver::getName()
{
   return _T("QTECH-SW");
}

/**
 * Get driver version
 */
const TCHAR* QtechSWDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int QtechSWDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return (oid.startsWith({ 1, 3, 6, 1, 4, 1, 27514, 1, 1, 1 }) ||
           oid.startsWith({ 1, 3, 6, 1, 4, 1, 27514, 1, 1, 2 }) ||
           oid.startsWith({ 1, 3, 6, 1, 4, 1, 27514, 1, 1, 10 }) ||
           oid.startsWith({ 1, 3, 6, 1, 4, 1, 29763, 1, 1, 10 })) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool QtechSWDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 */
InterfaceList* QtechSWDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, true);
   if (ifList == nullptr)
      return nullptr;

   const char *eptr;
   int eoffset;

   // Standalone switch port names: Gi1/0, Te1/0, Ethernet1/1, Fo1/0, TF1/0
   PCREHandle reBase(_pcre_compile_t(
         reinterpret_cast<const PCRE_TCHAR*>(_T("^(Ethernet|Gi|Te|Fo|TF)([0-9]+)/([0-9]+)$")),
         PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr));
   if (reBase == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_QTECH, 5, _T("QtechSWDriver::getInterfaces: cannot compile base regexp: %hs at offset %d"), eptr, eoffset);
      return ifList;
   }

   // Clustered (stacked) switch port names: Gi1/0/0, Te2/1/3, ...
   PCREHandle reStack(_pcre_compile_t(
         reinterpret_cast<const PCRE_TCHAR*>(_T("^(Ethernet|Gi|Te|Fo|TF)([0-9]+)/([0-9]+)/([0-9]+)$")),
         PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr));
   if (reStack == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_QTECH, 5, _T("QtechSWDriver::getInterfaces: cannot compile stack regexp: %hs at offset %d"), eptr, eoffset);
      return ifList;
   }

   // Management port in clustered configuration: Mg1/0, Mg2/0, ...
   PCREHandle reMgmt(_pcre_compile_t(
         reinterpret_cast<const PCRE_TCHAR*>(_T("^Mg([0-9]+)/0$")),
         PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr));
   if (reMgmt == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_QTECH, 5, _T("QtechSWDriver::getInterfaces: cannot compile management regexp: %hs at offset %d"), eptr, eoffset);
      return ifList;
   }

   int pmatch[30];
   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      nxlog_debug_tag(DEBUG_TAG_QTECH, 6, _T("QtechSWDriver::getInterfaces(%s [%u]): ifName=%s ifDescr=%s ifIndex=%u"),
            node->getName(), node->getId(), iface->name, iface->description, iface->index);

      if (!_tcsncmp(iface->name, _T("Mg0"), 3))
      {
         // Standalone management port
         iface->isPhysicalPort = true;
         iface->location.chassis = 1;
         iface->location.port = 0;
      }
      else if (_pcre_exec_t(reBase.get(), nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30) == 4)
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
      else if (_pcre_exec_t(reMgmt.get(), nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30) == 2)
      {
         iface->isPhysicalPort = true;
         iface->location.chassis = IntegerFromCGroup(iface->name, pmatch, 1);
         iface->location.port = 0;
      }
   }

   return ifList;
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
bool QtechSWDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   _tcscpy(hwInfo->vendor, _T("Qtech"));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.27514.100.1.2.0")));  // Product code
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.2.1.1.1.0")));  // Product name
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.27514.100.1.3.0")));  // Firmware version
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.27514.100.1.101.0")));  // Serial number

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {

      const SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         v->getValueAsString(hwInfo->productCode, 32);
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         TCHAR buffer[256];
         v->getValueAsString(buffer, 256);
         StringList items = String::split(buffer, _T(" "), true);
         if (items.size() > 1)
         {
            _tcslcpy(hwInfo->productName, items.get(1), 128);
         }
         else
         {
            _tcslcpy(hwInfo->productName, _T("QTECH Switch"), 128);
         }
      }

      v = response->getVariable(2);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         v->getValueAsString(hwInfo->productVersion, 16);
      }

      v = response->getVariable(3);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         v->getValueAsString(hwInfo->serialNumber, sizeof(hwInfo->serialNumber) / sizeof(wchar_t));
      }

      delete response;
   }
   return true;
}
