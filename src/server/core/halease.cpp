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
** File: halease.cpp
**
**/

#include <halease.h>

#define DEBUG_TAG L"ha.lease"

/**
 * Get expression for "epoch seconds now" evaluated on the database server.
 * Returns nullptr for syntaxes not supported in cluster mode (an embedded
 * per-process database cannot be a shared arbiter).
 */
static const wchar_t *GetDbTimeExpression(int syntax)
{
   switch(syntax)
   {
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_TSDB:
         return L"CAST(EXTRACT(EPOCH FROM now()) AS bigint)";
      case DB_SYNTAX_MYSQL:
         return L"UNIX_TIMESTAMP()";
      case DB_SYNTAX_ORACLE:
         return L"TRUNC((CAST(SYS_EXTRACT_UTC(SYSTIMESTAMP) AS DATE) - DATE '1970-01-01') * 86400)";
      case DB_SYNTAX_MSSQL:
         return L"DATEDIFF_BIG(SECOND, '19700101', SYSUTCDATETIME())";   // requires SQL Server 2016+
      case DB_SYNTAX_DB2:
         // 719163 = DAYS('1970-01-01'); subtracting CURRENT TIMEZONE converts local timestamp to UTC
         return L"(DAYS(CURRENT TIMESTAMP - CURRENT TIMEZONE) - 719163) * 86400 + MIDNIGHT_SECONDS(CURRENT TIMESTAMP - CURRENT TIMEZONE)";
      default:
         return nullptr;
   }
}

/**
 * Create lease manager. Timings are in seconds; see doc/HA_Design.md section 2.7
 * for constraints (validity >= 3 * refreshInterval + statement time, etc.).
 */
HALeaseManager::HALeaseManager(const uuid& nodeGuid, const wchar_t *nodeName, std::function<DB_HANDLE()> connector,
      uint32_t refreshInterval, uint32_t validity, uint32_t fenceMargin) : m_nodeGuid(nodeGuid), m_connector(connector),
      m_state(static_cast<int>(HALeaseState::INIT)), m_fenceDeadline(0), m_term(0), m_releaseRequested(false),
      m_shutdown(false), m_statusLock(MutexType::FAST), m_wakeupCondition(false), m_watchdogWakeup(false)
{
   wcslcpy(m_nodeName, nodeName, 64);
   m_nodeAddress[0] = 0;
   do
   {
      GenerateRandomBytes(reinterpret_cast<BYTE*>(&m_incarnation), sizeof(m_incarnation));
   } while (m_incarnation == 0);
   m_refreshInterval = refreshInterval;
   m_validity = validity;
   m_fenceMargin = fenceMargin;
   m_hdb = nullptr;
   m_dbSyntax = DB_SYNTAX_UNKNOWN;
   m_dbTimeExpression = nullptr;
   m_thread = INVALID_THREAD_HANDLE;
   m_watchdogThread = INVALID_THREAD_HANDLE;
   memset(&m_status, 0, sizeof(m_status));
   m_status.state = HALeaseState::INIT;
}

/**
 * Destructor
 */
HALeaseManager::~HALeaseManager()
{
   stop();
}

/**
 * Start manager thread
 */
bool HALeaseManager::start()
{
   if ((m_thread != INVALID_THREAD_HANDLE) || m_shutdown.load())
      return false;
   nxlog_debug_tag(DEBUG_TAG, 2, L"Starting HA lease manager (node %s, incarnation " UINT64X_FMT(L"016") L", refresh %us, validity %us, fence margin %us)",
         m_nodeGuid.toString().cstr(), m_incarnation, m_refreshInterval, m_validity, m_fenceMargin);
   m_thread = ThreadCreateEx(this, &HALeaseManager::mainLoop);
   m_watchdogThread = ThreadCreateEx(this, &HALeaseManager::watchdogLoop);
   return true;
}

/**
 * Stop manager thread. Does not release the lease: graceful handover has to be
 * requested explicitly (requestRelease) before stopping; on abrupt stop the
 * lease is left to expire.
 */
void HALeaseManager::stop()
{
   m_shutdown = true;
   m_wakeupCondition.set();
   m_watchdogWakeup.set();
   if (m_thread != INVALID_THREAD_HANDLE)
   {
      ThreadJoin(m_thread);
      m_thread = INVALID_THREAD_HANDLE;
   }
   if (m_watchdogThread != INVALID_THREAD_HANDLE)
   {
      ThreadJoin(m_watchdogThread);
      m_watchdogThread = INVALID_THREAD_HANDLE;
   }
   disconnect();
   int expected = static_cast<int>(HALeaseState::INIT);
   m_state.compare_exchange_strong(expected, static_cast<int>(HALeaseState::STOPPED));
   expected = static_cast<int>(HALeaseState::STANDBY);
   m_state.compare_exchange_strong(expected, static_cast<int>(HALeaseState::STOPPED));
   expected = static_cast<int>(HALeaseState::ACTIVE);
   m_state.compare_exchange_strong(expected, static_cast<int>(HALeaseState::STOPPED));
}

