/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
** File: layer2.cpp
**
**/

#include "libnxcl.h"
#include <netxms_maps.h>


//
// Query layer 2 topology from device
//

UINT32 LIBNXCL_EXPORTABLE NXCQueryL2Topology(NXC_SESSION hSession, UINT32 dwNodeId, void **ppTopology)
{
   CSCPMessage msg, *pResponse;
   UINT32 dwRetCode, dwRqId;

	*ppTopology = NULL;
   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_QUERY_L2_TOPOLOGY);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwNodeId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
		{
			*ppTopology = new nxmap_ObjList(pResponse);
		}
      delete pResponse;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}


//
// Find connection point for node or interface
//

UINT32 LIBNXCL_EXPORTABLE NXCFindConnectionPoint(NXC_SESSION hSession, UINT32 objectId, NXC_CONNECTION_POINT *cpInfo)
{
   CSCPMessage msg, *response;
   UINT32 dwRetCode, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

	msg.SetCode(CMD_FIND_NODE_CONNECTION);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, objectId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   response = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (response != NULL)
   {
      dwRetCode = response->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
		{
			cpInfo->remoteNodeId = response->GetVariableLong(VID_OBJECT_ID);
			cpInfo->remoteInterfaceId = response->GetVariableLong(VID_INTERFACE_ID);
			cpInfo->remoteIfIndex = response->GetVariableLong(VID_IF_INDEX);
			cpInfo->localNodeId = response->GetVariableLong(VID_LOCAL_NODE_ID);
			cpInfo->localInterfaceId = response->GetVariableLong(VID_LOCAL_INTERFACE_ID);
			response->GetVariableBinary(VID_MAC_ADDR, cpInfo->localMacAddr, MAC_ADDR_LENGTH);
		}
      delete response;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}


//
// Find connection point for given MAC address
//

UINT32 LIBNXCL_EXPORTABLE NXCFindMACAddress(NXC_SESSION hSession, BYTE *macAddr, NXC_CONNECTION_POINT *cpInfo)
{
   CSCPMessage msg, *response;
   UINT32 dwRetCode, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

	msg.SetCode(CMD_FIND_MAC_LOCATION);
   msg.SetId(dwRqId);
	msg.SetVariable(VID_MAC_ADDR, macAddr, MAC_ADDR_LENGTH);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   response = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (response != NULL)
   {
      dwRetCode = response->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
		{
			cpInfo->remoteNodeId = response->GetVariableLong(VID_OBJECT_ID);
			cpInfo->remoteInterfaceId = response->GetVariableLong(VID_INTERFACE_ID);
			cpInfo->remoteIfIndex = response->GetVariableLong(VID_IF_INDEX);
			cpInfo->localNodeId = response->GetVariableLong(VID_LOCAL_NODE_ID);
			cpInfo->localInterfaceId = response->GetVariableLong(VID_LOCAL_INTERFACE_ID);
			response->GetVariableBinary(VID_MAC_ADDR, cpInfo->localMacAddr, MAC_ADDR_LENGTH);
		}
      delete response;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}
