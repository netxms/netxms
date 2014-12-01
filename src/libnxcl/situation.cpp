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
** File: situation.cpp
**
**/

#include "libnxcl.h"


//
// Create NXC_SITUATION from message
//

static void SituationFromMessage(NXCPMessage *msg, NXC_SITUATION *situation)
{
	int i, j, attrCount;
	TCHAR *attr, *value;
	UINT32 id;

	situation->m_id = msg->getFieldAsUInt32(VID_SITUATION_ID);
	situation->m_name = msg->getFieldAsString(VID_NAME);
	situation->m_comments = msg->getFieldAsString(VID_COMMENTS);
	situation->m_instanceCount = msg->getFieldAsUInt32(VID_INSTANCE_COUNT);
	situation->m_instanceList = (NXC_SITUATION_INSTANCE *)malloc(sizeof(NXC_SITUATION_INSTANCE) * situation->m_instanceCount);
	for(i = 0, id = VID_INSTANCE_LIST_BASE; i < situation->m_instanceCount; i++)
	{
		situation->m_instanceList[i].m_name = msg->getFieldAsString(id++);
		attrCount = msg->getFieldAsUInt32(id++);
		situation->m_instanceList[i].m_attrList = new StringMap;
		for(j = 0; j < attrCount; j++)
		{
			attr = msg->getFieldAsString(id++);
			value = msg->getFieldAsString(id++);
			situation->m_instanceList[i].m_attrList->setPreallocated(attr, value);
		}
	}
}


//
// Process CMD_SITUATION_CHANGE message
//

void ProcessSituationChange(NXCL_Session *pSession, NXCPMessage *pMsg)
{
   NXC_SITUATION st;
   UINT32 dwCode;

   dwCode = pMsg->getFieldAsUInt16(VID_NOTIFICATION_CODE);
	SituationFromMessage(pMsg, &st);
   pSession->callEventHandler(NXC_EVENT_SITUATION_UPDATE, dwCode, &st);
}


//
// Create situation
//

