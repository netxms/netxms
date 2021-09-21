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

#define DEBUG_TAG _T("business.service")

/* ************************************
 *
 * Base Business Service class
 *
 * *************************************
*/

/**
 * Base business service default constructor
 */
BaseBusinessService::BaseBusinessService() : m_checks(10, 10, Ownership::True), super(), AutoBindTarget(this)
{
   m_id = 0;
   m_busy = false;
   m_pollingDisabled = false;
   m_lastPollTime = time_t(0);
   m_prototypeId = 0;
   m_instance = nullptr;
   m_instanceDiscoveryMethod = 0;
   m_instanceDiscoveryData = nullptr;
   m_instanceDiscoveryFilter = nullptr;
   m_objectStatusThreshhold = 0;
   m_dciStatusThreshhold = 0;
}

/**
 * Base business service default constructor
 */
BaseBusinessService::BaseBusinessService(const TCHAR *name) : m_checks(10, 10, Ownership::True), super(name, 0), AutoBindTarget(this)
{
   m_busy = false;
   m_pollingDisabled = false;
   m_lastPollTime = time_t(0);
   m_prototypeId = 0;
   m_instance = nullptr;
   m_instanceDiscoveryMethod = 0;
   m_instanceDiscoveryData = nullptr;
   m_instanceDiscoveryFilter = nullptr;
   m_objectStatusThreshhold = 0;
   m_dciStatusThreshhold = 0;
   m_instanceSource = 0;
}

/**
 * Base business service default destructor
 */
BaseBusinessService::~BaseBusinessService()
{
   MemFree(m_instance);
   MemFree(m_instanceDiscoveryData);
   MemFree(m_instanceDiscoveryFilter);
}

/**
 * Load business service checks from database
 */
bool BaseBusinessService::loadChecksFromDatabase(DB_HANDLE hdb)
{
   nxlog_debug_tag(DEBUG_TAG, 4, _T("Loading service checks for business service %ld"), (long)m_id);

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT id,service_id,type,description,related_object,related_dci,status_threshold,content,current_ticket ")
                                       _T("FROM business_service_checks WHERE service_id=?"));

   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
   {
      DBFreeStatement(hStmt);
      return false;
   }

   int rows = DBGetNumRows(hResult);
   for (int i = 0; i < rows; i++)
   {
      BusinessServiceCheck *check = new BusinessServiceCheck();
      check->loadFromSelect(hResult, i);
      m_checks.add(check);
   }

   DBFreeResult(hResult);
   DBFreeStatement(hStmt);
   return true;
}

/**
 * Delete business service check from service and database
 */
void BaseBusinessService::deleteCheck(uint32_t checkId)
{
   for (auto it = m_checks.begin(); it.hasNext();)
   {
      if (it.next()->getId() == checkId)
      {
         it.remove();
         deleteCheckFromDatabase(checkId);
         break;
      }
   }
}

/**
 * Copy business service checks from checks array
 */
void BaseBusinessService::copyChecks(const ObjectArray<BusinessServiceCheck> &checks)
{
   for (int i = 0; i < checks.size(); i++)
   {
      BusinessServiceCheck *ch = new BusinessServiceCheck(m_id);
      m_checks.add(ch);
      ch->generateId();
      ch->setType(checks.get(i)->getType());
      ch->setRelatedObject(checks.get(i)->getRelatedObject());
      ch->setRelatedDCI(checks.get(i)->getRelatedDCI());
      ch->setThreshold(checks.get(i)->getThreshold());
      ch->setName(checks.get(i)->getName());
      ch->setScript(checks.get(i)->getScript());
      ch->saveToDatabase();
   }
}

/**
 * Delete business service check from database
 */
void BaseBusinessService::deleteCheckFromDatabase(uint32_t checkId)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM business_service_checks WHERE id=?"));
   if (hStmt != nullptr)
   {
      //lockProperties();
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, checkId);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
      NotifyClientsOnBusinessServiceCheckDelete(*this, checkId);
      //unlockProperties();
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Create or modify business service check from request
 */
void BaseBusinessService::modifyCheckFromMessage(NXCPMessage *request)
{
   uint32_t checkId = request->getFieldAsUInt32(VID_BUSINESS_SERVICE_CHECK_ID);
   BusinessServiceCheck *check = nullptr;
   if (checkId != 0)
   {
      for (auto c : m_checks)
      {
         if (c->getId() == checkId)
         {
            check = c;
            break;
         }
      }
   }
   if (check == nullptr)
   {
      check = new BusinessServiceCheck(m_id);
      m_checks.add(check);
   }
   check->modifyFromMessage(request);
   NotifyClientsOnBusinessServiceCheckUpdate(*this, check);
}

