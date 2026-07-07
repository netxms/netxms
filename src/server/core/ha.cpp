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
** File: ha.cpp
**
**/

#include "nxcore.h"
#include <nxcore_ha.h>
#include <nxproc.h>
#include <netxmsdb.h>

#define DEBUG_TAG L"ha"

extern Config g_serverConfig;

void ReseedCertificateActionLogRecordId();

/**
 * Cluster configuration ([CLUSTER] section of server configuration file)
 */
static uint32_t s_clusterMode = 0;
static wchar_t s_nodeName[64] = L"";
static wchar_t s_nodeAddress[256] = L"";     // this node's address as reachable by clients/agents; published in the lease when active
static wchar_t s_peerAddress[256] = L"";     // reserved for the phase 2 cluster channel
static uint32_t s_leaseRefreshInterval = 5;
static uint32_t s_leaseValidity = 20;
static uint32_t s_fenceMargin = 3;
static wchar_t s_onPromoteCommand[MAX_PATH] = L"";
static wchar_t s_onDemoteCommand[MAX_PATH] = L"";

static NX_CFG_TEMPLATE s_clusterConfigTemplate[] =
{
   { L"ClusterMode", CT_BOOLEAN_FLAG_32, 0, 0, 1, 0, &s_clusterMode, nullptr },
   { L"FenceMargin", CT_LONG, 0, 0, 0, 0, &s_fenceMargin, nullptr },
   { L"LeaseRefreshInterval", CT_LONG, 0, 0, 0, 0, &s_leaseRefreshInterval, nullptr },
   { L"LeaseValidity", CT_LONG, 0, 0, 0, 0, &s_leaseValidity, nullptr },
   { L"NodeAddress", CT_STRING, 0, 0, 256, 0, s_nodeAddress, nullptr },
   { L"NodeName", CT_STRING, 0, 0, 64, 0, s_nodeName, nullptr },
   { L"OnDemoteCommand", CT_STRING, 0, 0, MAX_PATH, 0, s_onDemoteCommand, nullptr },
   { L"OnPromoteCommand", CT_STRING, 0, 0, MAX_PATH, 0, s_onPromoteCommand, nullptr },
   { L"PeerAddress", CT_STRING, 0, 0, 256, 0, s_peerAddress, nullptr },
   { L"", CT_END_OF_LIST, 0, 0, 0, 0, nullptr, nullptr }
};

/**
 * Controller state
 */
static HALeaseManager *s_leaseManager = nullptr;
static std::atomic<bool> s_fenced(false);

/**
 * Read cluster configuration. Called before database lock acquisition -
 * lock semantics depend on cluster mode.
 */
bool HAReadConfiguration()
{
   if (!g_serverConfig.parseTemplate(L"CLUSTER", s_clusterConfigTemplate))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Error parsing CLUSTER section of server configuration file");
      return false;
   }

   if (!s_clusterMode)
      return true;

   if (g_dbSyntax == DB_SYNTAX_SQLITE)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Cluster mode is not supported with SQLite database (an embedded per-process database cannot be a shared arbiter)");
      return false;
   }

   // Enforce lease timing constraints (see doc/HA_Design.md section 2.7)
   if ((s_leaseRefreshInterval < 1) || (s_leaseValidity < s_leaseRefreshInterval * 3) ||
       (s_fenceMargin < 1) || (s_fenceMargin > s_leaseValidity - s_leaseRefreshInterval * 2))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG,
            L"Invalid cluster lease timing (refresh %u s, validity %u s, fence margin %u s); required: refresh >= 1, validity >= 3 * refresh, 1 <= margin <= validity - 2 * refresh",
            s_leaseRefreshInterval, s_leaseValidity, s_fenceMargin);
      return false;
   }

   if (s_nodeName[0] == 0)
      GetLocalHostName(s_nodeName, 64, false);
   if (s_nodeAddress[0] == 0)
      GetLocalHostName(s_nodeAddress, 256, true);

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Cluster mode enabled (node name %s, lease refresh %u s, validity %u s, fence margin %u s)",
         s_nodeName, s_leaseRefreshInterval, s_leaseValidity, s_fenceMargin);
   return true;
}

/**
 * Check if cluster mode is enabled
 */
bool NXCORE_EXPORTABLE HAIsClusterMode()
{
   return s_clusterMode != 0;
}

/**
 * Check if this node has been fenced
 */
bool NXCORE_EXPORTABLE HAIsFenced()
{
   return s_fenced.load();
}

