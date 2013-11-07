/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: stp.cpp
**
**/

#include "nxcore.h"

/**
 * STP port table walker's callback
 */
static UINT32 STPPortListHandler(UINT32 snmpVersion, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
	int state = var->GetValueAsInt();
	if ((state != 2) && (state != 5))
		return SNMP_ERR_SUCCESS;  // port state not "blocked" or "forwarding"

	Node *node = (Node *)((LinkLayerNeighbors *)arg)->getData();
	SNMP_ObjectId *oid = var->GetName();

	return SNMP_ERR_SUCCESS;
}

/**
 * Add STP-discovered neighbors
 */
void AddSTPNeighbors(Node *node, LinkLayerNeighbors *nbs)
{
	if (!(node->getFlags() & NF_IS_STP))
		return;

	DbgPrintf(5, _T("STP: collecting topology information for node %s [%d]"), node->Name(), node->Id());
	nbs->setData(node);
	node->callSnmpEnumerate(_T(".1.3.6.1.2.1.17.2.15.1.3"), STPPortListHandler, nbs);
	DbgPrintf(5, _T("STP: finished collecting topology information for node %s [%d]"), node->Name(), node->Id());
}