/**
 * Modify business service from request
 */
uint32_t BaseBusinessService::modifyFromMessageInternal(NXCPMessage *request)
{
   AutoBindTarget::modifyFromMessage(request);
   if (request->isFieldExist(VID_INSTD_METHOD))
   {
      m_instanceDiscoveryMethod = request->getFieldAsUInt32(VID_INSTD_METHOD);
   }
   if (request->isFieldExist(VID_NODE_ID))
   {
      m_instanceSource = request->getFieldAsUInt32(VID_NODE_ID);
   }
   if (request->isFieldExist(VID_INSTD_DATA))
   {
      MemFree(m_instanceDiscoveryData);
      m_instanceDiscoveryData = request->getFieldAsString(VID_INSTD_DATA);
   }
   if (request->isFieldExist(VID_INSTD_FILTER))
   {
      MemFree(m_instanceDiscoveryFilter);
      m_instanceDiscoveryFilter = request->getFieldAsString(VID_INSTD_FILTER);
   }
   if (request->isFieldExist(VID_OBJECT_STATUS_THRESHOLD))
   {
      m_objectStatusThreshhold = request->getFieldAsUInt32(VID_OBJECT_STATUS_THRESHOLD);
   }
   if (request->isFieldExist(VID_DCI_STATUS_THRESHOLD))
   {
      m_dciStatusThreshhold = request->getFieldAsUInt32(VID_DCI_STATUS_THRESHOLD);
   }
   return super::modifyFromMessageInternal(request);
}

/**
 * Fill message with business service data
 */
void BaseBusinessService::fillMessageInternal(NXCPMessage *msg, uint32_t userId)
{
   AutoBindTarget::fillMessage(msg);
   msg->setField(VID_INSTANCE, m_instance);
   msg->setField(VID_INSTD_METHOD, m_instanceDiscoveryMethod);
   msg->setField(VID_INSTD_DATA, m_instanceDiscoveryData);
   msg->setField(VID_INSTD_FILTER, m_instanceDiscoveryFilter);
   msg->setField(VID_OBJECT_STATUS_THRESHOLD, m_objectStatusThreshhold);
   msg->setField(VID_DCI_STATUS_THRESHOLD, m_dciStatusThreshhold);
   msg->setField(VID_NODE_ID, m_instanceSource);
   return super::fillMessageInternal(msg, userId);
}

/**
 * Load Business service from database
 */
bool BaseBusinessService::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   if (!super::loadFromDatabase(hdb, id))
      return false;
   if (!loadChecksFromDatabase(hdb))
      return false;
   if (!AutoBindTarget::loadFromDatabase(hdb, id))
      return false;

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT prototype_id,instance,instance_method,instance_data,instance_filter,object_status_threshold,dci_status_threshold,instance_source ")
                                       _T("FROM business_services WHERE service_id=?"));
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == NULL)
   {
      DBFreeStatement(hStmt);
      return false;
   }

   m_prototypeId = DBGetFieldULong(hResult, 0, 0);
   m_instance = DBGetField(hResult, 0, 1, nullptr, 0);
   m_instanceDiscoveryMethod = DBGetFieldULong(hResult, 0, 2);
   m_instanceDiscoveryData = DBGetField(hResult, 0, 3, nullptr, 0);
   m_instanceDiscoveryFilter = DBGetField(hResult, 0, 4, nullptr, 0);
   m_objectStatusThreshhold = DBGetFieldULong(hResult, 0, 5);
   m_dciStatusThreshhold = DBGetFieldULong(hResult, 0, 6);
   m_instanceSource = DBGetFieldULong(hResult, 0, 7);

   DBFreeResult(hResult);
   DBFreeStatement(hStmt);
   return true;
}

/**
 * Save business service to database
 */
