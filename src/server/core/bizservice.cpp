/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Raden Solutions
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
** File: bizservice.cpp
**
**/

#include "nxcore.h"

/**
 * Constructor for new service object
 */
BusinessService::BusinessService() : super(), Pollable(this, Pollable::STATUS | Pollable::CONFIGURATION)
{
   m_prototypeId = 0;
   m_instance = nullptr;
}

/**
 * Constructor for new service object
 */
BusinessService::BusinessService(const TCHAR *name) : super(name), Pollable(this, Pollable::STATUS | Pollable::CONFIGURATION)
{
   m_prototypeId = 0;
   m_instance = nullptr;
}

/**
 * Create new business service from prototype
 */
BusinessService::BusinessService(const BaseBusinessService& prototype, const TCHAR *name, const TCHAR *instance) : super(prototype, name), Pollable(this, Pollable::STATUS | Pollable::CONFIGURATION)
{
   m_prototypeId = prototype.getId();
   m_instance = MemCopyString(instance);
}

/**
 * Business service destructor
 */
BusinessService::~BusinessService()
{
   MemFree(m_instance);
}

/**
 * Load Business service from database
 */
bool BusinessService::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   if (!super::loadFromDatabase(hdb, id))
      return false;

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT prototype_id,instance,object_status_threshold,dci_status_threshold FROM business_services WHERE id=?"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
   {
      DBFreeStatement(hStmt);
      return false;
   }

   m_prototypeId = DBGetFieldULong(hResult, 0, 0);
   m_instance = DBGetField(hResult, 0, 1, nullptr, 0);
   m_objectStatusThreshhold = DBGetFieldULong(hResult, 0, 2);
   m_dciStatusThreshhold = DBGetFieldULong(hResult, 0, 3);

   DBFreeResult(hResult);
   DBFreeStatement(hStmt);

   if (!Pollable::loadFromDatabase(hdb, m_id))
      return false;

   return true;
}

/**
 * Save business service to database
 */
bool BusinessService::saveToDatabase(DB_HANDLE hdb)
{
   if (!super::saveToDatabase(hdb))
      return false;

   bool success = true;
   if (m_modified & MODIFY_BIZSVC_PROPERTIES)
   {
      static const TCHAR *columns[] = {
            _T("prototype_id"), _T("instance"), _T("object_status_threshold"), _T("dci_status_threshold"), nullptr
      };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("business_services"), _T("id"), m_id, columns);
      bool success = false;
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_prototypeId);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_instance, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_objectStatusThreshhold);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_dciStatusThreshhold);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         unlockProperties();
         DBFreeStatement(hStmt);
      }
   }

   return success;
}

/**
 * Fill message with business service data
 */
void BusinessService::fillMessageInternal(NXCPMessage *msg, uint32_t userId)
{
   AutoBindTarget::fillMessage(msg);
   msg->setField(VID_INSTANCE, m_instance);
   msg->setField(VID_PROTOTYPE_ID, m_prototypeId);
   return super::fillMessageInternal(msg, userId);
}

/**
 * Returns most critical status of DCI used for status calculation
 */
int BusinessService::getAdditionalMostCriticalStatus()
{
   int status = STATUS_NORMAL;
   unique_ptr<SharedObjectArray<BusinessServiceCheck>> checks = getChecks();
   for (const shared_ptr<BusinessServiceCheck>& check : *checks)
   {
      if (check->getStatus() > status)
         status = check->getStatus();
   }
   return status;
}

/**
 * Status poll
 */
