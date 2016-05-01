/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
** File: bridge.cpp
**
**/

#include "nxcore.h"

/**
 * Map port numbers from dot1dBasePortTable to interface indexes
 */
static UINT32 PortMapCallback(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   UINT32 ifIndex = var->getValueAsUInt();
   InterfaceList *ifList = (InterfaceList *)arg;
   for(int i = 0; i < ifList->size(); i++)
      if (ifList->get(i)->index == ifIndex)
      {
         ifList->get(i)->bridgePort = var->getName().getElement(11);
         break;
      }
	return SNMP_ERR_SUCCESS;
}

/**
 * Map bridge port numbers to interfaces
 */
void BridgeMapPorts(SNMP_Transport *transport, InterfaceList *ifList)
{
	SnmpWalk(transport, _T(".1.3.6.1.2.1.17.1.4.1.2"), PortMapCallback, ifList);
}