bool BaseBusinessService::saveToDatabase(DB_HANDLE hdb)
{
   if (!super::saveToDatabase(hdb))
      return false;

   static const TCHAR *columns[] = {
         _T("is_prototype") ,_T("prototype_id"), _T("instance"), _T("instance_method"), _T("instance_data"), _T("instance_filter"),
         _T("object_status_threshold"), _T("dci_status_threshold"), _T("instance_source"), nullptr
   };
   DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("business_services"), _T("service_id"), m_id, columns);
   bool success = false;
   if (hStmt != nullptr)
   {
      //lockProperties();
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, getObjectClass() == OBJECT_BUSINESS_SERVICE_PROTOTYPE ? _T("1") : _T("0"), DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_prototypeId);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_instance, DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_instanceDiscoveryMethod);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_instanceDiscoveryData, DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_TEXT, m_instanceDiscoveryFilter, DB_BIND_STATIC);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_objectStatusThreshhold);
      DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_dciStatusThreshhold);
      DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_instanceSource);
      DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_id);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
      //unlockProperties();
   }
   if (success)
   {
      success = AutoBindTarget::saveToDatabase(hdb);
   }
   return success;
}

/* ************************************
 *
 * Business Service class
 *
 * *************************************
*/

/**
 * Constructor for new service object
 */
BusinessService::BusinessService() : m_statusPollState(_T("status")), m_configurationPollState(_T("configuration"))
{
   m_hPollerMutex = MutexCreate();
}

/**
 * Constructor for new service object
 */
BusinessService::BusinessService(const TCHAR *name) : BaseBusinessService(name), m_statusPollState(_T("status")), m_configurationPollState(_T("configuration"))
{
   m_hPollerMutex = MutexCreate();
}

/**
 * Destructor
 */
BusinessService::~BusinessService()
{
   MutexDestroy(m_hPollerMutex);
}

/**
 * Entry point for status poll worker thread
 */
void BusinessService::statusPollWorkerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   poller->startExecution();
   statusPoll(poller, session, rqId);
   delete poller;
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
      m_busy = false;
      return;
   }

   poller->setStatus(_T("wait for lock"));
   pollerLock(status);

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Started polling of business service %s [%d]"), m_name, (int)m_id);
   sendPollerMsg(_T("Started status poll of business service %s [%d] \r\n"), m_name, (int)m_id);
   m_lastPollTime = time(nullptr);
   uint32_t lastPollStatus = m_status;
   m_status = STATUS_NORMAL;

   // Loop through the kids and execute their either scripts or thresholds
   readLockChildList();
   calculateCompoundStatus();
   unlockChildList();

   for (auto check : m_checks)
   {
      BusinessServiceTicketData data = {};
      uint32_t oldStatus = check->getStatus();
      uint32_t newStatus = check->execute(&data);

      if (data.ticket_id != 0)
      {
         unique_ptr<SharedObjectArray<NetObj>> parents = getParents();
         for (auto parent : *parents)
         {
            if (parent->getObjectClass() == OBJECT_BUSINESS_SERVICE)
            {
               static_pointer_cast<BusinessService>(parent)->addChildTicket(&data);
            }
         }
      }
      if (oldStatus != newStatus)
      {
         sendPollerMsg(_T("Business service check \"%s\" status changed, set to: %s\r\n"), check->getName(), newStatus == STATUS_CRITICAL ? _T("Critical") : newStatus == STATUS_NORMAL ? _T("Normal")
                                                                                                                                                                           : _T("Unknown"));
         NotifyClientsOnBusinessServiceCheckUpdate(*this, check);
      }
      if (newStatus > m_status)
      {
         sendPollerMsg(_T("Business service status changed, set to: %s\r\n"), newStatus == STATUS_CRITICAL ? _T("Critical") : newStatus == STATUS_NORMAL ? _T("Normal")
                                                                                                                                                         : _T("Unknown"));
         m_status = newStatus;
      }
   }

   if (m_status > lastPollStatus)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO business_service_downtime (record_id,service_id,from_timestamp,to_timestamp) VALUES (?,?,?,0)"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, CreateUniqueId(IDG_BUSINESS_SERVICE_RECORD));
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (uint32_t)time(nullptr));
         DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   if (m_status < lastPollStatus)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE business_service_downtime SET to_timestamp=? WHERE service_id=? AND to_timestamp=0"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (uint32_t)time(nullptr));
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
         DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }

   sendPollerMsg(_T("Finished status polling of business service %s [%d] \r\n"), m_name, (int)m_id);
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Finished status polling of business service %s [%d]"), m_name, (int)m_id);
   m_busy = false;
   pollerUnlock();
}

/**
 * Add ticket from child business service to parent business service.
 * Used to ensure, that we have all info about downtimes in parent business service.
 * Parent tickets closed simultaneously with original ticket
 */
