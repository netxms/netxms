/*
** NetXMS platform subagent for Windows
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: net.cpp
**
**/

#include "winnt_subagent.h"
#include <lm.h>
#include <interface_types.h>
#include <functional>

#if (_WIN32_WINNT >= 0x0600)
#include <wbemcli.h>
#endif

/**
 * Check availability of remote share
 * Usage: Net.RemoteShareStatus(share,domain,login,password)
 *    or: Net.RemoteShareStatusText(share,domain,login,password)
 */
LONG H_RemoteShareStatus(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	TCHAR share[MAX_PATH], domain[64], login[64], password[256];
#ifdef UNICODE
#define wshare share
#else
	WCHAR wshare[MAX_PATH], wdomain[64], wlogin[64], wpassword[64];
#endif
	USE_INFO_2 ui;
	NET_API_STATUS status;

	memset(share, 0, MAX_PATH * sizeof(TCHAR));
	memset(domain, 0, 64 * sizeof(TCHAR));
	memset(login, 0, 64 * sizeof(TCHAR));
	memset(password, 0, 256 * sizeof(TCHAR));

	AgentGetParameterArg(cmd, 1, share, MAX_PATH);
	AgentGetParameterArg(cmd, 2, domain, 64);
	AgentGetParameterArg(cmd, 3, login, 64);
	AgentGetParameterArg(cmd, 4, password, 256);

	if ((*share == 0) || (*domain == 0) || (*login == 0) || (*password == 0))
		return SYSINFO_RC_UNSUPPORTED;

	memset(&ui, 0, sizeof(USE_INFO_2));
	ui.ui2_asg_type = USE_WILDCARD;
	//ui.ui2_local = NULL;
#ifdef UNICODE
	ui.ui2_remote = share;
	ui.ui2_domainname = domain;
	ui.ui2_username = login;
	ui.ui2_password = password;
#else
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, share, -1, wshare, MAX_PATH);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, domain, -1, wdomain, 64);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, login, -1, wlogin, 64);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, password, -1, wpassword, 256);
	ui.ui2_remote = wshare;
	ui.ui2_domainname = wdomain;
	ui.ui2_username = wlogin;
	ui.ui2_password = wpassword;
#endif
	if ((status = NetUseAdd(NULL, 2, (LPBYTE)&ui, NULL)) == NERR_Success)
	{
		NetUseDel(NULL, wshare, USE_FORCE);
	}

	if (*arg == 'C')
	{
		ret_int(value, status);	// Code
	}
	else
	{
		if (status == NERR_Success)
			ret_string(value, _T("OK"));
		else
			GetSystemErrorText(status, value, MAX_RESULT_LENGTH);
	}
	return SYSINFO_RC_SUCCESS;
}

/**
 * Get local ARP cache
 */
LONG H_ArpCache(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
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

/**
 * Get adapter index from name
 */
static DWORD AdapterNameToIndex(const TCHAR *name)
{
   const ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
   DWORD ifIndex = 0;
   ULONG size = 0;
   if (GetAdaptersAddresses(AF_UNSPEC, flags, NULL, NULL, &size) != ERROR_BUFFER_OVERFLOW)
      return SYSINFO_RC_ERROR;

   IP_ADAPTER_ADDRESSES *buffer = (IP_ADAPTER_ADDRESSES *)MemAlloc(size);
   if (GetAdaptersAddresses(AF_UNSPEC, flags, NULL, buffer, &size) == ERROR_SUCCESS)
   {
#ifdef UNICODE
      char mbname[256];
      WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, name, -1, mbname, 256, NULL, NULL);
#else
      WCHAR wname[256];
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name, -1, wname, 256);
#endif
      for(IP_ADAPTER_ADDRESSES *iface = buffer; iface != NULL; iface = iface->Next)
      {
#ifdef UNICODE
         if (!wcsicmp(iface->FriendlyName, name) || !stricmp(iface->AdapterName, mbname))
#else
         if (!wcsicmp(iface->FriendlyName, wname) || !stricmp(iface->AdapterName, name))
#endif
         {
            ifIndex = iface->IfIndex;
            break;
         }
      }
   }

   MemFree(buffer);
   return ifIndex;
}

