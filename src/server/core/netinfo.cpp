/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
static UINT32 (__stdcall *imp_HrLanConnectionNameFromGuidOrPath)(LPWSTR, LPWSTR, LPWSTR, LPDWORD) = nullptr;
#else
static HMODULE s_hSubAgent = nullptr;
static BOOL (* imp_NxSubAgentGetIfList)(StringList *) = nullptr;
static BOOL (* imp_NxSubAgentGetArpCache)(StringList *) = nullptr;
#endif

/**
 * Initialize
 */
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
   if (uname(&un) != -1)
   {
      // Convert system name to lowercase
      strlwr(un.sysname);
      if (!strcmp(un.sysname, "hp-ux"))
         strcpy(un.sysname, "hpux");

      TCHAR libdir[MAX_PATH];
      GetNetXMSDirectory(nxDirLib, libdir);

      TCHAR subagentName[MAX_PATH], errorText[256];
      _sntprintf(subagentName, MAX_PATH, _T("%s/%hs.nsm"), libdir, un.sysname);
      s_hSubAgent = DLOpen(subagentName, errorText);
      if (s_hSubAgent != nullptr)
      {
         imp_NxSubAgentGetIfList = (BOOL (*)(StringList *))DLGetSymbolAddr(s_hSubAgent, "__NxSubAgentGetIfList", nullptr);
         imp_NxSubAgentGetArpCache = (BOOL (*)(StringList *))DLGetSymbolAddr(s_hSubAgent, "__NxSubAgentGetArpCache", nullptr);
         if ((imp_NxSubAgentGetIfList == nullptr) && (imp_NxSubAgentGetArpCache == nullptr))
         {
            DLClose(s_hSubAgent);
            s_hSubAgent = nullptr;
            nxlog_write(NXLOG_ERROR, _T("Cannot load platform subagent \"%s\" (Subagent doesn't provide any usable parameters)"), subagentName);
         }
         else
         {
            nxlog_write(NXLOG_INFO, _T("Platform subagent \"%s\" successfully loaded"), subagentName);
         }
      }
      else
      {
         nxlog_write(NXLOG_ERROR, _T("Cannot load platform subagent \"%s\" (%s)"), subagentName, errorText);
      }
   }
#endif
}

/**
 * Get local ARP cache
 */
