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
static uint32_t s_journalRetentionTime = 86400;   // seconds
static uint32_t s_channelPort = 4704;
static uint32_t s_peerPort = 0;      // 0 = same as ChannelPort
static uint32_t s_enableDataCollectionFeed = 1;
static wchar_t s_onPromoteCommand[MAX_PATH] = L"";
static wchar_t s_onDemoteCommand[MAX_PATH] = L"";

static NX_CFG_TEMPLATE s_clusterConfigTemplate[] =
{
   { L"ChannelPort", CT_LONG, 0, 0, 0, 0, &s_channelPort, nullptr },
   { L"ClusterMode", CT_BOOLEAN_FLAG_32, 0, 0, 1, 0, &s_clusterMode, nullptr },
   { L"EnableDataCollectionFeed", CT_BOOLEAN_FLAG_32, 0, 0, 1, 0, &s_enableDataCollectionFeed, nullptr },
   { L"FenceMargin", CT_LONG, 0, 0, 0, 0, &s_fenceMargin, nullptr },
   { L"JournalRetentionTime", CT_LONG, 0, 0, 0, 0, &s_journalRetentionTime, nullptr },
   { L"LeaseRefreshInterval", CT_LONG, 0, 0, 0, 0, &s_leaseRefreshInterval, nullptr },
   { L"LeaseValidity", CT_LONG, 0, 0, 0, 0, &s_leaseValidity, nullptr },
   { L"NodeAddress", CT_STRING, 0, 0, 256, 0, s_nodeAddress, nullptr },
   { L"NodeName", CT_STRING, 0, 0, 64, 0, s_nodeName, nullptr },
   { L"OnDemoteCommand", CT_STRING, 0, 0, MAX_PATH, 0, s_onDemoteCommand, nullptr },
   { L"OnPromoteCommand", CT_STRING, 0, 0, MAX_PATH, 0, s_onPromoteCommand, nullptr },
   { L"PeerAddress", CT_STRING, 0, 0, 256, 0, s_peerAddress, nullptr },
   { L"PeerPort", CT_LONG, 0, 0, 0, 0, &s_peerPort, nullptr },
   { L"", CT_END_OF_LIST, 0, 0, 0, 0, nullptr, nullptr }
};

/**
 * Controller state
 */
static HALeaseManager *s_leaseManager = nullptr;
static std::atomic<bool> s_fenced(false);

/**
 * Activation coordination. The activation thread runs the full phase 2 startup
 * (including ID table rebase and DB writer start) on its own pooled database
 * connections; shutdown must wait for it to finish before tearing down the
 * connection pool, and must not let a promotion start a new activation once
 * shutdown has begun.
 */
static Mutex s_activationLock(MutexType::FAST);
static THREAD s_activationThread = INVALID_THREAD_HANDLE;
static bool s_shutdownStarted = false;

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
 * Get retention time for change journal entries (seconds)
 */
uint32_t NXCORE_EXPORTABLE HAGetJournalRetentionTime()
{
   return s_journalRetentionTime;
}

/**
 * Get lease manager (for status display); can be null when not in cluster mode
 */
HALeaseManager NXCORE_EXPORTABLE *HAGetLeaseManager()
{
   return s_leaseManager;
}

/**
 * Get this node's name ([CLUSTER] NodeName, default local hostname)
 */
const wchar_t *HAGetLocalNodeName()
{
   return s_nodeName;
}

/**
 * Get this node's client-reachable address ([CLUSTER] NodeAddress)
 */
const wchar_t *HAGetLocalNodeAddress()
{
   return s_nodeAddress;
}

/**
 * Ensure cluster member node is bound to the given cluster object. Nodes can
 * only be member of one cluster; a node already placed into a different
 * cluster by the operator is left alone.
 */
static void EnsureClusterMembership(const shared_ptr<Cluster>& cluster, const shared_ptr<Node>& node)
{
   shared_ptr<Cluster> current = node->getCluster();
   if (current == nullptr)
   {
      if (static_cast<Cluster&>(*cluster).addNode(node))
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Node %s [%u] registered as member of server cluster object %s [%u]",
               node->getName(), node->getId(), cluster->getName(), cluster->getId());
      else
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Cannot register node %s [%u] as member of server cluster object %s [%u] (zone mismatch)",
               node->getName(), node->getId(), cluster->getName(), cluster->getId());
   }
   else if (current->getId() != cluster->getId())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Node %s [%u] is a member of another cluster %s [%u], not registering in server cluster object",
            node->getName(), node->getId(), current->getName(), current->getId());
   }
}