void BusinessService::addChildTicket(BusinessServiceTicketData *data)
{
   unique_ptr<SharedObjectArray<NetObj>> parents = getParents();
   for (auto parent : *parents)
   {
      if (parent->getObjectClass() == OBJECT_BUSINESS_SERVICE)
      {
         static_pointer_cast<BusinessService>(parent)->addChildTicket(data);
      }
   }

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO business_service_tickets (ticket_id,original_ticket_id,original_service_id,check_id,check_description,service_id,create_timestamp,close_timestamp,reason) VALUES (?,?,?,?,?,?,?,0,?)"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, CreateUniqueId(IDG_BUSINESS_SERVICE_TICKET));
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, data->ticket_id);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, data->service_id);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, data->check_id);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, data->description, DB_BIND_TRANSIENT);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_id);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (uint32_t)(data->create_timestamp));
      DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, data->reason, DB_BIND_TRANSIENT);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
}

/**
 * Configuration poll worker entry
 */
void BusinessService::configurationPollWorkerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   poller->startExecution();
   poller->startObjectTransaction();
   configurationPoll(poller, session, rqId);
   poller->endObjectTransaction();
   delete poller;
}

/**
 * Configuration poll
 */
void BusinessService::configurationPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   m_pollRequestor = session;
   m_pollRequestId = rqId;

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

   objectCheckAutoBinding();
   dciCheckAutoBinding();

   sendPollerMsg(_T("Configuration poll finished\r\n"));
   nxlog_debug_tag(DEBUG_TAG, 6, _T("BusinessServiceConfPoll(%s): finished"), m_name);

   pollerUnlock();
}

/**
 * Business service object checks autobinding
 */
void BusinessService::objectCheckAutoBinding()
{
   if (isFirstFilterBindingEnabled())
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Business service(%s): Looking for object checks to create"), m_name);
      sendPollerMsg(_T("Business service(%s): Looking for object checks to create \r\n"), m_name);
      unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects();
      uint32_t bindedCount = 0;
      uint32_t unbindedCount = 0;
      for (int i = 0; i < objects->size(); i++)
      {
         shared_ptr<NetObj> object = objects->getShared(i);
         AutoBindDecision decision = isFirstFilterApplicable(object);
         if (decision != AutoBindDecision_Ignore)
         {
            uint32_t foundCheckId = 0;
            for (auto check : m_checks)
            {
               if (check->getType() == BusinessServiceCheck::CheckType::OBJECT && check->getRelatedObject() == object->getId())
               {
                  foundCheckId = check->getId();
                  break;
               }
            }
            if (foundCheckId != 0 && decision == AutoBindDecision_Unbind && isFirstFilterUnbindingEnabled())
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("Business service(%s): object check %u deleted"), foundCheckId);
               deleteCheck(foundCheckId);
               unbindedCount++;
            }
            if (foundCheckId == 0 && decision == AutoBindDecision_Bind)
            {
               BusinessServiceCheck *check = new BusinessServiceCheck(m_id);
               m_checks.add(check);
               check->setRelatedObject(object->getId());
               TCHAR checkName[MAX_OBJECT_NAME];
               _sntprintf(checkName, MAX_OBJECT_NAME, _T("%s[%u] check"), object->getName(), object->getId());
               check->setName(checkName);
               check->generateId();
               check->setThreshold(m_objectStatusThreshhold);
               check->saveToDatabase();
               nxlog_debug_tag(DEBUG_TAG, 6, _T("Business service(%s): object check %s[%u] created"), checkName, foundCheckId);
               NotifyClientsOnBusinessServiceCheckUpdate(*this, check);
               bindedCount++;
            }
         }
      }
      sendPollerMsg(_T("Business service object checks created: %u, business service object checks deleted: %u \r\n"), bindedCount, unbindedCount);
   }
   else
   {
      sendPollerMsg(_T("Business service object checks creation disabled \r\n"));
   }
}

/**
 * Business service DCI checks autobinding
 */
