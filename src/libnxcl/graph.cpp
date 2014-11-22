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
** File: graph.cpp
**
**/

#include "libnxcl.h"


//
// Get list of configured graphs
//

UINT32 LIBNXCL_EXPORTABLE NXCGetGraphList(NXC_SESSION hSession, UINT32 *pdwNumGraphs, NXC_GRAPH **ppGraphList)
{
   UINT32 i, j, dwId, dwRqId, dwResult;
   NXCPMessage msg, *pResponse;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_GET_GRAPH_LIST);
   msg.setId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

	*pdwNumGraphs = 0;
	*ppGraphList = NULL;

	pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
	if (pResponse != NULL)
	{
		dwResult = pResponse->getFieldAsUInt32(VID_RCC);
		if (dwResult == RCC_SUCCESS)
		{
			*pdwNumGraphs = pResponse->getFieldAsUInt32(VID_NUM_GRAPHS);
			if (*pdwNumGraphs > 0)
			{
				*ppGraphList = (NXC_GRAPH *)malloc(sizeof(NXC_GRAPH) * (*pdwNumGraphs));
				memset(*ppGraphList, 0, sizeof(NXC_GRAPH) * (*pdwNumGraphs));
				for(i = 0, dwId = VID_GRAPH_LIST_BASE; i < *pdwNumGraphs; i++)
				{
					(*ppGraphList)[i].dwId = pResponse->getFieldAsUInt32(dwId++);
					(*ppGraphList)[i].dwOwner = pResponse->getFieldAsUInt32(dwId++);
					(*ppGraphList)[i].pszName = pResponse->getFieldAsString(dwId++);
					(*ppGraphList)[i].pszConfig = pResponse->getFieldAsString(dwId++);
					(*ppGraphList)[i].dwAclSize = pResponse->getFieldAsUInt32(dwId++);
					if ((*ppGraphList)[i].dwAclSize > 0)
					{
						UINT32 *pdwData;

						(*ppGraphList)[i].pACL = (NXC_GRAPH_ACL_ENTRY *)malloc(sizeof(NXC_GRAPH_ACL_ENTRY) * (*ppGraphList)[i].dwAclSize);
						pdwData = (UINT32 *)malloc(sizeof(UINT32) * (*ppGraphList)[i].dwAclSize * 2);
						pResponse->getFieldAsInt32Array(dwId++, (*ppGraphList)[i].dwAclSize, pdwData);
						pResponse->getFieldAsInt32Array(dwId++, (*ppGraphList)[i].dwAclSize, pdwData + (*ppGraphList)[i].dwAclSize);
						for(j = 0; j < (*ppGraphList)[i].dwAclSize; j++)
						{
							(*ppGraphList)[i].pACL[j].dwUserId = pdwData[j];
							(*ppGraphList)[i].pACL[j].dwAccess = pdwData[j + (*ppGraphList)[i].dwAclSize];
						}
						free(pdwData);
					}
					else
					{
						dwId += 2;
					}
					dwId += 3;
				}
			}
		}
		delete pResponse;
	}
	else
	{
		dwResult = RCC_TIMEOUT;
	}
	return dwResult;
}


//
// Destroy graph list
//

void LIBNXCL_EXPORTABLE NXCDestroyGraphList(UINT32 dwNumGraphs, NXC_GRAPH *pList)
{
	UINT32 i;

	for(i = 0; i < dwNumGraphs; i++)
	{
		safe_free(pList[i].pACL);
		safe_free(pList[i].pszConfig);
		safe_free(pList[i].pszName);
	}
	safe_free(pList);
}


//
// Delete graph
//

UINT32 LIBNXCL_EXPORTABLE NXCDeleteGraph(NXC_SESSION hSession, UINT32 dwGraphId)
{
   UINT32 dwRqId;
   NXCPMessage msg;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_DELETE_GRAPH);
   msg.setId(dwRqId);
   msg.setField(VID_GRAPH_ID, dwGraphId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Define graph
//

UINT32 LIBNXCL_EXPORTABLE NXCDefineGraph(NXC_SESSION hSession, NXC_GRAPH *pGraph)
{
   UINT32 i, dwId, dwRqId;
   NXCPMessage msg;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_SAVE_GRAPH);
   msg.setId(dwRqId);
   msg.setField(VID_GRAPH_ID, pGraph->dwId);
	msg.setField(VID_NAME, pGraph->pszName);
	msg.setField(VID_GRAPH_CONFIG, pGraph->pszConfig);
	msg.setField(VID_ACL_SIZE, pGraph->dwAclSize);
	for(i = 0, dwId = VID_GRAPH_ACL_BASE; i < pGraph->dwAclSize; i++)
	{
		msg.setField(dwId++, pGraph->pACL[i].dwUserId);
		msg.setField(dwId++, pGraph->pACL[i].dwAccess);
	}
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}
