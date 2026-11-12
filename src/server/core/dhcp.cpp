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
 * Time to live for cached DHCP scope list used by internal metrics (seconds)
 */
#define DHCP_SCOPE_CACHE_TTL   30

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
 * Parse agent's DHCP.Scopes table
 */
static bool ParseScopeTable(const Table& table, ObjectArray<DhcpScopeInfo> *scopes)
{
   int cId = table.getColumnIndex(L"ID");
   int cNetwork = table.getColumnIndex(L"NETWORK");
   int cName = table.getColumnIndex(L"NAME");
   int cRanges = table.getColumnIndex(L"RANGES");
   int cEnabled = table.getColumnIndex(L"ENABLED");
   int cTotal = table.getColumnIndex(L"TOTAL");
   int cUsed = table.getColumnIndex(L"USED");
   int cDeclined = table.getColumnIndex(L"DECLINED");
   int cFree = table.getColumnIndex(L"FREE");
   if (cId == -1)
      return false;

   for(int i = 0; i < table.getNumRows(); i++)
   {
      auto scope = new DhcpScopeInfo();
      scope->id = table.getAsString(i, cId, L"");
      scope->network = ParseNetworkAddress(table.getAsString(i, cNetwork));
      scope->name = table.getAsString(i, cName, L"");
      scope->ranges = table.getAsString(i, cRanges, L"");
      scope->enabled = (cEnabled == -1) || (table.getAsInt(i, cEnabled) != 0);
      scope->totalAddresses = table.getAsUInt64(i, cTotal);
      scope->usedAddresses = table.getAsUInt64(i, cUsed);
      // Empty DECLINED cell means that DHCP server does not report declined addresses
      const wchar_t *declined = (cDeclined != -1) ? table.getAsString(i, cDeclined) : nullptr;
      if ((declined != nullptr) && (*declined != 0))
      {
         scope->declinedAddresses = table.getAsUInt64(i, cDeclined);
         scope->declinedAddressesKnown = true;
      }
      if (cFree != -1)
      {
         scope->freeAddresses = table.getAsUInt64(i, cFree);
      }
      else if (scope->totalAddresses > scope->usedAddresses + scope->declinedAddresses)
      {
         scope->freeAddresses = scope->totalAddresses - scope->usedAddresses - scope->declinedAddresses;
      }
      scopes->add(scope);
   }
   return true;
}

/**
 * Parse agent's DHCP.Leases table
 */
static bool ParseLeaseTable(const Table& table, ObjectArray<DhcpLeaseInfo> *leases)
{
   int cIpAddress = table.getColumnIndex(L"IP_ADDRESS");
   int cPrefixLength = table.getColumnIndex(L"PREFIX_LENGTH");
   int cScopeId = table.getColumnIndex(L"SCOPE_ID");
   int cMacAddress = table.getColumnIndex(L"MAC_ADDRESS");
   int cClientId = table.getColumnIndex(L"CLIENT_ID");
   int cHostname = table.getColumnIndex(L"HOSTNAME");
   int cType = table.getColumnIndex(L"TYPE");
   int cState = table.getColumnIndex(L"STATE");
   int cLastTransaction = table.getColumnIndex(L"LAST_TRANSACTION");
   int cExpiration = table.getColumnIndex(L"EXPIRATION");
   if (cIpAddress == -1)
      return false;

   for(int i = 0; i < table.getNumRows(); i++)
   {
      InetAddress ipAddress = InetAddress::parse(table.getAsString(i, cIpAddress));
      if (!ipAddress.isValid())
         continue;

      if (cPrefixLength != -1)
      {
         int prefixLength = table.getAsInt(i, cPrefixLength);
         if (prefixLength > 0)
            ipAddress.setMaskBits(prefixLength);
      }

      auto lease = new DhcpLeaseInfo();
      lease->ipAddress = ipAddress;
      lease->scopeId = table.getAsString(i, cScopeId, L"");
      const wchar_t *mac = table.getAsString(i, cMacAddress);
      if ((mac != nullptr) && (*mac != 0))
         lease->macAddress = MacAddress::parse(mac);
      lease->clientId = table.getAsString(i, cClientId, L"");
      lease->hostname = table.getAsString(i, cHostname, L"");
      lease->type = static_cast<int16_t>(table.getAsInt(i, cType));
      lease->state = static_cast<int16_t>(table.getAsInt(i, cState));
      lease->lastTransaction = static_cast<time_t>(table.getAsInt64(i, cLastTransaction));
      lease->expiration = static_cast<time_t>(table.getAsInt64(i, cExpiration));
      leases->add(lease);
   }
   return true;
}