static ArpCache *SysGetLocalArpCache()
{
   ArpCache *arpCache = nullptr;

#ifdef _WIN32
   MIB_IPNETTABLE *sysArpCache;
   DWORD i, dwError, dwSize;

   sysArpCache = (MIB_IPNETTABLE *)MemAlloc(SIZEOF_IPNETTABLE(4096));
   if (sysArpCache == nullptr)
      return nullptr;

   dwSize = SIZEOF_IPNETTABLE(4096);
   dwError = GetIpNetTable(sysArpCache, &dwSize, FALSE);
   if (dwError != NO_ERROR)
   {
      TCHAR buffer[1024];
      nxlog_write(NXLOG_ERROR, _T("Call to GetIpNetTable() failed (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
      MemFree(sysArpCache);
      return nullptr;
   }

   arpCache = new ArpCache();

   for(i = 0; i < sysArpCache->dwNumEntries; i++)
      if ((sysArpCache->table[i].dwType == 3) ||
          (sysArpCache->table[i].dwType == 4))  // Only static and dynamic entries
      {
         arpCache->addEntry(ntohl(sysArpCache->table[i].dwAddr), MacAddress(sysArpCache->table[i].bPhysAddr, 6), sysArpCache->table[i].dwIndex);
      }

   MemFree(sysArpCache);
#else
	TCHAR szByte[4], *pChar;

	if (imp_NxSubAgentGetArpCache != nullptr)
	{
		StringList list;

      if (imp_NxSubAgentGetArpCache(&list))
      {
         arpCache = new ArpCache();

         szByte[2] = 0;

         // Parse data lines
         // Each line has form of XXXXXXXXXXXX a.b.c.d n
         // where XXXXXXXXXXXX is a MAC address (12 hexadecimal digits)
         // a.b.c.d is an IP address in decimal dotted notation
         // n is an interface index
         TCHAR temp[128];
         for (int i = 0; i < list.size(); i++)
         {
            _tcslcpy(temp, list.get(i), 128);
            TCHAR *pBuf = temp;
            if (_tcslen(pBuf) < 20)     // Invalid line
               continue;

            // MAC address
            BYTE macAddr[6];
            for (int j = 0; j < 6; j++)
            {
               memcpy(szByte, pBuf, sizeof(TCHAR) * 2);
               macAddr[j] = (BYTE)_tcstol(szByte, NULL, 16);
               pBuf += 2;
            }

            // IP address
            while (*pBuf == ' ')
               pBuf++;
            pChar = _tcschr(pBuf, _T(' '));
            if (pChar != NULL)
               *pChar = 0;
            InetAddress ipAddr = InetAddress::parse(pBuf);

            // Interface index
            UINT32 ifIndex = (pChar != NULL) ? _tcstoul(pChar + 1, NULL, 10) : 0;

            arpCache->addEntry(ipAddr, MacAddress(macAddr, 6), ifIndex);
         }
      }
   }
#endif

   return arpCache;
}

/**
 * Get local interface list (built-in system dependent code)
 */
static InterfaceList *SysGetLocalIfList()
{
   InterfaceList *pIfList = nullptr;

#ifdef _WIN32
   TCHAR szAdapterName[MAX_OBJECT_NAME];
   TCHAR szMacAddr[MAX_ADAPTER_ADDRESS_LENGTH * 2 + 1];
   IP_ADDR_STRING *pAddr;

   DWORD dwSize;
   if (GetAdaptersInfo(nullptr, &dwSize) != ERROR_BUFFER_OVERFLOW)
      return nullptr;

   IP_ADAPTER_INFO *pBuffer = (IP_ADAPTER_INFO *)MemAlloc(dwSize);
   if (GetAdaptersInfo(pBuffer, &dwSize) == ERROR_SUCCESS)
   {
      pIfList = new InterfaceList;

      for(IP_ADAPTER_INFO *pInfo = pBuffer; pInfo != nullptr; pInfo = pInfo->Next)
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
					wcslcpy(szAdapterName, wName, MAX_OBJECT_NAME);
            }
            else
            {
					MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInfo->AdapterName, -1, szAdapterName, MAX_OBJECT_NAME);
               szAdapterName[MAX_OBJECT_NAME - 1] = 0;
            }
         }
         else
         {
            // We don't have a GUID resolving function, use GUID as name
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pInfo->AdapterName, -1, szAdapterName, MAX_OBJECT_NAME);
            szAdapterName[MAX_OBJECT_NAME - 1] = 0;
         }

         BinToStr(pInfo->Address, pInfo->AddressLength, szMacAddr);

         // Compose result for each ip address
         for(pAddr = &pInfo->IpAddressList; pAddr != nullptr; pAddr = pAddr->Next)
         {
            InterfaceInfo *iface = pIfList->findByIfIndex(pInfo->Index);
            if (iface != nullptr)
            {
               iface->ipAddrList.add(InetAddress::parse(pAddr->IpAddress.String, pAddr->IpMask.String));
            }
            else
            {
               iface = new InterfaceInfo(pInfo->Index);
               _tcslcpy(iface->name, szAdapterName, MAX_OBJECT_NAME);
               memcpy(iface->macAddr, pInfo->Address, MAC_ADDR_LENGTH);
               iface->ipAddrList.add(InetAddress::parse(pAddr->IpAddress.String, pAddr->IpMask.String));
               iface->type = pInfo->Type;
				   pIfList->add(iface);
            }
         }
      }
   }

   MemFree(pBuffer);

