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
   return (oid.startsWith({ 1, 3, 6, 1, 4, 1, 27514, 1, 1, 1 }) || oid.startsWith({ 1, 3, 6, 1, 4, 1, 27514, 1, 1, 10 })) ? 127 : 0;
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

   for (int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type == 6)
      {
         iface->location.chassis = 1;
         iface->isPhysicalPort = true;
         iface->location.module = 0;
         iface->location.port = iface->index;
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
         if (items.size() > 0)
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
         v->getValueAsString(hwInfo->serialNumber, 32);
      }

      delete response;
   }
   return true;
}
