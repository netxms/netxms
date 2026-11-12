/*
** NetXMS - Network Management System
** Drivers for Cisco devices
** Copyright (C) 2003-2026 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: dhcp.cpp
**
**/

#include "cisco.h"

/**
 * Split line into whitespace separated tokens
 */
static StringList SplitTokens(const wchar_t *line)
{
   StringList tokens;
   const wchar_t *p = line;
   while (*p != 0)
   {
      while (iswspace(*p))
         p++;
      if (*p == 0)
         break;
      const wchar_t *start = p;
      while ((*p != 0) && !iswspace(*p))
         p++;
      tokens.add(String(start, p - start));
   }
   return tokens;
}

/**
 * Parse value of "<label> : <value>" line from "show ip dhcp pool" output. Returns false if line does not have given label.
 */
static bool ParsePoolCounter(const wchar_t *line, const wchar_t *label, uint64_t *value)
{
   const wchar_t *p = line;
   while (iswspace(*p))
      p++;
   size_t len = wcslen(label);
   if (wcsnicmp(p, label, len))
      return false;
   p += len;
   while (iswspace(*p))
      p++;
   if (*p != L':')
      return false;
   *value = wcstoull(p + 1, nullptr, 10);
   return true;
}

/**
 * Derive network from usable address range of IOS DHCP pool subnet (first host - last host)
 */
static InetAddress NetworkFromHostRange(const InetAddress& first, const InetAddress& last)
{
   uint32_t low = first.getAddressV4() - 1;
   uint32_t high = last.getAddressV4() + 1;
   int bits = 32;
   while ((bits > 0) && ((low >> (32 - bits)) != (high >> (32 - bits))))
      bits--;
   uint32_t mask = (bits > 0) ? (0xFFFFFFFF << (32 - bits)) : 0;
   return InetAddress(low & mask, mask);
}

/**
 * Parse output of "show ip dhcp pool" command
 */
static void ParsePools(const StringList& lines, ObjectArray<DhcpScopeInfo> *scopes)
{
   DhcpScopeInfo *scope = nullptr;
   uint64_t excluded = 0;
   StringBuffer ranges;
   bool pendingScope = false;

   auto finishScope = [&] () -> void
   {
      if (scope == nullptr)
         return;
      if (pendingScope && scope->network.isValid())
      {
         if (scope->totalAddresses >= excluded)
            scope->totalAddresses -= excluded;
         scope->ranges = ranges;
         scope->freeAddresses = (scope->totalAddresses > scope->usedAddresses) ? scope->totalAddresses - scope->usedAddresses : 0;
         scopes->add(scope);
      }
      else
      {
         delete scope;   // Pool without subnet (for example, manual binding host pool)
      }
      scope = nullptr;
   };

   for(int i = 0; i < lines.size(); i++)
   {
      const wchar_t *line = lines.get(i);
      if (!wcsncmp(line, L"Pool ", 5))
      {
         finishScope();
         StringList tokens = SplitTokens(line + 5);
         scope = new DhcpScopeInfo();
         if (!tokens.isEmpty())
            scope->name = tokens.get(0);
         excluded = 0;
         ranges.clear();
         pendingScope = false;
         continue;
      }
      if (scope == nullptr)
         continue;

      uint64_t value;
      if (ParsePoolCounter(line, L"Total addresses", &value))
      {
         scope->totalAddresses = value;
         continue;
      }
      if (ParsePoolCounter(line, L"Leased addresses", &value))
      {
         scope->usedAddresses = value;
         continue;
      }
      if (ParsePoolCounter(line, L"Excluded addresses", &value))
      {
         excluded = value;
         continue;
      }

      // Subnet line: <current index> <first> - <last> <counters>
      StringList tokens = SplitTokens(line);
      if ((tokens.size() >= 4) && !wcscmp(tokens.get(2), L"-"))
      {
         InetAddress first = InetAddress::parse(tokens.get(1));
         InetAddress last = InetAddress::parse(tokens.get(3));
         if ((first.getFamily() == AF_INET) && (last.getFamily() == AF_INET) && (first.getAddressV4() <= last.getAddressV4()))
         {
            if (!pendingScope)
            {
               scope->setNetwork(NetworkFromHostRange(first, last));
               pendingScope = true;
            }
            if (!ranges.isEmpty())
               ranges.append(L", ");
            ranges.append(tokens.get(1));
            ranges.append(L'-');
            ranges.append(tokens.get(3));
         }
      }
   }
   finishScope();
}

/**
 * Read DHCP pools from IOS device
 */
static DataCollectionError ReadPools(SSHInteractiveChannel *ssh, ObjectArray<DhcpScopeInfo> *scopes)
{
   unique_ptr<StringList> lines(ssh->executeCommand("show ip dhcp pool"));
   if (lines == nullptr)
      return DCE_COMM_ERROR;
   ParsePools(*lines, scopes);
   return DCE_SUCCESS;
}

