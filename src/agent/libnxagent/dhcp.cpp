/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** Helpers for generic DHCP server tables shared by all DHCP server subagents.
** Column layout is part of agent-server contract (server parses tables by column name).
**/

#include "libnxagent.h"

/**
 * Create columns for DHCP.Scopes table
 */
void LIBNXAGENT_EXPORTABLE CreateDHCPScopeTableColumns(Table *table)
{
   table->addColumn(_T("ID"), DCI_DT_STRING, _T("ID"), true);
   table->addColumn(_T("IP_VERSION"), DCI_DT_INT, _T("IP version"));
   table->addColumn(_T("NETWORK"), DCI_DT_STRING, _T("Network"));
   table->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"));
   table->addColumn(_T("RANGES"), DCI_DT_STRING, _T("Ranges"));
   table->addColumn(_T("ENABLED"), DCI_DT_INT, _T("Enabled"));
   table->addColumn(_T("TOTAL"), DCI_DT_UINT64, _T("Total"));
   table->addColumn(_T("USED"), DCI_DT_UINT64, _T("Used"));
   table->addColumn(_T("DECLINED"), DCI_DT_UINT64, _T("Declined"));
   table->addColumn(_T("FREE"), DCI_DT_UINT64, _T("Free"));
   table->addColumn(_T("UTILIZATION"), DCI_DT_FLOAT, _T("Utilization"));
}

/**
 * Add row to DHCP.Scopes table with all columns except DECLINED
 */
static void AddScopeRow(Table *table, const TCHAR *id, int ipVersion, const TCHAR *network, const TCHAR *name, const TCHAR *ranges, bool enabled,
      uint64_t total, uint64_t used, uint64_t free)
{
   table->addRow();
   table->set(DHCP_SCOPE_COL_ID, id);
   table->set(DHCP_SCOPE_COL_IP_VERSION, static_cast<int32_t>(ipVersion));
   table->set(DHCP_SCOPE_COL_NETWORK, network);
   table->set(DHCP_SCOPE_COL_NAME, name);
   table->set(DHCP_SCOPE_COL_RANGES, ranges);
   table->set(DHCP_SCOPE_COL_ENABLED, static_cast<int32_t>(enabled ? 1 : 0));
   table->set(DHCP_SCOPE_COL_TOTAL, total);
   table->set(DHCP_SCOPE_COL_USED, used);
   table->set(DHCP_SCOPE_COL_FREE, free);
   table->set(DHCP_SCOPE_COL_UTILIZATION, DHCPScopeUtilization(total, used), 2);
}

/**
 * Add row to DHCP.Scopes table
 */
void LIBNXAGENT_EXPORTABLE AddDHCPScopeTableRow(Table *table, const TCHAR *id, int ipVersion, const TCHAR *network, const TCHAR *name, const TCHAR *ranges, bool enabled,
      uint64_t total, uint64_t used, uint64_t declined)
{
   AddScopeRow(table, id, ipVersion, network, name, ranges, enabled, total, used, DHCPScopeFreeAddresses(total, used, declined));
   table->set(DHCP_SCOPE_COL_DECLINED, declined);
}

/**
 * Add row to DHCP.Scopes table for DHCP server that does not report declined addresses (DECLINED cell is left empty)
 */
void LIBNXAGENT_EXPORTABLE AddDHCPScopeTableRow(Table *table, const TCHAR *id, int ipVersion, const TCHAR *network, const TCHAR *name, const TCHAR *ranges, bool enabled,
      uint64_t total, uint64_t used)
{
   AddScopeRow(table, id, ipVersion, network, name, ranges, enabled, total, used, DHCPScopeFreeAddresses(total, used, 0));
}

/**
 * Create columns for DHCP.Leases table
 */
void LIBNXAGENT_EXPORTABLE CreateDHCPLeaseTableColumns(Table *table)
{
   table->addColumn(_T("IP_ADDRESS"), DCI_DT_STRING, _T("IP address"), true);
   table->addColumn(_T("PREFIX_LENGTH"), DCI_DT_INT, _T("Prefix length"));
   table->addColumn(_T("SCOPE_ID"), DCI_DT_STRING, _T("Scope ID"));
   table->addColumn(_T("MAC_ADDRESS"), DCI_DT_STRING, _T("MAC address"));
   table->addColumn(_T("CLIENT_ID"), DCI_DT_STRING, _T("Client ID"));
   table->addColumn(_T("HOSTNAME"), DCI_DT_STRING, _T("Host name"));
   table->addColumn(_T("TYPE"), DCI_DT_INT, _T("Type"));
   table->addColumn(_T("STATE"), DCI_DT_INT, _T("State"));
   table->addColumn(_T("LAST_TRANSACTION"), DCI_DT_INT64, _T("Last transaction"));
   table->addColumn(_T("EXPIRATION"), DCI_DT_INT64, _T("Expiration"));
}
