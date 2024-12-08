/* 
** NetXMS - Network Management System
** Driver for Cisco Small Business switches
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
** File: sb.cpp
**/

#include "cisco.h"

#define DEBUG_TAG_CISCO_SB DEBUG_TAG_CISCO _T(".sb")

/**
 * Get driver name
 */
const TCHAR *CiscoSbDriver::getName()
{
	return _T("CISCO-SB");
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int CiscoSbDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	if (!oid.startsWith({ 1, 3, 6, 1, 4, 1, 9, 6, 1 }))
		return 0;

	uint32_t model = oid.getElement(9);
	if ((model == 80) ||  // SF500
	    (model == 81) ||  // SG500
       (model == 82) ||  // SF300
       (model == 83) ||  // SG300
       (model == 84) ||  // SG220, SF220
       (model == 85) ||  // SG500X
       (model == 86) ||  // ESW2-350G, ESW2-550X
       (model == 87) ||  // SF200
       (model == 88) ||  // SG200, SF200
       (model == 90) ||  // SG550XG
       (model == 91) ||  // SG350XG
       (model == 92) ||  // SF550X
       (model == 94) ||  // SG350X
       (model == 95) ||  // SG350, SG355
       (model == 96) ||  // SF350
       (model == 97) ||  // SG250
       (model == 98))    // SF250
		return 252;
	return 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool CiscoSbDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
	return true;
}

/**
 * Handler for physical port positions
 */
static uint32_t HandlerPhysicalPorts(SNMP_Variable *var, SNMP_Transport *snmp, SB_MODULE_LAYOUT *layout)
{
   uint32_t moduleIndex = var->getValueAsUInt();
   if (moduleIndex >= SB_MAX_MODULE_NUMBER)
      return SNMP_ERR_SUCCESS;

   SB_MODULE_LAYOUT *module = &layout[moduleIndex - 1];
   module->index = moduleIndex;

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   uint32_t oid[16];
   size_t oidLen = var->getName().length();
   memcpy(oid, var->getName().value(), oidLen * sizeof(uint32_t));
   oid[13] = 6;   // rlPhdPortsRow
   request.bindVariable(new SNMP_Variable(oid, oidLen));
   oid[13] = 7;   // rlPhdPortsColumn
   request.bindVariable(new SNMP_Variable(oid, oidLen));
   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      if ((response->getNumVariables() == 2) &&
          response->getVariable(0)->isInteger() &&
          response->getVariable(1)->isInteger())
      {
         UINT32 row = response->getVariable(0)->getValueAsUInt();
         UINT32 column = response->getVariable(1)->getValueAsUInt();
         if ((row >= 1) && (row <= 2) && (column >= 1) && (column <= SB_MAX_INTERFACES_PER_ROW))
         {
            if (module->rows < row)
               module->rows = row;
            if (module->columns < column)
               module->columns = column;
            if ((module->minIfIndex > oid[14]) || (module->minIfIndex == 0))
               module->minIfIndex = oid[14];
            if (module->maxIfIndex < oid[14])
               module->maxIfIndex = oid[14];
            module->interfaces[(row - 1) * SB_MAX_INTERFACES_PER_ROW + column - 1] = oid[14];
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_CISCO_SB, 7,
                  _T("HandlerPhysicalPorts: invalid response for interface %u (row=%u column=%u)"), oid[14], row, column);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_CISCO_SB, 7, _T("HandlerPhysicalPorts: invalid response for interface %u"), oid[14]);
      }
      delete response;
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Get physical layout information for modules
 */
int CiscoSbDriver::getPhysicalPortLayout(SNMP_Transport *snmp, SB_MODULE_LAYOUT *layout)
{
   memset(layout, 0, sizeof(SB_MODULE_LAYOUT) * SB_MAX_MODULE_NUMBER);
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.9.6.1.101.53.3.1.5"), HandlerPhysicalPorts, layout) != SNMP_ERR_SUCCESS)
      return 0;

   int count = 0;
   for(int i = 0; (i < SB_MAX_MODULE_NUMBER) && (layout[i].index != 0); i++)
      count++;
   return count;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *CiscoSbDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
	if (ifList == nullptr)
		return nullptr;

	SB_MODULE_LAYOUT layout[SB_MAX_MODULE_NUMBER];
	if (getPhysicalPortLayout(snmp, layout) > 0)
	{
      for(int i = 0; i < ifList->size(); i++)
      {
         InterfaceInfo *iface = ifList->get(i);
         for(int j = 0; j < SB_MAX_MODULE_NUMBER; j++)
         {
            SB_MODULE_LAYOUT *module = &layout[j];
            if ((iface->index >= module->minIfIndex) && (iface->index <= module->maxIfIndex))
            {
               for(UINT32 n = 0; n < SB_MAX_INTERFACES_PER_ROW * module->rows; n++)
               {
                  if (module->interfaces[n] == iface->index)
                  {
                     iface->isPhysicalPort = true;
                     iface->location.module = module->index;
                     iface->location.port = (n / SB_MAX_INTERFACES_PER_ROW) * module->columns + n % SB_MAX_INTERFACES_PER_ROW + 1;
                     break;
                  }
               }
               break;
            }
         }
      }
	}
	else  // No physical layout information, try to calculate port numbers from ifIndex
	{
      // Check if there are indexes below 49
      bool highBase = true;
      for(int i = 0; i < ifList->size(); i++)
      {
         if (ifList->get(i)->index < 49)
         {
            highBase = false;
            break;
         }
      }

      // Find physical ports
      for(int i = 0; i < ifList->size(); i++)
      {
         InterfaceInfo *iface = ifList->get(i);
         if (iface->index < 1000)
         {
            iface->isPhysicalPort = true;
            iface->location.module = (iface->index - 1) / 110 + 1;
            iface->location.port = (iface->index - 1) % 110 + 1;
            if (highBase)
               iface->location.port -= 48;
         }
      }
	}
	return ifList;
}

/**
 * Get port layout of given module. Default implementation always set NDD_PN_UNKNOWN as layout.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param module Module number (starting from 1)
 * @param layout Layout structure to fill
 */
void CiscoSbDriver::getModuleLayout(SNMP_Transport *snmp, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout)
{
   layout->numberingScheme = NDD_PN_LR_UD;
   SB_MODULE_LAYOUT modules[SB_MAX_MODULE_NUMBER];
   if (getPhysicalPortLayout(snmp, modules) >= module)
   {
      layout->rows = modules[module - 1].rows;
      layout->columns = modules[module - 1].columns;
   }
   else
   {
      layout->rows = 2;
      layout->columns = 24;
   }
}
