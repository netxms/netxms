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
** File: mp.cpp
**
**/

#include "libnxcl.h"


//
// Create management pack file
//

UINT32 LIBNXCL_EXPORTABLE NXCExportConfiguration(NXC_SESSION hSession, TCHAR *pszDescr,
                                                UINT32 dwNumEvents, UINT32 *pdwEventList,
                                                UINT32 dwNumTemplates, UINT32 *pdwTemplateList,
                                                UINT32 dwNumTraps, UINT32 *pdwTrapList,
                                                TCHAR **ppszContent)
{
   NXCPMessage msg, *pResponse;
   UINT32 dwRqId, dwResult;

   *ppszContent = NULL;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_EXPORT_CONFIGURATION);
   msg.setId(dwRqId);
   msg.setField(VID_DESCRIPTION, pszDescr);
   msg.setField(VID_NUM_EVENTS, dwNumEvents);
   msg.setFieldFromInt32Array(VID_EVENT_LIST, dwNumEvents, pdwEventList);
   msg.setField(VID_NUM_OBJECTS, dwNumTemplates);
   msg.setFieldFromInt32Array(VID_OBJECT_LIST, dwNumTemplates, pdwTemplateList);
   msg.setField(VID_NUM_TRAPS, dwNumTraps);
   msg.setFieldFromInt32Array(VID_TRAP_LIST, dwNumTraps, pdwTrapList);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *ppszContent = pResponse->getFieldAsString(VID_NXMP_CONTENT);
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
// Install management pack
//

UINT32 LIBNXCL_EXPORTABLE NXCImportConfiguration(NXC_SESSION hSession, TCHAR *pszContent,
                                                UINT32 dwFlags, TCHAR *pszErrorText, int nErrorLen)
{
   NXCPMessage msg, *pResponse;
   UINT32 dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_IMPORT_CONFIGURATION);
   msg.setId(dwRqId);
   msg.setField(VID_NXMP_CONTENT, pszContent);
   msg.setField(VID_FLAGS, dwFlags);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *pszErrorText = 0;
      }
      else
      {
         pResponse->getFieldAsString(VID_ERROR_TEXT, pszErrorText, nErrorLen);
      }
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }
   return dwResult;
}
