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

#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#endif   /* _WIN32 */


//
// Static data
//

#ifdef _WIN32
static DWORD (__stdcall *imp_HrLanConnectionNameFromGuidOrPath)(LPWSTR, LPWSTR, LPWSTR, LPDWORD) = NULL;
#else
static HMODULE m_hSubAgent = NULL;
static BOOL (* imp_NxSubAgentGetIfList)(NETXMS_VALUES_LIST *) = NULL;
static BOOL (* imp_NxSubAgentGetArpCache)(NETXMS_VALUES_LIST *) = NULL;
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
#elif HAVE_SYS_UTSNAME_H
   struct utsname un;
   char szName[MAX_PATH], szErrorText[256];
   int i;

   if (uname(&un) != -1)
   {
      // Convert system name to lowercase
      for(i = 0; un.sysname[i] != 0; i++)
         un.sysname[i] = tolower(un.sysname[i]);
      if (!strcmp(un.sysname, "hp-ux"))
         strcpy(un.sysname, "hpux");
      snprintf(szName, MAX_PATH, LIBDIR "/libnsm_%s" SHL_SUFFIX, un.sysname);

      m_hSubAgent = DLOpen(szName, szErrorText);
      if (m_hSubAgent != NULL)
      {
         imp_NxSubAgentGetIfList = (BOOL (*)(NETXMS_VALUES_LIST *))DLGetSymbolAddr(m_hSubAgent, "__NxSubAgentGetIfList", NULL);
         imp_NxSubAgentGetArpCache = (BOOL (*)(NETXMS_VALUES_LIST *))DLGetSymbolAddr(m_hSubAgent, "__NxSubAgentGetArpCache", NULL);
         if ((imp_NxSubAgentGetIfList == NULL) &&
             (imp_NxSubAgentGetArpCache == NULL))
         {
            DLClose(m_hSubAgent);
            m_hSubAgent = NULL;
            nxlog_write(MSG_PLATFORM_SUBAGENT_NOT_LOADED, EVENTLOG_ERROR_TYPE,
                     "ss", szName, _T("Subagent doesn't provide any usable parameters"));
         }
         else
         {
            nxlog_write(MSG_PLATFORM_SUBAGENT_LOADED,
                     EVENTLOG_INFORMATION_TYPE, "s", szName);
         }
      }
      else
      {
         nxlog_write(MSG_PLATFORM_SUBAGENT_NOT_LOADED, EVENTLOG_ERROR_TYPE,
                  "ss", szName, szErrorText);
      }
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
      nxlog_write(MSG_GETIPNETTABLE_FAILED, EVENTLOG_ERROR_TYPE, "e", NULL);
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
   NETXMS_VALUES_LIST list;
   TCHAR szByte[4], *pBuf, *pChar;
   DWORD i, j;

   if (imp_NxSubAgentGetArpCache != NULL)
   {
		memset(&list, 0, sizeof(NETXMS_VALUES_LIST));
      if (imp_NxSubAgentGetArpCache(&list))
      {
         // Create empty structure
         pArpCache = (ARP_CACHE *)malloc(sizeof(ARP_CACHE));
         pArpCache->dwNumEntries = list.dwNumStrings;
         pArpCache->pEntries = (ARP_ENTRY *)malloc(sizeof(ARP_ENTRY) * list.dwNumStrings);
         memset(pArpCache->pEntries, 0, sizeof(ARP_ENTRY) * list.dwNumStrings);

         szByte[2] = 0;

         // Parse data lines
         // Each line has form of XXXXXXXXXXXX a.b.c.d n
         // where XXXXXXXXXXXX is a MAC address (12 hexadecimal digits)
         // a.b.c.d is an IP address in decimal dotted notation
         // n is an interface index
         for(i = 0; i < list.dwNumStrings; i++)
         {
            pBuf = list.ppStringList[i];
            if (_tcslen(pBuf) < 20)     // Invalid line
               continue;

            // MAC address
            for(j = 0; j < 6; j++)
            {
               memcpy(szByte, pBuf, sizeof(TCHAR) * 2);
               pArpCache->pEntries[i].bMacAddr[j] = (BYTE)_tcstol(szByte, NULL, 16);
               pBuf+=2;
            }

            // IP address
            while(*pBuf == ' ')
               pBuf++;
            pChar = _tcschr(pBuf, _T(' '));
            if (pChar != NULL)
               *pChar = 0;
            pArpCache->pEntries[i].dwIpAddr = ntohl(_t_inet_addr(pBuf));

            // Interface index
            if (pChar != NULL)
               pArpCache->pEntries[i].dwIndex = _tcstoul(pChar + 1, NULL, 10);
         }

         for(i = 0; i < list.dwNumStrings; i++)
            safe_free(list.ppStringList[i]);
         safe_free(list.ppStringList);
      }
   }
#endif

   return pArpCache;
}


//
// Get local interface list (built-in system dependent code)
//

