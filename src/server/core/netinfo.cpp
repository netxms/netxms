/* 
** NetXMS - Network Management System
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

#include "nxcore.h"


#ifdef _WIN32

#include <iphlpapi.h>
#include <iprtrmib.h>
#include <rtinfo.h>

#else

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
#if HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif
#if HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif

#endif   /* _WIN32 */


//
// Static data
//

#ifdef _WIN32
static DWORD (__stdcall *imp_HrLanConnectionNameFromGuidOrPath)(LPWSTR, LPWSTR, LPWSTR, LPDWORD) = NULL;
#endif


//
// Initialize
//

void InitLocalNetInfo(void)
{
#ifdef _WIN32
   HMODULE hModule;

   hModule = LoadLibrary("NETMAN.DLL");
   if (hModule != NULL)
   {
      imp_HrLanConnectionNameFromGuidOrPath = 
         (DWORD (__stdcall *)(LPWSTR, LPWSTR, LPWSTR, LPDWORD))GetProcAddress(hModule, "HrLanConnectionNameFromGuidOrPath");
   }
#endif
}


//
// Convert string representation of MAC address to binary form
//

void StrToMac(char *pszStr, BYTE *pBuffer)
{
   DWORD byte1, byte2, byte3, byte4, byte5, byte6;

   memset(pBuffer, 6, 0);
   if (sscanf(pszStr, "%x:%x:%x:%x:%x:%x", &byte1, &byte2, &byte3, 
              &byte4, &byte5, &byte6) == 6)
   {
      pBuffer[0] = (BYTE)byte1;
      pBuffer[1] = (BYTE)byte2;
      pBuffer[2] = (BYTE)byte3;
      pBuffer[3] = (BYTE)byte4;
      pBuffer[4] = (BYTE)byte5;
      pBuffer[5] = (BYTE)byte6;
   }
}


//
// Get interface index from name
//

static DWORD InterfaceIndexFromName(char *pszIfName)
{
#if HAVE_IF_NAMETOINDEX
   return if_nametoindex(pszIfName);
#else
   return 0;   // By default, return 0, which means error
#endif
}


//
// Get local ARP cache
//

static ARP_CACHE *SysGetLocalArpCache(void)
{
   ARP_CACHE *pArpCache = NULL;

#ifdef _WIN32
   MIB_IPNETTABLE *sysArpCache;
   DWORD i, dwError, dwSize;

   sysArpCache = (MIB_IPNETTABLE *)malloc(SIZEOF_IPNETTABLE(4096));
   if (sysArpCache == NULL)
      return NULL;

   dwSize = SIZEOF_IPNETTABLE(4096);
   dwError = GetIpNetTable(sysArpCache, &dwSize, FALSE);
   if (dwError != NO_ERROR)
   {
      WriteLog(MSG_GETIPNETTABLE_FAILED, EVENTLOG_ERROR_TYPE, "e", NULL);
      free(sysArpCache);
      return NULL;
   }

   pArpCache = (ARP_CACHE *)malloc(sizeof(ARP_CACHE));
   pArpCache->dwNumEntries = 0;
   pArpCache->pEntries = (ARP_ENTRY *)malloc(sizeof(ARP_ENTRY) * sysArpCache->dwNumEntries);

   for(i = 0; i < sysArpCache->dwNumEntries; i++)
      if ((sysArpCache->table[i].dwType == 3) ||
          (sysArpCache->table[i].dwType == 4))  // Only static and dynamic entries
      {
         pArpCache->pEntries[pArpCache->dwNumEntries].dwIndex = sysArpCache->table[i].dwIndex;
         pArpCache->pEntries[pArpCache->dwNumEntries].dwIpAddr = ntohl(sysArpCache->table[i].dwAddr);
         memcpy(pArpCache->pEntries[pArpCache->dwNumEntries].bMacAddr, sysArpCache->table[i].bPhysAddr, 6);
         pArpCache->dwNumEntries++;
      }

   free(sysArpCache);
#else
   FILE *fp;
   char szBuffer[256], szIpAddr[256], szMacAddr[256], szIfName[256];
   char *pNext;
   DWORD i;

   fp = fopen("/proc/net/arp", "r");
   if (fp != NULL)
   {
      pArpCache = (ARP_CACHE *)malloc(sizeof(ARP_CACHE));
      pArpCache->dwNumEntries = 0;
      pArpCache->pEntries = NULL;

      fgets(szBuffer, 255, fp);  // Skip first line

      while(1)
      {
         fgets(szBuffer, 255, fp);
         if (feof(fp))
            break;

         // Remove newline character
         pNext = strchr(szBuffer, '\n');
         if (pNext != NULL)
            *pNext = 0;

         // Field #1 is IP address, #4 is MAC address and #6 is device
         pNext = ExtractWord(szBuffer, szIpAddr);
         pNext = ExtractWord(pNext, szMacAddr);
         pNext = ExtractWord(pNext, szMacAddr);
         pNext = ExtractWord(pNext, szMacAddr);
         pNext = ExtractWord(pNext, szIfName);
         pNext = ExtractWord(pNext, szIfName);

         // Create new entry in ARP_CACHE structure
         i = pArpCache->dwNumEntries++;
         pArpCache->pEntries = (ARP_ENTRY *)realloc(pArpCache->pEntries,
                                 sizeof(ARP_ENTRY) * pArpCache->dwNumEntries);
         pArpCache->pEntries[i].dwIndex = InterfaceIndexFromName(szIfName);
         pArpCache->pEntries[i].dwIpAddr = ntohl(inet_addr(szIpAddr));
         StrToMac(szMacAddr, pArpCache->pEntries[i].bMacAddr);
      }
      fclose(fp);
   }
#endif

   return pArpCache;
}


