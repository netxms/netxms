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
// Destroy interface list created by discovery functions
//

void LIBNXSRV_EXPORTABLE DestroyInterfaceList(INTERFACE_LIST *pIfList)
{
   if (pIfList != NULL)
   {
      if (pIfList->pInterfaces != NULL)
         MemFree(pIfList->pInterfaces);
      MemFree(pIfList);
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
         MemFree(pArpCache->pEntries);
      MemFree(pArpCache);
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