void BusinessService::dciCheckAutoBinding()
{
   if (isSecondFilterBindingEnabled())
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Business service(%s): Looking for DCI checks to create"), m_name);
      sendPollerMsg(_T("Business service(%s): Looking for DCI checks to create \r\n"), m_name);
      unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects();
      uint32_t bindedCount = 0;
      uint32_t unbindedCount = 0;
      for (int i = 0; i < objects->size(); i++)
      {
         shared_ptr<NetObj> object = objects->getShared(i);
         if (object->isDataCollectionTarget())
         {
            shared_ptr<DataCollectionTarget> target = static_pointer_cast<DataCollectionTarget>(object);
            unique_ptr<IntegerArray<uint32_t>> dciIds = target->getDCIIds();
            for (int j = 0; j < dciIds->size(); j++)
            {
               shared_ptr<DCObject> dci = target->getDCObjectById(dciIds->get(j), 0);
               if (dci != nullptr)
               {
                  AutoBindDecision decision = isSecondFilterApplicable(object, dci);
                  if (decision != AutoBindDecision_Ignore)
                  {
                     uint32_t foundCheckId = 0;
                     for (auto check : m_checks)
                     {
                        if (check->getType() == BusinessServiceCheck::CheckType::DCI && check->getRelatedObject() == object->getId() && check->getRelatedDCI() == dci->getId())
                        {
                           foundCheckId = check->getId();
                           break;
                        }
                     }
                     if (foundCheckId != 0 && decision == AutoBindDecision_Unbind && isFirstFilterUnbindingEnabled())
                     {
                        nxlog_debug_tag(DEBUG_TAG, 6, _T("Business service(%s): DCI check %u deleted"), foundCheckId);
                        deleteCheck(foundCheckId);
                        unbindedCount++;
                     }
                     if (foundCheckId == 0 && decision == AutoBindDecision_Bind)
                     {
                        BusinessServiceCheck *check = new BusinessServiceCheck(m_id);
                        m_checks.add(check);
                        check->setType(BusinessServiceCheck::DCI);
                        check->setRelatedObject(object->getId());
                        check->setRelatedDCI(dci->getId());
                        TCHAR checkName[MAX_OBJECT_NAME];
                        _sntprintf(checkName, MAX_OBJECT_NAME, _T("%s in %s[%u] DCI check"), dci->getName().cstr(), object->getName(), object->getId());
                        check->setName(checkName);
                        check->generateId();
                        check->setThreshold(m_dciStatusThreshhold);
                        check->saveToDatabase();
                        nxlog_debug_tag(DEBUG_TAG, 6, _T("Business service(%s): DCI check %s[%u] created"), checkName, foundCheckId);
                        NotifyClientsOnBusinessServiceCheckUpdate(*this, check);
                        bindedCount++;
                     }
                  }
               }
            }
         }
      }
      sendPollerMsg(_T("Business service DCI checks created: %u, business service DCI checks deleted: %u  \r\n"), bindedCount, unbindedCount);
   }
   else
   {
      sendPollerMsg(_T("Business service DCI checks creation disabled \r\n"));
   }
}

/**
 * Lock node for status poll
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
 * Lock object for configuration poll
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

/* ************************************
 *
 * Business Service Prototype
 *
 * *************************************
*/

/**
 * Business service prototype constructor
 */
BusinessServicePrototype::BusinessServicePrototype() : m_discoveryPollState(_T("discovery"))
{
   m_pCompiledInstanceDiscoveryFilterScript = nullptr;
}

/**
 * Business service prototype constructor
 */
BusinessServicePrototype::BusinessServicePrototype(const TCHAR *name, uint32_t instanceDiscoveryMethod) : BaseBusinessService(name), m_discoveryPollState(_T("discovery"))
{
   m_instanceDiscoveryMethod = instanceDiscoveryMethod;
   m_pCompiledInstanceDiscoveryFilterScript = nullptr;
}

/**
 * Business service prototype destructor
 */
BusinessServicePrototype::~BusinessServicePrototype()
{
   delete m_pCompiledInstanceDiscoveryFilterScript;
}

/**
 * Load business service prototype from database
 */
bool BusinessServicePrototype::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   if (!super::loadFromDatabase(hdb, id))
      return false;

   delete_and_null(m_pCompiledInstanceDiscoveryFilterScript);
   compileInstanceDiscoveryFilterScript();
   return true;
}

/**
 * Modify business service prototype from request
 */
uint32_t BusinessServicePrototype::modifyFromMessageInternal(NXCPMessage *request)
{
   uint32_t rcc = super::modifyFromMessageInternal(request);
   if (request->isFieldExist(VID_INSTD_FILTER))
   {
      compileInstanceDiscoveryFilterScript();
   }
   return rcc;
}

/**
 * Compile instance discovery filter script if there is one
 */