/**
 * Maintain the object model representation of the server cluster: a cluster
 * object (identified by the HAClusterObjectId metadata entry, so the operator
 * can rename it freely) with node objects for both members. Runs on the
 * active node at activation (after CheckForMgmtNode) and from the poll
 * manager's periodic check; the peer's member node can only be registered
 * once the peer channel has delivered its identity.
 */
void HAUpdateServerClusterObject()
{
   if (!HAIsClusterMode())
      return;

   // Serialize concurrent runs (activation path vs. HELLO-triggered update)
   static Mutex updateLock(MutexType::FAST);
   LockGuard lockGuard(updateLock);

   shared_ptr<NetObj> object = FindObjectById(MetaDataReadInt32(L"HAClusterObjectId", 0), OBJECT_CLUSTER);
   if (object == nullptr)
   {
      object = make_shared<Cluster>(L"NetXMS Server Cluster", 0);
      NetObjInsert(object, true, false);
      object->setComments(L"NetXMS server high availability cluster (created automatically by server process)");
      NetObj::linkObjects(g_infrastructureServiceRoot, object);
      object->publish();
      MetaDataWriteInt32(L"HAClusterObjectId", object->getId());
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Created server cluster object %s [%u]", object->getName(), object->getId());
   }
   shared_ptr<Cluster> cluster = static_pointer_cast<Cluster>(object);
   g_serverClusterObjectId = cluster->getId();

   // This node's host
   shared_ptr<NetObj> localNode = FindObjectById(g_dwMgmtNode, OBJECT_NODE);
   if (localNode != nullptr)
      EnsureClusterMembership(cluster, static_pointer_cast<Node>(localNode));

   // Peer node's host (identity learned over the peer channel). Check
   // existing members before the address-based lookup: the IP index cannot
   // find nodes by loopback address (single-host test clusters), and the
   // peer's announced address may not match any indexed interface address.
   wchar_t peerName[64], peerAddress[256];
   if (HAChannelGetPeerNodeInfo(peerName, 64, peerAddress, 256))
   {
      unique_ptr<SharedObjectArray<NetObj>> members = cluster->getChildren(OBJECT_NODE);
      for(int i = 0; i < members->size(); i++)
      {
         Node *member = static_cast<Node*>(members->get(i));
         if (!wcsicmp(member->getName(), peerName) || !wcsicmp(member->getPrimaryHostName().cstr(), peerAddress))
            return;   // peer's host already registered
      }

      InetAddress addr = InetAddress::resolveHostName(peerAddress);
      if (addr.isValidUnicast() || addr.isLoopback())   // loopback supports single-host test clusters, same as the channel dialer
      {
         shared_ptr<Node> peerNode = FindNodeByIP(0, addr);
         if (peerNode != nullptr)
         {
            EnsureClusterMembership(cluster, peerNode);
         }
         else
         {
            NewNodeData newNodeData(addr);
            wcslcpy(newNodeData.name, peerName, MAX_OBJECT_NAME);
            newNodeData.cluster = cluster;
            peerNode = PollNewNode(&newNodeData);
            if (peerNode != nullptr)
            {
               peerNode->setComments(L"NetXMS server cluster node (created automatically by server process)");
               nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Created node object %s [%u] for cluster peer at %s",
                     peerNode->getName(), peerNode->getId(), peerAddress);
            }
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"Cannot resolve cluster peer address %s, peer member node not registered", peerAddress);
      }
   }
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
 * Run demote hook command at most once per process incarnation. Every path
 * out of the ACTIVE role ends the process, but more than one of them can
 * pass through here (e.g. graceful switchover runs the hook up front to
 * drop the virtual IP before the drains, then the common shutdown path
 * ends at HAShutdownController).
 */
static void RunDemoteHook()
{
   static std::atomic<bool> executed(false);
   if (!executed.exchange(true))
      RunHookCommand(s_onDemoteCommand, 10000);
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

   // Stop the live journal applier and replay the remaining tail against the
   // warm state (mandatory reconcile - the old active's writes have quiesced).
   // If the warm state cannot be trusted the only safe path is a fresh start:
   // the restarted process re-warms from the current database state and wins
   // the lease again.
   if (!HAChannelFinalizeSync())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Warm state cannot be reconciled with the change journal; restarting to rebuild from database");
      _exit(NETXMSD_EXIT_RESTART_STANDBY);
   }

   // Seed change journal sequence from the journal head and enable journal
   // writes for this node (it is about to become the writing active)
   if (!HAJournalInit())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Change journal initialization failed at activation; restarting into standby");
      _exit(NETXMSD_EXIT_RESTART_STANDBY);
   }

   RunHookCommand(s_onPromoteCommand, 0);

   if (!ActivateServer())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Server activation failed; restarting into standby");
      _exit(NETXMSD_EXIT_RESTART_STANDBY);
   }

   EventBuilder(EVENT_HA_NODE_ACTIVATED, GetServerEventSourceId())
      .param(L"nodeName", s_nodeName)
      .param(L"term", term)
      .post();
}