/**
 * Get DHCP scopes from node. Information is read from agent's DHCP.Scopes table if node has agent that
 * provides it, otherwise from network device driver.
 */
DataCollectionError Node::getDhcpScopes(ObjectArray<DhcpScopeInfo> *scopes)
{
   DataCollectionError rc = DCE_NOT_SUPPORTED;
   if ((m_capabilities & NC_IS_NATIVE_AGENT) && !(m_flags & NF_DISABLE_NXCP))
   {
      shared_ptr<Table> table;
      rc = getTableFromAgent(L"DHCP.Scopes", &table);
      if (rc == DCE_SUCCESS)
      {
         if (ParseScopeTable(*table, scopes))
            return DCE_SUCCESS;
         nxlog_debug_tag(DEBUG_TAG, 5, L"Node::getDhcpScopes(%s [%u]): invalid DHCP.Scopes table format", m_name, m_id);
         return DCE_INVALID_DATA;
      }
      nxlog_debug_tag(DEBUG_TAG, 5, L"Node::getDhcpScopes(%s [%u]): cannot read DHCP.Scopes table from agent (%hs)", m_name, m_id, DataCollectionErrorName(rc));
      if (rc != DCE_NOT_SUPPORTED)
         return rc;
   }

   NodeDeviceContext context(self());
   auto driver = getUsableDriverWithData(&context);
   if (driver.first == nullptr)
      return isDHCPServer() ? DCE_COMM_ERROR : rc;

   rc = driver.first->getDhcpScopes(&context, this, driver.second.get(), scopes);
   nxlog_debug_tag(DEBUG_TAG, 5, L"Node::getDhcpScopes(%s [%u]): DHCP scopes read from device driver %s (%hs, %d scopes)",
         m_name, m_id, driver.first->getName(), DataCollectionErrorName(rc), scopes->size());
   return rc;
}

/**
 * Get DHCP leases from node. If scope ID is nullptr, leases from all scopes are returned.
 * Information is read from agent's DHCP.Leases table if node has agent that provides it, otherwise
 * from network device driver. Empty scope ID or scope ID containing double quote cannot be passed
 * as metric argument and is reported as non-existing scope.
 */
DataCollectionError Node::getDhcpLeases(const wchar_t *scopeId, ObjectArray<DhcpLeaseInfo> *leases)
{
   if ((scopeId != nullptr) && ((*scopeId == 0) || (wcschr(scopeId, L'"') != nullptr)))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Node::getDhcpLeases(%s [%u]): invalid scope ID \"%s\"", m_name, m_id, scopeId);
      return DCE_NO_SUCH_INSTANCE;
   }

   DataCollectionError rc = DCE_NOT_SUPPORTED;
   if ((m_capabilities & NC_IS_NATIVE_AGENT) && !(m_flags & NF_DISABLE_NXCP))
   {
      StringBuffer metric(L"DHCP.Leases");
      if (scopeId != nullptr)
      {
         metric.append(L"(\"");
         metric.append(scopeId);
         metric.append(L"\")");
      }

      shared_ptr<Table> table;
      rc = getTableFromAgent(metric, &table);
      if (rc == DCE_SUCCESS)
      {
         if (ParseLeaseTable(*table, leases))
            return DCE_SUCCESS;
         nxlog_debug_tag(DEBUG_TAG, 5, L"Node::getDhcpLeases(%s [%u]): invalid DHCP.Leases table format", m_name, m_id);
         return DCE_INVALID_DATA;
      }
      nxlog_debug_tag(DEBUG_TAG, 5, L"Node::getDhcpLeases(%s [%u]): cannot read %s table from agent (%hs)", m_name, m_id, metric.cstr(), DataCollectionErrorName(rc));
      if (rc != DCE_NOT_SUPPORTED)
         return rc;
   }

   NodeDeviceContext context(self());
   auto driver = getUsableDriverWithData(&context);
   if (driver.first == nullptr)
      return isDHCPServer() ? DCE_COMM_ERROR : rc;

   int start = leases->size();
   rc = driver.first->getDhcpLeases(&context, this, driver.second.get(), leases);
   if ((rc == DCE_SUCCESS) && (scopeId != nullptr))
   {
      for(int i = leases->size() - 1; i >= start; i--)
      {
         if (wcscmp(leases->get(i)->scopeId, scopeId))
            leases->remove(i);
      }
   }
   nxlog_debug_tag(DEBUG_TAG, 5, L"Node::getDhcpLeases(%s [%u]): DHCP leases read from device driver %s (%hs, %d leases)",
         m_name, m_id, driver.first->getName(), DataCollectionErrorName(rc), leases->size() - start);
   return rc;
}