/**
 * Net.InterfaceList list handler
 */
LONG H_InterfaceList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   LONG result = SYSINFO_RC_SUCCESS;

   const ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
   ULONG size = 0;
   if (GetAdaptersAddresses(AF_UNSPEC, flags, NULL, NULL, &size) != ERROR_BUFFER_OVERFLOW)
      return SYSINFO_RC_ERROR;

   IP_ADAPTER_ADDRESSES *buffer = (IP_ADAPTER_ADDRESSES *)MemAlloc(size);
   if (GetAdaptersAddresses(AF_UNSPEC, flags, NULL, buffer, &size) == ERROR_SUCCESS)
   {
      for(IP_ADAPTER_ADDRESSES *iface = buffer; iface != NULL; iface = iface->Next)
      {
         TCHAR macAddr[32], adapterInfo[MAX_ADAPTER_NAME_LENGTH + 128];

         if (iface->PhysicalAddressLength > 0)
            BinToStr(iface->PhysicalAddress, iface->PhysicalAddressLength, macAddr);
         else
            _tcscpy(macAddr, _T("000000000000"));

         // Compose result string for each IP address
         for(IP_ADAPTER_UNICAST_ADDRESS *pAddr = iface->FirstUnicastAddress; pAddr != NULL; pAddr = pAddr->Next)
         {
            TCHAR ipAddr[64];
            if ((pAddr->Address.lpSockaddr->sa_family == AF_INET) || session->isIPv6Aware())
            {
               InetAddress addr = InetAddress::createFromSockaddr(pAddr->Address.lpSockaddr);
#if (_WIN32_WINNT >= 0x0600)
               // OnLinkPrefixLength was added to IP_ADAPTER_UNICAST_ADDRESS in
               // Vista; on XP the address keeps its default (host) mask.
               addr.setMaskBits(pAddr->OnLinkPrefixLength);
#endif
               _sntprintf(adapterInfo, MAX_ADAPTER_NAME_LENGTH + 128, _T("%d %s/%d %d(%u) %s %s"), iface->IfIndex,
                          addr.toString(ipAddr), addr.getMaskBits(), iface->IfType,
                          iface->Mtu & 0x7FFFFFFF, macAddr, iface->FriendlyName);
               value->add(adapterInfo);
            }
         }
      }
   }
   else
   {
      result = SYSINFO_RC_ERROR;
   }

   MemFree(buffer);
   return result;
}

/**
 * Net.InterfaceNames list handler
 */
LONG H_InterfaceNames(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   LONG result = SYSINFO_RC_SUCCESS;

   const ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
   ULONG size = 0;
   if (GetAdaptersAddresses(AF_UNSPEC, flags, NULL, NULL, &size) != ERROR_BUFFER_OVERFLOW)
      return SYSINFO_RC_ERROR;

   IP_ADAPTER_ADDRESSES *buffer = (IP_ADAPTER_ADDRESSES *)MemAlloc(size);
   if (GetAdaptersAddresses(AF_UNSPEC, flags, NULL, buffer, &size) == ERROR_SUCCESS)
   {
      for(IP_ADAPTER_ADDRESSES *iface = buffer; iface != NULL; iface = iface->Next)
      {
         value->add(iface->FriendlyName);
      }
   }
   else
   {
      result = SYSINFO_RC_ERROR;
   }

   MemFree(buffer);
   return result;
}

#if (_WIN32_WINNT >= 0x0600)

/**
 * Read pairs of "Name" and "Team" properties from instances of NIC teaming (LBFO) WMI class
 */