/**
 * Fence check for role-sensitive dispatch points (DB writer dequeue, event
 * processing, poller dispatch, scheduler task launch, notification send).
 * Opportunistically enforces the fence deadline - a process resuming from a
 * pause longer than the lease validity fences here before dispatching any
 * new work, even before the HA watchdog gets scheduled. Returns true if the
 * caller must not start the work item.
 */
bool NXCORE_EXPORTABLE HACheckFence()
{
   if (s_leaseManager != nullptr)
      s_leaseManager->checkFenceDeadline();
   return s_fenced.load();
}

/**
 * Get lease manager (for status display); can be null when not in cluster mode
 */
HALeaseManager NXCORE_EXPORTABLE *HAGetLeaseManager()
{
   return s_leaseManager;
}

/**
 * Get client-reachable address of the current lease holder as published in the
 * lease record (used by standby nodes to redirect clients and agents). Buffer
 * is set to empty string if unknown.
 */
void NXCORE_EXPORTABLE HAGetActiveServerAddress(wchar_t *buffer, size_t size)
{
   if (s_leaseManager != nullptr)
   {
      HALeaseStatus status = s_leaseManager->getStatus();
      wcslcpy(buffer, status.holderAddress, size);
   }
   else
   {
      buffer[0] = 0;
   }
}

/**
 * Run role change hook command (OnPromoteCommand / OnDemoteCommand)
 */
static bool RunHookCommand(const wchar_t *command, uint32_t waitTime)
{
   if (command[0] == 0)
      return true;

   nxlog_debug_tag(DEBUG_TAG, 3, L"Executing role change hook command \"%s\"", command);
   ProcessExecutor executor(command, true);
   if (!executor.execute())
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Failed to execute role change hook command \"%s\"", command);
      return false;
   }
   if (waitTime > 0)
      executor.waitForCompletion(waitTime);
   return true;
}

/**
 * Server activation thread (runs the full phase 2 startup after this node
 * wins the cluster lease)
 */
static void ActivationThread(int64_t term)
{
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Cluster lease acquired (term " INT64_FMTW L"), activating server", term);

   // Schema guard: refuse activation if database schema no longer matches
   // this binary (rolling upgrade in progress)
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   int32_t major, minor;
   bool schemaOk = DBGetSchemaVersion(hdb, &major, &minor);
   DBConnectionPoolReleaseConnection(hdb);
   if (!schemaOk || (major != DB_SCHEMA_VERSION_MAJOR) || (minor != DB_SCHEMA_VERSION_MINOR))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Database schema version mismatch at activation (database %d.%d, server expects %d.%d); restarting into standby",
            major, minor, DB_SCHEMA_VERSION_MAJOR, DB_SCHEMA_VERSION_MINOR);
      _exit(NETXMSD_EXIT_RESTART_STANDBY);
   }

   // Rebase in-memory ID allocators seeded during passive bring-up - they are
   // stale by however long this node was standing by (doc/HA_Design.md 7.2).
   // InitIdTable() takes max(current, database maximum) for every group and
   // re-reads the last event ID, so re-running it is safe.
   if (!InitIdTable())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"ID allocator rebase failed at activation; restarting into standby");
      _exit(NETXMSD_EXIT_RESTART_STANDBY);
   }
   ReseedCertificateActionLogRecordId();

   RunHookCommand(s_onPromoteCommand, 0);

   if (!ActivateServer())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Server activation failed; restarting into standby");
      _exit(NETXMSD_EXIT_RESTART_STANDBY);
   }

   EventBuilder(EVENT_HA_NODE_ACTIVATED, g_dwMgmtNode)
      .param(L"nodeName", s_nodeName)
      .param(L"term", term)
      .post();
}

/**
 * Start cluster controller (end of passive bring-up)
 */
