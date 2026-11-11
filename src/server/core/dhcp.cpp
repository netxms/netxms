/*
** NetXMS - Network Management System
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
**/

#include "nxcore.h"

#define DEBUG_TAG L"dhcp"

/**
 * Parse network address in "address/bits" notation
 */
static InetAddress ParseNetworkAddress(const wchar_t *text)
{
   if (text == nullptr)
      return InetAddress();

   const wchar_t *slash = wcschr(text, L'/');
   if (slash == nullptr)
      return InetAddress::parse(text);

   wchar_t address[64];
   wcslcpy(address, text, std::min(static_cast<size_t>(slash - text + 1), sizeof(address) / sizeof(wchar_t)));
   InetAddress a = InetAddress::parse(address);
   if (a.isValid())
      a.setMaskBits(wcstol(slash + 1, nullptr, 10));
   return a;
}

/**
 * Get DHCP scopes from node. Information is read from agent's DHCP.Scopes table.
 */
DataCollectionError Node::getDhcpScopes(ObjectArray<DhcpScopeInfo> *scopes)
{
   shared_ptr<Table> table;
   DataCollectionError rc = getTableFromAgent(L"DHCP.Scopes", &table);
   if (rc != DCE_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Node::getDhcpScopes(%s [%u]): cannot read DHCP.Scopes table from agent (%hs)", m_name, m_id, DataCollectionErrorName(rc));
      return rc;
   }

   int cId = table->getColumnIndex(L"ID");
   int cNetwork = table->getColumnIndex(L"NETWORK");
   int cName = table->getColumnIndex(L"NAME");
   int cRanges = table->getColumnIndex(L"RANGES");
   int cEnabled = table->getColumnIndex(L"ENABLED");
   int cTotal = table->getColumnIndex(L"TOTAL");
   int cUsed = table->getColumnIndex(L"USED");
   int cDeclined = table->getColumnIndex(L"DECLINED");
   int cFree = table->getColumnIndex(L"FREE");
   if (cId == -1)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Node::getDhcpScopes(%s [%u]): invalid DHCP.Scopes table format", m_name, m_id);
      return DCE_INVALID_DATA;
   }

   for(int i = 0; i < table->getNumRows(); i++)
   {
      auto scope = new DhcpScopeInfo();
      scope->id = table->getAsString(i, cId, L"");
      scope->network = ParseNetworkAddress(table->getAsString(i, cNetwork));
      scope->name = table->getAsString(i, cName, L"");
      scope->ranges = table->getAsString(i, cRanges, L"");
      scope->enabled = (cEnabled == -1) || (table->getAsInt(i, cEnabled) != 0);
      scope->totalAddresses = table->getAsUInt64(i, cTotal);
      scope->usedAddresses = table->getAsUInt64(i, cUsed);
      // Empty DECLINED cell means that DHCP server does not report declined addresses
      const wchar_t *declined = (cDeclined != -1) ? table->getAsString(i, cDeclined) : nullptr;
      if ((declined != nullptr) && (*declined != 0))
      {
         scope->declinedAddresses = table->getAsUInt64(i, cDeclined);
         scope->declinedAddressesKnown = true;
      }
      if (cFree != -1)
      {
         scope->freeAddresses = table->getAsUInt64(i, cFree);
      }
      else if (scope->totalAddresses > scope->usedAddresses + scope->declinedAddresses)
      {
         scope->freeAddresses = scope->totalAddresses - scope->usedAddresses - scope->declinedAddresses;
      }
      scopes->add(scope);
   }
   return DCE_SUCCESS;
}

/**
 * Get DHCP leases from node. If scope ID is nullptr, leases from all scopes are returned.
 * Information is read from agent's DHCP.Leases table. Empty scope ID or scope ID containing
 * double quote cannot be passed as metric argument and is reported as non-existing scope.
 */
DataCollectionError Node::getDhcpLeases(const wchar_t *scopeId, ObjectArray<DhcpLeaseInfo> *leases)
{
   StringBuffer metric(L"DHCP.Leases");
   if (scopeId != nullptr)
   {
      if ((*scopeId == 0) || (wcschr(scopeId, L'"') != nullptr))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"Node::getDhcpLeases(%s [%u]): invalid scope ID \"%s\"", m_name, m_id, scopeId);
         return DCE_NO_SUCH_INSTANCE;
      }
      metric.append(L"(\"");
      metric.append(scopeId);
      metric.append(L"\")");
   }

   shared_ptr<Table> table;
   DataCollectionError rc = getTableFromAgent(metric, &table);
   if (rc != DCE_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Node::getDhcpLeases(%s [%u]): cannot read %s table from agent (%hs)", m_name, m_id, metric.cstr(), DataCollectionErrorName(rc));
      return rc;
   }

   int cIpAddress = table->getColumnIndex(L"IP_ADDRESS");
   int cPrefixLength = table->getColumnIndex(L"PREFIX_LENGTH");
   int cScopeId = table->getColumnIndex(L"SCOPE_ID");
   int cMacAddress = table->getColumnIndex(L"MAC_ADDRESS");
   int cClientId = table->getColumnIndex(L"CLIENT_ID");
   int cHostname = table->getColumnIndex(L"HOSTNAME");
   int cType = table->getColumnIndex(L"TYPE");
   int cState = table->getColumnIndex(L"STATE");
   int cLastTransaction = table->getColumnIndex(L"LAST_TRANSACTION");
   int cExpiration = table->getColumnIndex(L"EXPIRATION");
   if (cIpAddress == -1)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Node::getDhcpLeases(%s [%u]): invalid DHCP.Leases table format", m_name, m_id);
      return DCE_INVALID_DATA;
   }

   for(int i = 0; i < table->getNumRows(); i++)
   {
      InetAddress ipAddress = InetAddress::parse(table->getAsString(i, cIpAddress));
      if (!ipAddress.isValid())
         continue;

      if (cPrefixLength != -1)
      {
         int prefixLength = table->getAsInt(i, cPrefixLength);
         if (prefixLength > 0)
            ipAddress.setMaskBits(prefixLength);
      }

      auto lease = new DhcpLeaseInfo();
      lease->ipAddress = ipAddress;
      lease->scopeId = table->getAsString(i, cScopeId, L"");
      const wchar_t *mac = table->getAsString(i, cMacAddress);
      if ((mac != nullptr) && (*mac != 0))
         lease->macAddress = MacAddress::parse(mac);
      lease->clientId = table->getAsString(i, cClientId, L"");
      lease->hostname = table->getAsString(i, cHostname, L"");
      lease->type = static_cast<int16_t>(table->getAsInt(i, cType));
      lease->state = static_cast<int16_t>(table->getAsInt(i, cState));
      lease->lastTransaction = static_cast<time_t>(table->getAsInt64(i, cLastTransaction));
      lease->expiration = static_cast<time_t>(table->getAsInt64(i, cExpiration));
      leases->add(lease);
   }
   return DCE_SUCCESS;
}
