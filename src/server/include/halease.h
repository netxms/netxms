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
** File: halease.h
**
**/

#ifndef _halease_h_
#define _halease_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxdbapi.h>
#include <functional>
#include <atomic>

/**
 * Get expression for "epoch seconds now" evaluated on the database server
 * (nullptr for syntaxes not supported in cluster mode)
 */
const wchar_t *HAGetDbTimeExpression(int syntax);

/**
 * HA lease manager state
 */
enum class HALeaseState
{
   INIT = 0,      // not yet connected to database
   STANDBY = 1,   // watching the lease
   ACTIVE = 2,    // holding the lease
   FENCED = 3,    // lost the lease or missed the fence deadline while active (terminal)
   STOPPED = 4    // voluntarily released or shut down (terminal)
};

/**
 * Last observed lease information (cached snapshot for status display)
 */
struct HALeaseStatus
{
   HALeaseState state;
   int64_t term;                 // current term as last observed in the database
   uuid holderGuid;
   wchar_t holderName[64];
   wchar_t holderAddress[256];   // client-reachable address published by the lease holder (may be empty)
   int64_t remainingValidity;    // seconds until expiry as computed by the database; negative if expired
   time_t lastUpdate;            // local time of last successful lease table read
};

/**
 * HA lease manager. Implements the database lease algorithm from doc/HA_Design.md:
 * term-based acquisition via atomic conditional update, periodic refresh, self-fencing
 * with a monotonic-clock deadline. Owns a dedicated database connection (never uses
 * the shared pool); all lease timestamps and expiry comparisons are computed by the
 * database server. Deliberately depends only on libnetxms and libnxdb so the same
 * code can be exercised by the standalone HA test harness.
 */
class HALeaseManager
{
private:
   uuid m_nodeGuid;
   wchar_t m_nodeName[64];
   wchar_t m_nodeAddress[256];
   uint64_t m_incarnation;
   uint32_t m_refreshInterval;   // seconds
   uint32_t m_validity;          // seconds
   uint32_t m_fenceMargin;       // seconds
   std::function<DB_HANDLE()> m_connector;
   std::function<void(int64_t)> m_promotionHandler;
   std::function<void(const wchar_t*)> m_fenceHandler;

   DB_HANDLE m_hdb;
   int m_dbSyntax;
   const wchar_t *m_dbTimeExpression;
   MutableString m_acquireQuery;
   MutableString m_refreshQuery;
   MutableString m_releaseQuery;
   MutableString m_pollQuery;

   std::atomic<int> m_state;
   std::atomic<int64_t> m_fenceDeadline;  // monotonic clock ms; valid while ACTIVE
   std::atomic<int64_t> m_term;           // term held by this node; valid while ACTIVE
   std::atomic<bool> m_releaseRequested;
   std::atomic<bool> m_shutdown;
   std::atomic<int> m_fastPollCycles;     // poll at 250 ms while > 0 (handover expected)

   Mutex m_statusLock;
   HALeaseStatus m_status;

   THREAD m_thread;
   THREAD m_watchdogThread;
   Condition m_wakeupCondition;
   Condition m_watchdogWakeup;

   void mainLoop();
   void watchdogLoop();
   bool connect();
   void disconnect();
   bool buildQueries();
   int64_t poll();      // returns remaining validity in seconds, INT64_MIN on query failure
   void tryAcquire();
   void refresh();
   void doRelease();
   void fence(const wchar_t *reason);
   void updateStatus(int64_t term, const uuid& holderGuid, const wchar_t *holderName, const wchar_t *holderAddress, int64_t remainingValidity);

public:
   HALeaseManager(const uuid& nodeGuid, const wchar_t *nodeName, std::function<DB_HANDLE()> connector,
         uint32_t refreshInterval, uint32_t validity, uint32_t fenceMargin);
   ~HALeaseManager();

   /**
    * Called from the manager thread when this node wins the lease (argument = new term).
    * Must return promptly; long activation work has to be dispatched to another thread
    * by the callee, because lease refresh resumes only after this handler returns.
    */
   void setPromotionHandler(std::function<void(int64_t)> handler) { m_promotionHandler = handler; }

   /**
    * Called exactly once when the node is fenced (from whichever thread detects it first).
    * Must be quick and non-blocking; typically raises the global fence flag and initiates
    * an orderly restart into standby.
    */
   void setFenceHandler(std::function<void(const wchar_t*)> handler) { m_fenceHandler = handler; }

   /**
    * Set client-reachable address of this node, published in the lease record when
    * this node becomes the holder (standby nodes read it to redirect clients and
    * agents to the active node). Call before start().
    */
   void setNodeAddress(const wchar_t *address) { wcslcpy(m_nodeAddress, CHECK_NULL_EX_W(address), 256); }

   bool start();
   void stop();

   /**
    * Wake the manager thread for an immediate poll/acquisition attempt and
    * keep polling fast for a short window (peer channel handover
    * notification; the lease stays authoritative)
    */
   void wakeup()
   {
      m_fastPollCycles = 20;
      m_wakeupCondition.set();
   }

   /**
    * Request graceful lease release (switchover, after drain). Executed asynchronously
    * by the manager thread; the manager transitions to STOPPED afterwards.
    */
   void requestRelease();

   HALeaseState getState() const { return static_cast<HALeaseState>(m_state.load()); }
   bool isFenced() const { return getState() == HALeaseState::FENCED; }
   int64_t getTerm() const { return m_term.load(); }
   const uuid& getNodeGuid() const { return m_nodeGuid; }
   uint64_t getIncarnation() const { return m_incarnation; }

   /**
    * Opportunistic watchdog check, callable from any thread at role-sensitive dispatch
    * points. Fences if the node is ACTIVE and the monotonic fence deadline has passed
    * (covers hung refresh statements and process pauses). Returns true if the node is
    * fenced after the check.
    */
   bool checkFenceDeadline();

   /**
    * True if this node currently holds a valid, unexpired-by-deadline lease
    */
   bool isActive()
   {
      return (getState() == HALeaseState::ACTIVE) && !checkFenceDeadline();
   }

   HALeaseStatus getStatus();
};

#endif   /* _halease_h_ */