UINT32 LIBNXCL_EXPORTABLE NXCCreateSituation(NXC_SESSION hSession, const TCHAR *name, const TCHAR *comments, UINT32 *pdwId)
{
   NXCPMessage msg, *pResponse;
   UINT32 dwRqId, rcc;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_CREATE_SITUATION);
   msg.setId(dwRqId);
   msg.setField(VID_NAME, name);
   msg.setField(VID_COMMENTS, CHECK_NULL_EX(comments));
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
   	rcc = pResponse->getFieldAsUInt32(VID_RCC);
		if (rcc == RCC_SUCCESS)
		{
			*pdwId = pResponse->getFieldAsUInt32(VID_SITUATION_ID);
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

UINT32 LIBNXCL_EXPORTABLE NXCModifySituation(NXC_SESSION hSession, UINT32 id,
                                            const TCHAR *name, const TCHAR *comments)
{
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_UPDATE_SITUATION);
   msg.setId(dwRqId);
   msg.setField(VID_SITUATION_ID, id);
   if (name != NULL)
   	msg.setField(VID_NAME, name);
   if (comments != NULL)
   	msg.setField(VID_COMMENTS, comments);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Delete situation
//

UINT32 LIBNXCL_EXPORTABLE NXCDeleteSituation(NXC_SESSION hSession, UINT32 id)
{
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_DELETE_SITUATION);
   msg.setId(dwRqId);
   msg.setField(VID_SITUATION_ID, id);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Delete situation instance
//

UINT32 LIBNXCL_EXPORTABLE NXCDeleteSituationInstance(NXC_SESSION hSession, UINT32 id, const TCHAR *instance)
{
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_DEL_SITUATION_INSTANCE);
   msg.setId(dwRqId);
   msg.setField(VID_SITUATION_ID, id);
   msg.setField(VID_SITUATION_INSTANCE, instance);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Get situation list
//

UINT32 LIBNXCL_EXPORTABLE NXCGetSituationList(NXC_SESSION hSession, NXC_SITUATION_LIST **list)
{
   NXCPMessage msg, *pResponse;
   UINT32 dwRqId, rcc;
	int i;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_GET_SITUATION_LIST);
   msg.setId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
   	rcc = pResponse->getFieldAsUInt32(VID_RCC);
		if (rcc == RCC_SUCCESS)
		{
			*list = (NXC_SITUATION_LIST *)malloc(sizeof(NXC_SITUATION_LIST));
			(*list)->m_count = pResponse->getFieldAsUInt32(VID_SITUATION_COUNT);
			(*list)->m_situations = (NXC_SITUATION *)malloc(sizeof(NXC_SITUATION) * (*list)->m_count);
			memset((*list)->m_situations, 0, sizeof(NXC_SITUATION) * (*list)->m_count);
			delete pResponse;
			for(i = 0; i < (*list)->m_count; i++)
			{
				pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_SITUATION_DATA, dwRqId);
				if (pResponse != NULL)
				{
					SituationFromMessage(pResponse, &((*list)->m_situations[i]));
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
// Copy NXC_SITUATION structure
//

static void CopySituation(NXC_SITUATION *dst, NXC_SITUATION *src)
{
	int i;

	dst->m_id = src->m_id;
	dst->m_name = _tcsdup(CHECK_NULL_EX(src->m_name));
	dst->m_comments = _tcsdup(CHECK_NULL_EX(src->m_comments));
	dst->m_instanceCount = src->m_instanceCount;
	dst->m_instanceList = (NXC_SITUATION_INSTANCE *)malloc(sizeof(NXC_SITUATION_INSTANCE) * dst->m_instanceCount);
	for(i = 0; i < src->m_instanceCount; i++)
	{
		dst->m_instanceList[i].m_name = _tcsdup(CHECK_NULL_EX(src->m_instanceList[i].m_name));
		dst->m_instanceList[i].m_attrList = new StringMap(*(src->m_instanceList[i].m_attrList));
	}
}


//
// Find situation in list
//

static int FindSituationInList(NXC_SITUATION_LIST *list, UINT32 id)
{
	int i;

	for(i = 0; i < list->m_count; i++)
		if (list->m_situations[i].m_id == id)
			return i;
	return INVALID_INDEX;
}


//
// Destroy situation
//

static void DestroySituation(NXC_SITUATION *s)
{
	int i;

	safe_free(s->m_name);
	safe_free(s->m_comments);
	for(i = 0; i < s->m_instanceCount; i++)
	{
		safe_free(s->m_instanceList[i].m_name);
		delete s->m_instanceList[i].m_attrList;
	}
}


//
// Update existing situation list with new data
//

void LIBNXCL_EXPORTABLE NXCUpdateSituationList(NXC_SITUATION_LIST *list, int code, NXC_SITUATION *update)
{
	int index;

	switch(code)
	{
		case SITUATION_UPDATE:
			index = FindSituationInList(list, update->m_id);
			if (index != INVALID_INDEX)
			{
				DestroySituation(&list->m_situations[index]);
				CopySituation(&list->m_situations[index], update);
				break;
			}
		case SITUATION_CREATE:
			list->m_count++;
			list->m_situations = (NXC_SITUATION *)realloc(list->m_situations, sizeof(NXC_SITUATION) * list->m_count);
			CopySituation(&list->m_situations[list->m_count - 1], update);
			break;
		case SITUATION_DELETE:
			index = FindSituationInList(list, update->m_id);
			if (index != INVALID_INDEX)
			{
				DestroySituation(&list->m_situations[index]);
				list->m_count--;
				memmove(&list->m_situations[index], &list->m_situations[index + 1], sizeof(NXC_SITUATION) * (list->m_count - index));
			}
			break;
		default:
			break;
	}
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
				DestroySituation(&list->m_situations[i]);
			}
			free(list->m_situations);
		}
		free(list);
	}
}
