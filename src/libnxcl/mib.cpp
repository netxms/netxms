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
// Download MIB file from server
//

DWORD LIBNXCL_EXPORTABLE NXCDownloadMIBFile(NXC_SESSION hSession, TCHAR *pszName)
{
   DWORD dwRqId, dwRetCode;
   CSCPMessage msg;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();
   dwRetCode = ((NXCL_Session *)hSession)->PrepareFileTransfer(pszName, dwRqId);
   if (dwRetCode == RCC_SUCCESS)
   {
      msg.SetCode(CMD_GET_MIB);
      msg.SetId(dwRqId);
      ((NXCL_Session *)hSession)->SendMsg(&msg);

      // Loading file can take time, so timeout is 300 sec. instead of default
      dwRetCode = ((NXCL_Session *)hSession)->WaitForFileTransfer(300000);
   }
   return dwRetCode;
}


//
// Get timestamp of server's MIB file
//

DWORD LIBNXCL_EXPORTABLE NXCGetMIBFileTimeStamp(NXC_SESSION hSession, DWORD *pdwTimeStamp)
{
   DWORD dwRqId, dwRetCode;
   CSCPMessage msg, *pResponse;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_MIB_TIMESTAMP);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
         *pdwTimeStamp = pResponse->GetVariableLong(VID_TIMESTAMP);
      delete pResponse;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}
