/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004, 2005, 2006, 2007 Victor Kirhenshtein
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
** File: layer2.cpp
**
**/

#include "libnxcl.h"
#include <netxms_maps.h>


//
// Query layer 2 topology from device
//

DWORD LIBNXCL_EXPORTABLE NXCQueryL2Topology(NXC_SESSION hSession, DWORD dwNodeId, void **ppTopology)
{
   CSCPMessage msg, *pResponse;
   DWORD dwRetCode, dwRqId;

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