/**
 * Check if token consists only of hex digits and dots (client ID in IOS dotted notation)
 */
static bool IsDottedHex(const wchar_t *token)
{
   if (*token == 0)
      return false;
   for(const wchar_t *p = token; *p != 0; p++)
      if (!iswxdigit(*p) && (*p != L'.'))
         return false;
   return true;
}

/**
 * Set client ID and MAC address of lease from IOS client ID / hardware address in dotted hex notation.
 * Client ID of type 1 (Ethernet) is 01 followed by MAC address.
 */
static void SetLeaseClientId(DhcpLeaseInfo *lease, const wchar_t *dottedHex)
{
   StringBuffer hex;
   for(const wchar_t *p = dottedHex; *p != 0; p++)
      if (*p != L'.')
         hex.append(static_cast<wchar_t>(towlower(*p)));

   if (hex.length() == 12)
   {
      lease->macAddress = MacAddress::parse(hex);
      return;
   }

   if ((hex.length() == 14) && !wcsncmp(hex, L"01", 2))
      lease->macAddress = MacAddress::parse(hex.cstr() + 2);

   if ((hex.length() % 2) == 0)
   {
      StringBuffer clientId;
      for(size_t i = 0; i < hex.length(); i += 2)
      {
         if (i > 0)
            clientId.append(L':');
         clientId.append(&hex.cstr()[i], 2);
      }
      lease->clientId = clientId;
   }
   else
   {
      lease->clientId = hex;
   }
}

/**
 * Parse lease expiration time in IOS format (Mar 02 2024 10:22 AM). Device time zone is not known,
 * so time is interpreted as server's local time.
 */
