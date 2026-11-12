/*
** NetXMS - Network Management System
** Driver for Mikrotik routers
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
**/

#include "mikrotik.h"

/**
 * Record from "print terse" output: flags and property map
 */
struct TerseRecord
{
   StringMap properties;
   MutableString flags;

   const wchar_t *get(const wchar_t *name) const { return properties.get(name); }
   bool hasFlag(wchar_t flag) const { return wcschr(flags, flag) != nullptr; }
};

/**
 * Parse single line of RouterOS "print terse" output. Line format is
 *    <index> [<flags>] <name>=<value> <name>=<value> ...
 * where value may be quoted. Returns false if line is not a record line (empty line, comment, error message).
 */
static bool ParseTerseLine(const wchar_t *line, TerseRecord *record)
{
   const wchar_t *p = line;
   while (iswspace(*p))
      p++;
   if (!iswdigit(*p))
      return false;
   while (iswdigit(*p))
      p++;
   if ((*p != 0) && !iswspace(*p))
      return false;

   StringBuffer flags;
   while (*p != 0)
   {
      while (iswspace(*p))
         p++;
      if (*p == 0)
         break;

      const wchar_t *start = p;
      while ((*p != 0) && (*p != L'=') && !iswspace(*p))
         p++;
      if (*p != L'=')
      {
         // Token without value is a set of flags
         flags.append(start, p - start);
         continue;
      }

      String name(start, p - start);
      p++;  // skip =
      StringBuffer value;
      if (*p == L'"')
      {
         p++;
         while ((*p != 0) && (*p != L'"'))
         {
            if ((*p == L'\\') && (*(p + 1) != 0))
               p++;
            value.append(*p);
            p++;
         }
         if (*p == L'"')
            p++;
      }
      else
      {
         while ((*p != 0) && !iswspace(*p))
         {
            value.append(*p);
            p++;
         }
      }
      record->properties.set(name, value);
   }
   record->flags = flags;
   return true;
}

/**
 * Execute RouterOS command and parse "print terse" output. Uses SSH command channel if available,
 * interactive channel otherwise. Returns nullptr if command cannot be executed.
 */
static ObjectArray<TerseRecord> *ExecuteTersePrint(DeviceContext *context, const char *command)
{
   StringList *lines;
   if (context->isSSHCommandChannelAvailable())
   {
      ByteStream output;
      if (!context->executeSSHCommand(command, &output))
         return nullptr;
      output.write('\0');
      wchar_t *text = WideStringFromUTF8String(reinterpret_cast<const char*>(output.buffer()));
      lines = new StringList(text, L"\n");
      MemFree(text);
   }
   else
   {
      SSHInteractiveChannel *ssh = context->getInteractiveSSH();
      if (ssh == nullptr)
         return nullptr;
      lines = ssh->executeCommand(command);
      if (lines == nullptr)
         return nullptr;
   }

   auto records = new ObjectArray<TerseRecord>(0, 64, Ownership::True);
   for(int i = 0; i < lines->size(); i++)
   {
      auto record = new TerseRecord();
      if (ParseTerseLine(lines->get(i), record))
         records->add(record);
      else
         delete record;
   }
   delete lines;
   return records;
}

/**
 * Parse RouterOS time interval (like 1w2d3h4m5s, 9m54s, 500ms, or 00:09:54). Returns -1 if interval cannot be parsed.
 */
static int64_t ParseInterval(const wchar_t *text)
{
   if ((text == nullptr) || (*text == 0) || !wcscmp(text, L"never"))
      return -1;

   if (wcschr(text, L':') != nullptr)
   {
      int64_t result = 0;
      for(const wchar_t *p = text; *p != 0; p++)
      {
         if (iswdigit(*p))
         {
            wchar_t *eptr;
            result = result * 60 + wcstoll(p, &eptr, 10);
            p = eptr - 1;
         }
         else if (*p != L':')
         {
            return -1;
         }
      }
      return result;
   }

   int64_t result = 0;
   const wchar_t *p = text;
   while (*p != 0)
   {
      if (!iswdigit(*p))
         return -1;
      wchar_t *eptr;
      int64_t n = wcstoll(p, &eptr, 10);
      p = eptr;
      if (!wcsncmp(p, L"ms", 2))
      {
         p += 2;
         continue;
      }
      switch(*p)
      {
         case L'w':
            result += n * 604800;
            break;
         case L'd':
            result += n * 86400;
            break;
         case L'h':
            result += n * 3600;
            break;
         case L'm':
            result += n * 60;
            break;
         case L's':
            result += n;
            break;
         default:
            return -1;
      }
      p++;
   }
   return result;
}