static bool ReadLbfoObjects(IWbemServices *services, const WCHAR *query, std::function<void (const WCHAR*, const WCHAR*)> handler)
{
   BSTR language = SysAllocString(L"WQL");
   BSTR queryText = SysAllocString(query);
   IEnumWbemClassObject *enumObject = nullptr;
   HRESULT hr = services->ExecQuery(language, queryText, WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, nullptr, &enumObject);
   SysFreeString(language);
   SysFreeString(queryText);
   if (hr != S_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("ReadLbfoObjects: query \"%s\" failed (0x%08x)"), query, hr);
      return false;
   }

   IWbemClassObject *object;
   ULONG count;
   while((hr = enumObject->Next(WBEM_INFINITE, 1, &object, &count)) == S_OK)
   {
      VARIANT name, team;
      VariantInit(&name);
      VariantInit(&team);
      if ((object->Get(L"Name", 0, &name, nullptr, nullptr) == S_OK) && (object->Get(L"Team", 0, &team, nullptr, nullptr) == S_OK) &&
          (name.vt == VT_BSTR) && (team.vt == VT_BSTR) && (name.bstrVal != nullptr) && (team.bstrVal != nullptr))
      {
         handler(name.bstrVal, team.bstrVal);
      }
      VariantClear(&name);
      VariantClear(&team);
      object->Release();
   }
   enumObject->Release();

   if (hr != WBEM_S_FALSE)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("ReadLbfoObjects: cannot read results of query \"%s\" (0x%08x)"), query, hr);
      return false;
   }
   return true;
}

/**
 * Read NIC teaming (LBFO) configuration. On success, "aggregates" maps name of each team member adapter to name of
 * primary team interface, and "teamInterfaces" contains names of all primary team interfaces. Returns false if
 * teaming configuration cannot be read (for example, on Windows versions without LBFO support).
 */
static bool ReadLbfoConfiguration(StringMap *aggregates, StringSet *teamInterfaces)
{
   HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
   if ((hr != S_OK) && (hr != S_FALSE))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("ReadLbfoConfiguration: call to CoInitializeEx failed (0x%08x)"), hr);
      return false;
   }

   // CoInitializeSecurity can only succeed once per process; RPC_E_TOO_LATE means that security is already set
   // (same settings are used by WMI subagent)
   hr = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
   if ((hr != S_OK) && (hr != RPC_E_TOO_LATE))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("ReadLbfoConfiguration: call to CoInitializeSecurity failed (0x%08x)"), hr);
      CoUninitialize();
      return false;
   }

   bool success = false;
   IWbemLocator *locator;
   hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<void**>(&locator));
   if (hr == S_OK)
   {
      BSTR ns = SysAllocString(L"root\\StandardCimv2");
      IWbemServices *services;
      hr = locator->ConnectServer(ns, nullptr, nullptr, nullptr, 0, nullptr, nullptr, &services);
      SysFreeString(ns);
      if (hr == S_OK)
      {
         StringMap primaryInterfaces;  // team name -> primary team interface name
         success = ReadLbfoObjects(services, L"SELECT Name,Team FROM MSFT_NetLbfoTeamNic WHERE Primary=TRUE",
            [&primaryInterfaces, teamInterfaces] (const WCHAR *name, const WCHAR *team) -> void
            {
               primaryInterfaces.set(team, name);
               teamInterfaces->add(name);
            });
         if (success)
         {
            success = ReadLbfoObjects(services, L"SELECT Name,Team FROM MSFT_NetLbfoTeamMember",
               [&primaryInterfaces, aggregates] (const WCHAR *name, const WCHAR *team) -> void
               {
                  const TCHAR *teamInterface = primaryInterfaces.get(team);
                  if (teamInterface != nullptr)
                     aggregates->set(name, teamInterface);
               });
         }
         services->Release();
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("ReadLbfoConfiguration: cannot connect to WMI namespace root\\StandardCimv2 (0x%08x)"), hr);
      }
      locator->Release();
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("ReadLbfoConfiguration: call to CoCreateInstance(WbemLocator) failed (0x%08x)"), hr);
   }

   CoUninitialize();
   return success;
}

