/*
** NetXMS platform subagent for Windows
** Copyright (C) 2026 Raden Solutions
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
** File: dhcp.cpp
**
** Generic DHCP.* scope and lease information for Microsoft DHCP Server (IPv4 scopes).
** dhcpsapi.dll is loaded on first use, so the subagent has no link-time dependency on it
** and handlers report "unsupported" on hosts without DHCP Server role.
**/

#include "winnt_subagent.h"
#include <nxcldefs.h>

/**
 * DHCP Server Management API definitions (from Microsoft documentation for dhcpsapi.h).
 * Defined locally because MinGW headers lack most of them; prefixed to avoid clashes with SDK headers.
 */
typedef DWORD NXDHCP_IP_ADDRESS;
typedef DWORD NXDHCP_IP_MASK;
typedef DWORD NXDHCP_RESUME_HANDLE;

struct NXDHCP_BINARY_DATA
{
   DWORD DataLength;
   BYTE *Data;
};

struct NXDHCP_IP_ARRAY
{
   DWORD NumElements;
   NXDHCP_IP_ADDRESS *Elements;
};

struct NXDHCP_HOST_INFO
{
   NXDHCP_IP_ADDRESS IpAddress;
   LPWSTR NetBiosName;
   LPWSTR HostName;
};

enum NXDHCP_SUBNET_STATE
{
   NXDhcpSubnetEnabled = 0,
   NXDhcpSubnetDisabled = 1,
   NXDhcpSubnetEnabledSwitched = 2,
   NXDhcpSubnetDisabledSwitched = 3,
   NXDhcpSubnetInvalidState = 4
};

struct NXDHCP_SUBNET_INFO
{
   NXDHCP_IP_ADDRESS SubnetAddress;
   NXDHCP_IP_MASK SubnetMask;
   LPWSTR SubnetName;
   LPWSTR SubnetComment;
   NXDHCP_HOST_INFO PrimaryHost;
   NXDHCP_SUBNET_STATE SubnetState;
};

struct NXDHCP_IP_RANGE
{
   NXDHCP_IP_ADDRESS StartAddress;
   NXDHCP_IP_ADDRESS EndAddress;
};

struct NXDHCP_IP_RESERVATION
{
   NXDHCP_IP_ADDRESS ReservedIpAddress;
   NXDHCP_BINARY_DATA *ReservedForClient;
};

enum NXDHCP_SUBNET_ELEMENT_TYPE
{
   NXDhcpIpRanges = 0,
   NXDhcpSecondaryHosts = 1,
   NXDhcpReservedIps = 2,
   NXDhcpExcludedIpRanges = 3,
   NXDhcpIpUsedClusters = 4
};

struct NXDHCP_SUBNET_ELEMENT_DATA
{
   NXDHCP_SUBNET_ELEMENT_TYPE ElementType;
   union
   {
      NXDHCP_IP_RANGE *IpRange;
      NXDHCP_HOST_INFO *SecondaryHost;
      NXDHCP_IP_RESERVATION *ReservedIp;
      NXDHCP_IP_RANGE *ExcludeIpRange;
      void *IpUsedCluster;
   } Element;
};

struct NXDHCP_SUBNET_ELEMENT_INFO_ARRAY
{
   DWORD NumElements;
   NXDHCP_SUBNET_ELEMENT_DATA *Elements;
};

struct NXDHCP_DATE_TIME
{
   DWORD dwLowDateTime;
   DWORD dwHighDateTime;
};

#define NXDHCP_DATE_TIME_INFINITE_HIGH 0x7FFFFFFF
#define NXDHCP_DATE_TIME_INFINITE_LOW  0xFFFFFFFF

#define NX_V5_ADDRESS_STATE_OFFERED    0x00
#define NX_V5_ADDRESS_STATE_ACTIVE     0x01
#define NX_V5_ADDRESS_STATE_DECLINED   0x02
#define NX_V5_ADDRESS_STATE_DOOM       0x03

struct NXDHCP_CLIENT_INFO_V5
{
   NXDHCP_IP_ADDRESS ClientIpAddress;
   NXDHCP_IP_MASK SubnetMask;
   NXDHCP_BINARY_DATA ClientHardwareAddress;
   LPWSTR ClientName;
   LPWSTR ClientComment;
   NXDHCP_DATE_TIME ClientLeaseExpires;
   NXDHCP_HOST_INFO OwnerHost;
   BYTE bClientType;
   BYTE AddressState;
};