void BusinessServicePrototype::compileInstanceDiscoveryFilterScript()
{
   if (m_instanceDiscoveryFilter == nullptr)
      return;

   const int errorMsgLen = 512;
   TCHAR errorMsg[errorMsgLen];

   delete m_pCompiledInstanceDiscoveryFilterScript;
   m_pCompiledInstanceDiscoveryFilterScript = NXSLCompile(m_instanceDiscoveryFilter, errorMsg, errorMsgLen, nullptr);
   if (m_pCompiledInstanceDiscoveryFilterScript == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Failed to compile filter script for service instance discovery %s [%u] (%s)"), m_name, m_id, errorMsg);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Compiled filter script for service instance discovery %s [%u] : (%s)"), m_name, m_id, m_instanceDiscoveryFilter);
   }
}

/**
 * Get map of instances (key - name, value - display name) for business service prototype instance discovery
 */
unique_ptr<StringMap> BusinessServicePrototype::getInstances()
{
   StringMap *instanceMap = nullptr;
   if (m_instanceDiscoveryMethod == IDM_SCRIPT)
   {
      if (m_instanceDiscoveryData != nullptr)
      {
         NXSL_VM *vm = GetServerScriptLibrary()->createVM(m_instanceDiscoveryData, new NXSL_ServerEnv());
         if (vm != nullptr)
         {
            vm->setGlobalVariable("$prototype", createNXSLObject(vm));
            vm->run();
            NXSL_Value *value = vm->getResult();
            if (value->isArray())
            {
               instanceMap = new StringMap();
               StringList list;
               value->getValueAsArray()->toStringList(&list);
               for (int i = 0; i < list.size(); i++)
               {
                  instanceMap->set(list.get(i), list.get(i));
               }
            }
            if (value->isHashMap())
            {
               instanceMap = value->getValueAsHashMap()->toStringMap();
            }
            if (value->isString())
            {
               instanceMap = new StringMap();
               instanceMap->set(value->getValueAsCString(), value->getValueAsCString());
            }
            delete vm;
         }
      }
   }

   if (m_instanceDiscoveryMethod == IDM_AGENT_LIST)
   {
      shared_ptr<NetObj> obj = FindObjectById(m_instanceSource);
      if (obj != nullptr && obj->getObjectClass() == OBJECT_NODE && m_instanceDiscoveryData != nullptr)
      {
         StringList *instances = nullptr;
         static_pointer_cast<Node>(obj)->getListFromAgent(m_instanceDiscoveryData, &instances);
         if (instances != nullptr)
         {
            for (int i = 0; i < instances->size(); i++)
            {
               instanceMap->set(instances->get(i), instances->get(i));
            }
            delete instances;
         }
      }
   }

   if (m_instanceDiscoveryMethod == IDM_AGENT_TABLE)
   {
      shared_ptr<NetObj> obj = FindObjectById(m_instanceSource);
      if (obj != nullptr && obj->getObjectClass() == OBJECT_NODE && m_instanceDiscoveryData != nullptr)
      {
         shared_ptr<Table> instanceTable;
         static_pointer_cast<Node>(obj)->getTableFromAgent(m_instanceDiscoveryData, &instanceTable);
         if (instanceTable != nullptr)
         {
            TCHAR buffer[1024];
            instanceMap = new StringMap();
            for(int i = 0; i < instanceTable->getNumRows(); i++)
            {
               instanceTable->buildInstanceString(i, buffer, 1024);
               instanceMap->set(buffer, buffer);
            }
         }
      }
   }

   auto resultMap = make_unique<StringMap>();
   if (m_pCompiledInstanceDiscoveryFilterScript != nullptr && instanceMap != nullptr)
   {
      for (auto instance : *instanceMap)
      {
         NXSL_VM *filter = CreateServerScriptVM(m_pCompiledInstanceDiscoveryFilterScript, shared_ptr<NetObj>());
         if (filter != nullptr)
         {
            shared_ptr<NetObj> obj = FindObjectById(m_instanceSource);
            filter->setGlobalVariable("$1", filter->createValue(instance->key));
            filter->setGlobalVariable("$2", filter->createValue(instance->value));
            filter->setGlobalVariable("$prototype", createNXSLObject(filter));
            filter->setGlobalVariable("$node", obj->createNXSLObject(filter));
            if (filter->run())
            {
               NXSL_Value *value = filter->getResult();
               if (value->isArray())
               {
                  if (value->getValueAsArray()->size() == 1)
                  {
                     resultMap->set(value->getValueAsArray()->get(0)->getValueAsCString(), value->getValueAsArray()->get(0)->getValueAsCString());
                  }
                  if (value->getValueAsArray()->size() > 1)
                  {
                     resultMap->set(value->getValueAsArray()->get(0)->getValueAsCString(), value->getValueAsArray()->get(1)->getValueAsCString());
                  }
               }
               if (value->isBoolean() && value->getValueAsBoolean())
               {
                  resultMap->set(instance->key, instance->value);
               }

            }
            else
            {
               TCHAR buffer[1024];
               _sntprintf(buffer, 1024, _T("%s::%s::%d"), getObjectClassName(), m_name, m_id);
               PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, filter->getErrorText(), m_id);
               nxlog_debug_tag(DEBUG_TAG, 2, _T("Failed to execute instance discovery script for business service prototype %s [%u] (%s)"), m_name, m_id, filter->getErrorText());
            }
            delete filter;
         }
         else
         {
            TCHAR buffer[1024];
            _sntprintf(buffer, 1024, _T("%s::%s::%d"), getObjectClassName(), m_name, m_id);
            PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, _T("Script load error"), m_id);
            nxlog_debug_tag(DEBUG_TAG, 2, _T("Failed to load instance discovery script for business service prototype %s [%u]"), m_name, m_id);
         }
      }      
   }
   delete instanceMap;
   return resultMap;
}