/**
 * Request graceful lease release (executed asynchronously by the manager thread)
 */
void HALeaseManager::requestRelease()
{
   m_releaseRequested = true;
   m_wakeupCondition.set();
}

/**
 * Establish dedicated database connection and prepare lease queries
 */
bool HALeaseManager::connect()
{
   m_hdb = (m_connector != nullptr) ? m_connector() : nullptr;
   if (m_hdb == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Cannot establish dedicated database connection");
      return false;
   }

   m_dbSyntax = DBGetSyntax(m_hdb);
   m_dbTimeExpression = GetDbTimeExpression(m_dbSyntax);
   if (m_dbTimeExpression == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Database syntax %d is not supported in cluster mode", m_dbSyntax);
      disconnect();
      m_state = static_cast<int>(HALeaseState::STOPPED);
      return false;
   }

   // Bound lease statement execution time where the backend supports it; the
   // monotonic fence deadline remains the authoritative bound in all cases
   if ((m_dbSyntax == DB_SYNTAX_PGSQL) || (m_dbSyntax == DB_SYNTAX_TSDB))
   {
      wchar_t query[64];
      nx_swprintf(query, 64, L"SET statement_timeout=%u", m_refreshInterval * 1000);
      DBQuery(m_hdb, query);
   }

   buildQueries();
   nxlog_debug_tag(DEBUG_TAG, 4, L"Dedicated database connection established (syntax %d)", m_dbSyntax);
   return true;
}

/**
 * Drop dedicated database connection
 */
void HALeaseManager::disconnect()
{
   if (m_hdb != nullptr)
   {
      DBDisconnect(m_hdb);
      m_hdb = nullptr;
   }
}

/**
 * Build lease SQL statements with DB time expression and validity embedded
 */
bool HALeaseManager::buildQueries()
{
   StringBuffer sb(L"UPDATE ha_lease SET term=term+1,holder_guid=?,holder_incarnation=?,holder_name=?,holder_address=?,acquired_at=");
   sb.append(m_dbTimeExpression);
   sb.append(L",expires_at=");
   sb.append(m_dbTimeExpression);
   sb.append(L"+");
   sb.append(m_validity);
   sb.append(L" WHERE lease_id=1 AND expires_at<");
   sb.append(m_dbTimeExpression);
   m_acquireQuery = sb;

   sb = L"UPDATE ha_lease SET expires_at=";
   sb.append(m_dbTimeExpression);
   sb.append(L"+");
   sb.append(m_validity);
   sb.append(L" WHERE lease_id=1 AND term=? AND holder_guid=? AND holder_incarnation=?");
   m_refreshQuery = sb;

   m_releaseQuery = L"UPDATE ha_lease SET expires_at=0 WHERE lease_id=1 AND term=? AND holder_guid=? AND holder_incarnation=?";

   sb = L"SELECT term,holder_guid,holder_incarnation,holder_name,expires_at-(";
   sb.append(m_dbTimeExpression);
   sb.append(L"),holder_address FROM ha_lease WHERE lease_id=1");
   m_pollQuery = sb;

   return true;
}

/**
 * Update cached status snapshot
 */
void HALeaseManager::updateStatus(int64_t term, const uuid& holderGuid, const wchar_t *holderName, const wchar_t *holderAddress, int64_t remainingValidity)
{
   LockGuard lockGuard(m_statusLock);
   m_status.state = getState();
   m_status.term = term;
   m_status.holderGuid = holderGuid;
   wcslcpy(m_status.holderName, CHECK_NULL_EX_W(holderName), 64);
   wcslcpy(m_status.holderAddress, CHECK_NULL_EX_W(holderAddress), 256);
   m_status.remainingValidity = remainingValidity;
   m_status.lastUpdate = time(nullptr);
}

/**
 * Get cached status snapshot
 */
HALeaseStatus HALeaseManager::getStatus()
{
   LockGuard lockGuard(m_statusLock);
   m_status.state = getState();   // state may have changed since last table read
   return m_status;
}

/**
 * Read lease table and update cached status. Returns remaining validity in
 * seconds as computed by the database (negative if expired), or INT64_MIN on
 * query failure.
 */