void BusinessService::statusPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   m_pollRequestor = session;
   m_pollRequestId = rqId;

   if (IsShutdownInProgress())
   {
      sendPollerMsg(_T("Server shutdown in progress, poll canceled \r\n"));
      return;
   }

   poller->setStatus(_T("wait for lock"));
   pollerLock(status);

   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("BusinessService::statusPoll(%s [%u]): poll started"), m_name, m_id);
   sendPollerMsg(_T("Started status poll of business service %s [%d] \r\n"), m_name, (int)m_id);
   int prevStatus = m_status;

   poller->setStatus(_T("executing checks"));
   sendPollerMsg(_T("Executing business service checks\r\n"));

   int mostCriticalCheckStatus = STATUS_NORMAL;
   unique_ptr<SharedObjectArray<BusinessServiceCheck>> checks = getChecks();
   for (const shared_ptr<BusinessServiceCheck>& check : *checks)
   {
      BusinessServiceTicketData data = {};
      SharedString checkDescription = check->getDescription();

      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("BusinessService::statusPoll(%s [%u]): executing check %s [%u]"), m_name, m_id, checkDescription.cstr(), check->getId());
      sendPollerMsg(_T("   Executing business service check \"%s\"\r\n"), checkDescription.cstr());
      int oldCheckStatus = check->getStatus();
      int newCheckStatus = check->execute(&data);

      if (data.ticketId != 0)
      {
         unique_ptr<SharedObjectArray<NetObj>> parents = getParents(OBJECT_BUSINESS_SERVICE);
         for (const shared_ptr<NetObj>& parent : *parents)
            static_cast<BusinessService&>(*parent).addChildTicket(data);
      }
      if (oldCheckStatus != newCheckStatus)
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("BusinessService::statusPoll(%s [%u]): status of check %s [%u] changed to %s"),
               m_name, m_id, checkDescription.cstr(), check->getId(), GetStatusAsText(newCheckStatus, true));
         sendPollerMsg(_T("   Status of business service check \"%s\" changed to %s\r\n"), checkDescription.cstr(), GetStatusAsText(newCheckStatus, true));
         NotifyClientsOnBusinessServiceCheckUpdate(*this, check);
      }
      if (newCheckStatus > mostCriticalCheckStatus)
      {
         mostCriticalCheckStatus = newCheckStatus;
      }
   }
   sendPollerMsg(_T("All business service checks executed\r\n"));

   calculateCompoundStatus();

   if (prevStatus != m_status)
   {
      sendPollerMsg(_T("Status of business service changed to %s\r\n"), GetStatusAsText(m_status, true));
      if (m_status > prevStatus)
      {
         if  (m_status == STATUS_CRITICAL)
         {
            DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
            DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO business_service_downtime (record_id,service_id,from_timestamp,to_timestamp) VALUES (?,?,?,0)"));
            if (hStmt != nullptr)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, CreateUniqueId(IDG_BUSINESS_SERVICE_RECORD));
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(time(nullptr)));
               DBExecute(hStmt);
               DBFreeStatement(hStmt);
            }
            DBConnectionPoolReleaseConnection(hdb);
            PostSystemEvent(EVENT_BUSINESS_SERVICE_CRITICAL, m_id, nullptr);
         }
         else
         {
            PostSystemEvent(EVENT_BUSINESS_SERVICE_MINOR, m_id, nullptr);
         }
      }
      else if (m_status < prevStatus)
      {
         if (prevStatus == STATUS_CRITICAL)
         {
            DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
            DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE business_service_downtime SET to_timestamp=? WHERE service_id=? AND to_timestamp=0"));
            if (hStmt != nullptr)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(time(nullptr)));
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
               DBExecute(hStmt);
               DBFreeStatement(hStmt);
            }
            DBConnectionPoolReleaseConnection(hdb);
         }
         PostSystemEvent((m_status == STATUS_NORMAL) ? EVENT_BUSINESS_SERVICE_NORMAL : EVENT_BUSINESS_SERVICE_MINOR, m_id, nullptr);
      }
   }

   lockProperties();
   sendPollerMsg(_T("Finished status poll of business service %s [%u] \r\n"), m_name, m_id);
   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("BusinessService::statusPoll(%s [%u]): poll finished"), m_name, m_id);
   unlockProperties();

   pollerUnlock();
}

/**
 * Add ticket from child business service to parent business service.
 * Used to ensure, that we have all info about downtimes in parent business service.
 * Parent tickets closed simultaneously with original ticket
 */