/**
 * Returns loaded business services, created by this business service prototype
 */
unique_ptr<SharedObjectArray<BusinessService>> BusinessServicePrototype::getServices()
{
   unique_ptr<SharedObjectArray<BusinessService>> services = make_unique<SharedObjectArray<BusinessService>>();
   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxBusinessServicesById.getObjects();
   for (int i = 0; i < objects->size(); i++)
   {
      shared_ptr<NetObj> object = objects->getShared(i);
      if (object->getObjectClass() == OBJECT_BUSINESS_SERVICE && static_pointer_cast<BusinessService>(object)->getPrototypeId() == m_id)
      {
         services->add(static_pointer_cast<BusinessService>(object));
      }
   }
   return services;
}

/**
 * Instance discovery poll. Used for automatic creation and deletion of business services
 */
void BusinessServicePrototype::instanceDiscoveryPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   m_pollRequestor = session;
   m_pollRequestId = rqId;

   sendPollerMsg(_T("   Started instance discovery poll for Business service prototype %s[%ld]\r\n"), m_name, m_id);

   poller->startExecution();
   unique_ptr<StringMap> instances = getInstances();
   sendPollerMsg(_T("   Found %ld instances\r\n"), instances->size());
   unique_ptr<SharedObjectArray<BusinessService>> services = getServices();
   sendPollerMsg(_T("   Found %ld services\r\n"), services->size());

   for (auto it = services->begin(); it.hasNext();)
   {
      shared_ptr<BusinessService> service = it.next();
      if (instances->contains(service->getInstance()))
      {
         instances->remove(service->getInstance());
         it.remove();
      }
   }

   for (auto it = services->begin(); it.hasNext();)
   {
      ObjectTransactionStart();
      auto service = it.next();
      sendPollerMsg(_T("   Removing '%s' service\r\n"), service->getName());
      service->deleteObject();
      ObjectTransactionEnd();
      it.remove();
   }

   for (auto inst : *instances)
   {
      auto service = make_shared<BusinessService>(inst->value);
      if (service != nullptr)
      {
         service->setInstance(inst->key);
         service->setPrototypeId(m_id);
         service->setFirstFilter(m_firstBindFilterSource);
         service->setSecondFilter(m_secondBindFilterSource);
         service->copyChecks(m_checks);
         service->setAutoBindFlags(m_autoBindFlags);
         NetObjInsert(service, true, false); // Insert into indexes
         g_businessServiceRoot->addChild(service);
         service->addParent(getParents()->getShared(0));
         g_businessServiceRoot->calculateCompoundStatus();
         service->unhide();
         sendPollerMsg(_T("   Created '%s' service\r\n"), service->getName());
      }
   }

   delete poller;
}

/**
 * Lock object for configuration poll
 */
bool BusinessServicePrototype::lockForDiscoveryPoll()
{
   bool success = false;

   lockProperties();
   if (static_cast<uint32_t>(time(nullptr) - m_discoveryPollState.getLastCompleted()) > g_discoveryPollingInterval)
   {
      success = m_discoveryPollState.schedule();
   }
   unlockProperties();
   return success;
}

