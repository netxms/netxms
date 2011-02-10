/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
** File: netinfo.cpp
**
**/

#include "nxagentd.h"
#include <iphlpapi.h>
#include <iprtrmib.h>
#include <rtinfo.h>


//
// Get local ARP cache
//

LONG H_ArpCache(const TCHAR *cmd, const TCHAR *arg, StringList *value)
{
   MIB_IPNETTABLE *arpCache;
   DWORD i, j, dwError, dwSize;
   TCHAR szBuffer[128];

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
            _sntprintf(&szBuffer[j << 1], 4, _T("%02X"), arpCache->table[i].bPhysAddr[j]);
         _sntprintf(&szBuffer[j << 1], 25, _T(" %d.%d.%d.%d %d"), arpCache->table[i].dwAddr & 255,
                    (arpCache->table[i].dwAddr >> 8) & 255, 
                    (arpCache->table[i].dwAddr >> 16) & 255, arpCache->table[i].dwAddr >> 24,
                    arpCache->table[i].dwIndex);
			value->add(szBuffer);
      }

   free(arpCache);
   return SYSINFO_RC_SUCCESS;
}


//
// Send IP interface list to server
//

LONG H_InterfaceList(const TCHAR *cmd, const TCHAR *arg, StringList *value)
{
   DWORD dwSize;
   IP_ADAPTER_INFO *pBuffer, *pInfo;
   LONG iResult = SYSINFO_RC_SUCCESS;
   TCHAR szAdapterName[MAX_OBJECT_NAME], szBuffer[256], ipAddr[32];
   TCHAR szMacAddr[MAX_ADAPTER_ADDRESS_LENGTH * 2 + 1];
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
            WCHAR wGUID[256], wName[256];

            // Resolve GUID to network connection name
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInfo->AdapterName, -1, wGUID, 256);
            dwSize = 256;
            if (imp_HrLanConnectionNameFromGuidOrPath(NULL, wGUID, wName, &dwSize) == 0)
            {
#ifdef UNICODE
					nx_strncpy(szAdapterName, wName, MAX_OBJECT_NAME);
#else
               WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, 
                                   wName, dwSize, szAdapterName, MAX_OBJECT_NAME, NULL, NULL);
#endif
            }
            else
            {
#ifdef UNICODE
					MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInfo->AdapterName, -1, szAdapterName, MAX_OBJECT_NAME);
					szAdapterName[MAX_OBJECT_NAME - 1] = 0;
#else
               nx_strncpy(szAdapterName, pInfo->AdapterName, MAX_OBJECT_NAME);
#endif
            }
         }
         else
         {
            // We don't have a GUID resolving function, use GUID as name
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInfo->AdapterName, -1, szAdapterName, MAX_OBJECT_NAME);
				szAdapterName[MAX_OBJECT_NAME - 1] = 0;
#else
            nx_strncpy(szAdapterName, pInfo->AdapterName, MAX_OBJECT_NAME);
#endif
         }

         BinToStr(pInfo->Address, pInfo->AddressLength, szMacAddr);

         // Compose result string for each ip address
         for(pAddr = &pInfo->IpAddressList; pAddr != NULL; pAddr = pAddr->Next)
         {
            _sntprintf(szBuffer, 256, _T("%d %s/%d %d %s %s"), pInfo->Index, 
                       IpToStr(ntohl(inet_addr(pAddr->IpAddress.String)), ipAddr), 
                       BitsInMask(ntohl(inet_addr(pAddr->IpMask.String))),
                       pInfo->Type, szMacAddr, szAdapterName);
            value->add(szBuffer);
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

LONG H_NetIPStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
   MIB_IPSTATS ips;
   LONG iResult = SYSINFO_RC_SUCCESS;

   if (GetIpStatistics(&ips) == NO_ERROR)
   {
      switch(CAST_FROM_POINTER(arg, int))
      {
         case NETINFO_IP_FORWARDING:
				ret_int(value, (ips.dwForwarding == MIB_IP_FORWARDING) ? 1 : 0);
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

static DWORD AdapterNameToIndex(TCHAR *pszName)
{
   DWORD dwSize, dwIndex = 0;
   IP_ADAPTER_INFO *pBuffer, *pInfo;
   TCHAR szAdapterName[MAX_OBJECT_NAME];

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
            WCHAR wGUID[256], wName[256];

            // Resolve GUID to network connection name
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInfo->AdapterName, -1, wGUID, 256);
            dwSize = 256;
            if (imp_HrLanConnectionNameFromGuidOrPath(NULL, wGUID, wName, &dwSize) == 0)
            {
#ifdef UNICODE
					nx_strncpy(szAdapterName, wName, MAX_OBJECT_NAME);
#else
               WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, 
                                   wName, dwSize, szAdapterName, MAX_OBJECT_NAME, NULL, NULL);
#endif
            }
            else
            {
#ifdef UNICODE
					MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInfo->AdapterName, -1, szAdapterName, MAX_OBJECT_NAME);
					szAdapterName[MAX_OBJECT_NAME - 1] = 0;
#else
               nx_strncpy(szAdapterName, pInfo->AdapterName, MAX_OBJECT_NAME);
#endif
            }
         }
         else
         {
            // We don't have a GUID resolving function, use GUID as name
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInfo->AdapterName, -1, szAdapterName, MAX_OBJECT_NAME);
				szAdapterName[MAX_OBJECT_NAME - 1] = 0;