int64_t HALeaseManager::poll()
{
   DB_RESULT hResult = DBSelect(m_hdb, m_pollQuery);
   if (hResult == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Lease table read failed, dropping database connection");
      disconnect();
      return INT64_MIN;
   }

   if (DBGetNumRows(hResult) == 0)
   {
      DBFreeResult(hResult);
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Lease table is empty (missing seed row) - database is not initialized correctly");
      return INT64_MIN;
   }

   int64_t term = DBGetFieldInt64(hResult, 0, 0);
   uuid holderGuid = DBGetFieldGUID(hResult, 0, 1);
   wchar_t holderName[64];
   DBGetField(hResult, 0, 3, holderName, 64);
   int64_t remainingValidity = DBGetFieldInt64(hResult, 0, 4);
   wchar_t holderAddress[256];
   DBGetField(hResult, 0, 5, holderAddress, 256);
   DBFreeResult(hResult);

   updateStatus(term, holderGuid, holderName, holderAddress, remainingValidity);
   return remainingValidity;
}

/**
 * Attempt lease acquisition. libnxdb does not expose affected row counts, so the
 * conditional UPDATE is followed by a verification read: only this process can
 * write its own GUID + incarnation, so reading them back proves the update applied.
 */
void HALeaseManager::tryAcquire()
{
   int64_t sendTime = GetMonotonicClockTime();

   DB_STATEMENT hStmt = DBPrepare(m_hdb, m_acquireQuery);
   if (hStmt == nullptr)
   {
      disconnect();
      return;
   }

   wchar_t guidText[64];
   m_nodeGuid.toString(guidText);
   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guidText, DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, static_cast<int64_t>(m_incarnation));
   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_nodeName, DB_BIND_STATIC);
   DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_nodeAddress, DB_BIND_STATIC);
   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   if (!success)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Lease acquisition attempt failed (SQL error), dropping database connection");
      disconnect();
      return;
   }

   // Verification read
   DB_RESULT hResult = DBSelect(m_hdb, m_pollQuery);
   if (hResult == nullptr)
   {
      disconnect();
      return;
   }
   if (DBGetNumRows(hResult) == 0)
   {
      DBFreeResult(hResult);
      return;
   }

   int64_t term = DBGetFieldInt64(hResult, 0, 0);
   uuid holderGuid = DBGetFieldGUID(hResult, 0, 1);
   int64_t holderIncarnation = DBGetFieldInt64(hResult, 0, 2);
   wchar_t holderName[64];
   DBGetField(hResult, 0, 3, holderName, 64);
   int64_t remainingValidity = DBGetFieldInt64(hResult, 0, 4);
   wchar_t holderAddress[256];
   DBGetField(hResult, 0, 5, holderAddress, 256);
   DBFreeResult(hResult);

   updateStatus(term, holderGuid, holderName, holderAddress, remainingValidity);

   if (holderGuid.equals(m_nodeGuid) && (holderIncarnation == static_cast<int64_t>(m_incarnation)))
   {
      m_term = term;
      m_fenceDeadline = sendTime + static_cast<int64_t>(m_validity - m_fenceMargin) * 1000;
      m_state = static_cast<int>(HALeaseState::ACTIVE);
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Cluster lease acquired (term " INT64_FMTW L")", term);
      if (m_promotionHandler != nullptr)
         m_promotionHandler(term);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Lease acquisition lost to node %s (term " INT64_FMTW L")", holderGuid.toString().cstr(), term);
   }
}

/**
 * Refresh held lease and advance the fence deadline on confirmation
 */
void HALeaseManager::refresh()
{
   int64_t sendTime = GetMonotonicClockTime();

   DB_STATEMENT hStmt = DBPrepare(m_hdb, m_refreshQuery);
   if (hStmt == nullptr)
   {
      disconnect();
      return;
   }

   wchar_t guidText[64];
   m_nodeGuid.toString(guidText);
   DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, m_term.load());
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, guidText, DB_BIND_STATIC);
   DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, static_cast<int64_t>(m_incarnation));
   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   if (!success)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Lease refresh failed (SQL error), dropping database connection");
      disconnect();
      return;
   }

   // Verification read
   DB_RESULT hResult = DBSelect(m_hdb, m_pollQuery);
   if (hResult == nullptr)
   {
      disconnect();
      return;
   }
   if (DBGetNumRows(hResult) == 0)
   {
      DBFreeResult(hResult);
      return;
   }

   int64_t term = DBGetFieldInt64(hResult, 0, 0);
   uuid holderGuid = DBGetFieldGUID(hResult, 0, 1);
   int64_t holderIncarnation = DBGetFieldInt64(hResult, 0, 2);
   wchar_t holderName[64];
   DBGetField(hResult, 0, 3, holderName, 64);
   int64_t remainingValidity = DBGetFieldInt64(hResult, 0, 4);
   wchar_t holderAddress[256];
   DBGetField(hResult, 0, 5, holderAddress, 256);
   DBFreeResult(hResult);

   updateStatus(term, holderGuid, holderName, holderAddress, remainingValidity);

   if (holderGuid.equals(m_nodeGuid) && (holderIncarnation == static_cast<int64_t>(m_incarnation)) && (term == m_term.load()))
   {
      m_fenceDeadline = sendTime + static_cast<int64_t>(m_validity - m_fenceMargin) * 1000;
      nxlog_debug_tag(DEBUG_TAG, 7, L"Lease refreshed (term " INT64_FMTW L")", term);
   }
   else
   {
      wchar_t reason[256];
      nx_swprintf(reason, 256, L"lease lost (term " INT64_FMTW L" held by node %s)", term, holderGuid.toString().cstr());
      fence(reason);
   }
}

