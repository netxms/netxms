/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: situation.cpp
**
**/

#include "libnxcl.h"


//
// Create situation
//

DWORD LIBNXCL_EXPORTABLE NXCCreateSituation(NXC_SESSION hSession, const TCHAR *name, const TCHAR *comments, DWORD *pdwId)
{
   CSCPMessage msg, *pResponse;
   DWORD dwRqId, rcc;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_CREATE_SITUATION);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_NAME, name);
   msg.SetVariable(VID_COMMENTS, CHECK_NULL_EX(comments));
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
   	rcc = pResponse->GetVariableLong(VID_RCC);
		if (rcc == RCC_SUCCESS)
		{
			*pdwId = pResponse->GetVariableLong(VID_SITUATION_ID);
		}
      delete pResponse;
   }
   else
   {
      rcc = RCC_TIMEOUT;
   }
	return rcc;
}


//
// Modify situation
//

DWORD LIBNXCL_EXPORTABLE NXCModifySituation(NXC_SESSION hSession, DWORD id,
                                            const TCHAR *name, const TCHAR *comments)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_UPDATE_SITUATION);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_SITUATION_ID, id);
   if (name != NULL)
   	msg.SetVariable(VID_NAME, name);
   if (comments != NULL)
   	msg.SetVariable(VID_COMMENTS, comments);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Delete situation
//

DWORD LIBNXCL_EXPORTABLE NXCDeleteSituation(NXC_SESSION hSession, DWORD id)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_DELETE_SITUATION);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_SITUATION_ID, id);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Delete situation instance
//

DWORD LIBNXCL_EXPORTABLE NXCDeleteSituationInstance(NXC_SESSION hSession, DWORD id, const TCHAR *instance)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_DEL_SITUATION_INSTANCE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_SITUATION_ID, id);
   msg.SetVariable(VID_SITUATION_INSTANCE, instance);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Get situation list
//

DWORD LIBNXCL_EXPORTABLE NXCGetSituationList(NXC_SESSION hSession, NXC_SITUATION_LIST **list)
{
   CSCPMessage msg, *pResponse;
   DWORD dwRqId, rcc, id;
	TCHAR *attr, *value;
	int i, j, k;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_SITUATION_LIST);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
   	rcc = pResponse->GetVariableLong(VID_RCC);
		if (rcc == RCC_SUCCESS)
		{
			*list = (NXC_SITUATION_LIST *)malloc(sizeof(NXC_SITUATION_LIST));
			(*list)->m_count = pResponse->GetVariableLong(VID_SITUATION_COUNT);
			(*list)->m_situations = (NXC_SITUATION *)malloc(sizeof(NXC_SITUATION) * (*list)->m_count);
			memset((*list)->m_situations, 0, sizeof(NXC_SITUATION) * (*list)->m_count);
			delete pResponse;
			for(i = 0; i < (*list)->m_count; i++)
			{
				pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_SITUATION_DATA, dwRqId);
				if (pResponse != NULL)
				{
					(*list)->m_situations[i].m_id = pResponse->GetVariableLong(VID_SITUATION_ID);
					(*list)->m_situations[i].m_name = pResponse->GetVariableStr(VID_NAME);
					(*list)->m_situations[i].m_comments = pResponse->GetVariableStr(VID_COMMENTS);
					(*list)->m_situations[i].m_instanceCount = pResponse->GetVariableLong(VID_INSTANCE_COUNT);
					(*list)->m_situations[i].m_instanceList = (NXC_SITUATION_INSTANCE *)malloc(sizeof(NXC_SITUATION_INSTANCE) * (*list)->m_situations[i].m_instanceCount);
					for(j = 0, id = VID_INSTANCE_LIST_BASE; j < (*list)->m_situations[i].m_instanceCount; j++)
					{
						(*list)->m_situations[i].m_instanceList[j].m_name = pResponse->GetVariableStr(id++);
						(*list)->m_situations[i].m_instanceList[j].m_attrCount = pResponse->GetVariableLong(id++);
						(*list)->m_situations[i].m_instanceList[j].m_attrList = new StringMap;
						for(k = 0; k < (*list)->m_situations[i].m_instanceList[j].m_attrCount; k++)
						{
							attr = pResponse->GetVariableStr(id++);
							value = pResponse->GetVariableStr(id++);
							(*list)->m_situations[i].m_instanceList[j].m_attrList->Set(attr, value);
							free(attr);
							free(value);
						}
					}
					delete pResponse;
				}
				else
				{
					NXCDestroySituationList(*list);
					*list = NULL;
					rcc = RCC_TIMEOUT;
					break;
				}
			}	
		}
		else
		{
      	delete pResponse;
      }
   }
   else
   {
      rcc = RCC_TIMEOUT;
   }
	return rcc;
}


//
// Destroy situation list
//

void LIBNXCL_EXPORTABLE NXCDestroySituationList(NXC_SITUATION_LIST *list)
{
	int i;

	if (list != NULL)
	{
		if (list->m_situations != NULL)
		{
			for(i = 0; i < list->m_count; i++)
			{
				safe_free(list->m_situations[i].m_name);
				safe_free(list->m_situations[i].m_comments);
			}
			free(list->m_situations);
		}
		free(list);
	}
}

