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
** File: hajournal.cpp
**
** HA change journal (doc/HA_Design.md section 3). The active node appends a
** row for every object or alarm mutation the standby must know about; the
** standby persists its contiguous applied watermark and replays the journal
** from it at activation or after a channel reconnect. The peer channel is
** pure acceleration - correctness always comes from the journal.
**
**/

#include "nxcore.h"
#include <nxcore_ha.h>

#define DEBUG_TAG L"ha.journal"

/**
 * Metadata entry recording the highest pruned-by-age sequence number. A
 * standby whose watermark is below this cannot fill the gap by replay and
 * must discard its warm state (restart into a fresh standby).
 */
#define TRUNCATION_METADATA_ENTRY L"HAJournalTruncatedAt"

/**
 * Journal sequence counter; seeded from max(seq) at activation
 */
static std::atomic<int64_t> s_sequence(0);

/**
 * True once this node has initialized the journal writer (activation)
 */
static std::atomic<bool> s_writerActive(false);

/**
 * Initialize journal writer: seed the sequence counter from the journal head
 * in the database. Part of the activation-time ID allocator rebase - the same
 * max()-idempotent pattern as InitIdTable().
 */
bool HAJournalInit()
{
   if (!HAIsClusterMode())
      return true;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, L"SELECT max(seq) FROM ha_change_journal");
   if (hResult == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Cannot read change journal head");
      return false;
   }
   int64_t head = (DBGetNumRows(hResult) > 0) ? DBGetFieldInt64(hResult, 0, 0) : 0;
   DBFreeResult(hResult);
   DBConnectionPoolReleaseConnection(hdb);

   int64_t current = s_sequence.load();
   while((current < head) && !s_sequence.compare_exchange_weak(current, head))
      ;
   s_writerActive = true;
   nxlog_debug_tag(DEBUG_TAG, 2, L"Change journal writer initialized (head " INT64_FMTW L")", s_sequence.load());
   return true;
}

/**
 * Append journal entry on the given database connection (same transaction as
 * the entity write when the caller runs one). Returns false on SQL failure so
 * the caller can roll the whole transaction back - an entity change must not
 * commit without its journal entry. No-op outside cluster mode; a fenced node
 * refuses the append (and thereby fails the enclosing transaction).
 */
bool HAJournalAppend(DB_HANDLE hdb, HAJournalEntityType entityType, HAJournalChangeType changeType, uint32_t entityId, int entityClass)
{
   if (!s_writerActive.load())
      return true;
   if (HAIsFenced())
      return false;

   wchar_t query[256];
   nx_swprintf(query, 256,
      L"INSERT INTO ha_change_journal (seq,entity_type,change_type,entity_id,entity_class,created_at) VALUES (?,'%c','%c',?,?,%s)",
      L'0' + static_cast<int>(entityType), L'0' + static_cast<int>(changeType), HAGetDbTimeExpression(g_dbSyntax));

   DB_STATEMENT hStmt = DBPrepare(hdb, query);
   if (hStmt == nullptr)
      return false;
   DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, ++s_sequence);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, entityId);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, entityClass);
   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Append journal entry through the lazy SQL writer queue. For entity deletes
 * issued via QueueSQLRequest (queue order preserves delete-then-journal
 * sequencing on the same serialized writer).
 */
void HAJournalAppendAsync(HAJournalEntityType entityType, HAJournalChangeType changeType, uint32_t entityId, int entityClass)
{
   if (!s_writerActive.load() || HAIsFenced())
      return;

   wchar_t query[256];
   nx_swprintf(query, 256,
      L"INSERT INTO ha_change_journal (seq,entity_type,change_type,entity_id,entity_class,created_at) VALUES (" INT64_FMTW L",'%c','%c',%u,%d,%s)",
      static_cast<int64_t>(++s_sequence), L'0' + static_cast<int>(entityType), L'0' + static_cast<int>(changeType),
      entityId, entityClass, HAGetDbTimeExpression(g_dbSyntax));
   QueueSQLRequest(query);
}

/**
 * Get current journal head (last allocated sequence number)
 */
int64_t HAJournalGetHead()
{
   return s_sequence.load();
}

/**
 * Persist this node's contiguous applied watermark (called by the standby
 * sync applier; writing the node's own sync row is not role-sensitive work)
 */