struct NXDHCP_CLIENT_INFO_ARRAY_V5
{
   DWORD NumElements;
   NXDHCP_CLIENT_INFO_V5 **Clients;
};

struct NXSCOPE_MIB_INFO
{
   NXDHCP_IP_ADDRESS Subnet;
   DWORD NumAddressesInuse;
   DWORD NumAddressesFree;
   DWORD NumPendingOffers;
};

struct NXDHCP_MIB_INFO
{
   DWORD Discovers;
   DWORD Offers;
   DWORD Requests;
   DWORD Acks;
   DWORD Naks;
   DWORD Declines;
   DWORD Releases;
   NXDHCP_DATE_TIME ServerStartTime;
   DWORD Scopes;
   NXSCOPE_MIB_INFO *ScopeInfo;
};

#define DEBUG_TAG_DHCP  _T("winnt.dhcp")

/**
 * Preferred maximum response size for enumeration calls
 */
#define DHCP_ENUM_PREFERRED_MAX  65536

/**
 * dhcpsapi.dll entry points
 */
typedef DWORD (WINAPI *DhcpEnumSubnets_t)(const WCHAR*, NXDHCP_RESUME_HANDLE*, DWORD, NXDHCP_IP_ARRAY**, DWORD*, DWORD*);
typedef DWORD (WINAPI *DhcpGetSubnetInfo_t)(const WCHAR*, NXDHCP_IP_ADDRESS, NXDHCP_SUBNET_INFO**);
typedef DWORD (WINAPI *DhcpEnumSubnetElements_t)(const WCHAR*, NXDHCP_IP_ADDRESS, NXDHCP_SUBNET_ELEMENT_TYPE, NXDHCP_RESUME_HANDLE*, DWORD, NXDHCP_SUBNET_ELEMENT_INFO_ARRAY**, DWORD*, DWORD*);
typedef DWORD (WINAPI *DhcpEnumSubnetClientsV5_t)(const WCHAR*, NXDHCP_IP_ADDRESS, NXDHCP_RESUME_HANDLE*, DWORD, NXDHCP_CLIENT_INFO_ARRAY_V5**, DWORD*, DWORD*);
typedef DWORD (WINAPI *DhcpGetMibInfo_t)(const WCHAR*, NXDHCP_MIB_INFO**);
typedef VOID (WINAPI *DhcpRpcFreeMemory_t)(PVOID);

static DhcpEnumSubnets_t s_DhcpEnumSubnets = nullptr;
static DhcpGetSubnetInfo_t s_DhcpGetSubnetInfo = nullptr;
static DhcpEnumSubnetElements_t s_DhcpEnumSubnetElements = nullptr;
static DhcpEnumSubnetClientsV5_t s_DhcpEnumSubnetClientsV5 = nullptr;
static DhcpGetMibInfo_t s_DhcpGetMibInfo = nullptr;
static DhcpRpcFreeMemory_t s_DhcpRpcFreeMemory = nullptr;
static bool s_apiLoaded = false;
static bool s_apiLoadAttempted = false;
static Mutex s_apiLock;

/**
 * DHCP server address (local server by default)
 */
static WCHAR s_serverAddress[256] = L"127.0.0.1";

/**
 * Cache time for scope information (seconds)
 */
static const time_t s_cacheTime = 5;

/**
 * Load dhcpsapi.dll and resolve entry points
 */