static INTERFACE_LIST *SysGetLocalIfList(void)
{
   INTERFACE_LIST *pIfList = NULL;

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
            WCHAR wGUID[256], wName[256];

            // Resolve GUID to network connection name
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInfo->AdapterName, -1, wGUID, 256);
            wGUID[255] = 0;
            dwSize = 256;
            if (imp_HrLanConnectionNameFromGuidOrPath(NULL, wGUID, wName, &dwSize) == 0)
            {
               WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, 
                                   wName, dwSize, szAdapterName, MAX_OBJECT_NAME, NULL, NULL);
               szAdapterName[MAX_OBJECT_NAME - 1] = 0;
            }
            else
            {
               nx_strncpy(szAdapterName, pInfo->AdapterName, MAX_OBJECT_NAME);
            }
         }
         else
         {
            // We don't have a GUID resolving function, use GUID as name
            nx_strncpy(szAdapterName, pInfo->AdapterName, MAX_OBJECT_NAME);
         }

         BinToStr(pInfo->Address, pInfo->AddressLength, szMacAddr);

         // Compose result for each ip address
         for(pAddr = &pInfo->IpAddressList; pAddr != NULL; pAddr = pAddr->Next)
         {
            pIfList->pInterfaces = (INTERFACE_INFO *)realloc(pIfList->pInterfaces,
                                          sizeof(INTERFACE_INFO) * (pIfList->iNumEntries + 1));
            nx_strncpy(pIfList->pInterfaces[pIfList->iNumEntries].szName, szAdapterName, MAX_OBJECT_NAME);
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
   NETXMS_VALUES_LIST list;
   DWORD i, dwBits;
   TCHAR *pChar, *pBuf;

   if (imp_NxSubAgentGetIfList != NULL)
   {
		memset(&list, 0, sizeof(NETXMS_VALUES_LIST));
      if (imp_NxSubAgentGetIfList(&list))
      {
         pIfList = (INTERFACE_LIST *)malloc(sizeof(INTERFACE_LIST));
         pIfList->iNumEntries = list.dwNumStrings;
         pIfList->pInterfaces = (INTERFACE_INFO *)malloc(sizeof(INTERFACE_INFO) * list.dwNumStrings);
         memset(pIfList->pInterfaces, 0, sizeof(INTERFACE_INFO) * list.dwNumStrings);
         for(i = 0; i < list.dwNumStrings; i++)
         {
            pBuf = list.ppStringList[i];

            // Index
            pChar = _tcschr(pBuf, ' ');
            if (pChar != NULL)
            {
               *pChar = 0;
               pIfList->pInterfaces[i].dwIndex = _tcstoul(pBuf, NULL, 10);
               pBuf = pChar + 1;
            }

            // Address and mask
            pChar = _tcschr(pBuf, _T(' '));
            if (pChar != NULL)
            {
               TCHAR *pSlash;
               static TCHAR defaultMask[] = _T("24");

               *pChar = 0;
               pSlash = _tcschr(pBuf, _T('/'));
               if (pSlash != NULL)
               {
                  *pSlash = 0;
                  pSlash++;
               }
               else     // Just a paranoia protection, should'n happen if subagent working correctly
               {
                  pSlash = defaultMask;
               }
               pIfList->pInterfaces[i].dwIpAddr = ntohl(_t_inet_addr(pBuf));
               dwBits = _tcstoul(pSlash, NULL, 10);
               pIfList->pInterfaces[i].dwIpNetMask = (dwBits == 32) ? 0xFFFFFFFF : (~(0xFFFFFFFF >> dwBits));
               pBuf = pChar + 1;
            }

            // Interface type
            pChar = _tcschr(pBuf, ' ');
            if (pChar != NULL)
            {
               *pChar = 0;
               pIfList->pInterfaces[i].dwType = _tcstoul(pBuf, NULL, 10);
               pBuf = pChar + 1;
            }

            // MAC address
            pChar = _tcschr(pBuf, ' ');
            if (pChar != NULL)
            {
               *pChar = 0;
               StrToBin(pBuf, pIfList->pInterfaces[i].bMacAddr, MAC_ADDR_LENGTH);
               pBuf = pChar + 1;
            }

            // Name
            nx_strncpy(pIfList->pInterfaces[i].szName, pBuf, MAX_OBJECT_NAME - 1);
         }

         for(i = 0; i < list.dwNumStrings; i++)
            safe_free(list.ppStringList[i]);
         safe_free(list.ppStringList);
      }
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

   // Get ARP cache from built-in code or platform subagent
   pArpCache = SysGetLocalArpCache();

   // Try to get ARP cache from agent via loopback address
   if (pArpCache == NULL)
   {
      AgentConnection conn(inet_addr("127.0.0.1"));

      if (conn.Connect(g_pServerKey))
      {
         pArpCache = conn.GetArpCache();
         conn.Disconnect();
      }
   }

   return pArpCache;
}


//
// Get local interface list
//

INTERFACE_LIST *GetLocalInterfaceList(void)
{
   INTERFACE_LIST *pIfList = NULL;

   // Get interface list from built-in code or platform subagent
   pIfList = SysGetLocalIfList();

   // Try to get local interface list from agent via loopback address
   if (pIfList == NULL)
   {
      AgentConnection conn(inet_addr("127.0.0.1"));

      if (conn.Connect(g_pServerKey))
      {
         pIfList = conn.GetInterfaceList();
         conn.Disconnect();
      }
   }

   CleanInterfaceList(pIfList);
   return pIfList;
}