/**
 * Get DHCP scopes from short-lived cache, so DCIs for individual scopes collected at the same time
 * do not cause separate requests to agent or device. Errors are cached as well.
 */
DataCollectionError Node::getCachedDhcpScopes(shared_ptr<ObjectArray<DhcpScopeInfo>> *scopes)
{
   LockGuard lockGuard(m_dhcpScopeCacheLock);
   time_t now = time(nullptr);
   if ((now >= m_dhcpScopeCacheTimestamp + DHCP_SCOPE_CACHE_TTL) || (now < m_dhcpScopeCacheTimestamp))
   {
      auto list = make_shared<ObjectArray<DhcpScopeInfo>>(0, 16, Ownership::True);
      m_dhcpScopeCacheError = getDhcpScopes(list.get());
      if (m_dhcpScopeCacheError == DCE_SUCCESS)
         m_dhcpScopeCache = list;
      else
         m_dhcpScopeCache.reset();
      m_dhcpScopeCacheTimestamp = now;
   }
   *scopes = m_dhcpScopeCache;
   return m_dhcpScopeCacheError;
}

/**
 * Get internal table DHCP.Scopes (same structure as agent's DHCP.Scopes table, but works for any DHCP server
 * source - agent or network device driver)
 */
DataCollectionError Node::getDhcpScopeTable(shared_ptr<Table> *result)
{
   shared_ptr<ObjectArray<DhcpScopeInfo>> scopes;
   DataCollectionError rc = getCachedDhcpScopes(&scopes);
   if (rc != DCE_SUCCESS)
      return rc;

   auto table = make_shared<Table>();
   table->addColumn(L"ID", DCI_DT_STRING, L"ID", true);
   table->addColumn(L"IP_VERSION", DCI_DT_INT, L"IP version");
   table->addColumn(L"NETWORK", DCI_DT_STRING, L"Network");
   table->addColumn(L"NAME", DCI_DT_STRING, L"Name");
   table->addColumn(L"RANGES", DCI_DT_STRING, L"Ranges");
   table->addColumn(L"ENABLED", DCI_DT_INT, L"Enabled");
   table->addColumn(L"TOTAL", DCI_DT_UINT64, L"Total addresses");
   table->addColumn(L"USED", DCI_DT_UINT64, L"Used addresses");
   table->addColumn(L"DECLINED", DCI_DT_UINT64, L"Declined addresses");
   table->addColumn(L"FREE", DCI_DT_UINT64, L"Free addresses");
   table->addColumn(L"UTILIZATION", DCI_DT_FLOAT, L"Utilization");

   for(int i = 0; i < scopes->size(); i++)
   {
      DhcpScopeInfo *scope = scopes->get(i);
      table->addRow();
      table->set(0, scope->id);
      table->set(1, (scope->network.getFamily() == AF_INET6) ? 6 : 4);
      if (scope->network.isValid())
      {
         wchar_t text[64];
         StringBuffer network(scope->network.toString(text));
         network.append(L'/');
         network.append(scope->network.getMaskBits());
         table->set(2, network);
      }
      table->set(3, scope->name);
      table->set(4, scope->ranges);
      table->set(5, scope->enabled ? 1 : 0);
      table->set(6, scope->totalAddresses);
      table->set(7, scope->usedAddresses);
      if (scope->declinedAddressesKnown)
         table->set(8, scope->declinedAddresses);
      table->set(9, scope->freeAddresses);
      table->set(10, (scope->totalAddresses > 0) ? static_cast<double>(scope->usedAddresses) * 100.0 / static_cast<double>(scope->totalAddresses) : 0.0, 2);
   }
   *result = table;
   return DCE_SUCCESS;
}

/**
 * Get internal metric for individual DHCP scope (DHCP.Scope.Total(*), DHCP.Scope.Used(*), DHCP.Scope.Free(*),
 * DHCP.Scope.Declined(*), DHCP.Scope.Utilization(*)). Metric argument is scope ID.
 */