#endif   /* _WIN32_WINNT >= 0x0600 */

/**
 * Get interface index for adapter
 */
static uint32_t GetAdapterIndex(IP_ADAPTER_ADDRESSES *iface)
{
   if (iface->IfIndex != 0)
      return iface->IfIndex;
   if (iface->Ipv6IfIndex != 0)
      return iface->Ipv6IfIndex;
#if (_WIN32_WINNT >= 0x0600)
   // Adapter without IP protocols bound to it (member of NIC team, for example)
   NET_IFINDEX index;
   if (ConvertInterfaceLuidToIndex(&iface->Luid, &index) == NO_ERROR)
      return index;
#endif
   return 0;
}

/**
 * Handler for Net.Interfaces table
 */
LONG H_InterfaceTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
#if (_WIN32_WINNT >= 0x0600)
   // All NDIS interfaces are requested because adapters without IP protocols bound to them (which is the case
   // for members of NIC team) are not returned otherwise; unrelated interfaces are filtered out below
   const ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_ALL_INTERFACES;
#else
   const ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
#endif
   ULONG size = 0;
   if (GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, nullptr, &size) != ERROR_BUFFER_OVERFLOW)
      return SYSINFO_RC_ERROR;

   IP_ADAPTER_ADDRESSES *buffer = static_cast<IP_ADAPTER_ADDRESSES*>(MemAlloc(size));
   if (GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, buffer, &size) != ERROR_SUCCESS)
   {
      MemFree(buffer);
      return SYSINFO_RC_ERROR;
   }

   value->addColumn(_T("INDEX"), DCI_DT_UINT, _T("Index"), true);
   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"));
   value->addColumn(_T("ALIAS"), DCI_DT_STRING, _T("Alias"));
   value->addColumn(_T("TYPE"), DCI_DT_UINT, _T("Type"));
   value->addColumn(_T("MTU"), DCI_DT_UINT, _T("MTU"));
   value->addColumn(_T("MAC_ADDRESS"), DCI_DT_STRING, _T("MAC address"));
   value->addColumn(_T("IP_ADDRESSES"), DCI_DT_STRING, _T("IP addresses"));
   value->addColumn(_T("PEER_IP"), DCI_DT_STRING, _T("Peer IP addresses"));
   value->addColumn(_T("SPEED"), DCI_DT_UINT64, _T("Speed"));
   value->addColumn(_T("MAX_SPEED"), DCI_DT_UINT64, _T("Max speed"));

#if (_WIN32_WINNT >= 0x0600)
   // Column AGGREGATE is provided only if NIC teaming configuration can be read, so that server
   // can distinguish between "not a member of any team" and "membership is unknown"
   StringMap aggregates;
   StringSet teamInterfaces;
   bool teamingInfoAvailable = ReadLbfoConfiguration(&aggregates, &teamInterfaces);
   if (teamingInfoAvailable)
      value->addColumn(_T("AGGREGATE"), DCI_DT_UINT, _T("Aggregate interface index"));