static bool LoadDhcpApi()
{
   LockGuard lockGuard(s_apiLock);
   if (s_apiLoadAttempted)
      return s_apiLoaded;
   s_apiLoadAttempted = true;

   HMODULE module = LoadLibraryW(L"dhcpsapi.dll");
   if (module == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DHCP, 4, _T("Cannot load dhcpsapi.dll (error %u)"), GetLastError());
      return false;
   }

   s_DhcpEnumSubnets = reinterpret_cast<DhcpEnumSubnets_t>(reinterpret_cast<void*>(GetProcAddress(module, "DhcpEnumSubnets")));
   s_DhcpGetSubnetInfo = reinterpret_cast<DhcpGetSubnetInfo_t>(reinterpret_cast<void*>(GetProcAddress(module, "DhcpGetSubnetInfo")));
   s_DhcpEnumSubnetElements = reinterpret_cast<DhcpEnumSubnetElements_t>(reinterpret_cast<void*>(GetProcAddress(module, "DhcpEnumSubnetElements")));
   s_DhcpEnumSubnetClientsV5 = reinterpret_cast<DhcpEnumSubnetClientsV5_t>(reinterpret_cast<void*>(GetProcAddress(module, "DhcpEnumSubnetClientsV5")));
   s_DhcpGetMibInfo = reinterpret_cast<DhcpGetMibInfo_t>(reinterpret_cast<void*>(GetProcAddress(module, "DhcpGetMibInfo")));
   s_DhcpRpcFreeMemory = reinterpret_cast<DhcpRpcFreeMemory_t>(reinterpret_cast<void*>(GetProcAddress(module, "DhcpRpcFreeMemory")));
   if ((s_DhcpEnumSubnets == nullptr) || (s_DhcpGetSubnetInfo == nullptr) || (s_DhcpEnumSubnetElements == nullptr) ||
       (s_DhcpEnumSubnetClientsV5 == nullptr) || (s_DhcpGetMibInfo == nullptr) || (s_DhcpRpcFreeMemory == nullptr))
   {
      nxlog_debug_tag(DEBUG_TAG_DHCP, 4, _T("Cannot resolve DHCP server management API entry points in dhcpsapi.dll"));
      FreeLibrary(module);
      return false;
   }

   nxlog_debug_tag(DEBUG_TAG_DHCP, 4, _T("DHCP server management API loaded"));
   s_apiLoaded = true;
   return true;
}

/**
 * Check if DHCP Server service is installed and running. Returns SYSINFO_RC_SUCCESS if it is,
 * SYSINFO_RC_UNSUPPORTED if service is not installed, and SYSINFO_RC_ERROR if it is not running.
 */
static LONG CheckDhcpService()
{
   SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
   if (scm == nullptr)
      return SYSINFO_RC_ERROR;

   LONG rc;
   SC_HANDLE service = OpenService(scm, _T("DHCPServer"), SERVICE_QUERY_STATUS);
   if (service != nullptr)
   {
      SERVICE_STATUS status;
      rc = (QueryServiceStatus(service, &status) && (status.dwCurrentState == SERVICE_RUNNING)) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
      CloseServiceHandle(service);
   }
   else
   {
      rc = (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) ? SYSINFO_RC_UNSUPPORTED : SYSINFO_RC_ERROR;
   }
   CloseServiceHandle(scm);
   return rc;
}

/**
 * Check that DHCP server management is available
 */
static LONG CheckDhcpAvailable()
{
   LONG rc = CheckDhcpService();
   if (rc != SYSINFO_RC_SUCCESS)
      return rc;
   return LoadDhcpApi() ? SYSINFO_RC_SUCCESS : SYSINFO_RC_UNSUPPORTED;
}

/**
 * Format IPv4 address (DHCP API uses host byte order)
 */
static inline TCHAR *FormatAddress(NXDHCP_IP_ADDRESS addr, TCHAR *buffer)
{
   return IpToStr(addr, buffer);
}

/**
 * Build scope ID (network/bits)
 */
static String ScopeId(NXDHCP_IP_ADDRESS subnet, NXDHCP_IP_MASK mask)
{
   TCHAR buffer[64];
   StringBuffer id(FormatAddress(subnet, buffer));
   id.append(_T('/'));
   id.append(BitsInMask(mask));
   return id;
}

/**
 * Convert DHCP client UID to MAC address string. Reservation UIDs may be prefixed with
 * 4 bytes of subnet address and 1 byte of hardware type.
 */
static String ClientUidToMacAddress(const NXDHCP_BINARY_DATA *uid)
{
   if ((uid == nullptr) || (uid->Data == nullptr))
      return String();

   const BYTE *data = uid->Data;
   DWORD length = uid->DataLength;
   if (length == 11)
   {
      data += 5;
      length = 6;
   }
   if (length != 6)
      return String();
   return MacAddress(data, length).toString(MacAddressNotation::COLON_SEPARATED);
}

/**
 * Convert NXDHCP_DATE_TIME to UNIX time. Returns 0 for "infinite" and "not set".
 */