DataCollectionError Node::getDhcpScopeMetric(const wchar_t *name, wchar_t *buffer, size_t size)
{
   static const wchar_t *metrics[] = { L"DHCP.Scope.Total(", L"DHCP.Scope.Used(", L"DHCP.Scope.Free(", L"DHCP.Scope.Declined(", L"DHCP.Scope.Utilization(" };
   int metric = -1;
   for(int i = 0; i < static_cast<int>(sizeof(metrics) / sizeof(metrics[0])); i++)
   {
      if (!wcsnicmp(name, metrics[i], wcslen(metrics[i])))
      {
         metric = i;
         break;
      }
   }
   if (metric == -1)
      return DCE_NOT_SUPPORTED;

   wchar_t scopeId[256];
   if (!AgentGetParameterArgW(name, 1, scopeId, 256) || (scopeId[0] == 0))
      return DCE_NOT_SUPPORTED;

   shared_ptr<ObjectArray<DhcpScopeInfo>> scopes;
   DataCollectionError rc = getCachedDhcpScopes(&scopes);
   if (rc != DCE_SUCCESS)
      return rc;

   DhcpScopeInfo *scope = nullptr;
   for(int i = 0; i < scopes->size(); i++)
   {
      if (!wcsicmp(scopes->get(i)->id, scopeId))
      {
         scope = scopes->get(i);
         break;
      }
   }
   if (scope == nullptr)
      return DCE_NO_SUCH_INSTANCE;

   switch(metric)
   {
      case 0:
         IntegerToString(scope->totalAddresses, buffer);
         break;
      case 1:
         IntegerToString(scope->usedAddresses, buffer);
         break;
      case 2:
         IntegerToString(scope->freeAddresses, buffer);
         break;
      case 3:
         if (!scope->declinedAddressesKnown)
            return DCE_NOT_SUPPORTED;
         IntegerToString(scope->declinedAddresses, buffer);
         break;
      case 4:
         nx_swprintf(buffer, size, L"%.2f", (scope->totalAddresses > 0) ? static_cast<double>(scope->usedAddresses) * 100.0 / static_cast<double>(scope->totalAddresses) : 0.0);
         break;
   }
   return DCE_SUCCESS;
}

/**
 * Check if node is a DHCP server (configuration poll). DHCP information is checked via agent first and then
 * via network device driver. Capability is kept unchanged if neither source could be checked or
 * one of them failed with communication error.
 */
bool Node::confPollDhcpServer(NodeDeviceContext *deviceContext)
{
   const wchar_t *source = nullptr;
   bool definitelyNot = false;
   bool unknown = false;

   if ((m_capabilities & NC_IS_NATIVE_AGENT) && !(m_flags & NF_DISABLE_NXCP))
   {
      shared_ptr<Table> table;
      DataCollectionError rc = getTableFromAgent(L"DHCP.Scopes", &table);
      if (rc == DCE_SUCCESS)
         source = L"agent";
      else if (rc == DCE_COMM_ERROR)
         unknown = true;
      else
         definitelyNot = true;
   }

   if (source == nullptr)
   {
      auto driver = getUsableDriverWithData(deviceContext);
      if (driver.first != nullptr)
      {
         ObjectArray<DhcpScopeInfo> scopes(0, 16, Ownership::True);
         DataCollectionError rc = driver.first->getDhcpScopes(deviceContext, this, driver.second.get(), &scopes);
         if (rc == DCE_SUCCESS)
            source = driver.first->getName();
         else if (rc == DCE_COMM_ERROR)
            unknown = true;
         else
            definitelyNot = true;
      }
   }

   bool hasChanges = false;
   lockProperties();
   if (source != nullptr)
   {
      if (!(m_capabilities & NC_IS_DHCP_SERVER))
         hasChanges = true;
      m_capabilities |= NC_IS_DHCP_SERVER;
      unlockProperties();
      sendPollerMsg(L"   DHCP server information is available from %s\r\n", source);
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, L"ConfPoll(%s): DHCP server information is available from %s", m_name, source);
   }
   else if (definitelyNot && !unknown)
   {
      if (m_capabilities & NC_IS_DHCP_SERVER)
         hasChanges = true;
      m_capabilities &= ~NC_IS_DHCP_SERVER;
      unlockProperties();
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, L"ConfPoll(%s): DHCP server information is not available", m_name);
   }
   else
   {
      unlockProperties();
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, L"ConfPoll(%s): cannot check if node is a DHCP server, capability unchanged", m_name);
   }
   return hasChanges;
}