#else
            nx_strncpy(szAdapterName, pInfo->AdapterName, MAX_OBJECT_NAME);
#endif
         }

         if (!_tcsicmp(szAdapterName, pszName))
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

LONG H_NetInterfaceStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
   LONG iResult = SYSINFO_RC_SUCCESS;
   TCHAR *eptr, szBuffer[256];
   DWORD dwIndex;

   AgentGetParameterArg(cmd, 1, szBuffer, 256);
   if (szBuffer[0] == 0)
   {
      iResult = SYSINFO_RC_UNSUPPORTED;
   }
   else
   {
      // Determine if it index or name
      dwIndex = _tcstoul(szBuffer, &eptr, 10);
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
            switch(CAST_FROM_POINTER(arg, int))
            {
               case NETINFO_IF_BYTES_IN:
                  ret_uint(value, info.dwInOctets);
                  break;
               case NETINFO_IF_BYTES_OUT:
                  ret_uint(value, info.dwOutOctets);
                  break;
               case NETINFO_IF_DESCR:
#ifdef UNICODE
						MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char *)info.bDescr, -1, value, MAX_RESULT_LENGTH);
						value[MAX_RESULT_LENGTH - 1] = 0;
#else
                  ret_string(value, (char *)info.bDescr);
#endif
                  break;
               case NETINFO_IF_IN_ERRORS:
                  ret_uint(value, info.dwInErrors);
                  break;
               case NETINFO_IF_OPER_STATUS:
                  ret_uint(value, (info.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL) ? 1 : 0);
                  break;
               case NETINFO_IF_OUT_ERRORS:
                  ret_uint(value, info.dwOutErrors);
                  break;
               case NETINFO_IF_PACKETS_IN:
                  ret_uint(value, info.dwInUcastPkts + info.dwInNUcastPkts);
                  break;
               case NETINFO_IF_PACKETS_OUT:
                  ret_uint(value, info.dwOutUcastPkts + info.dwOutNUcastPkts);
                  break;
               case NETINFO_IF_SPEED:
                  ret_uint(value, info.dwSpeed);
                  break;
               case NETINFO_IF_MTU:
                  ret_uint(value, info.dwMtu);
                  break;
               case NETINFO_IF_ADMIN_STATUS:
                  ret_uint(value, info.dwAdminStatus ? 1 : 2);
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


//
// Get IP routing table
//

LONG H_IPRoutingTable(const TCHAR *pszCmd, const TCHAR *pArg, StringList *value)
{
   MIB_IPFORWARDTABLE *pRoutingTable;
   DWORD i, dwError, dwSize;
   TCHAR szBuffer[256], szDestIp[16], szNextHop[16];

   // Determine required buffer size
   dwSize = 0;
   dwError = GetIpForwardTable(NULL, &dwSize, FALSE);
   if ((dwError != NO_ERROR) && (dwError != ERROR_INSUFFICIENT_BUFFER))
      return SYSINFO_RC_ERROR;

   pRoutingTable = (MIB_IPFORWARDTABLE *)malloc(dwSize);
   if (pRoutingTable == NULL)
      return SYSINFO_RC_ERROR;

   dwError = GetIpForwardTable(pRoutingTable, &dwSize, FALSE);
   if (dwError != NO_ERROR)
   {
      free(pRoutingTable);
      return SYSINFO_RC_ERROR;
   }

   for(i = 0; i < pRoutingTable->dwNumEntries; i++)
   {
      _sntprintf(szBuffer, 256, _T("%s/%d %s %d %d"), 
                 IpToStr(ntohl(pRoutingTable->table[i].dwForwardDest), szDestIp),
                 BitsInMask(ntohl(pRoutingTable->table[i].dwForwardMask)),
                 IpToStr(ntohl(pRoutingTable->table[i].dwForwardNextHop), szNextHop),
                 pRoutingTable->table[i].dwForwardIfIndex,
                 pRoutingTable->table[i].dwForwardType);
		value->add(szBuffer);
   }

   free(pRoutingTable);
   return SYSINFO_RC_SUCCESS;
}