static time_t DateTimeToUnixTime(const NXDHCP_DATE_TIME& dt)
{
   if (((dt.dwHighDateTime == 0) && (dt.dwLowDateTime == 0)) ||
       ((dt.dwHighDateTime == NXDHCP_DATE_TIME_INFINITE_HIGH) && (dt.dwLowDateTime == NXDHCP_DATE_TIME_INFINITE_LOW)))
      return 0;
   uint64_t ft = (static_cast<uint64_t>(dt.dwHighDateTime) << 32) | dt.dwLowDateTime;
   return (ft > EPOCHFILETIME) ? static_cast<time_t>((ft - EPOCHFILETIME) / 10000000) : 0;
}

/**
 * Reservation information
 */
struct DhcpReservation
{
   NXDHCP_IP_ADDRESS address;
   String macAddress;

   DhcpReservation(NXDHCP_IP_ADDRESS a, const String& mac) : macAddress(mac)
   {
      address = a;
   }
};

/**
 * Scope information
 */
struct DhcpScope
{
   String id;
   NXDHCP_IP_ADDRESS subnet;
   NXDHCP_IP_MASK mask;
   String name;
   MutableString ranges;
   bool enabled;
   uint64_t total;
   uint64_t used;
   ObjectArray<DhcpReservation> reservations;

   DhcpScope(const String& _id, NXDHCP_IP_ADDRESS _subnet, NXDHCP_IP_MASK _mask, const TCHAR *_name, bool _enabled) :
         id(_id), name(_name), reservations(0, 16, Ownership::True)
   {
      subnet = _subnet;
      mask = _mask;
      enabled = _enabled;
      total = 0;
      used = 0;
   }
};

/**
 * Cached scope list
 */
static shared_ptr<ObjectArray<DhcpScope>> s_scopes;
static time_t s_scopesTimestamp = 0;
static Mutex s_scopesLock;

/**
 * Read address ranges and reservations of given subnet
 */
static void ReadSubnetElements(DhcpScope *scope)
{
   static const NXDHCP_SUBNET_ELEMENT_TYPE types[] = { NXDhcpIpRanges, NXDhcpReservedIps };
   StringBuffer ranges;
   for(int t = 0; t < 2; t++)
   {
      NXDHCP_RESUME_HANDLE resumeHandle = 0;
      DWORD rc;
      do
      {
         NXDHCP_SUBNET_ELEMENT_INFO_ARRAY* elements = nullptr;
         DWORD read = 0, total = 0;
         rc = s_DhcpEnumSubnetElements(s_serverAddress, scope->subnet, types[t], &resumeHandle, DHCP_ENUM_PREFERRED_MAX, &elements, &read, &total);
         if (((rc != ERROR_SUCCESS) && (rc != ERROR_MORE_DATA)) || (elements == nullptr))
         {
            if ((rc != ERROR_SUCCESS) && (rc != ERROR_MORE_DATA) && (rc != ERROR_NO_MORE_ITEMS))
               nxlog_debug_tag(DEBUG_TAG_DHCP, 5, _T("DhcpEnumSubnetElements(%s, %d) failed (error %u)"), scope->id.cstr(), types[t], rc);
            break;
         }

         for(DWORD i = 0; i < elements->NumElements; i++)
         {
            const NXDHCP_SUBNET_ELEMENT_DATA& e = elements->Elements[i];
            if ((e.ElementType == NXDhcpIpRanges) && (e.Element.IpRange != nullptr))
            {
               TCHAR start[32], end[32];
               if (!ranges.isEmpty())
                  ranges.append(_T(", "));
               ranges.append(FormatAddress(e.Element.IpRange->StartAddress, start));
               ranges.append(_T('-'));
               ranges.append(FormatAddress(e.Element.IpRange->EndAddress, end));
            }
            else if ((e.ElementType == NXDhcpReservedIps) && (e.Element.ReservedIp != nullptr))
            {
               scope->reservations.add(new DhcpReservation(e.Element.ReservedIp->ReservedIpAddress, ClientUidToMacAddress(e.Element.ReservedIp->ReservedForClient)));
            }
         }
         s_DhcpRpcFreeMemory(elements);
      } while(rc == ERROR_MORE_DATA);
   }
   scope->ranges = ranges;
}