void HAJournalSaveWatermark(int64_t seq)
{
   HALeaseManager *manager = HAGetLeaseManager();
   if (manager == nullptr)
      return;

   wchar_t guidText[64];
   manager->getNodeGuid().toString(guidText);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   wchar_t query[256];
   bool exists = IsDatabaseRecordExist(hdb, L"ha_sync_state", L"node_guid", guidText);
   if (exists)
      nx_swprintf(query, 256, L"UPDATE ha_sync_state SET applied_seq=" INT64_FMTW L",updated_at=%s WHERE node_guid='%s'",
            seq, HAGetDbTimeExpression(g_dbSyntax), guidText);
   else
      nx_swprintf(query, 256, L"INSERT INTO ha_sync_state (node_guid,applied_seq,updated_at) VALUES ('%s'," INT64_FMTW L",%s)",
            guidText, seq, HAGetDbTimeExpression(g_dbSyntax));
   if (!DBQuery(hdb, query))
      nxlog_debug_tag(DEBUG_TAG, 5, L"Cannot persist journal watermark " INT64_FMTW, seq);
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Read this node's persisted watermark (replay starting point); 0 if none
 */
int64_t HAJournalReadWatermark()
{
   HALeaseManager *manager = HAGetLeaseManager();
   if (manager == nullptr)
      return 0;

   wchar_t guidText[64];
   manager->getNodeGuid().toString(guidText);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   wchar_t query[128];
   nx_swprintf(query, 128, L"SELECT applied_seq FROM ha_sync_state WHERE node_guid='%s'", guidText);
   int64_t watermark = 0;
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         watermark = DBGetFieldInt64(hResult, 0, 0);
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return watermark;
}

/**
 * Get sequence number of the last age-based journal truncation (0 if never
 * truncated). A watermark below this value cannot be reconciled by replay.
 */
int64_t HAJournalGetTruncationPoint()
{
   wchar_t buffer[64];
   if (!MetaDataReadStr(TRUNCATION_METADATA_ENTRY, buffer, 64, L"0"))
      return 0;
   return wcstoll(buffer, nullptr, 10);
}

/**
 * Replay journal entries above the given watermark in sequence order,
 * invoking the callback for each entry (the same handler the live channel
 * applier uses). Returns the new watermark (last replayed seq), or the
 * passed watermark if nothing to replay / on failure.
 */
int64_t HAJournalReplay(int64_t watermark, std::function<void(const HAJournalEntry&)> handler)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT seq,entity_type,change_type,entity_id,entity_class FROM ha_change_journal WHERE seq>? ORDER BY seq");
   if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return watermark;
   }
   DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, watermark);

   int count = 0;
   DB_UNBUFFERED_RESULT hResult = DBSelectPreparedUnbuffered(hStmt);
   if (hResult != nullptr)
   {
      while(DBFetch(hResult))
      {
         HAJournalEntry entry;
         entry.seq = DBGetFieldInt64(hResult, 0);
         wchar_t flag[2];
         DBGetField(hResult, 1, flag, 2);
         entry.entityType = static_cast<HAJournalEntityType>(flag[0] - L'0');
         DBGetField(hResult, 2, flag, 2);
         entry.changeType = static_cast<HAJournalChangeType>(flag[0] - L'0');
         entry.entityId = DBGetFieldUInt32(hResult, 3);
         entry.entityClass = DBGetFieldInt32(hResult, 4);
         handler(entry);
         watermark = entry.seq;
         count++;
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);

   nxlog_debug_tag(DEBUG_TAG, 3, L"Journal replay completed (%d entries, watermark " INT64_FMTW L")", count, watermark);
   return watermark;
}

/**
 * Prune applied journal entries (housekeeper, active node only). Entries at
 * or below the slowest live peer's watermark are deleted; when no peer has
 * reported within the retention window, entries are pruned by age and the
 * truncation point is recorded so a returning standby knows its warm state
 * is unreconcilable by replay.
 */
void HAJournalPrune(DB_HANDLE hdb)
{
   if (!s_writerActive.load())
      return;

   HALeaseManager *manager = HAGetLeaseManager();
   if (manager == nullptr)
      return;

   uint32_t retentionTime = HAGetJournalRetentionTime();
   wchar_t ownGuid[64];
   manager->getNodeGuid().toString(ownGuid);

   // Slowest watermark among peers that reported within the retention window
   // (count(*) distinguishes "no live peer" from a NULL aggregate)
   wchar_t query[512];
   nx_swprintf(query, 512, L"SELECT count(*),min(applied_seq) FROM ha_sync_state WHERE node_guid<>'%s' AND updated_at>%s-%u",
         ownGuid, HAGetDbTimeExpression(g_dbSyntax), retentionTime);
   int64_t pruneTo = -1;
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      if ((DBGetNumRows(hResult) > 0) && (DBGetFieldInt32(hResult, 0, 0) > 0))
         pruneTo = DBGetFieldInt64(hResult, 0, 1);
      DBFreeResult(hResult);
   }

   if (pruneTo >= 0)
   {
      nx_swprintf(query, 512, L"DELETE FROM ha_change_journal WHERE seq<=" INT64_FMTW, pruneTo);
      if (DBQuery(hdb, query))
         nxlog_debug_tag(DEBUG_TAG, 4, L"Change journal pruned up to peer watermark " INT64_FMTW, pruneTo);
      return;
   }

   // No live peer: age-based fallback with truncation marker
   nx_swprintf(query, 512, L"SELECT count(*),max(seq) FROM ha_change_journal WHERE created_at<%s-%u",
         HAGetDbTimeExpression(g_dbSyntax), retentionTime);
   int64_t truncateTo = 0;
   hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      if ((DBGetNumRows(hResult) > 0) && (DBGetFieldInt32(hResult, 0, 0) > 0))
         truncateTo = DBGetFieldInt64(hResult, 0, 1);
      DBFreeResult(hResult);
   }
   if (truncateTo > 0)
   {
      wchar_t buffer[32];
      IntegerToString(truncateTo, buffer);
      MetaDataWriteStr(TRUNCATION_METADATA_ENTRY, buffer);
      nx_swprintf(query, 512, L"DELETE FROM ha_change_journal WHERE seq<=" INT64_FMTW, truncateTo);
      if (DBQuery(hdb, query))
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Change journal truncated by age up to " INT64_FMTW L" (no live peer within retention window); an absent standby must rebuild its warm state", truncateTo);
   }
}