//
// Get local interface list (built-in system dependent code)
//

static INTERFACE_LIST *SysGetLocalIfList(void)
{
   INTERFACE_LIST *pIfList;

#ifdef _WIN32
   DWORD dwSize;
   IP_ADAPTER_INFO *pBuffer, *pInfo;
   char szAdapterName[MAX_OBJECT_NAME];
   char szMacAddr[MAX_ADAPTER_ADDRESS_LENGTH * 2 + 1];
   IP_ADDR_STRING *pAddr;

   if (GetAdaptersInfo(NULL, &dwSize) != ERROR_BUFFER_OVERFLOW)
   {
      return NULL;
   }

   pBuffer = (IP_ADAPTER_INFO *)malloc(dwSize);
   if (GetAdaptersInfo(pBuffer, &dwSize) == ERROR_SUCCESS)
   {
      // Allocate and initialize interface list structure
      pIfList = (INTERFACE_LIST *)malloc(sizeof(INTERFACE_LIST));
      pIfList->iNumEntries = 0;
      pIfList->pInterfaces = NULL;

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

         // Compose result for each ip address
         for(pAddr = &pInfo->IpAddressList; pAddr != NULL; pAddr = pAddr->Next)
         {
            pIfList->pInterfaces = (INTERFACE_INFO *)realloc(pIfList->pInterfaces,
                                          sizeof(INTERFACE_INFO) * (pIfList->iNumEntries + 1));
            _tcsncpy(pIfList->pInterfaces[pIfList->iNumEntries].szName, szAdapterName, MAX_OBJECT_NAME);
            memcpy(pIfList->pInterfaces[pIfList->iNumEntries].bMacAddr, pInfo->Address, MAC_ADDR_LENGTH);
            pIfList->pInterfaces[pIfList->iNumEntries].dwIndex = pInfo->Index;
            pIfList->pInterfaces[pIfList->iNumEntries].dwIpAddr = ntohl(_t_inet_addr(pAddr->IpAddress.String));
            pIfList->pInterfaces[pIfList->iNumEntries].dwIpNetMask = ntohl(_t_inet_addr(pAddr->IpMask.String));
            pIfList->pInterfaces[pIfList->iNumEntries].dwType = pInfo->Type;
            pIfList->pInterfaces[pIfList->iNumEntries].iNumSecondary = 0;
            pIfList->iNumEntries++;
         }
      }
   }

   free(pBuffer);