#endif

   for(IP_ADAPTER_ADDRESSES *iface = buffer; iface != nullptr; iface = iface->Next)
   {
#if (_WIN32_WINNT >= 0x0600)
      // Skip interfaces without IP protocols bound to them (filter drivers, kernel debugger, etc.) unless it is a NIC team member
      const TCHAR *aggregateName = aggregates.get(iface->FriendlyName);
      if (!iface->Ipv4Enabled && !iface->Ipv6Enabled && (aggregateName == nullptr))
         continue;
#endif

      uint32_t ifIndex = GetAdapterIndex(iface);
      if (ifIndex == 0)
         continue;

      value->addRow();
      value->set(0, ifIndex);
      value->set(1, iface->FriendlyName);
      value->set(4, static_cast<uint32_t>(iface->Mtu & 0x7FFFFFFF));

      TCHAR macAddr[32];
      if (iface->PhysicalAddressLength > 0)
         value->set(5, BinToStr(iface->PhysicalAddress, iface->PhysicalAddressLength, macAddr));
      else
         value->set(5, _T("000000000000"));

      StringBuffer addrList;
      for(IP_ADAPTER_UNICAST_ADDRESS *a = iface->FirstUnicastAddress; a != nullptr; a = a->Next)
      {
         InetAddress addr = InetAddress::createFromSockaddr(a->Address.lpSockaddr);
#if (_WIN32_WINNT >= 0x0600)
         // OnLinkPrefixLength was added to IP_ADAPTER_UNICAST_ADDRESS in
         // Vista; on XP the address keeps its default (host) mask.
         addr.setMaskBits(a->OnLinkPrefixLength);
#endif
         if (!addrList.isEmpty())
            addrList.append(_T(", "));
         addrList.append(addr.toString());
         addrList.append(_T('/'));
         addrList.append(addr.getMaskBits());
      }
      value->set(6, addrList);

#if (_WIN32_WINNT >= 0x0600)
      value->set(3, static_cast<uint32_t>(teamInterfaces.contains(iface->FriendlyName) ? IFTYPE_IEEE8023ADLAG : iface->IfType));

      // Link speed is reported as all ones when it is unknown (no link, for example)
      if (iface->TransmitLinkSpeed != static_cast<ULONG64>(-1))
         value->set(8, static_cast<uint64_t>(iface->TransmitLinkSpeed));

      if (teamingInfoAvailable)
      {
         uint32_t aggregateIndex = 0;
         if (aggregateName != nullptr)
         {
            for(IP_ADAPTER_ADDRESSES *a = buffer; a != nullptr; a = a->Next)
            {
               if (!_tcscmp(a->FriendlyName, aggregateName))
               {
                  aggregateIndex = GetAdapterIndex(a);
                  break;
               }
            }
         }
         value->set(10, aggregateIndex);
      }
#else
      value->set(3, static_cast<uint32_t>(iface->IfType));
#endif
   }

   MemFree(buffer);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Get IP statistics
 */
LONG H_NetIPStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
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

/**
 * Handler for Net.IP.Stats.TCPConnections and Net.IP.Stats.TCPConnections(*)
 */
/**
 * Parse TCP state name to Windows MIB_TCP_STATE code
 */
static int TCPStateFromName(const TCHAR *name)
{
   static struct { const TCHAR *name; DWORD code; } stateMap[] =
   {
      { _T("ESTABLISHED"), MIB_TCP_STATE_ESTAB },
      { _T("SYN_SENT"), MIB_TCP_STATE_SYN_SENT },
      { _T("SYN_RECV"), MIB_TCP_STATE_SYN_RCVD },
      { _T("FIN_WAIT1"), MIB_TCP_STATE_FIN_WAIT1 },
      { _T("FIN_WAIT2"), MIB_TCP_STATE_FIN_WAIT2 },
      { _T("TIME_WAIT"), MIB_TCP_STATE_TIME_WAIT },
      { _T("CLOSE"), MIB_TCP_STATE_CLOSED },
      { _T("CLOSE_WAIT"), MIB_TCP_STATE_CLOSE_WAIT },
      { _T("LAST_ACK"), MIB_TCP_STATE_LAST_ACK },
      { _T("LISTEN"), MIB_TCP_STATE_LISTEN },
      { _T("CLOSING"), MIB_TCP_STATE_CLOSING },
      { nullptr, 0 }
   };

   for (int i = 0; stateMap[i].name != nullptr; i++)
   {
      if (!_tcsicmp(name, stateMap[i].name))
         return static_cast<int>(stateMap[i].code);
   }
   return -1;
}

/**
 * Handler for Net.IP.Stats.TCPConnections and Net.IP.Stats.TCPConnections(*)
 */