/**
 * Address range from IP pool
 */
struct PoolRange
{
   InetAddress start;
   InetAddress end;

   uint64_t size() const { return static_cast<uint64_t>(end.getAddressV4()) - static_cast<uint64_t>(start.getAddressV4()) + 1; }
   bool contains(const InetAddress& a) const { return a.inRange(start, end); }
};

/**
 * Parse RouterOS pool ranges (comma separated list of single addresses, address ranges, or prefixes)
 */
static void ParsePoolRanges(const wchar_t *text, StructArray<PoolRange> *ranges)
{
   if (text == nullptr)
      return;
   String::split(text, L",", true,
      [ranges] (const String& element) -> void
      {
         PoolRange range;
         ssize_t sep = element.find(L"-");
         if (sep != String::npos)
         {
            range.start = InetAddress::parse(element.substring(0, sep));
            range.end = InetAddress::parse(element.substring(sep + 1, -1));
         }
         else if (element.find(L"/") != String::npos)
         {
            ssize_t slash = element.find(L"/");
            InetAddress prefix = InetAddress::parse(element.substring(0, slash));
            prefix.setMaskBits(wcstol(element.substring(slash + 1, -1), nullptr, 10));
            range.start = prefix.getSubnetAddress();
            range.end = prefix.getSubnetBroadcast();
         }
         else
         {
            range.start = InetAddress::parse(element);
            range.end = range.start;
         }
         if ((range.start.getFamily() == AF_INET) && (range.end.getFamily() == AF_INET) && (range.start.getAddressV4() <= range.end.getAddressV4()))
            ranges->add(range);
      });
}

/**
 * Scope being built from RouterOS DHCP configuration
 */
struct ScopeBuilder
{
   DhcpScopeInfo *scope;
   StructArray<PoolRange> ranges;
   uint64_t staticLeases;
   uint64_t boundLeases;
   bool hasServer;
   bool hasEnabledServer;

   ScopeBuilder(DhcpScopeInfo *s)
   {
      scope = s;
      staticLeases = 0;
      boundLeases = 0;
      hasServer = false;
      hasEnabledServer = false;
   }

   bool inRanges(const InetAddress& a) const
   {
      for(int i = 0; i < ranges.size(); i++)
         if (ranges.get(i)->contains(a))
            return true;
      return false;
   }
};

/**
 * Read DHCP server configuration and leases from RouterOS device. Scope is created for each DHCP network
 * (/ip dhcp-server network). Address pools of DHCP servers are assigned to scopes by network containment.
 * Either of output arrays can be nullptr.
 */
