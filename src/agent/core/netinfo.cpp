/* 
** NetXMS multiplatform core agent
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
** $module: netinfo.cpp
**
**/

#include "nxagentd.h"
#include <iphlpapi.h>
#include <iprtrmib.h>
#include <rtinfo.h>


//
// Get local ARP cache
//

LONG H_ArpCache(char *cmd, char *arg, NETXMS_VALUES_LIST *value)
{
   MIB_IPNETTABLE *arpCache;
   DWORD i, j, dwError, dwSize;
   char szBuffer[128];

   arpCache = (MIB_IPNETTABLE *)malloc(SIZEOF_IPNETTABLE(4096));
   if (arpCache == NULL)
      return SYSINFO_RC_ERROR;

   dwSize = SIZEOF_IPNETTABLE(4096);
   dwError = GetIpNetTable(arpCache, &dwSize, FALSE);
   if (dwError != NO_ERROR)
   {
      free(arpCache);
      return SYSINFO_RC_ERROR;
   }

   for(i = 0; i < arpCache->dwNumEntries; i++)
      if ((arpCache->table[i].dwType == 3) ||
          (arpCache->table[i].dwType == 4))  // Only static and dynamic entries
      {
         for(j = 0; j < arpCache->table[i].dwPhysAddrLen; j++)
            sprintf(&szBuffer[j << 1], "%02X", arpCache->table[i].bPhysAddr[j]);
         sprintf(&szBuffer[j << 1], " %d.%d.%d.%d %d", arpCache->table[i].dwAddr & 255,
                 (arpCache->table[i].dwAddr >> 8) & 255, 
                 (arpCache->table[i].dwAddr >> 16) & 255, arpCache->table[i].dwAddr >> 24,
                 arpCache->table[i].dwIndex);
         NxAddResultString(value, szBuffer);
      }

   free(arpCache);
   return SYSINFO_RC_SUCCESS;
}


//
// Send IP interface list to server
//

LONG H_InterfaceList(char *cmd, char *arg, NETXMS_VALUES_LIST *value)
{
   MIB_IPADDRTABLE *ifTable;
   MIB_IFROW ifRow;
   DWORD i, dwSize, dwError, dwIfType;
   char szBuffer[2048], szIfName[32], *pszIfName;

   ifTable = (MIB_IPADDRTABLE *)malloc(SIZEOF_IPADDRTABLE(256));
   if (ifTable == NULL)
      return SYSINFO_RC_ERROR;

   dwSize = SIZEOF_IPADDRTABLE(256);
   dwError = GetIpAddrTable(ifTable, &dwSize, FALSE);
   if (dwError != NO_ERROR)
   {
      free(ifTable);
      return SYSINFO_RC_ERROR;
   }

   for(i = 0; i < ifTable->dwNumEntries; i++)
   {
      ifRow.dwIndex = ifTable->table[i].dwIndex;
      if (GetIfEntry(&ifRow) == NO_ERROR)
      {
         dwIfType = ifRow.dwType;
         pszIfName = (char *)ifRow.bDescr;
      }
      else
      {
         dwIfType = IFTYPE_OTHER;
         sprintf(szIfName, "IF-UNKNOWN-%d", ifTable->table[i].dwIndex);
         pszIfName = szIfName;
      }
      sprintf(szBuffer, "%d %d.%d.%d.%d/%d %d %s", ifTable->table[i].dwIndex,
              ifTable->table[i].dwAddr & 255, (ifTable->table[i].dwAddr >> 8) & 255,
              (ifTable->table[i].dwAddr >> 16) & 255, ifTable->table[i].dwAddr >> 24,
              BitsInMask(ifTable->table[i].dwMask), dwIfType, pszIfName);
      NxAddResultString(value, szBuffer);
   }

   free(ifTable);
   return SYSINFO_RC_SUCCESS;
}