/**
 * Read scope list from DHCP server
 */
static shared_ptr<ObjectArray<DhcpScope>> ReadScopes()
{
   auto scopes = make_shared<ObjectArray<DhcpScope>>(0, 32, Ownership::True);

   NXDHCP_RESUME_HANDLE resumeHandle = 0;
   DWORD rc;
   do
   {
      NXDHCP_IP_ARRAY* subnets = nullptr;
      DWORD read = 0, total = 0;
      rc = s_DhcpEnumSubnets(s_serverAddress, &resumeHandle, DHCP_ENUM_PREFERRED_MAX, &subnets, &read, &total);
      if ((rc != ERROR_SUCCESS) && (rc != ERROR_MORE_DATA))
      {
         if (rc == ERROR_NO_MORE_ITEMS)
            break;
         nxlog_debug_tag(DEBUG_TAG_DHCP, 4, _T("DhcpEnumSubnets failed (error %u)"), rc);
         return shared_ptr<ObjectArray<DhcpScope>>();
      }
      if (subnets == nullptr)
         break;

      for(DWORD i = 0; i < subnets->NumElements; i++)
      {
         NXDHCP_SUBNET_INFO* info = nullptr;
         DWORD rcInfo = s_DhcpGetSubnetInfo(s_serverAddress, subnets->Elements[i], &info);
         if ((rcInfo != ERROR_SUCCESS) || (info == nullptr))
         {
            nxlog_debug_tag(DEBUG_TAG_DHCP, 5, _T("DhcpGetSubnetInfo failed (error %u)"), rcInfo);
            continue;
         }
         bool enabled = (info->SubnetState == NXDhcpSubnetEnabled) || (info->SubnetState == NXDhcpSubnetEnabledSwitched);
         auto scope = new DhcpScope(ScopeId(info->SubnetAddress, info->SubnetMask), info->SubnetAddress, info->SubnetMask,
               CHECK_NULL_EX(info->SubnetName), enabled);
         s_DhcpRpcFreeMemory(info);
         ReadSubnetElements(scope);
         scopes->add(scope);
      }
      s_DhcpRpcFreeMemory(subnets);
   } while(rc == ERROR_MORE_DATA);

   // Address usage counters
   NXDHCP_MIB_INFO* mib = nullptr;
   rc = s_DhcpGetMibInfo(s_serverAddress, &mib);
   if ((rc == ERROR_SUCCESS) && (mib != nullptr))
   {
      for(DWORD i = 0; i < mib->Scopes; i++)
      {
         const NXSCOPE_MIB_INFO& s = mib->ScopeInfo[i];
         for(int j = 0; j < scopes->size(); j++)
         {
            DhcpScope *scope = scopes->get(j);
            if (scope->subnet == s.Subnet)
            {
               scope->used = s.NumAddressesInuse;
               scope->total = static_cast<uint64_t>(s.NumAddressesInuse) + s.NumAddressesFree + s.NumPendingOffers;
               break;
            }
         }
      }
      s_DhcpRpcFreeMemory(mib);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DHCP, 5, _T("DhcpGetMibInfo failed (error %u)"), rc);
   }
   return scopes;
}

/**
 * Get scope list (from cache if still valid)
 */
static shared_ptr<ObjectArray<DhcpScope>> GetScopes()
{
   LockGuard lockGuard(s_scopesLock);
   time_t now = time(nullptr);
   if ((s_scopes != nullptr) && (now - s_scopesTimestamp < s_cacheTime))
      return s_scopes;
   s_scopes = ReadScopes();
   s_scopesTimestamp = now;
   return s_scopes;
}

/**
 * Find scope by ID
 */
static DhcpScope *FindScope(ObjectArray<DhcpScope> *scopes, const TCHAR *id)
{
   for(int i = 0; i < scopes->size(); i++)
   {
      DhcpScope *s = scopes->get(i);
      if (!_tcsicmp(s->id, id))
         return s;
   }
   return nullptr;
}

/**
 * Handler for DHCP.Scope.* metrics
 */