#else
   TCHAR *pChar;

   if (imp_NxSubAgentGetIfList != nullptr)
   {
		StringList list;
      if (imp_NxSubAgentGetIfList(&list))
      {
         pIfList = new InterfaceList(list.size());
         TCHAR temp[1024];
         for(int i = 0; i < list.size(); i++)
         {
            _tcslcpy(temp, list.get(i), 1024);
            TCHAR *pBuf = temp;

            // Index
            uint32_t ifIndex = 0;
            pChar = _tcschr(pBuf, ' ');
            if (pChar != NULL)
            {
               *pChar = 0;
               ifIndex = _tcstoul(pBuf, nullptr, 10);
               pBuf = pChar + 1;
            }

            bool newInterface = false;
            InterfaceInfo *iface = pIfList->findByIfIndex(ifIndex);
            if (iface == nullptr)
            {
               iface = new InterfaceInfo(ifIndex);
               newInterface = true;
            }

            // Address and mask
            pChar = _tcschr(pBuf, _T(' '));
            if (pChar != nullptr)
            {
               TCHAR *pSlash;
               static TCHAR defaultMask[] = _T("24");

               *pChar = 0;
               pSlash = _tcschr(pBuf, _T('/'));
               if (pSlash != nullptr)
               {
                  *pSlash = 0;
                  pSlash++;
               }
               else     // Just a paranoia protection, should'n happen if subagent working correctly
               {
                  pSlash = defaultMask;
               }
               InetAddress addr = InetAddress::parse(pBuf);
               if (addr.isValid())
               {
                  addr.setMaskBits(_tcstol(pSlash, nullptr, 10));
                  // Agent may return 0.0.0.0/0 for interfaces without IP address
                  if ((addr.getFamily() != AF_INET) || (addr.getAddressV4() != 0))
                     iface->ipAddrList.add(addr);
               }
               pBuf = pChar + 1;
            }

            if (newInterface)
            {
               // Interface type
               pChar = _tcschr(pBuf, ' ');
               if (pChar != nullptr)
               {
                  *pChar = 0;

                  TCHAR *eptr;
                  iface->type = _tcstoul(pBuf, &eptr, 10);

                  // newer agents can return if_type(mtu)
                  if (*eptr == _T('('))
                  {
                     pBuf = eptr + 1;
                     eptr = _tcschr(pBuf, _T(')'));
                     if (eptr != nullptr)
                     {
                        *eptr = 0;
                        iface->mtu = _tcstol(pBuf, nullptr, 10);
                     }
                  }

                  pBuf = pChar + 1;
               }

               // MAC address
               pChar = _tcschr(pBuf, ' ');
               if (pChar != nullptr)
               {
                  *pChar = 0;
                  StrToBin(pBuf, iface->macAddr, MAC_ADDR_LENGTH);
                  pBuf = pChar + 1;
               }

               // Name (set description to name)
               _tcslcpy(iface->name, pBuf, MAX_DB_STRING);
               _tcslcpy(iface->description, pBuf, MAX_DB_STRING);

               pIfList->add(iface);
            }
         }
      }
   }
#endif

   return pIfList;
}

/**
 * Get local ARP cache
 */
shared_ptr<ArpCache> GetLocalArpCache()
{
   // Get ARP cache from built-in code or platform subagent
   ArpCache *arpCache = SysGetLocalArpCache();
   if (arpCache != nullptr)
      return shared_ptr<ArpCache>(arpCache);

   // Try to get ARP cache from agent via loopback address
   auto conn = make_shared<AgentConnection>(InetAddress::LOOPBACK);
   if (conn->connect(g_serverKey))
      return conn->getArpCache();

   return shared_ptr<ArpCache>();
}

/**
 * Get local interface list
 */
InterfaceList *GetLocalInterfaceList()
{
   // Get interface list from built-in code or platform subagent
   InterfaceList *ifList = SysGetLocalIfList();

   // Try to get local interface list from agent via loopback address
   if (ifList == nullptr)
   {
      auto conn = make_shared<AgentConnection>(InetAddress::LOOPBACK);
      if (conn->connect(g_serverKey))
      {
         ifList = conn->getInterfaceList();
      }
   }
   return ifList;
}
