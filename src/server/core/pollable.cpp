/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Raden Solutions
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
** File: pollable.cpp
**
**/

#include "nxcore.h"

/**
 * Pollable object constructor
 */
Pollable::Pollable(NetObj *_this, uint32_t acceptablePolls) : m_statusPollState(_T("status")), m_configurationPollState(_T("configuration"), true),
      m_instancePollState(_T("instance"), true), m_discoveryPollState(_T("discovery")), m_topologyPollState(_T("topology"), true), m_routingPollState(_T("routing")),
      m_icmpPollState(_T("icmp")), m_autobindPollState(_T("autobind"), true), m_mapUpdatePollState(_T("map"))
{
   m_this = _this;
   _this->m_asPollable = this;
   m_acceptablePolls = acceptablePolls;
}

/**
 * Destructor
 */
Pollable::~Pollable()
{
}

/**
 * Start forced status poll
 */
void Pollable::doForcedStatusPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   startForcedStatusPoll();
   poller->startExecution();
   statusPoll(poller, session, rqId);
   delete poller;
}

/**
 * Used in parent polls
 */
void Pollable::doStatusPoll(PollerInfo *parentPoller, ClientSession *session, uint32_t rqId)
{
   statusPoll(parentPoller, session, rqId);
}

/**
 * Start scheduled status poll
 */
void Pollable::doStatusPoll(PollerInfo *poller)
{
   poller->startExecution();
   statusPoll(poller, nullptr, 0);
   delete poller;
}

/**
 * Default status poll lock
 */
bool Pollable::lockForStatusPoll()
{
   bool success = false;
   m_this->lockProperties();
   if (!m_this->m_isDeleted && !m_this->m_isDeleteInitiated)
   {
      if (m_this->m_runtimeFlags & ODF_FORCE_STATUS_POLL)
      {
         success = m_statusPollState.schedule();
         if (success)
            m_this->m_runtimeFlags &= ~ODF_FORCE_STATUS_POLL;
      }
      else if ((m_this->m_status != STATUS_UNMANAGED) &&
               !(m_this->m_flags & DCF_DISABLE_STATUS_POLL) &&
               !(m_this->m_runtimeFlags & ODF_CONFIGURATION_POLL_PENDING) &&
               (static_cast<uint32_t>(time(nullptr) - m_statusPollState.getLastCompleted()) > m_this->getCustomAttributeAsUInt32(_T("SysConfig:Objects.StatusPollingInterval"), g_statusPollingInterval)))
      {
         success = m_statusPollState.schedule();
      }
   }
   m_this->unlockProperties();
   return success;
}

/**
 * Start forced configuration poll
 */
void Pollable::doForcedConfigurationPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   startForcedConfigurationPoll();
   poller->startExecution();
   configurationPoll(poller, session, rqId);
   delete poller;
}

/**
 * Start scheduled configuration poll
 */
void Pollable::doConfigurationPoll(PollerInfo *poller)
{
   poller->startExecution();
   configurationPoll(poller, nullptr, 0);
   delete poller;
}

/**
 * Default configuration poll lock
 */
bool Pollable::lockForConfigurationPoll()
{
   bool success = false;
   m_this->lockProperties();
   if (!m_this->m_isDeleted && !m_this->m_isDeleteInitiated)
   {
      if (m_this->m_runtimeFlags & ODF_FORCE_CONFIGURATION_POLL)
      {
         success = m_configurationPollState.schedule();
         if (success)
            m_this->m_runtimeFlags &= ~ODF_FORCE_CONFIGURATION_POLL;
      }
      else if ((m_this->m_status != STATUS_UNMANAGED) &&
               (!(m_this->m_flags & DCF_DISABLE_CONF_POLL)) &&
               (static_cast<uint32_t>(time(nullptr) - m_configurationPollState.getLastCompleted()) > m_this->getCustomAttributeAsUInt32(_T("SysConfig:Objects.ConfigurationPollingInterval"), g_configurationPollingInterval)))
      {
         success = m_configurationPollState.schedule();
      }
   }
   m_this->unlockProperties();
   return success;
}

/**
 * Start forced instance discovery poll
 */
void Pollable::doForcedInstanceDiscoveryPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   startForcedInstanceDiscoveryPoll();
   poller->startExecution();
   instanceDiscoveryPoll(poller, session, rqId);
   delete poller;
}

/**
 * Start scheduled instance discovery poll
 */
void Pollable::doInstanceDiscoveryPoll(PollerInfo *poller)
{
   poller->startExecution();
   instanceDiscoveryPoll(poller, nullptr, 0);
   delete poller;
}

/**
 * Start forced topology poll
 */
void Pollable::doForcedTopologyPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   startForcedTopologyPoll();
   poller->startExecution();
   topologyPoll(poller, session, rqId);
   delete poller;
}

