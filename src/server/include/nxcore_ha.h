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
** File: nxcore_ha.h
**
**/

#ifndef _nxcore_ha_h_
#define _nxcore_ha_h_

#include <halease.h>

#ifndef NXCORE_EXPORTABLE
#ifdef NXCORE_EXPORTS
#define NXCORE_EXPORTABLE __EXPORT
#else
#define NXCORE_EXPORTABLE __IMPORT
#endif
#endif

/**
 * Exit code requesting restart into standby (service manager must be
 * configured to restart the process; it will come up passive)
 */
#define NETXMSD_EXIT_RESTART_STANDBY   94

/**
 * HA cluster controller (see doc/HA_Design.md). Phase 1: lease-based
 * active/standby roles, passive parking, activation on promotion,
 * demote-via-restart on fencing.
 */
bool HAReadConfiguration();
bool NXCORE_EXPORTABLE HAIsClusterMode();
bool HAStartController();
void HAShutdownController();
bool NXCORE_EXPORTABLE HAIsFenced();
bool NXCORE_EXPORTABLE HACheckFence();
bool NXCORE_EXPORTABLE HAInitiateSwitchover();
HALeaseManager NXCORE_EXPORTABLE *HAGetLeaseManager();
void NXCORE_EXPORTABLE HAGetActiveServerAddress(wchar_t *buffer, size_t size);

/**
 * Set process exit code used by the shutdown path (switchover sets
 * NETXMSD_EXIT_RESTART_STANDBY so the service manager restarts the process
 * into standby)
 */
void NXCORE_EXPORTABLE SetServerExitCode(int exitCode);

/**
 * HA change journal entity type (stored as char(1) digit)
 */
enum class HAJournalEntityType
{
   OBJECT = 0,
   ALARM = 1
};

/**
 * HA change journal change type (stored as char(1) digit)
 */
enum class HAJournalChangeType
{
   CHANGE = 0,
   DELETE = 1
};

/**
 * HA change journal entry (as read by replay)
 */
struct HAJournalEntry
{
   int64_t seq;
   HAJournalEntityType entityType;
   HAJournalChangeType changeType;
   uint32_t entityId;
   int entityClass;
};

/**
 * HA change journal (doc/HA_Design.md section 3). Writes are no-ops outside
 * cluster mode and before HAJournalInit() runs at activation.
 */
bool HAJournalInit();
bool NXCORE_EXPORTABLE HAJournalAppend(DB_HANDLE hdb, HAJournalEntityType entityType, HAJournalChangeType changeType, uint32_t entityId, int entityClass);
void NXCORE_EXPORTABLE HAJournalAppendAsync(HAJournalEntityType entityType, HAJournalChangeType changeType, uint32_t entityId, int entityClass);
int64_t NXCORE_EXPORTABLE HAJournalGetHead();
int64_t HAJournalQueryHead();
void NXCORE_EXPORTABLE HAJournalSaveWatermark(int64_t seq);
int64_t NXCORE_EXPORTABLE HAJournalReadWatermark();
int64_t NXCORE_EXPORTABLE HAJournalGetTruncationPoint();
int64_t NXCORE_EXPORTABLE HAJournalReplay(int64_t watermark, std::function<void(const HAJournalEntry&)> handler);
void HAJournalPrune(DB_HANDLE hdb);
uint32_t NXCORE_EXPORTABLE HAGetJournalRetentionTime();

/**
 * HA cluster peer channel (TLS link between nodes; pure acceleration for
 * journal-based synchronization) and standby-side journal applier.
 */
void HAChannelConfigure(const wchar_t *peerAddress, uint16_t listenPort, uint16_t peerPort);
bool HAChannelStart();
void HAChannelShutdown();
void HAChannelNotifyDemotion();
void HAChannelSetupSync(std::function<void(const std::vector<HAJournalEntry>&)> handler, int64_t initialWatermark);
bool HAChannelFinalizeSync();
bool NXCORE_EXPORTABLE HAChannelIsPeerConnected();
int64_t NXCORE_EXPORTABLE HAChannelGetPeerWatermark();
int64_t HAChannelGetAppliedWatermark();

/**
 * Warm standby state synchronization: journal entry application to the
 * in-memory object model and alarm list (hasync.cpp)
 */
void HASyncApplyJournalBatch(const std::vector<HAJournalEntry>& entries);
void HASyncSetDirty(const wchar_t *reason);
bool HASyncIsDirty();

/**
 * Phase 2 of server startup (activation): everything beyond passive
 * bring-up. Called directly in standalone mode, or by the HA controller
 * when this node wins the cluster lease.
 */
bool ActivateServer();

#endif   /* _nxcore_ha_h_ */
