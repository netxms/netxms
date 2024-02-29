/*
** NetXMS - Network Management System
** Driver for Qtech OLT switches
**
** Licensed under MIT lisence, as stated by the original
** author: https://dev.raden.solutions/issues/779#note-4
** 
** Copyright (c) 2015 Procyshin Dmitriy
** Copyright (c) 2023 Raden Solutions
** 
** Permission is hereby granted, free of charge, to any person obtaining a copy of
** this software and associated documentation files (the “Software”), to deal in
** the Software without restriction, including without limitation the rights to
** use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
** of the Software, and to permit persons to whom the Software is furnished to do
** so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
**
** File: olt.cpp
**/

#include "qtech.h"
#include <netxms-version.h>

/**
 * Get driver name
 */
const TCHAR *QtechOLTDriver::getName()
{
	return _T("QTECH-OLT");
}

/**
 * Get driver version
 */
const TCHAR *QtechOLTDriver::getVersion()
{
	return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int QtechOLTDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	return oid.equals({ 1, 3, 6, 1, 4, 1, 27514, 1, 10, 4, 1 }) ? 254 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool QtechOLTDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
	return true;
}

/**
 * Handler for enumerating indexes
 */
static uint32_t HandlerIndex(SNMP_Variable *pVar, SNMP_Transport *pTransport, InterfaceList *ifList)
{
   uint32_t slot = pVar->getName().getElement(14);
   InterfaceInfo *info = new InterfaceInfo(pVar->getValueAsUInt() + slot * 1000);
   info->location.module = slot;
   info->location.port = info->index - slot * 1000;
   info->isPhysicalPort = true;
   ifList->add(info);
   return SNMP_ERR_SUCCESS;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 */
InterfaceList *QtechOLTDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, false);
   if (ifList == nullptr)
      return nullptr;

   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.27514.1.11.4.1.1.1"), HandlerIndex, ifList) == SNMP_ERR_SUCCESS)
   {
      for(int i = 0; i < ifList->size(); i++)
      {
         InterfaceInfo *iface = ifList->get(i);
         if (iface->index > 1000)
         {
            uint32_t oid[MAX_OID_LEN];
            SnmpParseOID(_T(".1.3.6.1.4.1.27514.1.11.4.1.1.0.0.0.0"), oid, MAX_OID_LEN);
            oid[12] = 22;
            oid[14] = iface->location.module;
            oid[15] = iface->index - iface->location.module * 1000;
            iface->type = IFTYPE_GPON;
            if (SnmpGetEx(snmp, nullptr, oid, 16, iface->alias, MAX_DB_STRING * sizeof(TCHAR), 0, nullptr) == SNMP_ERR_SUCCESS)
            {
               Trim(iface->alias);
            }
            oid[12] = 13;
            if (SnmpGetEx(snmp, nullptr, oid, 16, iface->description, MAX_DB_STRING * sizeof(TCHAR), 0, nullptr) == SNMP_ERR_SUCCESS)
            {
               Trim(iface->description);
            }
            oid[12] = 2;
            if (SnmpGetEx(snmp, nullptr, oid, 16, iface->name, MAX_DB_STRING * sizeof(TCHAR), 0, nullptr) != SNMP_ERR_SUCCESS)
            {
               iface->type = IFTYPE_OTHER;
            }
         }
      }
   }
   return ifList;
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
void QtechOLTDriver::getInterfaceState(SNMP_Transport *snmp, NObject *node, DriverData *driverData, uint32_t ifIndex,const TCHAR *ifName,
      uint32_t ifType, int ifTableSuffixLen, const uint32_t *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState, uint64_t *speed)
{
   *adminState = IF_ADMIN_STATE_UP;
   if (ifIndex > 1000)
   {
      uint32_t oid[MAX_OID_LEN];
      SnmpParseOID(_T(".1.3.6.1.4.1.27514.1.11.4.1.1.3.0.0.0"), oid, MAX_OID_LEN);
      oid[14] = ifIndex / 1000;
      oid[15] = ifIndex - oid[14] * 1000;
      uint32_t operStatus = 0;
      SnmpGetEx(snmp, nullptr, oid, 16, &operStatus, sizeof(uint32_t), 0, nullptr);
      switch(operStatus)
      {
         case 1:
            *operState = IF_OPER_STATE_UP;
            break;
         case 0:
            *operState = IF_OPER_STATE_DOWN;
            break;
         default:
            *operState = IF_OPER_STATE_UNKNOWN;
            break;
      }
   }
   else
   {
      NetworkDeviceDriver::getInterfaceState(snmp, node, driverData, ifIndex, ifName, ifType, ifTableSuffixLen, ifTableSuffix, adminState, operState, speed);
   }
}
