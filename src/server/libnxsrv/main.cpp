/* 
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: main.cpp
**
**/

#include "libnxsrv.h"


//
// Agent result codes
//

static struct
{
   int iCode;
   char *pszText;
} m_agentErrors[] =
{
   { ERR_SUCCESS, "Success" },
   { ERR_UNKNOWN_COMMAND, "Unknown command" },
   { ERR_AUTH_REQUIRED, "Authentication required" },
   { ERR_UNKNOWN_PARAMETER, "Unknown parameter" },
   { ERR_REQUEST_TIMEOUT, "Request timeout" },
   { ERR_AUTH_FAILED, "Authentication failed" },
   { ERR_ALREADY_AUTHENTICATED, "Already authenticated" },
   { ERR_AUTH_NOT_REQUIRED, "Authentication not required" },
   { ERR_INTERNAL_ERROR, "Internal error" },
   { ERR_NOT_IMPLEMENTED, "Not implemented" },
   { ERR_OUT_OF_RESOURCES, "Out of resources" },
   { ERR_NOT_CONNECTED, "Not connected" },
   { ERR_CONNECTION_BROKEN, "Connection broken" },
   { ERR_BAD_RESPONCE, "Bad responce" },
   { -1, NULL }
};


//
// Resolve agent's error code to text
//

const char LIBNXSRV_EXPORTABLE *AgentErrorCodeToText(int iError)
{
   int i;

   for(i = 0; m_agentErrors[i].pszText != NULL; i++)
      if (iError == m_agentErrors[i].iCode)
         return m_agentErrors[i].pszText;
   return "Unknown error code";
}


//
// Destroy interface list created by discovery functions
//

void LIBNXSRV_EXPORTABLE DestroyInterfaceList(INTERFACE_LIST *pIfList)
{
   if (pIfList != NULL)
   {
      if (pIfList->pInterfaces != NULL)
         free(pIfList->pInterfaces);
      free(pIfList);
   }
}


//
// Destroy ARP cache created by discovery functions
//

void LIBNXSRV_EXPORTABLE DestroyArpCache(ARP_CACHE *pArpCache)
{
   if (pArpCache != NULL)
   {
      if (pArpCache->pEntries != NULL)
         free(pArpCache->pEntries);
      free(pArpCache);
   }
}


//
// DLL entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   return TRUE;
}

#endif   /* _WIN32 */