static DataCollectionError ReadDhcpData(DeviceContext *context, DriverData *driverData, ObjectArray<DhcpScopeInfo> *scopes, ObjectArray<DhcpLeaseInfo> *leases)
{
   if (!context->isSSHCommandChannelAvailable() && !context->isSSHInteractiveChannelAvailable())
      return DCE_NOT_SUPPORTED;

   const wchar_t *nodeName = (driverData != nullptr) ? driverData->getNodeName() : L"";

   unique_ptr<ObjectArray<TerseRecord>> networks(ExecuteTersePrint(context, "/ip dhcp-server network print terse without-paging"));
   if (networks == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"MikrotikDriver::ReadDhcpData(%s): cannot read DHCP network list", nodeName);
      return DCE_COMM_ERROR;
   }
   unique_ptr<ObjectArray<TerseRecord>> servers(ExecuteTersePrint(context, "/ip dhcp-server print terse without-paging"));
   unique_ptr<ObjectArray<TerseRecord>> pools(ExecuteTersePrint(context, "/ip pool print terse without-paging"));
   unique_ptr<ObjectArray<TerseRecord>> leaseRecords(ExecuteTersePrint(context, "/ip dhcp-server lease print terse without-paging"));
   if ((servers == nullptr) || (pools == nullptr) || (leaseRecords == nullptr))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"MikrotikDriver::ReadDhcpData(%s): cannot read DHCP server configuration", nodeName);
      return DCE_COMM_ERROR;
   }
   if (servers->isEmpty())
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"MikrotikDriver::ReadDhcpData(%s): no DHCP servers configured", nodeName);
      return DCE_NOT_SUPPORTED;
   }

   ObjectArray<DhcpScopeInfo> scopeList(0, 16, Ownership::True);
   ObjectArray<ScopeBuilder> builders(0, 16, Ownership::True);
   for(int i = 0; i < networks->size(); i++)
   {
      const wchar_t *address = networks->get(i)->get(L"address");
      if (address == nullptr)
         continue;
      const wchar_t *slash = wcschr(address, L'/');
      InetAddress network = InetAddress::parse(String(address, (slash != nullptr) ? slash - address : wcslen(address)));
      if (network.getFamily() != AF_INET)
         continue;
      network.setMaskBits((slash != nullptr) ? wcstol(slash + 1, nullptr, 10) : 32);
      network = network.getSubnetAddress();

      auto scope = new DhcpScopeInfo();
      scope->setNetwork(network);
      scopeList.add(scope);
      builders.add(new ScopeBuilder(scope));
   }

   // Assign server pools to scopes
   for(int i = 0; i < servers->size(); i++)
   {
      TerseRecord *server = servers->get(i);
      const wchar_t *serverName = server->get(L"name");
      bool enabled = !server->hasFlag(L'X') && !server->hasFlag(L'I');

      StructArray<PoolRange> serverRanges;
      const wchar_t *poolName = server->get(L"address-pool");
      for(int j = 0; (poolName != nullptr) && (j < pools->size()); j++)
      {
         const wchar_t *name = pools->get(j)->get(L"name");
         if ((name != nullptr) && !wcscmp(name, poolName))
            ParsePoolRanges(pools->get(j)->get(L"ranges"), &serverRanges);
      }

      for(int j = 0; j < builders.size(); j++)
      {
         ScopeBuilder *b = builders.get(j);
         bool match = false;
         for(int k = 0; k < serverRanges.size(); k++)
         {
            PoolRange *r = serverRanges.get(k);
            if (b->scope->network.contains(r->start))
            {
               b->ranges.add(r);
               match = true;
            }
         }
         if (!match)
            continue;

         b->hasServer = true;
         if (enabled)
            b->hasEnabledServer = true;
         if (serverName != nullptr)
         {
            if (!b->scope->name.isEmpty())
            {
               StringBuffer sb(b->scope->name);
               sb.append(L", ");
               sb.append(serverName);
               b->scope->name = sb;
            }
            else
            {
               b->scope->name = serverName;
            }
         }
      }
   }

   for(int i = 0; i < builders.size(); i++)
   {
      ScopeBuilder *b = builders.get(i);
      StringBuffer rangeText;
      for(int j = 0; j < b->ranges.size(); j++)
      {
         PoolRange *r = b->ranges.get(j);
         b->scope->totalAddresses += r->size();
         if (!rangeText.isEmpty())
            rangeText.append(L", ");
         wchar_t buffer[64];
         rangeText.append(r->start.toString(buffer));
         rangeText.append(L'-');
         rangeText.append(r->end.toString(buffer));
      }
      b->scope->ranges = rangeText;
      b->scope->enabled = !b->hasServer || b->hasEnabledServer;
      b->scope->declinedAddressesKnown = true;
   }

   time_t now = time(nullptr);
   for(int i = 0; i < leaseRecords->size(); i++)
   {
      TerseRecord *record = leaseRecords->get(i);
      InetAddress address = InetAddress::parse(record->get(L"address"));
      if (!address.isValid())
         address = InetAddress::parse(record->get(L"active-address"));
      if (address.getFamily() != AF_INET)
         continue;

      bool dynamic = record->hasFlag(L'D');
      bool disabled = record->hasFlag(L'X');
      const wchar_t *status = record->get(L"status");
      int16_t state;
      if (disabled)
         state = DHCP_LEASE_STATE_INACTIVE;
      else if ((status != nullptr) && !wcscmp(status, L"bound"))
         state = DHCP_LEASE_STATE_ACTIVE;
      else if ((status != nullptr) && !wcscmp(status, L"offered"))
         state = DHCP_LEASE_STATE_OFFERED;
      else if ((status != nullptr) && !wcscmp(status, L"busy"))
         state = DHCP_LEASE_STATE_DECLINED;
      else
         state = dynamic ? DHCP_LEASE_STATE_EXPIRED : DHCP_LEASE_STATE_INACTIVE;

      DhcpScopeInfo *scope = FindDhcpScopeForAddress(scopeList, address);
      if (scope != nullptr)
      {
         ScopeBuilder *b = builders.get(scopeList.indexOf(scope));
         if (!dynamic && !disabled)
            b->staticLeases++;
         if (state == DHCP_LEASE_STATE_ACTIVE)
            b->boundLeases++;
         if (b->inRanges(address))
         {
            if (state == DHCP_LEASE_STATE_ACTIVE)
               scope->usedAddresses++;
            else if (state == DHCP_LEASE_STATE_DECLINED)
               scope->declinedAddresses++;
         }
      }

      if (leases == nullptr)
         continue;

      auto lease = new DhcpLeaseInfo();
      lease->ipAddress = address;
      if (scope != nullptr)
         lease->scopeId = scope->id;
      const wchar_t *mac = record->get(L"mac-address");
      if ((mac == nullptr) || (*mac == 0))
         mac = record->get(L"active-mac-address");
      if ((mac != nullptr) && (*mac != 0))
         lease->macAddress = MacAddress::parse(mac);
      const wchar_t *clientId = record->get(L"client-id");
      if ((clientId == nullptr) || (*clientId == 0))
         clientId = record->get(L"active-client-id");
      if (clientId != nullptr)
         lease->clientId = clientId;
      const wchar_t *hostname = record->get(L"host-name");
      if (hostname != nullptr)
         lease->hostname = hostname;
      lease->type = dynamic ? DHCP_LEASE_TYPE_DYNAMIC : DHCP_LEASE_TYPE_RESERVATION;
      lease->state = state;
      int64_t lastSeen = ParseInterval(record->get(L"last-seen"));
      if (lastSeen >= 0)
         lease->lastTransaction = now - static_cast<time_t>(lastSeen);
      int64_t expiresAfter = ParseInterval(record->get(L"expires-after"));
      if (expiresAfter >= 0)
         lease->expiration = now + static_cast<time_t>(expiresAfter);
      leases->add(lease);
   }

   // Scopes served only by static leases (no address pool) - static leases are the address space
   for(int i = 0; i < builders.size(); i++)
   {
      ScopeBuilder *b = builders.get(i);
      if (b->ranges.isEmpty())
      {
         b->scope->totalAddresses = b->staticLeases;
         b->scope->usedAddresses = b->boundLeases;
      }
      b->scope->freeAddresses = (b->scope->totalAddresses > b->scope->usedAddresses + b->scope->declinedAddresses) ?
            b->scope->totalAddresses - b->scope->usedAddresses - b->scope->declinedAddresses : 0;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, L"MikrotikDriver::ReadDhcpData(%s): %d scopes, %d lease records", nodeName, scopeList.size(), leaseRecords->size());

   if (scopes != nullptr)
   {
      scopeList.setOwner(Ownership::False);
      for(int i = 0; i < scopeList.size(); i++)
         scopes->add(scopeList.get(i));
   }
   return DCE_SUCCESS;
}

/**
 * Get DHCP scopes (read via SSH)
 */
DataCollectionError MikrotikDriver::getDhcpScopes(DeviceContext *context, NObject *node, DriverData *driverData, ObjectArray<DhcpScopeInfo> *scopes)
{
   return ReadDhcpData(context, driverData, scopes, nullptr);
}

/**
 * Get DHCP leases (read via SSH)
 */
DataCollectionError MikrotikDriver::getDhcpLeases(DeviceContext *context, NObject *node, DriverData *driverData, ObjectArray<DhcpLeaseInfo> *leases)
{
   return ReadDhcpData(context, driverData, nullptr, leases);
}