/**
 * Start scheduled topology poll
 */
void Pollable::doTopologyPoll(PollerInfo *poller)
{
   poller->startExecution();
   topologyPoll(poller, nullptr, 0);
   delete poller;
}

/**
 * Start forced routing table poll
 */
void Pollable::doForcedRoutingTablePoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   startForcedRoutingTablePoll();
   poller->startExecution();
   routingTablePoll(poller, session, rqId);
   delete poller;
}

/**
 * Start scheduled routing table poll
 */
void Pollable::doRoutingTablePoll(PollerInfo *poller)
{
   poller->startExecution();
   routingTablePoll(poller, nullptr, 0);
   delete poller;
}

// External function, from poll.cpp
// FIXME: any reason for not going to standart poll?
void DiscoveryPoller(PollerInfo *poller);

/**
 * Start forced discovery poll
 */
void Pollable::doForcedDiscoveryPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   startForcedDiscoveryPoll();
   DiscoveryPoller(poller);
}

/**
 * Start scheduled discovery poll
 */
void Pollable::doDiscoveryPoll(PollerInfo *poller)
{
   DiscoveryPoller(poller);
}

/**
 * Start scheduled ICMP poll
 */
void Pollable::doIcmpPoll(PollerInfo *poller)
{
   poller->startExecution();
   icmpPoll(poller);
   delete poller;
}

/**
 * Start forced configuration poll
 */
void Pollable::doForcedAutobindPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   startForcedAutobindPoll();
   poller->startExecution();
   autobindPoll(poller, session, rqId);
   delete poller;
}

/**
 * Start scheduled configuration poll
 */
void Pollable::doAutobindPoll(PollerInfo *poller)
{
   poller->startExecution();
   autobindPoll(poller, nullptr, 0);
   delete poller;
}

/**
 * Lock object for automatic binding poll
 */
bool Pollable::lockForAutobindPoll()
{
   bool success = false;
   m_this->lockProperties();
   if (!m_this->m_isDeleted && !m_this->m_isDeleteInitiated &&
       (m_this->m_status != STATUS_UNMANAGED) &&
       (static_cast<uint32_t>(time(nullptr) - m_autobindPollState.getLastCompleted()) > m_this->getCustomAttributeAsUInt32(_T("SysConfig:Objects.AutobindPollingInterval"), g_autobindPollingInterval)))
   {
      success = m_autobindPollState.schedule();
   }
   m_this->unlockProperties();
   return success;
}

/**
 * Start forced map update poll
 */
void Pollable::doForcedMapUpdatePoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   startForcedMapUpdatePoll();
   poller->startExecution();
   mapUpdatePoll(poller, session, rqId);
   delete poller;
}

/**
 * Start scheduled map update poll
 */
void Pollable::doMapUpdatePoll(PollerInfo *poller)
{
   poller->startExecution();
   mapUpdatePoll(poller, nullptr, 0);
   delete poller;
}

/**
 * Lock object for map update poll
 */
bool Pollable::lockForMapUpdatePoll()
{
   bool success = false;
   m_this->lockProperties();
   if (!m_this->m_isDeleted && !m_this->m_isDeleteInitiated &&
       (m_this->m_status != STATUS_UNMANAGED) &&
       (static_cast<uint32_t>(time(nullptr) - m_mapUpdatePollState.getLastCompleted()) > m_this->getCustomAttributeAsUInt32(_T("SysConfig:Objects.NetworkMaps.UpdateInterval"), g_mapUpdatePollingInterval)))
   {
      success = m_mapUpdatePollState.schedule();
   }
   m_this->unlockProperties();
   return success;
}

/**
 * Reset poll timers
 */
void Pollable::resetPollTimers()
{
   m_statusPollState.resetTimer();
   m_configurationPollState.resetTimer();
   m_instancePollState.resetTimer();
   m_topologyPollState.resetTimer();
   m_discoveryPollState.resetTimer();
   m_routingPollState.resetTimer();
   m_icmpPollState.resetTimer();
   m_autobindPollState.resetTimer();
   m_mapUpdatePollState.resetTimer();
}

/**
 * Get poll time for given poller and aggregation type
 */
static inline DataCollectionError GetPollTime(PollState *state, const TCHAR *type, TCHAR *buffer)
{
   if (!_tcsicmp(type, _T("Average")))
   {
      IntegerToString(state->getTimerAverage(), buffer);
      return DCE_SUCCESS;
   }
   if (!_tcsicmp(type, _T("Last")))
   {
      IntegerToString(state->getTimerLast(), buffer);
      return DCE_SUCCESS;
   }
   if (!_tcsicmp(type, _T("Max")))
   {
      IntegerToString(state->getTimerMax(), buffer);
      return DCE_SUCCESS;
   }
   if (!_tcsicmp(type, _T("Min")))
   {
      IntegerToString(state->getTimerMin(), buffer);
      return DCE_SUCCESS;
   }
   return DCE_NOT_SUPPORTED;
}

