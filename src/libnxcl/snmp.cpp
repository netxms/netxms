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
** File: snmp.cpp
**
**/

#include "libnxcl.h"


//
// Do SNMP walk
//

UINT32 LIBNXCL_EXPORTABLE NXCSnmpWalk(NXC_SESSION hSession, UINT32 dwNode,
                                     TCHAR *pszRootOID, void *pUserData,
                                     void (* pfCallback)(TCHAR *, UINT32, TCHAR *, void *))
{
   NXCPMessage msg, *pData;
   UINT32 i, dwNumVars, dwRetCode, dwRqId, dwId, dwType;
   TCHAR szVarName[4096], szValue[4096];
   BOOL bStop = FALSE;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_START_SNMP_WALK);
   msg.setId(dwRqId);
   msg.setField(VID_SNMP_OID, pszRootOID);
   msg.setField(VID_OBJECT_ID, dwNode);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   dwRetCode = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   if (dwRetCode == RCC_SUCCESS)
   {
      do
      {
         pData = ((NXCL_Session *)hSession)->WaitForMessage(CMD_SNMP_WALK_DATA, dwRqId);
         if (pData != NULL)
         {
            dwNumVars = pData->getFieldAsUInt32(VID_NUM_VARIABLES);
            for(i = 0, dwId = VID_SNMP_WALKER_DATA_BASE; i < dwNumVars; i++)
            {
               pData->getFieldAsString(dwId++, szVarName, 4096);
               dwType = pData->getFieldAsUInt32(dwId++);
               pData->getFieldAsString(dwId++, szValue, 4096);
               pfCallback(szVarName, dwType, szValue, pUserData);
            }
            bStop = pData->isEndOfSequence();
            delete pData;
         }
         else
         {
            dwRetCode = RCC_TIMEOUT;
         }
      } while((dwRetCode == RCC_SUCCESS) && (!bStop));
   }

   return dwRetCode;
}

/**
 * Set SNMP variable
 */
UINT32 LIBNXCL_EXPORTABLE NXCSnmpSet(NXC_SESSION hSession, UINT32 dwNode,
                                    TCHAR *pszVarName, UINT32 dwType, void *pValue)
{
   return 0;
}


//
// Get list of community strings
//

UINT32 LIBNXCL_EXPORTABLE NXCGetSnmpCommunityList(NXC_SESSION hSession, UINT32 *pdwNumStrings,
																 TCHAR ***pppszStringList)
{
   NXCPMessage msg, *pResponse;
   UINT32 i, count, dwRetCode, dwRqId, id;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_GET_COMMUNITY_LIST);
   msg.setId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
		dwRetCode = pResponse->getFieldAsUInt32(VID_RCC);
		if (dwRetCode == RCC_SUCCESS)
		{
			count = pResponse->getFieldAsUInt32(VID_NUM_STRINGS);
			*pdwNumStrings = count;
			if (count > 0)
			{
				*pppszStringList = (TCHAR **)malloc(sizeof(TCHAR *) * count);
				for(i = 0, id = VID_STRING_LIST_BASE; i < count; i++)
				{
					(*pppszStringList)[i] = pResponse->getFieldAsString(id++);
				}
			}
			else
			{
				*pppszStringList = NULL;
			}
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
// Update list of community strings
//

UINT32 LIBNXCL_EXPORTABLE NXCUpdateSnmpCommunityList(NXC_SESSION hSession, UINT32 dwNumStrings,
											    					 TCHAR **ppszStringList)
{
   NXCPMessage msg;
   UINT32 i, id, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_UPDATE_COMMUNITY_LIST);
   msg.setId(dwRqId);

	msg.setField(VID_NUM_STRINGS, dwNumStrings);
	for(i = 0, id = VID_STRING_LIST_BASE; i < dwNumStrings; i++)
	{
		msg.setField(id++, ppszStringList[i]);
	}

   ((NXCL_Session *)hSession)->SendMsg(&msg);

	return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Get list of USM credentials
//

UINT32 LIBNXCL_EXPORTABLE NXCGetSnmpUsmCredentials(NXC_SESSION hSession, UINT32 *listSize, NXC_SNMP_USM_CRED **list)
{
   NXCPMessage msg, *pResponse;
   UINT32 i, count, dwRetCode, dwRqId, id;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_GET_USM_CREDENTIALS);
   msg.setId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
		dwRetCode = pResponse->getFieldAsUInt32(VID_RCC);
		if (dwRetCode == RCC_SUCCESS)
		{
			count = pResponse->getFieldAsUInt32(VID_NUM_RECORDS);
			*listSize = count;
			if (count > 0)
			{
				*list = (NXC_SNMP_USM_CRED *)malloc(sizeof(NXC_SNMP_USM_CRED) * count);
				NXC_SNMP_USM_CRED *curr = *list;
				for(i = 0, id = VID_USM_CRED_LIST_BASE; i < count; i++, id += 5, curr++)
				{
					pResponse->getFieldAsString(id++, curr->name, MAX_DB_STRING);
					curr->authMethod = (int)pResponse->getFieldAsUInt16(id++);
					curr->privMethod = (int)pResponse->getFieldAsUInt16(id++);
					pResponse->getFieldAsString(id++, curr->authPassword, MAX_DB_STRING);
					pResponse->getFieldAsString(id++, curr->privPassword, MAX_DB_STRING);
				}
			}
			else
			{
				*list = NULL;
			}
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
// Update list of USM credentials
//

UINT32 LIBNXCL_EXPORTABLE NXCUpdateSnmpUsmCredentials(NXC_SESSION hSession, UINT32 count, NXC_SNMP_USM_CRED *list)
{
   NXCPMessage msg;
   UINT32 i, id, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_UPDATE_USM_CREDENTIALS);
   msg.setId(dwRqId);

	msg.setField(VID_NUM_RECORDS, count);
	for(i = 0, id = VID_USM_CRED_LIST_BASE; i < count; i++, id += 5)
	{
		msg.setField(id++, list[i].name);
		msg.setField(id++, (WORD)list[i].authMethod);
		msg.setField(id++, (WORD)list[i].privMethod);
		msg.setField(id++, list[i].authPassword);
		msg.setField(id++, list[i].privPassword);
	}

   ((NXCL_Session *)hSession)->SendMsg(&msg);

	return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}