LONG H_NetTCPConnections(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int targetState = -1;
   int ipVersion = 0; // 0 = both, 4 = IPv4 only, 6 = IPv6 only

   if (arg[0] == _T('O'))
   {
      OptionList options(cmd);
      if (!options.isValid())
         return SYSINFO_RC_UNSUPPORTED;

      const TCHAR *state = options.get(_T("state"));
      if (state != nullptr)
      {
         targetState = TCPStateFromName(state);
         if (targetState == -1)
            return SYSINFO_RC_UNSUPPORTED;
      }

      const TCHAR *version = options.get(_T("version"));
      if (version != nullptr)
      {
         ipVersion = _tcstol(version, nullptr, 10);
         if (ipVersion != 4 && ipVersion != 6)
            return SYSINFO_RC_UNSUPPORTED;
      }
   }

   int count = 0;

   // Count IPv4 TCP connections
   if (ipVersion != 6)
   {
      DWORD size = 0;
      if (GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET, TCP_TABLE_BASIC_ALL, 0) == ERROR_INSUFFICIENT_BUFFER)
      {
         MIB_TCPTABLE *tcpTable = static_cast<MIB_TCPTABLE*>(MemAlloc(size));
         if (GetExtendedTcpTable(tcpTable, &size, FALSE, AF_INET, TCP_TABLE_BASIC_ALL, 0) == NO_ERROR)
         {
            for (DWORD i = 0; i < tcpTable->dwNumEntries; i++)
            {
               if (targetState == -1 || static_cast<int>(tcpTable->table[i].dwState) == targetState)
                  count++;
            }
         }
         MemFree(tcpTable);
      }
   }

#if (_WIN32_WINNT >= 0x0600)
   // Count IPv6 TCP connections. The IPv6 TCP table (MIB_TCP6TABLE, and
   // GetExtendedTcpTable for AF_INET6) is Vista+, so IPv6 connections are not
   // counted on Windows XP.
   if (ipVersion != 4)
   {
      DWORD size = 0;
      if (GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET6, TCP_TABLE_BASIC_ALL, 0) == ERROR_INSUFFICIENT_BUFFER)
      {
         MIB_TCP6TABLE *tcp6Table = static_cast<MIB_TCP6TABLE*>(MemAlloc(size));
         if (GetExtendedTcpTable(tcp6Table, &size, FALSE, AF_INET6, TCP_TABLE_BASIC_ALL, 0) == NO_ERROR)
         {
            for (DWORD i = 0; i < tcp6Table->dwNumEntries; i++)
            {
               if (targetState == -1 || static_cast<int>(tcp6Table->table[i].State) == targetState)
                  count++;
            }
         }
         MemFree(tcp6Table);
      }
   }
#endif

   ret_int(value, count);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Get interface statistics
 */
LONG H_NetInterfaceStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
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
			int metric = CAST_FROM_POINTER(arg, int);