/**
 * Get value for server's internal parameter
 */
DataCollectionError Pollable::getInternalMetric(const TCHAR *name, TCHAR *buffer, size_t size)
{
   if (_tcsnicmp(name, _T("PollTime."), 9))
      return DCE_NOT_SUPPORTED;

   if (!_tcsnicmp(&name[9], _T("AutoBind."), 9))
      return isAutobindPollAvailable() ? GetPollTime(&m_autobindPollState, &name[18], buffer) : DCE_NOT_SUPPORTED;

   if (!_tcsnicmp(&name[9], _T("Configuration."), 14))
      return isConfigurationPollAvailable() ? GetPollTime(&m_configurationPollState, &name[23], buffer) : DCE_NOT_SUPPORTED;

   if (!_tcsnicmp(&name[9], _T("Instance."), 9))
      return isInstanceDiscoveryPollAvailable() ? GetPollTime(&m_instancePollState, &name[18], buffer) : DCE_NOT_SUPPORTED;

   if (!_tcsnicmp(&name[9], _T("Status."), 7))
      return isStatusPollAvailable() ? GetPollTime(&m_statusPollState, &name[16], buffer) : DCE_NOT_SUPPORTED;

   if (!_tcsnicmp(&name[9], _T("Topology."), 9))
      return isTopologyPollAvailable() ? GetPollTime(&m_topologyPollState, &name[18], buffer) : DCE_NOT_SUPPORTED;

   if (!_tcsnicmp(&name[9], _T("RoutingTable."), 13))
      return isRoutingTablePollAvailable() ? GetPollTime(&m_routingPollState, &name[22], buffer) : DCE_NOT_SUPPORTED;

   return DCE_NOT_SUPPORTED;
}

/**
 * Create pollable object from database data
 */
bool Pollable::loadFromDatabase(DB_HANDLE hdb, uint32_t id)
{
   TCHAR query[512];
   _sntprintf(query, 512, _T("SELECT config_poll_timestamp,instance_poll_timestamp FROM pollable_objects WHERE id=%u"), id);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult == nullptr)
      return false;

   if (DBGetNumRows(hResult) > 0)
   {
      m_configurationPollState.setLastCompleted(DBGetFieldLong(hResult, 0, 0));
      m_instancePollState.setLastCompleted(DBGetFieldLong(hResult, 0, 1));
   }

   DBFreeResult(hResult);
   return true;
}

/**
 * Save pollable object data to database
 */
bool Pollable::saveToDatabase(DB_HANDLE hdb)
{
   bool success = false;
   static const TCHAR *columns[] = { _T("config_poll_timestamp"), _T("instance_poll_timestamp"), nullptr };
   DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("pollable_objects"), _T("id"), m_this->getId(), columns);
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_configurationPollState.getLastCompleted()));
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_instancePollState.getLastCompleted()));
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_this->getId());
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   return success;
}

/**
 * Fill NXCP message with specific poll state if poll type is enabled
 */
#define FillPollState(state, type) \
{ \
   if (m_acceptablePolls & type) \
   { \
      state.fillMessage(msg, baseId); \
      baseId += 10; \
      count++; \
   } \
}

/**
 * Copy current poll states to NXCP message
 */
void Pollable::pollStateToMessage(NXCPMessage *msg)
{
   uint32_t count = 0;
   uint32_t baseId = VID_POLL_STATE_LIST_BASE;
   FillPollState(m_statusPollState, Pollable::STATUS);
   FillPollState(m_configurationPollState, Pollable::CONFIGURATION);
   FillPollState(m_instancePollState, Pollable::INSTANCE_DISCOVERY);
   FillPollState(m_discoveryPollState, Pollable::DISCOVERY);
   FillPollState(m_topologyPollState, Pollable::TOPOLOGY);
   FillPollState(m_routingPollState, Pollable::ROUTING_TABLE);
   FillPollState(m_icmpPollState, Pollable::ICMP);
   FillPollState(m_autobindPollState, Pollable::AUTOBIND);
   FillPollState(m_mapUpdatePollState, Pollable::MAP_UPDATE);
   msg->setField(VID_NUM_POLL_STATES, count);
}

/**
 * Auto fill properties of linked asset
 */
void Pollable::autoFillAssetProperties()
{
   if (m_this->m_assetId == 0)
      return;  // No linked asset

   shared_ptr<Asset> asset = static_pointer_cast<Asset>(FindObjectById(m_this->m_assetId, OBJECT_ASSET));
   if (asset != nullptr)
   {
      m_this->sendPollerMsg(_T("Updating asset properties\r\n"));
      asset->autoFillProperties();
   }
}