#else

   struct if_nameindex *ifni;
   struct ifreq ifrq;
   int i, iSock, iCount;
   BOOL bIoFailed = FALSE;

   ifni = if_nameindex();
   if (ifni != NULL)
   {
      iSock = socket(AF_INET, SOCK_DGRAM, 0);
      if (iSock != -1)
      {
         // Count interfaces
         for(iCount = 0; ifni[iCount].if_index != 0; iCount++);

         // Allocate and initialize interface list structure
         pIfList = (INTERFACE_LIST *)malloc(sizeof(INTERFACE_LIST));
         pIfList->iNumEntries = iCount;
         pIfList->pInterfaces = (INTERFACE_INFO *)malloc(sizeof(INTERFACE_INFO) * iCount);
         memset(pIfList->pInterfaces, 0, sizeof(INTERFACE_INFO) * iCount);

         for(i = 0; i < iCount; i++)
         {
            // Interface name
            strcpy(pIfList->pInterfaces[i].szName, ifni[i].if_name);
            strcpy(ifrq.ifr_name, ifni[i].if_name);

            // IP address
            if (ioctl(iSock, SIOCGIFADDR, &ifrq) == 0)
            {
               memcpy(&pIfList->pInterfaces[i].dwIpAddr,
                      &(((struct sockaddr_in *)&ifrq.ifr_addr)->sin_addr.s_addr),
                      sizeof(DWORD));
               pIfList->pInterfaces[i].dwIpAddr = ntohl(pIfList->pInterfaces[i].dwIpAddr);
            }

            // IP netmask
            if (ioctl(iSock, SIOCGIFNETMASK, &ifrq) == 0)
            {
               memcpy(&pIfList->pInterfaces[i].dwIpNetMask,
                      &(((struct sockaddr_in *)&ifrq.ifr_addr)->sin_addr.s_addr),
                      sizeof(DWORD));
               pIfList->pInterfaces[i].dwIpNetMask = ntohl(pIfList->pInterfaces[i].dwIpNetMask);
            }

            // Interface type
#if HAVE_DECL_SIOCGIFHWADDR
            if (ioctl(iSock, SIOCGIFHWADDR, &ifrq) == 0)
            {
               switch(ifrq.ifr_hwaddr.sa_family)
               {
                  case ARPHRD_ETHER:
                     pIfList->pInterfaces[i].dwType = IFTYPE_ETHERNET_CSMACD;
                     break;
                  case ARPHRD_SLIP:
                  case ARPHRD_CSLIP:
                  case ARPHRD_SLIP6:
                  case ARPHRD_CSLIP6:
                     pIfList->pInterfaces[i].dwType = IFTYPE_SLIP;
                     break;
                  case ARPHRD_PPP:
                     pIfList->pInterfaces[i].dwType = IFTYPE_PPP;
                     break;
                  case ARPHRD_LOOPBACK:
                     pIfList->pInterfaces[i].dwType = IFTYPE_SOFTWARE_LOOPBACK;
                     break;
                  case ARPHRD_FDDI:
                     pIfList->pInterfaces[i].dwType = IFTYPE_FDDI;
                     break;
                  default:
                     pIfList->pInterfaces[i].dwType = IFTYPE_OTHER;
                     break;
               }
            }
#else
            pIfList->pInterfaces[i].dwType = 0;    // Unknown type
#endif

            // Interface index
            pIfList->pInterfaces[i].dwIndex = ifni[i].if_index;
         }
         close(iSock);
      }
      if_freenameindex(ifni);
   }
   else
   {
      WriteLog(MSG_IFNAMEINDEX_FAILED, EVENTLOG_ERROR_TYPE, "e", errno);
   }

#endif

   return pIfList;
}


//
// Get local ARP cache
//

ARP_CACHE *GetLocalArpCache(void)
{
   ARP_CACHE *pArpCache = NULL;
   AgentConnection conn(inet_addr("127.0.0.1"));

   // Try to get local interface list from agent via loopback address
   if (conn.Connect())
   {
      pArpCache = conn.GetArpCache();
      conn.Disconnect();
   }

   // Use built-in code as last resort
   if (pArpCache == NULL)
      pArpCache = SysGetLocalArpCache();

   return pArpCache;
}


//
// Get local interface list
//

INTERFACE_LIST *GetLocalInterfaceList(void)
{
   INTERFACE_LIST *pIfList = NULL;
   AgentConnection conn(inet_addr("127.0.0.1"));

   // Try to get local interface list from agent via loopback address
   if (conn.Connect())
   {
      pIfList = conn.GetInterfaceList();
      conn.Disconnect();
   }

   // Use built-in code as last resort
   if (pIfList == NULL)
      pIfList = SysGetLocalIfList();

   CleanInterfaceList(pIfList);
   return pIfList;
}
