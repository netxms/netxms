/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2006 Victor Kirhenshtein
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
** File: map.cpp
**
**/

#include "libnxcl.h"
#include <netxms_maps.h>


//
// Get map list
//

DWORD LIBNXCL_EXPORTABLE NXCGetMapList(NXC_SESSION hSession, DWORD *pdwNumMaps,
                                       NXC_MAP_INFO **ppMapList)
{
   CSCPMessage msg, *pResponse;
   DWORD i, dwRqId, dwResult, dwId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_MAP_LIST);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *pdwNumMaps = pResponse->GetVariableLong(VID_NUM_MAPS);
         *ppMapList = (NXC_MAP_INFO *)malloc(sizeof(NXC_MAP_INFO) * (*pdwNumMaps));
         for(i = 0, dwId = VID_MAP_LIST_BASE; i < *pdwNumMaps; i++)
         {
            (*ppMapList)[i].dwMapId = pResponse->GetVariableLong(dwId++);
            (*ppMapList)[i].dwObjectId = pResponse->GetVariableLong(dwId++);
            (*ppMapList)[i].dwAccess = pResponse->GetVariableLong(dwId++);
            pResponse->GetVariableStr(dwId++, (*ppMapList)[i].szName, MAX_DB_STRING);
            dwId += 6;  // Reserved ids for future use
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
// Save map
//

DWORD LIBNXCL_EXPORTABLE NXCSaveMap(NXC_SESSION hSession, void *pMap)
{
   nxSubmap *pSubmap;
   CSCPMessage msg;
   DWORD i, dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();
   ((nxMap *)pMap)->Lock();

   msg.SetCode(CMD_SAVE_MAP);
   msg.SetId(dwRqId);
   ((nxMap *)pMap)->CreateMessage(&msg);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for initial confirmation
   dwResult = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);

   // If OK, send all submaps to server
   if (dwResult == RCC_SUCCESS)
   {
      msg.SetCode(CMD_SUBMAP_DATA);
      for(i = 0; i < ((nxMap *)pMap)->GetSubmapCount(); i++)
      {
         msg.DeleteAllVariables();
         pSubmap = ((nxMap *)pMap)->GetSubmapByIndex(i);
         if (pSubmap != NULL)
            pSubmap->CreateMessage(&msg);
         if (i == ((nxMap *)pMap)->GetSubmapCount() - 1)
            msg.SetEndOfSequence();
         ((NXCL_Session *)hSession)->SendMsg(&msg);
      }

      // Wait for second confirmation
      dwResult = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   }

   ((nxMap *)pMap)->Unlock();
   return dwResult;
}


//
// Load map
//

DWORD LIBNXCL_EXPORTABLE NXCLoadMap(NXC_SESSION hSession, DWORD dwMapId, void **ppMap)
{
   nxMap *pMap;
   nxSubmap *pSubmap;
   CSCPMessage msg, *pResponse;
   DWORD dwRqId, dwResult, dwId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_LOAD_MAP);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_MAP_ID, dwMapId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         pMap = new nxMap(pResponse);
         if (pResponse->GetVariableLong(VID_NUM_SUBMAPS) > 0)
         {
            do
            {
               delete pResponse;
               pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_SUBMAP_DATA, dwRqId);
               if (pResponse != NULL)
               {
                  dwResult = pResponse->GetVariableLong(VID_RCC);
                  if (dwResult == RCC_SUCCESS)
                  {
                     dwId = pResponse->GetVariableLong(VID_OBJECT_ID);
                     pSubmap = pMap->GetSubmap(dwId);
                     pSubmap->ModifyFromMessage(pResponse);
                  }
               }
               else
               {
                  dwResult = RCC_TIMEOUT;
                  break;
               }
            } while((dwResult == RCC_SUCCESS) && (!pResponse->IsEndOfSequence()));
            if (dwResult == RCC_SUCCESS)
            {
               *ppMap = pMap;
            }
            else
            {
               delete pMap;
            }
         }
         else
         {
            *ppMap = pMap;
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
// Delete map
//

DWORD LIBNXCL_EXPORTABLE NXCDeleteMap(NXC_SESSION hSession, DWORD dwMapId)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_DELETE_MAP);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_MAP_ID, dwMapId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Resolve map name to id
//

DWORD LIBNXCL_EXPORTABLE NXCResolveMapName(NXC_SESSION hSession, TCHAR *pszMapName,
                                           DWORD *pdwMapId)
{
   CSCPMessage msg, *pResponse;
   DWORD dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_RESOLVE_MAP_NAME);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_NAME, pszMapName);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *pdwMapId = pResponse->GetVariableLong(VID_MAP_ID);
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
// Upload background image to server
//

DWORD LIBNXCL_EXPORTABLE NXCUploadSubmapBkImage(NXC_SESSION hSession, DWORD dwMapId,
                                                DWORD dwSubmapId, TCHAR *pszFile)
{
   CSCPMessage msg;
   DWORD dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_UPLOAD_SUBMAP_BK_IMAGE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_MAP_ID, dwMapId);
   msg.SetVariable(VID_OBJECT_ID, dwSubmapId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   dwResult = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   if (dwResult == RCC_SUCCESS)
   {
      dwResult = ((NXCL_Session *)hSession)->SendFile(dwRqId, pszFile);
      if (dwResult == RCC_SUCCESS)
      {
         // Wait for final confirmation
         dwResult = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
      }
   }
   return dwResult;
}


//
// Retrieve background image from server
//

DWORD LIBNXCL_EXPORTABLE NXCDownloadSubmapBkImage(NXC_SESSION hSession, DWORD dwMapId,
                                                  DWORD dwSubmapId, TCHAR *pszFile)
{
   CSCPMessage msg;
   DWORD dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();
   dwResult = ((NXCL_Session *)hSession)->PrepareFileTransfer(pszFile, dwRqId);
   if (dwResult == RCC_SUCCESS)
   {
      msg.SetCode(CMD_GET_SUBMAP_BK_IMAGE);
      msg.SetId(dwRqId);
      msg.SetVariable(VID_MAP_ID, dwMapId);
      msg.SetVariable(VID_OBJECT_ID, dwSubmapId);
      ((NXCL_Session *)hSession)->SendMsg(&msg);

      // Loading file may take time, so timeout is 300 sec. instead of default
      dwResult = ((NXCL_Session *)hSession)->WaitForFileTransfer(300000);
   }
   return dwResult;
}


//
// Create new map
//

DWORD LIBNXCL_EXPORTABLE NXCCreateMap(NXC_SESSION hSession, DWORD dwRootObj,
												  TCHAR *pszName, DWORD *pdwMapId)
{
   CSCPMessage msg, *pResponse;
   DWORD dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_CREATE_MAP);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwRootObj);
   msg.SetVariable(VID_NAME, pszName);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *pdwMapId = pResponse->GetVariableLong(VID_MAP_ID);
      }
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }
   return dwResult;
}