LONG H_DhcpScopeMetric(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   LONG rc = CheckDhcpAvailable();
   if (rc != SYSINFO_RC_SUCCESS)
      return rc;

   TCHAR id[256];
   if (!AgentGetParameterArg(metric, 1, id, 256))
      return SYSINFO_RC_UNSUPPORTED;

   shared_ptr<ObjectArray<DhcpScope>> scopes = GetScopes();
   if (scopes == nullptr)
      return SYSINFO_RC_ERROR;

   DhcpScope *scope = FindScope(scopes.get(), id);
   if (scope == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   switch(*arg)
   {
      case 'D':
         ret_uint64(value, 0);
         break;
      case 'F':
         ret_uint64(value, DHCPScopeFreeAddresses(scope->total, scope->used, 0));
         break;
      case 'T':
         ret_uint64(value, scope->total);
         break;
      case 'U':
         ret_uint64(value, scope->used);
         break;
      case 'P':
         ret_double(value, DHCPScopeUtilization(scope->total, scope->used), 2);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for DHCP.Scopes list
 */
LONG H_DhcpScopeList(const TCHAR *metric, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   LONG rc = CheckDhcpAvailable();
   if (rc != SYSINFO_RC_SUCCESS)
      return rc;

   shared_ptr<ObjectArray<DhcpScope>> scopes = GetScopes();
   if (scopes == nullptr)
      return SYSINFO_RC_ERROR;
   for(int i = 0; i < scopes->size(); i++)
      value->add(scopes->get(i)->id);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for DHCP.Scopes table
 */
LONG H_DhcpScopeTable(const TCHAR *metric, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   LONG rc = CheckDhcpAvailable();
   if (rc != SYSINFO_RC_SUCCESS)
      return rc;

   shared_ptr<ObjectArray<DhcpScope>> scopes = GetScopes();
   if (scopes == nullptr)
      return SYSINFO_RC_ERROR;

   CreateDHCPScopeTableColumns(value);
   for(int i = 0; i < scopes->size(); i++)
   {
      DhcpScope *s = scopes->get(i);
      AddDHCPScopeTableRow(value, s->id, 4, s->id, s->name, s->ranges, s->enabled, s->total, s->used, 0);
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Add leases from given scope to table
 */
static bool AddScopeLeases(DhcpScope *scope, Table *table)
{
   IntegerArray<NXDHCP_IP_ADDRESS> leasedAddresses;
   NXDHCP_RESUME_HANDLE resumeHandle = 0;
   DWORD rc;
   do
   {
      NXDHCP_CLIENT_INFO_ARRAY_V5* clients = nullptr;
      DWORD read = 0, total = 0;
      rc = s_DhcpEnumSubnetClientsV5(s_serverAddress, scope->subnet, &resumeHandle, DHCP_ENUM_PREFERRED_MAX, &clients, &read, &total);
      if ((rc != ERROR_SUCCESS) && (rc != ERROR_MORE_DATA))
      {
         if (rc == ERROR_NO_MORE_ITEMS)
            break;
         nxlog_debug_tag(DEBUG_TAG_DHCP, 4, _T("DhcpEnumSubnetClientsV5(%s) failed (error %u)"), scope->id.cstr(), rc);
         return false;
      }
      if (clients == nullptr)
         break;

      time_t now = time(nullptr);
      for(DWORD i = 0; i < clients->NumElements; i++)
      {
         const NXDHCP_CLIENT_INFO_V5 *c = clients->Clients[i];
         if (c == nullptr)
            continue;

         bool reserved = false;
         for(int j = 0; j < scope->reservations.size(); j++)
         {
            if (scope->reservations.get(j)->address == c->ClientIpAddress)
            {
               reserved = true;
               break;
            }
         }

         time_t expiration = DateTimeToUnixTime(c->ClientLeaseExpires);
         int state;
         switch(c->AddressState & 0x03)
         {
            case NX_V5_ADDRESS_STATE_OFFERED:
               state = DHCP_LEASE_STATE_OFFERED;
               break;
            case NX_V5_ADDRESS_STATE_DECLINED:
               state = DHCP_LEASE_STATE_DECLINED;
               break;
            case NX_V5_ADDRESS_STATE_DOOM:
               state = DHCP_LEASE_STATE_RELEASED;
               break;
            default:
               state = ((expiration != 0) && (expiration < now)) ? DHCP_LEASE_STATE_EXPIRED : DHCP_LEASE_STATE_ACTIVE;
               break;
         }
         // Reservation record that was never used by client has no lease time
         if (reserved && (c->ClientLeaseExpires.dwHighDateTime == 0) && (c->ClientLeaseExpires.dwLowDateTime == 0))
            state = DHCP_LEASE_STATE_INACTIVE;

         TCHAR ipText[32];
         table->addRow();
         table->set(DHCP_LEASE_COL_IP_ADDRESS, FormatAddress(c->ClientIpAddress, ipText));
         table->set(DHCP_LEASE_COL_PREFIX_LENGTH, static_cast<int32_t>(32));
         table->set(DHCP_LEASE_COL_SCOPE_ID, scope->id.cstr());
         table->set(DHCP_LEASE_COL_MAC_ADDRESS, ClientUidToMacAddress(&c->ClientHardwareAddress).cstr());
         table->set(DHCP_LEASE_COL_HOSTNAME, CHECK_NULL_EX(c->ClientName));
         table->set(DHCP_LEASE_COL_TYPE, static_cast<int32_t>(reserved ? DHCP_LEASE_TYPE_RESERVATION : DHCP_LEASE_TYPE_DYNAMIC));
         table->set(DHCP_LEASE_COL_STATE, static_cast<int32_t>(state));
         table->set(DHCP_LEASE_COL_LAST_TRANSACTION, static_cast<int64_t>(0));
         table->set(DHCP_LEASE_COL_EXPIRATION, static_cast<int64_t>(expiration));
         leasedAddresses.add(c->ClientIpAddress);
      }
      s_DhcpRpcFreeMemory(clients);
   } while(rc == ERROR_MORE_DATA);

   // Reservations without client record
   for(int i = 0; i < scope->reservations.size(); i++)
   {
      DhcpReservation *r = scope->reservations.get(i);
      if (leasedAddresses.contains(r->address))
         continue;
      TCHAR ipText[32];
      table->addRow();
      table->set(DHCP_LEASE_COL_IP_ADDRESS, FormatAddress(r->address, ipText));
      table->set(DHCP_LEASE_COL_PREFIX_LENGTH, static_cast<int32_t>(32));
      table->set(DHCP_LEASE_COL_SCOPE_ID, scope->id.cstr());
      table->set(DHCP_LEASE_COL_MAC_ADDRESS, r->macAddress.cstr());
      table->set(DHCP_LEASE_COL_TYPE, static_cast<int32_t>(DHCP_LEASE_TYPE_RESERVATION));
      table->set(DHCP_LEASE_COL_STATE, static_cast<int32_t>(DHCP_LEASE_STATE_INACTIVE));
      table->set(DHCP_LEASE_COL_LAST_TRANSACTION, static_cast<int64_t>(0));
      table->set(DHCP_LEASE_COL_EXPIRATION, static_cast<int64_t>(0));
   }
   return true;
}

/**
 * Handler for DHCP.Leases and DHCP.Leases(*) tables
 */
LONG H_DhcpLeaseTable(const TCHAR *metric, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   LONG rc = CheckDhcpAvailable();
   if (rc != SYSINFO_RC_SUCCESS)
      return rc;

   shared_ptr<ObjectArray<DhcpScope>> scopes = GetScopes();
   if (scopes == nullptr)
      return SYSINFO_RC_ERROR;

   DhcpScope *scopeFilter = nullptr;
   if (*arg == 'S')
   {
      TCHAR id[256];
      if (!AgentGetParameterArg(metric, 1, id, 256))
         return SYSINFO_RC_UNSUPPORTED;
      scopeFilter = FindScope(scopes.get(), id);
      if (scopeFilter == nullptr)
         return SYSINFO_RC_NO_SUCH_INSTANCE;
   }

   CreateDHCPLeaseTableColumns(value);
   if (scopeFilter != nullptr)
      return AddScopeLeases(scopeFilter, value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;

   for(int i = 0; i < scopes->size(); i++)
   {
      if (!AddScopeLeases(scopes->get(i), value))
         return SYSINFO_RC_ERROR;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Configure DHCP server management (called from subagent initialization)
 */
void ConfigureDhcpServerManagement(Config *config)
{
   const TCHAR *address = config->getValue(_T("/WinNT/DHCPServerAddress"));
   if (address != nullptr)
      wcslcpy(s_serverAddress, address, 256);
}
