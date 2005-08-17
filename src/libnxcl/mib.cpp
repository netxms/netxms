/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: mib.cpp
**
**/

#include "libnxcl.h"

#if defined(_WIN32) && !defined(UNDER_CE)
# include <io.h>
#endif


//
// Get list of available MIB files from server
//

DWORD LIBNXCL_EXPORTABLE NXCGetMIBList(NXC_SESSION hSession, NXC_MIB_LIST **ppMibList)
{
   DWORD i, dwRqId, dwRetCode, dwId1, dwId2;
   CSCPMessage msg, *pResponse;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_MIB_LIST);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_MIB_LIST, dwRqId, 
                                       ((NXCL_Session *)hSession)->m_dwCommandTimeout * 2);
   if (pResponse != NULL)
   {
      *ppMibList = (NXC_MIB_LIST *)malloc(sizeof(NXC_MIB_LIST));
      (*ppMibList)->dwNumFiles = pResponse->GetVariableLong(VID_NUM_MIBS);
      (*ppMibList)->ppszName = (TCHAR **)malloc(sizeof(TCHAR *) * (*ppMibList)->dwNumFiles);
      (*ppMibList)->ppHash = (BYTE **)malloc(sizeof(BYTE *) * (*ppMibList)->dwNumFiles);
      for(i = 0, dwId1 = VID_MIB_NAME_BASE, dwId2 = VID_MIB_HASH_BASE; 
          i < (*ppMibList)->dwNumFiles; i++, dwId1++, dwId2++)
      {
         (*ppMibList)->ppszName[i] = pResponse->GetVariableStr(dwId1);
         (*ppMibList)->ppHash[i] = (BYTE *)malloc(MD5_DIGEST_SIZE);
         pResponse->GetVariableBinary(dwId2, (*ppMibList)->ppHash[i], MD5_DIGEST_SIZE);
      }
      delete pResponse;
      dwRetCode = RCC_SUCCESS;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }

   return dwRetCode;
}


//
// Destroy MIB list
//

void LIBNXCL_EXPORTABLE NXCDestroyMIBList(NXC_MIB_LIST *pMibList)
{
   DWORD i;

   for(i = 0; i < pMibList->dwNumFiles; i++)
   {
      safe_free(pMibList->ppHash[i]);
      safe_free(pMibList->ppszName[i]);
   }
   safe_free(pMibList->ppHash);
   safe_free(pMibList->ppszName);
   free(pMibList);
}


//
// Download MIB file from server
//

DWORD LIBNXCL_EXPORTABLE NXCDownloadMIBFile(NXC_SESSION hSession, TCHAR *pszName, TCHAR *pszDestDir)
{
   DWORD i, dwRqId, dwRetCode, dwFileSize, dwNumBytes;
   CSCPMessage msg, *pResponse;
   BYTE *pBuffer;
   TCHAR cLastChar, szFileName[MAX_PATH];
   FILE *hFile;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_MIB);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_MIB_NAME, pszName);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Loading file can take time, so timeout is 60 sec. instead of default
   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_MIB, dwRqId, 60000);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         dwFileSize = pResponse->GetVariableLong(VID_MIB_FILE_SIZE);
         pBuffer = (BYTE *)malloc(dwFileSize);
         if (pBuffer != NULL)
         {
            pResponse->GetVariableBinary(VID_MIB_FILE, pBuffer, dwFileSize);
            cLastChar = pszDestDir[_tcslen(pszDestDir) - 1];
            _stprintf(szFileName, _T("%s%s%s"), pszDestDir, 
                    (cLastChar == _T('\\')) || (cLastChar == _T('/')) ? _T("") : _T("/"), pszName);
			   hFile = _tfopen(szFileName, _T("wb"));
            if (hFile != NULL)
            {
               for(i = 0; i < dwFileSize; i += dwNumBytes)
               {
                  dwNumBytes = min(16384, dwFileSize - i);
                  fwrite(&pBuffer[i], 1, dwNumBytes, hFile);
               }
               fclose(hFile);
            }
            else
            {
               dwRetCode = RCC_IO_ERROR;
            }
            free(pBuffer);
         }
         else
         {
            dwRetCode = RCC_OUT_OF_MEMORY;
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