void BusinessService::addChildTicket(const BusinessServiceTicketData& data)
{
   unique_ptr<SharedObjectArray<NetObj>> parents = getParents(OBJECT_BUSINESS_SERVICE);
   for (const shared_ptr<NetObj>& parent : *parents)
      static_cast<BusinessService&>(*parent).addChildTicket(data);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO business_service_tickets (ticket_id,original_ticket_id,original_service_id,check_id,check_description,service_id,create_timestamp,close_timestamp,reason) VALUES (?,?,?,?,?,?,?,0,?)"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, CreateUniqueId(IDG_BUSINESS_SERVICE_TICKET));
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, data.ticketId);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, data.serviceId);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, data.checkId);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, data.description, DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_id);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(data.timestamp));
      DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, data.reason, DB_BIND_STATIC);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
}

/**
 * Configuration poll
 */
void BusinessService::configurationPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   m_pollRequestor = session;
   m_pollRequestId = rqId;

   nxlog_debug_tag(DEBUG_TAG_BIZSVC, 6, _T("BusinessService::configurationPoll(%s): started"), m_name);
   sendPollerMsg(_T("Configuration poll started\r\n"));

   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_configurationPollState.complete(0);
      sendPollerMsg(_T("Server shutdown in progress, poll canceled \r\n"));
      unlockProperties();
      return;
   }
   unlockProperties();

   poller->setStatus(_T("wait for lock"));
   pollerLock(configuration);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   validateAutomaticObjectChecks();
   validateAutomaticDCIChecks();

   sendPollerMsg(_T("Configuration poll finished\r\n"));
   nxlog_debug_tag(DEBUG_TAG_BIZSVC, 6, _T("BusinessService::configurationPoll(%s): finished"), m_name);

   pollerUnlock();
}

/**
 * Validate automatically created object based checks (will add or remove checks as needed)
 */
void BusinessService::validateAutomaticObjectChecks()
{
   if (!isAutoBindEnabled(0))
   {
      sendPollerMsg(_T("Automatic creation of object based checks is disabled\r\n"));
      return;
   }

   nxlog_debug_tag(DEBUG_TAG_BIZSVC, 6, _T("BusinessService::validateAutomaticObjectChecks(%s): validating object based checks"), m_name);
   sendPollerMsg(_T("Validating automatically created object based checks\r\n"));
   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects();
   for (int i = 0; i < objects->size(); i++)
   {
      shared_ptr<NetObj> object = objects->getShared(i);
      AutoBindDecision decision = isApplicable(object);
      if (decision != AutoBindDecision_Ignore)
      {
         shared_ptr<BusinessServiceCheck> selectedCheck;
         unique_ptr<SharedObjectArray<BusinessServiceCheck>> checks = getChecks();
         for (shared_ptr<BusinessServiceCheck> check : *checks)
         {
            if ((check->getType() == BusinessServiceCheckType::OBJECT) && (check->getRelatedObject() == object->getId()))
            {
               selectedCheck = check;
               break;
            }
         }
         if ((selectedCheck != nullptr) && (decision == AutoBindDecision_Unbind) && isAutoUnbindEnabled(0))
         {
            nxlog_debug_tag(DEBUG_TAG_BIZSVC, 6, _T("BusinessService::validateAutomaticObjectChecks(%s): object check %s [%u] deleted"),
                     m_name, selectedCheck->getDescription().cstr(), selectedCheck->getId());
            sendPollerMsg(_T("   Object based check \"%s\" deleted\r\n"), selectedCheck->getDescription().cstr());
            deleteCheck(selectedCheck->getId());
         }
         if ((selectedCheck == nullptr) && (decision == AutoBindDecision_Bind))
         {
            TCHAR checkName[MAX_OBJECT_NAME];
            _sntprintf(checkName, MAX_OBJECT_NAME, _T("%s"), object->getName());
            auto check = make_shared<BusinessServiceCheck>(m_id, BusinessServiceCheckType::OBJECT, object->getId(), 0, checkName, m_objectStatusThreshhold);
            checksLock();
            m_checks.add(check);
            checksUnlock();
            check->saveToDatabase();
            nxlog_debug_tag(DEBUG_TAG_BIZSVC, 6, _T("BusinessService::validateAutomaticObjectChecks(%s): object check %s [%u] created"), m_name, checkName, check->getId());
            sendPollerMsg(_T("   Object based check \"%s\" created\r\n"), checkName);
            NotifyClientsOnBusinessServiceCheckUpdate(*this, check);
         }
      }
   }
}

