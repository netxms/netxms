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
** File: mib.cpp
**
**/

#include "libnxcl.h"

#if defined(_WIN32) && !defined(UNDER_CE)
# include <io.h>
#endif


//
// Download MIB file from server
//

UINT32 LIBNXCL_EXPORTABLE NXCDownloadMIBFile(NXC_SESSION hSession, const TCHAR *pszName)
{
   UINT32 dwRqId, dwRetCode;
   NXCPMessage msg;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();
   dwRetCode = ((NXCL_Session *)hSession)->PrepareFileTransfer(pszName, dwRqId);
   if (dwRetCode == RCC_SUCCESS)
   {
      msg.setCode(CMD_GET_MIB);
      msg.setId(dwRqId);
      ((NXCL_Session *)hSession)->SendMsg(&msg);

      // Loading file can take time, so timeout is 300 sec. instead of default
      dwRetCode = ((NXCL_Session *)hSession)->WaitForFileTransfer(300000);
   }
   return dwRetCode;
}


//
// Get timestamp of server's MIB file
//

UINT32 LIBNXCL_EXPORTABLE NXCGetMIBFileTimeStamp(NXC_SESSION hSession, UINT32 *pdwTimeStamp)
{
   UINT32 dwRqId, dwRetCode;
   NXCPMessage msg, *pResponse;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_GET_MIB_TIMESTAMP);
   msg.setId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
         *pdwTimeStamp = pResponse->getFieldAsUInt32(VID_TIMESTAMP);
      delete pResponse;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}