/**
 * Start the activation thread in response to winning the cluster lease. Skips
 * activation if shutdown has already begun (the process is exiting from
 * standby); otherwise records the thread handle so shutdown can wait for it.
 */
static void StartActivation(int64_t term)
{
   LockGuard lockGuard(s_activationLock);
   if (s_shutdownStarted)
   {
      nxlog_debug_tag(DEBUG_TAG, 2, L"Ignoring cluster promotion (term " INT64_FMTW L"): server shutdown in progress", term);
      return;
   }
   s_activationThread = ThreadCreateEx(ActivationThread, term);
}

/**
 * Block any further activation and wait for an activation already in progress
 * to complete. Called at the very start of shutdown, before the passive/active
 * branch is chosen and before the database connection pool is torn down: a
 * promotion that fires as the server is stopping must not run queries on a
 * connection pool that shutdown is about to destroy (would crash inside libpq),
 * and an activation that does complete must be reflected by s_activationStarted
 * so the full active shutdown path runs.
 */
void HAWaitForActivation()
{
   THREAD thread;
   {
      LockGuard lockGuard(s_activationLock);
      s_shutdownStarted = true;
      thread = s_activationThread;
      s_activationThread = INVALID_THREAD_HANDLE;
   }
   if (thread != INVALID_THREAD_HANDLE)
   {
      nxlog_debug_tag(DEBUG_TAG, 2, L"Waiting for cluster activation to complete before shutdown");
      ThreadJoin(thread);
   }
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
         StartActivation(term);
      });

   s_leaseManager->setFenceHandler(
      [] (const wchar_t *reason) -> void
      {
         s_fenced = true;
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Node fenced (%s); restarting into standby", reason);
         RunDemoteHook();
         nxlog_close();
         _exit(NETXMSD_EXIT_RESTART_STANDBY);
      });

   if (!s_leaseManager->start())
      return false;

   HAChannelConfigure(s_peerAddress, static_cast<uint16_t>(s_channelPort),
         static_cast<uint16_t>((s_peerPort != 0) ? s_peerPort : s_channelPort), s_enableDataCollectionFeed != 0);
   if (!HAChannelStart())
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
   RunDemoteHook();
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
      // This node is leaving the ACTIVE role - release resources claimed by
      // the promote hook (e.g. virtual IP address) so the peer can take them
      // over. No-op if the hook already ran (fence, switchover).
      RunDemoteHook();

      // Bounded wait for the peer to apply the journal up to the head
      // (doc/HA_Design.md 5.3 step 3) so the new active starts with fully
      // reconciled warm state; on timeout proceed anyway - the peer replays
      // the remainder at activation.
      int64_t head = HAJournalGetHead();
      if ((head > 0) && HAChannelIsPeerConnected() && (HAChannelGetPeerWatermark() < head))
      {
         nxlog_debug_tag(DEBUG_TAG, 2, L"Waiting for peer to reach journal head " INT64_FMTW, head);
         for(int i = 0; (i < 300) && (HAChannelGetPeerWatermark() < head) && HAChannelIsPeerConnected(); i++)
            ThreadSleepMs(100);
         if (HAChannelGetPeerWatermark() < head)
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Peer did not reach journal head " INT64_FMTW L" within handover wait (peer watermark " INT64_FMTW L"); peer will reconcile by replay",
                  head, HAChannelGetPeerWatermark());
      }

      HAChannelNotifyDemotion();   // peer polls for acquisition immediately instead of on its next cycle
      nxlog_debug_tag(DEBUG_TAG, 2, L"Releasing cluster lease");
      s_leaseManager->requestRelease();
      for(int i = 0; (i < 50) && (s_leaseManager->getState() == HALeaseState::ACTIVE); i++)
         ThreadSleepMs(100);
   }

   HAChannelShutdown();
   s_leaseManager->stop();
   delete_and_null(s_leaseManager);
   nxlog_debug_tag(DEBUG_TAG, 2, L"Cluster controller stopped");
}