/**
 * Business service DCI checks autobinding
 */
void BusinessService::validateAutomaticDCIChecks()
{
   if (!isAutoBindEnabled(1))
   {
      sendPollerMsg(_T("Automatic creation of DCI based checks is disabled\r\n"));
      return;
   }

   nxlog_debug_tag(DEBUG_TAG_BIZSVC, 6, _T("BusinessService::validateAutomaticObjectChecks(%s): validating DCI based checks"), m_name);
   sendPollerMsg(_T("Validating automatically created DCI based checks\r\n"));
   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects();
   for (int i = 0; i < objects->size(); i++)
   {
      shared_ptr<NetObj> object = objects->getShared(i);
      if (!object->isDataCollectionTarget())
         continue;

      unique_ptr<SharedObjectArray<DCObject>> allDCOObjects = static_pointer_cast<DataCollectionTarget>(object)->getAllDCObjects();
      for (shared_ptr<DCObject> dci : *allDCOObjects)
      {
         AutoBindDecision decision = isApplicable(object, dci, 1);
         if (decision != AutoBindDecision_Ignore)
         {
            shared_ptr<BusinessServiceCheck> selectedCheck;
            unique_ptr<SharedObjectArray<BusinessServiceCheck>> checks = getChecks();
            for (shared_ptr<BusinessServiceCheck> check : *checks)
            {
               if ((check->getType() == BusinessServiceCheckType::DCI) && (check->getRelatedObject() == object->getId()) && (check->getRelatedDCI() == dci->getId()))
               {
                  selectedCheck = check;
                  break;
               }
            }
            if ((selectedCheck != nullptr) && (decision == AutoBindDecision_Unbind) && isAutoUnbindEnabled(1))
            {
               nxlog_debug_tag(DEBUG_TAG_BIZSVC, 6, _T("BusinessService::validateAutomaticObjectChecks(%s): DCI check %s [%u] deleted"),
                        m_name, selectedCheck->getDescription().cstr(), selectedCheck->getId());
               sendPollerMsg(_T("   DCI based check \"%s\" deleted\r\n"), selectedCheck->getDescription().cstr());
               deleteCheck(selectedCheck->getId());
            }
            if ((selectedCheck == nullptr) && (decision == AutoBindDecision_Bind))
            {
               TCHAR checkDescription[1023];
               _sntprintf(checkDescription, 1023, _T("%s: %s"), object->getName(), dci->getName().cstr());
               auto check = make_shared<BusinessServiceCheck>(m_id, BusinessServiceCheckType::DCI, object->getId(), dci->getId(), checkDescription, m_dciStatusThreshhold);
               checksLock();
               m_checks.add(check);
               checksUnlock();
               check->saveToDatabase();
               nxlog_debug_tag(DEBUG_TAG_BIZSVC, 6, _T("BusinessService::validateAutomaticObjectChecks(%s): DCI check %s [%u] created"), m_name, checkDescription, check->getId());
               sendPollerMsg(_T("   DCI based check \"%s\" created\r\n"), checkDescription);
               NotifyClientsOnBusinessServiceCheckUpdate(*this, check);
            }
         }
      }
   }
}

/**
 * Lock business service for status poll
 */
