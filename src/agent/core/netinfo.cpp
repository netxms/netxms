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
   DWORD dwSize;
   IP_ADAPTER_INFO *pBuffer, *pInfo;
   LONG iResult = SYSINFO_RC_SUCCESS;
   char szAdapterName[MAX_OBJECT_NAME], szBuffer[256];
   char szMacAddr[MAX_ADAPTER_ADDRESS_LENGTH * 2 + 1];
   IP_ADDR_STRING *pAddr;

   if (GetAdaptersInfo(NULL, &dwSize) != ERROR_BUFFER_OVERFLOW)
      return SYSINFO_RC_ERROR;

   pBuffer = (IP_ADAPTER_INFO *)malloc(dwSize);
   if (GetAdaptersInfo(pBuffer, &dwSize) == ERROR_SUCCESS)
   {
      for(pInfo = pBuffer; pInfo != NULL; pInfo = pInfo->Next)
      {
         // Get network connection name from adapter name, if possible
         if (imp_HrLanConnectionNameFromGuidOrPath != NULL)
         {
            WORD wGUID[256], wName[256];

            // Resolve GUID to network connection name
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInfo->AdapterName, -1, wGUID, 256);
            dwSize = 256;
            if (imp_HrLanConnectionNameFromGuidOrPath(NULL, wGUID, wName, &dwSize) == 0)
            {
               WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, 
                                   wName, dwSize, szAdapterName, MAX_OBJECT_NAME, NULL, NULL);
            }
            else
            {
               strncpy(szAdapterName, pInfo->AdapterName, MAX_OBJECT_NAME);
            }
         }
         else
         {
            // We don't have a GUID resolving function, use GUID as name
            strncpy(szAdapterName, pInfo->AdapterName, MAX_OBJECT_NAME);
         }

         BinToStr(pInfo->Address, pInfo->AddressLength, szMacAddr);

         // Compose result string for each ip address
         for(pAddr = &pInfo->IpAddressList; pAddr != NULL; pAddr = pAddr->Next)
         {
            sprintf(szBuffer, "%d %s/%d %d %s %s", pInfo->Index, 
                    pAddr->IpAddress.String, 
                    BitsInMask(ntohl(inet_addr(pAddr->IpMask.String))),
                    pInfo->Type, szMacAddr, szAdapterName);
            NxAddResultString(value, szBuffer);
         }
      }
   }
   else
   {
      iResult = SYSINFO_RC_ERROR;
   }

   free(pBuffer);
   return iResult;
}


//
// Get IP statistics
//

LONG H_NetIPStats(char *cmd, char *arg, char *value)
{
   MIB_IPSTATS ips;
   LONG iResult = SYSINFO_RC_SUCCESS;

   if (GetIpStatistics(&ips) == NO_ERROR)
   {
      switch((int)arg)
      {
         case NET_IP_FORWARDING:
            ret_int(value, ips.dwForwarding);
            break;
         default:
            iResult = SYSINFO_RC_UNSUPPORTED;
            break;
      }
   }
   else
   {
      iResult = SYSINFO_RC_ERROR;
   }

   return iResult;
}


//
// Get adapter index from name
//

static DWORD AdapterNameToIndex(char *pszName)
{
   DWORD dwSize, dwIndex = 0;
   IP_ADAPTER_INFO *pBuffer, *pInfo;
   char szAdapterName[MAX_OBJECT_NAME];

   if (GetAdaptersInfo(NULL, &dwSize) != ERROR_BUFFER_OVERFLOW)
      return 0;

   pBuffer = (IP_ADAPTER_INFO *)malloc(dwSize);
   if (GetAdaptersInfo(pBuffer, &dwSize) == ERROR_SUCCESS)
   {
      for(pInfo = pBuffer; pInfo != NULL; pInfo = pInfo->Next)
      {
         // Get network connection name from adapter name, if possible
         if (imp_HrLanConnectionNameFromGuidOrPath != NULL)
         {
            WORD wGUID[256], wName[256];

            // Resolve GUID to network connection name
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInfo->AdapterName, -1, wGUID, 256);
            dwSize = 256;
            if (imp_HrLanConnectionNameFromGuidOrPath(NULL, wGUID, wName, &dwSize) == 0)
            {
               WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, 
                                   wName, dwSize, szAdapterName, MAX_OBJECT_NAME, NULL, NULL);
            }
            else
            {
               strncpy(szAdapterName, pInfo->AdapterName, MAX_OBJECT_NAME);
            }
         }
         else
         {
            // We don't have a GUID resolving function, use GUID as name
            strncpy(szAdapterName, pInfo->AdapterName, MAX_OBJECT_NAME);
         }

         if (!stricmp(szAdapterName, pszName))
         {
            dwIndex = pInfo->Index;
            break;
         }
      }
   }

   free(pBuffer);
   return dwIndex;
}


//
// Get interface statistics
//

LONG H_NetInterfaceStats(char *cmd, char *arg, char *value)
{
   LONG iResult = SYSINFO_RC_SUCCESS;
   char *eptr, szBuffer[256];
   DWORD dwIndex;

   NxGetParameterArg(cmd, 1, szBuffer, 256);
   if (szBuffer[0] == 0)
   {
      iResult = SYSINFO_RC_UNSUPPORTED;
   }
   else
   {
      // Determine if it index or name
      dwIndex = strtoul(szBuffer, &eptr, 10);
      if (*eptr != 0)
      {
         // It's a name, determine index
         dwIndex = AdapterNameToIndex(szBuffer);
      }

      if (dwIndex != 0)
      {
         MIB_IFROW info;

         info.dwIndex = dwIndex;
         if (GetIfEntry(&info) == NO_ERROR)
         {
            switch((int)arg)
            {
               case NET_IF_BYTES_IN:
                  ret_uint(value, info.dwInOctets);
                  break;
               case NET_IF_BYTES_OUT:
                  ret_uint(value, info.dwOutOctets);
                  break;
               case NET_IF_DESCR:
                  ret_string(value, (char *)info.bDescr);
                  break;
               case NET_IF_IN_ERRORS:
                  ret_uint(value, info.dwInErrors);
                  break;
               case NET_IF_LINK:
                  ret_uint(value, (info.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL) ? 1 : 0);
                  break;
               case NET_IF_OUT_ERRORS:
                  ret_uint(value, info.dwOutErrors);
                  break;
               case NET_IF_PACKETS_IN:
                  ret_uint(value, info.dwInUcastPkts + info.dwInNUcastPkts);
                  break;
               case NET_IF_PACKETS_OUT:
                  ret_uint(value, info.dwOutUcastPkts + info.dwOutNUcastPkts);
                  break;
               case NET_IF_SPEED:
                  ret_uint(value, info.dwSpeed);
                  break;
               case NET_IF_ADMIN_STATUS:
                  ret_uint(value, info.dwAdminStatus ? 1 : 0);
                  break;
               default:
                  iResult = SYSINFO_RC_UNSUPPORTED;
                  break;
            }
         }
         else
         {
            iResult = SYSINFO_RC_ERROR;
         }
      }
      else
      {
         iResult = SYSINFO_RC_UNSUPPORTED;
      }
   }

   return iResult;
}