static time_t ParseExpirationTime(const StringList& tokens, int start)
{
   static const wchar_t *months[] = { L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec" };

   struct tm t;
   memset(&t, 0, sizeof(t));
   t.tm_mon = -1;
   for(int i = 0; i < 12; i++)
   {
      if (!wcsicmp(tokens.get(start), months[i]))
      {
         t.tm_mon = i;
         break;
      }
   }
   if (t.tm_mon == -1)
      return 0;

   t.tm_mday = wcstol(tokens.get(start + 1), nullptr, 10);
   t.tm_year = wcstol(tokens.get(start + 2), nullptr, 10) - 1900;
   const wchar_t *timeText = tokens.get(start + 3);
   t.tm_hour = wcstol(timeText, nullptr, 10);
   const wchar_t *colon = wcschr(timeText, L':');
   if (colon != nullptr)
      t.tm_min = wcstol(colon + 1, nullptr, 10);
   const wchar_t *ampm = tokens.get(start + 4);
   if (!wcsicmp(ampm, L"PM") && (t.tm_hour < 12))
      t.tm_hour += 12;
   else if (!wcsicmp(ampm, L"AM") && (t.tm_hour == 12))
      t.tm_hour = 0;
   t.tm_isdst = -1;
   return mktime(&t);
}

/**
 * Parse output of "show ip dhcp binding" command. Output format (State and Interface columns
 * are present only in newer IOS versions; long client IDs continue on following lines):
 *
 * IP address          Client-ID/              Lease expiration        Type       State      Interface
 *                     Hardware address/
 *                     User name
 * 10.1.1.11           0100.1c42.0d6f.e2       Mar 01 2024 12:15 AM    Automatic  Active     Vlan10
 * 10.1.1.20           0063.6973.636f.2d30.    Infinite                Manual     Active     Unknown
 *                     3030.302e.3030.3030.
 *                     2e30.3030.312d.4769.
 *                     302f.30
 */
static void ParseBindings(const StringList& lines, const ObjectArray<DhcpScopeInfo>& scopes, ObjectArray<DhcpLeaseInfo> *leases)
{
   DhcpLeaseInfo *lease = nullptr;
   StringBuffer clientId;
   auto finishLease = [&] () -> void
   {
      if (lease == nullptr)
         return;
      SetLeaseClientId(lease, clientId);
      leases->add(lease);
      lease = nullptr;
   };

   for(int i = 0; i < lines.size(); i++)
   {
      const wchar_t *line = lines.get(i);
      StringList tokens = SplitTokens(line);
      if (tokens.isEmpty())
         continue;

      InetAddress address = InetAddress::parse(tokens.get(0));
      if (address.getFamily() != AF_INET)
      {
         // Continuation of long client ID
         if ((lease != nullptr) && iswspace(*line) && (tokens.size() == 1) && IsDottedHex(tokens.get(0)))
            clientId.append(tokens.get(0));
         continue;
      }

      finishLease();
      if (tokens.size() < 3)
         continue;

      lease = new DhcpLeaseInfo();
      lease->ipAddress = address;
      DhcpScopeInfo *scope = FindDhcpScopeForAddress(scopes, address);
      if (scope != nullptr)
         lease->scopeId = scope->id;
      clientId = tokens.get(1);

      int next;
      if (!wcsicmp(tokens.get(2), L"Infinite"))
      {
         next = 3;
      }
      else if (tokens.size() >= 7)
      {
         lease->expiration = ParseExpirationTime(tokens, 2);
         next = 7;
      }
      else
      {
         next = tokens.size();
      }

      if (next < tokens.size())
      {
         const wchar_t *type = tokens.get(next);
         lease->type = (!wcsicmp(type, L"Manual") || !wcsicmp(type, L"Static")) ? DHCP_LEASE_TYPE_RESERVATION : DHCP_LEASE_TYPE_DYNAMIC;
      }
      if (next + 1 < tokens.size())
      {
         const wchar_t *state = tokens.get(next + 1);
         if (!wcsicmp(state, L"Selecting"))
            lease->state = DHCP_LEASE_STATE_OFFERED;
         else if (!wcsicmp(state, L"Expired"))
            lease->state = DHCP_LEASE_STATE_EXPIRED;
         else if (!wcsicmp(state, L"Released"))
            lease->state = DHCP_LEASE_STATE_RELEASED;
         else if (!wcsicmp(state, L"Declined"))
            lease->state = DHCP_LEASE_STATE_DECLINED;
      }
   }
   finishLease();
}

/**
 * Read DHCP bindings from IOS device
 */
static DataCollectionError ReadBindings(SSHInteractiveChannel *ssh, const ObjectArray<DhcpScopeInfo>& scopes, ObjectArray<DhcpLeaseInfo> *leases)
{
   unique_ptr<StringList> lines(ssh->executeCommand("show ip dhcp binding"));
   if (lines == nullptr)
      return DCE_COMM_ERROR;
   ParseBindings(*lines, scopes, leases);
   return DCE_SUCCESS;
}

/**
 * Get interactive SSH channel for reading DHCP information. Returns DCE_NOT_SUPPORTED in error code if
 * channel is not available on this node.
 */
static SSHInteractiveChannel *GetSSHChannel(DeviceContext *context, DataCollectionError *rc)
{
   if (!context->isSSHInteractiveChannelAvailable())
   {
      *rc = DCE_NOT_SUPPORTED;
      return nullptr;
   }
   SSHInteractiveChannel *ssh = context->getInteractiveSSH();
   *rc = (ssh != nullptr) ? DCE_SUCCESS : DCE_COMM_ERROR;
   return ssh;
}

/**
 * Get DHCP scopes (IOS DHCP server pools, read via SSH). Number of declined (conflicting) addresses is not reported.
 */
DataCollectionError CiscoDeviceDriver::getDhcpScopes(DeviceContext *context, NObject *node, DriverData *driverData, ObjectArray<DhcpScopeInfo> *scopes)
{
   DataCollectionError rc;
   SSHInteractiveChannel *ssh = GetSSHChannel(context, &rc);
   if (ssh == nullptr)
      return rc;

   ObjectArray<DhcpScopeInfo> pools(0, 16, Ownership::True);
   rc = ReadPools(ssh, &pools);
   nxlog_debug_tag(DEBUG_TAG_CISCO, 6, L"CiscoDeviceDriver::getDhcpScopes: %d pools read (%hs)", pools.size(), DataCollectionErrorName(rc));
   if (rc != DCE_SUCCESS)
      return rc;
   if (pools.isEmpty())
      return DCE_NOT_SUPPORTED;   // DHCP server is not configured

   pools.setOwner(Ownership::False);
   for(int i = 0; i < pools.size(); i++)
      scopes->add(pools.get(i));
   return DCE_SUCCESS;
}

/**
 * Get DHCP leases (IOS DHCP server bindings, read via SSH)
 */
DataCollectionError CiscoDeviceDriver::getDhcpLeases(DeviceContext *context, NObject *node, DriverData *driverData, ObjectArray<DhcpLeaseInfo> *leases)
{
   DataCollectionError rc;
   SSHInteractiveChannel *ssh = GetSSHChannel(context, &rc);
   if (ssh == nullptr)
      return rc;

   ObjectArray<DhcpScopeInfo> pools(0, 16, Ownership::True);
   rc = ReadPools(ssh, &pools);
   if (rc != DCE_SUCCESS)
      return rc;
   if (pools.isEmpty())
      return DCE_NOT_SUPPORTED;

   rc = ReadBindings(ssh, pools, leases);
   nxlog_debug_tag(DEBUG_TAG_CISCO, 6, L"CiscoDeviceDriver::getDhcpLeases: %d leases read (%hs)", leases->size(), DataCollectionErrorName(rc));
   return rc;
}