/**
 * Voluntarily release held lease (graceful handover, after drain)
 */
void HALeaseManager::doRelease()
{
   DB_STATEMENT hStmt = DBPrepare(m_hdb, m_releaseQuery);
   if (hStmt != nullptr)
   {
      wchar_t guidText[64];
      m_nodeGuid.toString(guidText);
      DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, m_term.load());
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, guidText, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, static_cast<int64_t>(m_incarnation));
      if (DBExecute(hStmt))
      {
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Cluster lease released (term " INT64_FMTW L")", m_term.load());
      }
      else
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Cluster lease release failed; lease will be left to expire");
      }
      DBFreeStatement(hStmt);
   }
   int expected = static_cast<int>(HALeaseState::ACTIVE);
   m_state.compare_exchange_strong(expected, static_cast<int>(HALeaseState::STOPPED));
}

/**
 * Fence this node. Thread-safe and idempotent: only the ACTIVE -> FENCED
 * transition invokes the fence handler, exactly once.
 */
void HALeaseManager::fence(const wchar_t *reason)
{
   int expected = static_cast<int>(HALeaseState::ACTIVE);
   if (m_state.compare_exchange_strong(expected, static_cast<int>(HALeaseState::FENCED)))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Node fenced: %s", reason);
      if (m_fenceHandler != nullptr)
         m_fenceHandler(reason);
   }
}

/**
 * Opportunistic watchdog check (callable from any thread)
 */
bool HALeaseManager::checkFenceDeadline()
{
   if ((getState() == HALeaseState::ACTIVE) && (GetMonotonicClockTime() > m_fenceDeadline.load()))
      fence(L"fence deadline missed (refresh not confirmed in time)");
   return isFenced();
}

/**
 * Watchdog thread: enforces the fence deadline independently of the manager
 * thread, which can be stuck inside a hung lease statement (stalled network,
 * unresponsive database). Never touches the database.
 */
void HALeaseManager::watchdogLoop()
{
   while(!m_shutdown.load())
   {
      checkFenceDeadline();
      m_watchdogWakeup.wait(500);
   }
}

/**
 * Manager thread main loop
 */
void HALeaseManager::mainLoop()
{
   nxlog_debug_tag(DEBUG_TAG, 2, L"HA lease manager thread started");

   while(!m_shutdown.load())
   {
      if (getState() == HALeaseState::STOPPED)
         break;

      if (m_hdb == nullptr)
      {
         if (!connect())
         {
            if (getState() == HALeaseState::STOPPED)
               break;
            checkFenceDeadline();
            m_wakeupCondition.wait(2000);
            continue;
         }
      }

      uint32_t waitTime = m_refreshInterval * 1000;
      switch(getState())
      {
         case HALeaseState::INIT:
            m_state = static_cast<int>(HALeaseState::STANDBY);
            waitTime = 0;
            break;
         case HALeaseState::STANDBY:
         {
            int64_t remainingValidity = poll();
            if ((remainingValidity != INT64_MIN) && (remainingValidity <= 0) && !m_shutdown.load())
               tryAcquire();
            break;
         }
         case HALeaseState::ACTIVE:
            if (m_releaseRequested.load())
            {
               doRelease();
               break;
            }
            if (!checkFenceDeadline())
               refresh();
            break;
         default:   // FENCED - keep thread for status queries until shutdown
            waitTime = 10000;
            break;
      }

      if (m_shutdown.load())
         break;
      if (m_hdb == nullptr)
         waitTime = 1000;   // reconnect soon
      if (waitTime > 0)
         m_wakeupCondition.wait(waitTime);
   }

   nxlog_debug_tag(DEBUG_TAG, 2, L"HA lease manager thread stopped");
}