bool BusinessService::lockForStatusPoll()
{
   bool success = false;

   lockProperties();
   if (static_cast<uint32_t>(time(nullptr) - m_statusPollState.getLastCompleted()) > g_statusPollingInterval)
   {
      success = m_statusPollState.schedule();
   }
   unlockProperties();
   return success;
}

/**
 * Lock business service for configuration poll
 */
bool BusinessService::lockForConfigurationPoll()
{
   bool success = false;

   lockProperties();
   if (static_cast<uint32_t>(time(nullptr) - m_configurationPollState.getLastCompleted()) > g_configurationPollingInterval)
   {
      success = m_configurationPollState.schedule();
   }
   unlockProperties();
   return success;
}

/**
 * Get business service uptime in percents
 */
double GetServiceUptime(uint32_t serviceId, time_t from, time_t to)
{
   double res = 0;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb,
            _T("SELECT from_timestamp,to_timestamp FROM business_service_downtime ")
            _T("WHERE service_id=? AND ((from_timestamp BETWEEN ? AND ? OR to_timestamp BETWEEN ? and ?) OR (from_timestamp<=? AND (to_timestamp=0 OR to_timestamp>=?)))"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, serviceId);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (uint32_t)from);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (uint32_t)to);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (uint32_t)from);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (uint32_t)to);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (uint32_t)from);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (uint32_t)to);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         time_t totalUptime = to - from;
         int count = DBGetNumRows(hResult);
         for (int i = 0; i < count; i++)
         {
            time_t from_timestamp = DBGetFieldUInt64(hResult, i, 0);
            time_t to_timestamp = DBGetFieldUInt64(hResult, i, 1);
            if (to_timestamp == 0)
               to_timestamp = to;
            time_t downtime = (to_timestamp > to ? to : to_timestamp) - (from_timestamp < from ? from : from_timestamp);
            totalUptime -= downtime;
         }
         res = (double)totalUptime / (double)((to - from) / 100);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return res;
}

/**
 * Get business service tickets
 */
void GetServiceTickets(uint32_t serviceId, time_t from, time_t to, NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb,
            _T("SELECT ticket_id,original_ticket_id,original_service_id,check_id,create_timestamp,close_timestamp,reason,check_description FROM business_service_tickets ")
            _T("WHERE service_id=? AND ((create_timestamp BETWEEN ? AND ? OR close_timestamp BETWEEN ? and ?) OR (create_timestamp<? AND (close_timestamp=0 OR close_timestamp>?)))"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, serviceId);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (uint32_t)from);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (uint32_t)to);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (uint32_t)from);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (uint32_t)to);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (uint32_t)from);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (uint32_t)to);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         uint32_t fieldId = VID_TICKET_LIST_BASE;
         for (int i = 0; i < count; i++)
         {
            uint32_t ticketId = DBGetFieldULong(hResult, i, 0);
            uint32_t originalTicketId = DBGetFieldULong(hResult, i, 1);
            uint32_t originalServiceId = DBGetFieldULong(hResult, i, 2);
            uint32_t checkId = DBGetFieldLong(hResult, i, 3);
            time_t creationTimestamp = static_cast<time_t>(DBGetFieldULong(hResult, i, 4));
            time_t closureTimestamp = static_cast<time_t>(DBGetFieldULong(hResult, i, 5));
            TCHAR reason[256];
            DBGetField(hResult, i, 6, reason, 256);
            TCHAR checkDescription[1024];
            DBGetField(hResult, i, 7, checkDescription, 1024);

            msg->setField(fieldId++, originalTicketId != 0 ? originalTicketId : ticketId);
            msg->setField(fieldId++, originalTicketId != 0 ? originalServiceId : serviceId);
            msg->setField(fieldId++, checkId);
            msg->setFieldFromTime(fieldId++, creationTimestamp);
            msg->setFieldFromTime(fieldId++, closureTimestamp);
            msg->setField(fieldId++, reason);
            msg->setField(fieldId++, checkDescription);
            fieldId += 3;
         }
         msg->setField(VID_TICKET_COUNT, count);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
}