/* ************************************
 *
 * Functions
 *
 * *************************************
*/

/**
 * Get list of business service checks of business service or business service prototype
 */
void GetCheckList(uint32_t serviceId, NXCPMessage *response)
{
   shared_ptr<BaseBusinessService> service = static_pointer_cast<BaseBusinessService>(FindObjectById(serviceId));
   if (service == nullptr)
   {
      return;
   }

   int counter = 0;
   for (auto check : *service->getChecks())
   {
      check->fillMessage(response, VID_BUSINESS_SERVICE_CHECK_LIST_BASE + (counter * 10));
      counter++;
   }
   response->setField(VID_BUSINESS_SERVICE_CHECK_COUNT, counter);
}

/**
 * Modify business service check
 */
uint32_t ModifyCheck(NXCPMessage *request)
{
   uint32_t serviceId = request->getFieldAsUInt32(VID_OBJECT_ID);
   shared_ptr<BaseBusinessService> service = static_pointer_cast<BaseBusinessService>(FindObjectById(serviceId));
   if (service == nullptr)
   {
      return RCC_INVALID_OBJECT_ID;
   }
   service->modifyCheckFromMessage(request);
   return RCC_SUCCESS;
}

/**
 * Delete business service check
 */
uint32_t DeleteCheck(uint32_t serviceId, uint32_t checkId)
{
   shared_ptr<BaseBusinessService> service = static_pointer_cast<BaseBusinessService>(FindObjectById(serviceId));
   if (service == nullptr)
   {
      return RCC_INVALID_OBJECT_ID;
   }

   service->deleteCheck(checkId);

   return RCC_SUCCESS;
}

/**
 * Get business service uptime in percents
 */
double GetServiceUptime(uint32_t serviceId, time_t from, time_t to)
{
   double res = 0;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT from_timestamp,to_timestamp FROM business_service_downtime ") _T("WHERE service_id=? AND "
                                                                                                               "((from_timestamp BETWEEN ? AND ? OR to_timestamp BETWEEN ? and ?) OR (from_timestamp<? AND ( to_timestamp=0 OR to_timestamp>? )))"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, serviceId);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, from);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, to);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, from);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, to);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, from);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, to);
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
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT ticket_id,original_ticket_id,original_service_id,check_id,create_timestamp,close_timestamp,reason,check_description FROM business_service_tickets ") _T("WHERE service_id=? AND "
                                                                                                                                                                           "((create_timestamp BETWEEN ? AND ? OR close_timestamp BETWEEN ? and ?) OR (create_timestamp<? AND ( close_timestamp=0 OR close_timestamp>? )))"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, serviceId);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, from);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, to);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, from);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, to);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, from);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, to);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for (int i = 0; i < count; i++)
         {
            uint32_t ticket_id = DBGetFieldULong(hResult, i, 0);
            uint32_t original_ticket_id = DBGetFieldULong(hResult, i, 1);
            uint32_t original_service_id = DBGetFieldULong(hResult, i, 2);
            uint32_t check_id = DBGetFieldLong(hResult, i, 3);
            time_t create_timestamp = static_cast<time_t>(DBGetFieldULong(hResult, i, 4));
            time_t close_timestamp = static_cast<time_t>(DBGetFieldULong(hResult, i, 5));
            TCHAR reason[256];
            DBGetField(hResult, i, 6, reason, 256);
            TCHAR checkDescription[1023];
            DBGetField(hResult, i, 7, checkDescription, 1023);

            msg->setField(VID_BUSINESS_TICKET_LIST_BASE + (i * 10), original_ticket_id != 0 ? original_ticket_id : ticket_id);
            msg->setField(VID_BUSINESS_TICKET_LIST_BASE + (i * 10) + 1, original_ticket_id != 0 ? original_service_id : serviceId);
            msg->setField(VID_BUSINESS_TICKET_LIST_BASE + (i * 10) + 2, check_id);
            msg->setField(VID_BUSINESS_TICKET_LIST_BASE + (i * 10) + 3, create_timestamp);
            msg->setField(VID_BUSINESS_TICKET_LIST_BASE + (i * 10) + 4, close_timestamp);
            msg->setField(VID_BUSINESS_TICKET_LIST_BASE + (i * 10) + 5, reason);
            msg->setField(VID_BUSINESS_TICKET_LIST_BASE + (i * 10) + 6, checkDescription);
         }
         msg->setField(VID_BUSINESS_TICKET_COUNT, count);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
}