#if (_WIN32_WINNT >= 0x0600)
			if ((metric == NETINFO_IF_SPEED) ||
			    (metric == NETINFO_IF_PACKETS_IN_64) ||
			    (metric == NETINFO_IF_PACKETS_OUT_64) ||
			    (metric == NETINFO_IF_BYTES_IN_64) ||
				 (metric == NETINFO_IF_BYTES_OUT_64) ||
				 (metric == NETINFO_IF_IN_DROPS_64) ||
				 (metric == NETINFO_IF_OUT_DROPS_64))
			{
				MIB_IF_ROW2 info;

				memset(&info, 0, sizeof(MIB_IF_ROW2));
				info.InterfaceIndex = dwIndex;
				if (GetIfEntry2(&info) == NO_ERROR)
				{
					switch(metric)
					{
						case NETINFO_IF_BYTES_IN_64:
							ret_uint64(value, info.InOctets);
							break;
						case NETINFO_IF_BYTES_OUT_64:
							ret_uint64(value, info.OutOctets);
							break;
						case NETINFO_IF_PACKETS_IN_64:
							ret_uint64(value, info.InUcastPkts + info.InNUcastPkts);
							break;
						case NETINFO_IF_PACKETS_OUT_64:
							ret_uint64(value, info.OutUcastPkts + info.OutNUcastPkts);
							break;
						case NETINFO_IF_SPEED:
							ret_uint64(value, info.TransmitLinkSpeed);
							break;
						case NETINFO_IF_IN_DROPS_64:
							ret_uint64(value, info.InDiscards);
							break;
						case NETINFO_IF_OUT_DROPS_64:
							ret_uint64(value, info.OutDiscards);
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
#else
			// Windows XP lacks GetIfEntry2/MIB_IF_ROW2 (native 64-bit interface
			// counters), so the *_64 metrics are reported as unsupported.
			// NETINFO_IF_SPEED still works via the 32-bit MIB_IFROW handler below.
			if ((metric == NETINFO_IF_PACKETS_IN_64) ||
			    (metric == NETINFO_IF_PACKETS_OUT_64) ||
			    (metric == NETINFO_IF_BYTES_IN_64) ||
			    (metric == NETINFO_IF_BYTES_OUT_64) ||
			    (metric == NETINFO_IF_IN_DROPS_64) ||
			    (metric == NETINFO_IF_OUT_DROPS_64))
			{
				iResult = SYSINFO_RC_UNSUPPORTED;
			}
#endif
			else
			{
				MIB_IFROW info;

				info.dwIndex = dwIndex;
				if (GetIfEntry(&info) == NO_ERROR)
				{
					switch(metric)
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
						case NETINFO_IF_IN_DROPS:
							ret_uint(value, info.dwInDiscards);
							break;
						case NETINFO_IF_OUT_DROPS:
							ret_uint(value, info.dwOutDiscards);
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
      }
      else
      {
         iResult = SYSINFO_RC_UNSUPPORTED;
      }
   }

   return iResult;
}

/**
 * Get IP routing table
 */
LONG H_IPRoutingTable(const TCHAR *pszCmd, const TCHAR *pArg, StringList *value, AbstractCommSession *session)
{
   // Determine required buffer size
   DWORD size = 0;
   DWORD status = GetIpForwardTable(nullptr, &size, FALSE);
   if ((status != NO_ERROR) && (status != ERROR_INSUFFICIENT_BUFFER))
      return SYSINFO_RC_ERROR;

   MIB_IPFORWARDTABLE *routingTable = (MIB_IPFORWARDTABLE *)MemAlloc(size);
   if (routingTable == nullptr)
      return SYSINFO_RC_ERROR;

   status = GetIpForwardTable(routingTable, &size, FALSE);
   if (status != NO_ERROR)
   {
      MemFree(routingTable);
      return SYSINFO_RC_ERROR;
   }

   TCHAR buffer[256], destAddr[16], nextHop[16];
   for(DWORD i = 0; i < routingTable->dwNumEntries; i++)
   {
      // Report Windows specific protocols as "local"
      uint32_t proto = routingTable->table[i].dwForwardProto;
      if ((proto == MIB_IPPROTO_NT_AUTOSTATIC) || (proto == MIB_IPPROTO_NT_STATIC) || (proto == MIB_IPPROTO_NT_STATIC_NON_DOD))
         proto = MIB_IPPROTO_LOCAL;

      _sntprintf(buffer, 256, _T("%s/%d %s %u %u %u %u"),
                 IpToStr(ntohl(routingTable->table[i].dwForwardDest), destAddr),
                 BitsInMask(ntohl(routingTable->table[i].dwForwardMask)),
                 IpToStr(ntohl(routingTable->table[i].dwForwardNextHop), nextHop),
                 routingTable->table[i].dwForwardIfIndex,
                 routingTable->table[i].dwForwardType,
                 routingTable->table[i].dwForwardMetric1,
                 proto);
		value->add(buffer);
   }

   MemFree(routingTable);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Support for 64 bit interface counters
 */
LONG H_NetInterface64bitSupport(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   ret_int(value, 1);
   return SYSINFO_RC_SUCCESS;
}
