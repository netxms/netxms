/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
static UINT32 (__stdcall *imp_HrLanConnectionNameFromGuidOrPath)(LPWSTR, LPWSTR, LPWSTR, LPDWORD) = NULL;
#else
static HMODULE m_hSubAgent = NULL;
static BOOL (* imp_NxSubAgentGetIfList)(StringList *) = NULL;
static BOOL (* imp_NxSubAgentGetArpCache)(StringList *) = NULL;
#endif


//
// Initialize
//

void InitLocalNetInfo()
{
#ifdef _WIN32
   HMODULE hModule;

   hModule = LoadLibrary(_T("NETMAN.DLL"));
   if (hModule != NULL)
   {
      imp_HrLanConnectionNameFromGuidOrPath = 
         (UINT32 (__stdcall *)(LPWSTR, LPWSTR, LPWSTR, LPDWORD))GetProcAddress(hModule, "HrLanConnectionNameFromGuidOrPath");
   }
#elif HAVE_SYS_UTSNAME_H
   struct utsname un;
   TCHAR szName[MAX_PATH], szErrorText[256];
   int i;

   if (uname(&un) != -1)
   {
      // Convert system name to lowercase
      for(i = 0; un.sysname[i] != 0; i++)
         un.sysname[i] = tolower(un.sysname[i]);
      if (!strcmp(un.sysname, "hp-ux"))
         strcpy(un.sysname, "hpux");
      _sntprintf(szName, MAX_PATH, LIBDIR _T("/libnsm_%hs") SHL_SUFFIX, un.sysname);

      m_hSubAgent = DLOpen(szName, szErrorText);
      if (m_hSubAgent != NULL)
      {
         imp_NxSubAgentGetIfList = (BOOL (*)(StringList *))DLGetSymbolAddr(m_hSubAgent, "__NxSubAgentGetIfList", NULL);
         imp_NxSubAgentGetArpCache = (BOOL (*)(StringList *))DLGetSymbolAddr(m_hSubAgent, "__NxSubAgentGetArpCache", NULL);
         if ((imp_NxSubAgentGetIfList == NULL) &&
             (imp_NxSubAgentGetArpCache == NULL))
         {
            DLClose(m_hSubAgent);
            m_hSubAgent = NULL;
            nxlog_write(MSG_PLATFORM_SUBAGENT_NOT_LOADED, NXLOG_ERROR,
                     "ss", szName, _T("Subagent doesn't provide any usable parameters"));
         }
         else
         {
            nxlog_write(MSG_PLATFORM_SUBAGENT_LOADED, NXLOG_INFO, "s", szName);
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

/**
 * Convert string representation of MAC address to binary form
 */
void StrToMac(const TCHAR *pszStr, BYTE *pBuffer)
{
   UINT32 byte1, byte2, byte3, byte4, byte5, byte6;

   memset(pBuffer, 0, 6);
   if (_stscanf(pszStr, _T("%x:%x:%x:%x:%x:%x"), &byte1, &byte2, &byte3, 
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

/**
 * Get local ARP cache
 */
static ARP_CACHE *SysGetLocalArpCache()
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
	TCHAR szByte[4], *pChar;
	UINT32 i, j;

	if (imp_NxSubAgentGetArpCache != NULL)
	{
		StringList list;

      if (imp_NxSubAgentGetArpCache(&list))
      {
         // Create empty structure
         pArpCache = (ARP_CACHE *)malloc(sizeof(ARP_CACHE));
         pArpCache->dwNumEntries = list.getSize();
         pArpCache->pEntries = (ARP_ENTRY *)malloc(sizeof(ARP_ENTRY) * list.getSize());
         memset(pArpCache->pEntries, 0, sizeof(ARP_ENTRY) * list.getSize());

         szByte[2] = 0;

         // Parse data lines
         // Each line has form of XXXXXXXXXXXX a.b.c.d n
         // where XXXXXXXXXXXX is a MAC address (12 hexadecimal digits)
         // a.b.c.d is an IP address in decimal dotted notation
         // n is an interface index
         for(i = 0; i < list.getSize(); i++)
         {
            TCHAR *pTemp = _tcsdup(list.getValue(i));
            TCHAR *pBuf = pTemp;
            if (_tcslen(pBuf) < 20)     // Invalid line
               continue;

            // MAC address
            for(j = 0; j < 6; j++)
            {
               memcpy(szByte, pBuf, sizeof(TCHAR) * 2);
               pArpCache->pEntries[i].bMacAddr[j] = (BYTE)_tcstol(szByte, NULL, 16);
               pBuf += 2;
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

            free(pTemp);
         }
      }
   }
#endif

   return pArpCache;
}

/**
 * Get local interface list (built-in system dependent code)
 */
static InterfaceList *SysGetLocalIfList()
{
   InterfaceList *pIfList = NULL;
	NX_INTERFACE_INFO iface;

#ifdef _WIN32
   DWORD dwSize;
   IP_ADAPTER_INFO *pBuffer, *pInfo;
   TCHAR szAdapterName[MAX_OBJECT_NAME];
   TCHAR szMacAddr[MAX_ADAPTER_ADDRESS_LENGTH * 2 + 1];
   IP_ADDR_STRING *pAddr;

   if (GetAdaptersInfo(NULL, &dwSize) != ERROR_BUFFER_OVERFLOW)
   {
      return NULL;
   }

   pBuffer = (IP_ADAPTER_INFO *)malloc(dwSize);
   if (GetAdaptersInfo(pBuffer, &dwSize) == ERROR_SUCCESS)
   {
      pIfList = new InterfaceList;

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
#ifdef UNICODE
					nx_strncpy(szAdapterName, wName, MAX_OBJECT_NAME);
#else
               WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, 
                                   wName, dwSize, szAdapterName, MAX_OBJECT_NAME, NULL, NULL);
               szAdapterName[MAX_OBJECT_NAME - 1] = 0;
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

         // Compose result for each ip address
         for(pAddr = &pInfo->IpAddressList; pAddr != NULL; pAddr = pAddr->Next)
         {
				memset(&iface, 0, sizeof(NX_INTERFACE_INFO));
            nx_strncpy(iface.szName, szAdapterName, MAX_OBJECT_NAME);
            memcpy(iface.bMacAddr, pInfo->Address, MAC_ADDR_LENGTH);
            iface.dwIndex = pInfo->Index;
            iface.dwIpAddr = ntohl(inet_addr(pAddr->IpAddress.String));
            iface.dwIpNetMask = ntohl(inet_addr(pAddr->IpMask.String));
            iface.dwType = pInfo->Type;
				pIfList->add(&iface);
         }
      }
   }

   free(pBuffer);

#else
   UINT32 i, dwBits;
   TCHAR *pChar;

   if (imp_NxSubAgentGetIfList != NULL)
   {
		StringList list;
      if (imp_NxSubAgentGetIfList(&list))
      {
         pIfList = new InterfaceList(list.getSize());
         for(i = 0; i < list.getSize(); i++)
         {
				memset(&iface, 0, sizeof(NX_INTERFACE_INFO));
            TCHAR *pTemp = _tcsdup(list.getValue(i));
            TCHAR *pBuf = pTemp;

            // Index
            pChar = _tcschr(pBuf, ' ');
            if (pChar != NULL)
            {
               *pChar = 0;
               iface.dwIndex = _tcstoul(pBuf, NULL, 10);
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
               iface.dwIpAddr = ntohl(_t_inet_addr(pBuf));
               dwBits = _tcstoul(pSlash, NULL, 10);
               iface.dwIpNetMask = (dwBits == 32) ? 0xFFFFFFFF : (~(0xFFFFFFFF >> dwBits));
               pBuf = pChar + 1;
            }

            // Interface type
            pChar = _tcschr(pBuf, ' ');
            if (pChar != NULL)
            {
               *pChar = 0;
               iface.dwType = _tcstoul(pBuf, NULL, 10);
               pBuf = pChar + 1;
            }

            // MAC address
            pChar = _tcschr(pBuf, ' ');
            if (pChar != NULL)
            {
               *pChar = 0;
               StrToBin(pBuf, iface.bMacAddr, MAC_ADDR_LENGTH);
               pBuf = pChar + 1;
            }

            // Name
            nx_strncpy(iface.szName, pBuf, MAX_OBJECT_NAME - 1);

            free(pTemp);
				pIfList->add(&iface);
         }
      }
   }
#endif

   return pIfList;
}


//
// Get local ARP cache
//

ARP_CACHE *GetLocalArpCache()
{
   ARP_CACHE *pArpCache = NULL;

   // Get ARP cache from built-in code or platform subagent
   pArpCache = SysGetLocalArpCache();

   // Try to get ARP cache from agent via loopback address
   if (pArpCache == NULL)
   {
      AgentConnection conn(inet_addr("127.0.0.1"));

      if (conn.connect(g_pServerKey))
      {
         pArpCache = conn.getArpCache();
         conn.disconnect();
      }
   }

   return pArpCache;
}


//
// Get local interface list
//

InterfaceList *GetLocalInterfaceList()
{
   InterfaceList *pIfList = NULL;

   // Get interface list from built-in code or platform subagent
   pIfList = SysGetLocalIfList();

   // Try to get local interface list from agent via loopback address
   if (pIfList == NULL)
   {
      AgentConnection conn(inet_addr("127.0.0.1"));

      if (conn.connect(g_pServerKey))
      {
         pIfList = conn.getInterfaceList();
         conn.disconnect();
      }
   }
   return pIfList;
}