bool HAStartController()
{
   // Bind this node to the database's cluster identity (server ID). A node
   // mistakenly pointed at another cluster's database would otherwise join
   // that cluster's lease competition - and could win it - while its own
   // cluster silently loses redundancy. The binding is created on first
   // start; deliberate re-homing (database migration) is done by removing
   // the binding file.
   wchar_t bindingPath[MAX_PATH];
   nx_swprintf(bindingPath, MAX_PATH, L"%s" FS_PATH_SEPARATOR L"ha-cluster.id", g_netxmsdDataDir);

   wchar_t currentId[32];
   nx_swprintf(currentId, 32, UINT64X_FMT(L"016"), g_serverId);

   size_t size;
   BYTE *content = LoadFile(bindingPath, &size);
   if (content != nullptr)
   {
      char text[32];
      strlcpy(text, reinterpret_cast<char*>(content), std::min(size + 1, sizeof(text)));
      MemFree(content);
      TrimA(text);
      wchar_t storedId[32];
      mb_to_wchar(text, -1, storedId, 32);
      if (wcscmp(storedId, currentId))
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Database belongs to a different cluster (database server ID %s, this node is bound to cluster %s); remove %s to re-home this node",
               currentId, storedId, bindingPath);
         return false;
      }
   }
   else
   {
      char text[32];
      wchar_to_mb(currentId, -1, text, 32);
      if (SaveFile(bindingPath, text, strlen(text), false) != SaveFileStatus::SUCCESS)
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Cannot save cluster binding to %s", bindingPath);
         return false;
      }
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Node bound to cluster with server ID %s", currentId);
   }

   // Node identity: persistent GUID in the data directory
   wchar_t path[MAX_PATH];
   nx_swprintf(path, MAX_PATH, L"%s" FS_PATH_SEPARATOR L"ha-node.guid", g_netxmsdDataDir);

   uuid nodeGuid;
   content = LoadFile(path, &size);
   if (content != nullptr)
   {
      char text[64];
      strlcpy(text, reinterpret_cast<char*>(content), std::min(size + 1, sizeof(text)));
      MemFree(content);
      TrimA(text);
      nodeGuid = uuid::parseA(text);
   }
   if (nodeGuid.isNull())
   {
      nodeGuid = uuid::generate();
      char text[64];
      nodeGuid.toStringA(text);
      if (SaveFile(path, text, strlen(text), false) != SaveFileStatus::SUCCESS)
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Cannot save cluster node GUID to %s", path);
         return false;
      }
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Generated new cluster node GUID %s", nodeGuid.toString().cstr());
   }

   s_leaseManager = new HALeaseManager(nodeGuid, s_nodeName,
      [] () -> DB_HANDLE
      {
         wchar_t errorText[DBDRV_MAX_ERROR_TEXT];
         return DBConnect(g_dbDriver, g_szDbServer, g_szDbName, g_szDbLogin, g_szDbPassword, g_szDbSchema, errorText);
      },
      s_leaseRefreshInterval, s_leaseValidity, s_fenceMargin);

   s_leaseManager->setNodeAddress(s_nodeAddress);

   s_leaseManager->setPromotionHandler(
      [] (int64_t term) -> void
      {
         ThreadCreateEx(ActivationThread, term);
      });

   s_leaseManager->setFenceHandler(
      [] (const wchar_t *reason) -> void
      {
         s_fenced = true;
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Node fenced (%s); restarting into standby", reason);
         RunHookCommand(s_onDemoteCommand, 10000);
         nxlog_close();
         _exit(NETXMSD_EXIT_RESTART_STANDBY);
      });

   if (!s_leaseManager->start())
      return false;

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Cluster controller started (node %s, GUID %s)", s_nodeName, nodeGuid.toString().cstr());
   return true;
}

/**
 * Initiate graceful switchover: orderly shutdown with lease release (the
 * shutdown path drains all queues, then HAShutdownController releases the
 * lease so the peer promotes immediately), then restart into standby via
 * the service manager.
 */
bool NXCORE_EXPORTABLE HAInitiateSwitchover()
{
   if ((s_leaseManager == nullptr) || (s_leaseManager->getState() != HALeaseState::ACTIVE))
      return false;

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Graceful switchover initiated");
   RunHookCommand(s_onDemoteCommand, 10000);
   SetServerExitCode(NETXMSD_EXIT_RESTART_STANDBY);
   InitiateShutdown(ShutdownReason::FROM_REMOTE_CONSOLE);
   return true;
}

/**
 * Shut down cluster controller. On clean shutdown of the active node the
 * lease is released so the peer can promote immediately instead of waiting
 * out the validity window. Callers must complete all drains (DB writer
 * queues etc.) before calling this.
 */
void HAShutdownController()
{
   if (s_leaseManager == nullptr)
      return;

   if (s_leaseManager->getState() == HALeaseState::ACTIVE)
   {
      nxlog_debug_tag(DEBUG_TAG, 2, L"Releasing cluster lease");
      s_leaseManager->requestRelease();
      for(int i = 0; (i < 50) && (s_leaseManager->getState() == HALeaseState::ACTIVE); i++)
         ThreadSleepMs(100);
   }

   s_leaseManager->stop();
   delete_and_null(s_leaseManager);
   nxlog_debug_tag(DEBUG_TAG, 2, L"Cluster controller stopped");
}
