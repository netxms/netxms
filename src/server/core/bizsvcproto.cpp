/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: bizsvcproto.cpp
**
**/

#include "nxcore.h"

/**
 * Business service prototype constructor
 */
BusinessServicePrototype::BusinessServicePrototype() : super(), Pollable(this, Pollable::INSTANCE_DISCOVERY)
{
   m_instanceDiscoveryMethod = IDM_NONE;
   m_instanceSource = 0;
   m_instanceDiscoveryData = nullptr;
   m_instanceDiscoveryFilter = nullptr;
   m_compiledInstanceDiscoveryFilter = nullptr;
}

/**
 * Business service prototype constructor
 */
BusinessServicePrototype::BusinessServicePrototype(const TCHAR *name, uint32_t instanceDiscoveryMethod) : super(name), Pollable(this, Pollable::INSTANCE_DISCOVERY)
{
   m_instanceDiscoveryMethod = instanceDiscoveryMethod;
   m_instanceSource = 0;
   m_instanceDiscoveryData = nullptr;
   m_instanceDiscoveryFilter = nullptr;
   m_compiledInstanceDiscoveryFilter = nullptr;
}

/**
 * Business service prototype destructor
 */
BusinessServicePrototype::~BusinessServicePrototype()
{
   MemFree(m_instanceDiscoveryData);
   MemFree(m_instanceDiscoveryFilter);
   delete m_compiledInstanceDiscoveryFilter;
}

/**
 * Load business service prototype from database
 */
bool BusinessServicePrototype::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   if (!super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   if (!Pollable::loadFromDatabase(hdb, m_id))
      return false;

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT instance_method,instance_data,instance_source,instance_filter,object_status_threshold,dci_status_threshold FROM business_service_prototypes WHERE id=?"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
   {
      DBFreeStatement(hStmt);
      return false;
   }

   m_instanceDiscoveryMethod = DBGetFieldULong(hResult, 0, 0);
   m_instanceDiscoveryData = DBGetField(hResult, 0, 1, nullptr, 0);
   m_instanceSource = DBGetFieldULong(hResult, 0, 2);
   m_instanceDiscoveryFilter = DBGetField(hResult, 0, 3, nullptr, 0);
   m_objectStatusThreshhold = DBGetFieldULong(hResult, 0, 4);
   m_dciStatusThreshhold = DBGetFieldULong(hResult, 0, 5);

   DBFreeResult(hResult);
   DBFreeStatement(hStmt);

   compileInstanceDiscoveryFilterScript();
   return true;
}

/**
 * Save business service prototype to database
 */
bool BusinessServicePrototype::saveToDatabase(DB_HANDLE hdb)
{
   if (!super::saveToDatabase(hdb))
      return false;

   bool success = true;
   if (m_modified & MODIFY_BIZSVC_PROPERTIES)
   {
      static const TCHAR *columns[] = {
            _T("instance_method"), _T("instance_data"), _T("instance_filter"), _T("instance_source"), _T("object_status_threshold"), _T("dci_status_threshold"), nullptr
      };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("business_service_prototypes"), _T("id"), m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_instanceDiscoveryMethod);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_instanceDiscoveryData, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_TEXT, m_instanceDiscoveryFilter, DB_BIND_STATIC);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_instanceSource);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_objectStatusThreshhold);
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_dciStatusThreshhold);
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         unlockProperties();
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }
   return success;
}

/**
 * Delete object from database
 */
bool BusinessServicePrototype::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM business_service_prototypes WHERE id=?"));
   return success;
}

/**
 * Fill message with business service data
 */
void BusinessServicePrototype::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_INSTD_METHOD, m_instanceDiscoveryMethod);
   msg->setField(VID_INSTD_DATA, m_instanceDiscoveryData);
   msg->setField(VID_INSTD_FILTER, m_instanceDiscoveryFilter);
   msg->setField(VID_OBJECT_STATUS_THRESHOLD, m_objectStatusThreshhold);
   msg->setField(VID_DCI_STATUS_THRESHOLD, m_dciStatusThreshhold);
   msg->setField(VID_NODE_ID, m_instanceSource);
}

/**
 * Modify business service prototype from request
 */
uint32_t BusinessServicePrototype::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   if (msg.isFieldExist(VID_INSTD_METHOD))
      m_instanceDiscoveryMethod = msg.getFieldAsUInt32(VID_INSTD_METHOD);

   if (msg.isFieldExist(VID_NODE_ID))
      m_instanceSource = msg.getFieldAsUInt32(VID_NODE_ID);

   if (msg.isFieldExist(VID_INSTD_DATA))
   {
      MemFree(m_instanceDiscoveryData);
      m_instanceDiscoveryData = msg.getFieldAsString(VID_INSTD_DATA);
   }

   if (msg.isFieldExist(VID_INSTD_FILTER))
   {
      MemFree(m_instanceDiscoveryFilter);
      m_instanceDiscoveryFilter = msg.getFieldAsString(VID_INSTD_FILTER);
      compileInstanceDiscoveryFilterScript();
   }

   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Modify business service prototype from NXCP message - stage 2
 */
uint32_t BusinessServicePrototype::modifyFromMessageInternalStage2(const NXCPMessage& msg, ClientSession *session)
{
   // Update all services created from this prototype
   processRelatedServices([](BusinessServicePrototype *prototype, BusinessService *service) -> void { service->updateFromPrototype(*prototype); });
   return RCC_SUCCESS;
}

/**
 * Callback for check modification
 */
void BusinessServicePrototype::onCheckModify(const shared_ptr<BusinessServiceCheck>& check)
{
   // Update matching check on all services created from this prototype
   processRelatedServices([check](BusinessServicePrototype *prototype, BusinessService *service) -> void { service->updateCheckFromPrototype(*check); });
}

/**
 * Callback for check delete
 */
void BusinessServicePrototype::onCheckDelete(uint32_t checkId)
{
   // Delete matching check from all services created from this prototype
   processRelatedServices([checkId](BusinessServicePrototype *prototype, BusinessService *service) -> void { service->deleteCheckFromPrototype(checkId); });
}

/**
 * Thread function for BusinessServicePrototype::processRelatedServices
 */
static void ProcessRelatedServices(shared_ptr<BusinessServicePrototype> prototype, shared_ptr<SharedObjectArray<BusinessService>> services, std::function<void (BusinessServicePrototype*, BusinessService*)> callback)
{
   for(const shared_ptr<BusinessService>& s : *services)
      callback(prototype.get(), s.get());
}

/**
 * Process related business services in separate thread, executing provided callback for each service
 */
void BusinessServicePrototype::processRelatedServices(std::function<void (BusinessServicePrototype*, BusinessService*)> callback)
{
   TCHAR key[32];
   _sntprintf(key, 32, _T("BSUPDATE_%u"), m_id);
   unique_ptr<SharedObjectArray<BusinessService>> services = getServices();
   ThreadPoolExecuteSerialized(g_mainThreadPool, key, ProcessRelatedServices, self(), shared_ptr<SharedObjectArray<BusinessService>>(services.release()), callback);
}

/**
 * Compile instance discovery filter script if there is one
 */
void BusinessServicePrototype::compileInstanceDiscoveryFilterScript()
{
   if ((m_instanceDiscoveryFilter == nullptr) || (*m_instanceDiscoveryFilter == 0))
   {
      delete_and_null(m_compiledInstanceDiscoveryFilter);
      return;
   }

   delete m_compiledInstanceDiscoveryFilter;
   m_compiledInstanceDiscoveryFilter = CompileServerScript(m_instanceDiscoveryFilter, SCRIPT_CONTEXT_BIZSVC, this, 0, _T("%s::%s::InstanceDiscovery"), getObjectClassName(), m_name);
   if (m_compiledInstanceDiscoveryFilter != nullptr)
   {
      if (!m_compiledInstanceDiscoveryFilter->isEmpty())
      {
         nxlog_debug_tag(DEBUG_TAG_BIZSVC, 5, _T("Successfully compiled instance discovery filter script for business service prototype %s [%u]"), m_name, m_id);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_BIZSVC, 5, _T("Instance discovery filter script for business service prototype %s [%u] is empty"), m_name, m_id);
         delete_and_null(m_compiledInstanceDiscoveryFilter);
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_BIZSVC, 3, _T("Compilation error in instance discovery filter script for business service prototype %s [%u]"), m_name, m_id);
   }
}

/**
 * Get instances from agent list
 */
unique_ptr<StringMap> BusinessServicePrototype::getInstancesFromAgentList()
{
   StringMap *instances = nullptr;
   shared_ptr<NetObj> object = FindObjectById(m_instanceSource);
   if ((object != nullptr) && (object->getObjectClass() == OBJECT_NODE))
   {
      StringList *instanceList = nullptr;
      if (static_cast<Node&>(*object).getListFromAgent(m_instanceDiscoveryData, &instanceList) == ERR_SUCCESS)
      {
         instances = new StringMap();
         for (int i = 0; i < instanceList->size(); i++)
            instances->set(instanceList->get(i), instanceList->get(i));
         delete instanceList;
      }
   }
   return unique_ptr<StringMap>(instances);
}

/**
 * Get instances from agent table
 */
unique_ptr<StringMap> BusinessServicePrototype::getInstancesFromAgentTable()
{
   StringMap *instances = nullptr;
   shared_ptr<NetObj> object = FindObjectById(m_instanceSource);
   if ((object != nullptr) && (object->getObjectClass() == OBJECT_NODE))
   {
      shared_ptr<Table> instanceTable;
      if (static_cast<Node&>(*object).getTableFromAgent(m_instanceDiscoveryData, &instanceTable) == ERR_SUCCESS)
      {
         TCHAR buffer[1024];
         instances = new StringMap();
         for(int i = 0; i < instanceTable->getNumRows(); i++)
         {
            instanceTable->buildInstanceString(i, buffer, 1024);
            instances->set(buffer, buffer);
         }
      }
   }
   return unique_ptr<StringMap>(instances);
}

/**
 * Get instances from NXSL script
 */
unique_ptr<StringMap> BusinessServicePrototype::getInstancesFromScript()
{
   NXSL_VM *vm = CreateServerScriptVM(m_instanceDiscoveryData, self());
   if (vm == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_BIZSVC, 5, _T("Cannot create NXSL VM for instance discovery script for business service prototype %s [%u]"), m_name, m_id);
      return unique_ptr<StringMap>();
   }

   StringMap *instances = nullptr;
   if (vm->run())
   {
      NXSL_Value *value = vm->getResult();
      if (value->isArray())
      {
         instances = new StringMap();
         NXSL_Array *a = value->getValueAsArray();
         for (int i = 0; i < a->size(); i++)
         {
            const TCHAR *v = a->get(i)->getValueAsCString();
            instances->set(v, v);
         }
      }
      else if (value->isHashMap())
      {
         instances = value->getValueAsHashMap()->toStringMap();
      }
      else if (value->isString())
      {
         instances = new StringMap();
         instances->set(value->getValueAsCString(), value->getValueAsCString());
      }
   }
   else
   {
      ReportScriptError(SCRIPT_CONTEXT_BIZSVC, this, 0, vm->getErrorText(), _T("%s::%s::InstanceDiscovery"), getObjectClassName(), m_name);
      nxlog_debug_tag(DEBUG_TAG_BIZSVC, 5, _T("Failed to execute instance discovery script for business service prototype %s [%u] (%s)"), m_name, m_id, vm->getErrorText());
   }
   delete vm;
   return unique_ptr<StringMap>(instances);
}

/**
 * Get map of instances (key - name, value - display name) for business service prototype instance discovery
 */
unique_ptr<StringMap> BusinessServicePrototype::getInstances()
{
   if ((m_instanceDiscoveryData == nullptr) || (*m_instanceDiscoveryData == 0))
   {
      nxlog_debug_tag(DEBUG_TAG_BIZSVC, 5, _T("Invalid instance discovery data in business service prototype %s [%u]"), m_name, m_id);
      return unique_ptr<StringMap>();
   }

   unique_ptr<StringMap> instanceMap;
   switch(m_instanceDiscoveryMethod)
   {
      case IDM_AGENT_LIST:
         instanceMap = getInstancesFromAgentList();
         break;
      case IDM_AGENT_TABLE:
         instanceMap = getInstancesFromAgentTable();
         break;
      case IDM_SCRIPT:
         instanceMap = getInstancesFromScript();
         break;
      default:
         nxlog_debug_tag(DEBUG_TAG_BIZSVC, 5, _T("Unsupported instance discovery method %d in business service prototype %s [%u]"), m_instanceDiscoveryMethod, m_name, m_id);
         break;
   }

   if (instanceMap == nullptr)
      return unique_ptr<StringMap>();

   if ((m_compiledInstanceDiscoveryFilter == nullptr) || m_compiledInstanceDiscoveryFilter->isEmpty())
      return instanceMap;

   auto resultMap = make_unique<StringMap>();
   for (auto instance : *instanceMap)
   {
      NXSL_VM *filter = CreateServerScriptVM(m_compiledInstanceDiscoveryFilter, FindObjectById(m_instanceSource));
      if (filter != nullptr)
      {
         filter->setGlobalVariable("$1", filter->createValue(instance->key));
         filter->setGlobalVariable("$2", filter->createValue(instance->value));
         filter->setGlobalVariable("$prototype", createNXSLObject(filter));
         if (filter->run())
         {
            NXSL_Value *value = filter->getResult();
            if (value->isArray())
            {
               if (value->getValueAsArray()->size() == 1)
               {
                  resultMap->set(value->getValueAsArray()->get(0)->getValueAsCString(), value->getValueAsArray()->get(0)->getValueAsCString());
               }
               else if (value->getValueAsArray()->size() > 1)
               {
                  resultMap->set(value->getValueAsArray()->get(0)->getValueAsCString(), value->getValueAsArray()->get(1)->getValueAsCString());
               }
            }
            else if (value->isTrue())
            {
               resultMap->set(instance->key, instance->value);
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_BIZSVC, 6, _T("BusinessServicePrototype::getInstances(%s [%u]): instance \"%s\" blocked by filtering script"), m_name, m_id, instance->key);
            }
         }
         else
         {
            ReportScriptError(SCRIPT_CONTEXT_BIZSVC, this, 0, filter->getErrorText(), _T("%s::%s::InstanceDiscoveryFilter"), getObjectClassName(), m_name);
            nxlog_debug_tag(DEBUG_TAG_BIZSVC, 5, _T("Failed to execute instance discovery filter script for business service prototype %s [%u] (%s)"), m_name, m_id, filter->getErrorText());
            resultMap.reset();
         }
         delete filter;
      }
      else
      {
         ReportScriptError(SCRIPT_CONTEXT_BIZSVC, this, 0, _T("Script load error"), _T("%s::%s::InstanceDiscoveryFilter"), getObjectClassName(), m_name);
         nxlog_debug_tag(DEBUG_TAG_BIZSVC, 5, _T("Failed to load instance discovery filter script for business service prototype %s [%u]"), m_name, m_id);
         resultMap.reset();
      }
   }
   return resultMap;
}

/**
 * Returns loaded business services, created by this business service prototype
 */
unique_ptr<SharedObjectArray<BusinessService>> BusinessServicePrototype::getServices()
{
   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxBusinessServicesById.getObjects([](NetObj* object, void* context) -> bool {
      return (object->getObjectClass() == OBJECT_BUSINESSSERVICE) && (static_cast<BusinessService*>(object)->getPrototypeId() == CAST_FROM_POINTER(context, uint32_t));
   }, CAST_TO_POINTER(m_id, void*));

   unique_ptr<SharedObjectArray<BusinessService>> services = make_unique<SharedObjectArray<BusinessService>>(objects->size());
   for (int i = 0; i < objects->size(); i++)
      services->add(static_pointer_cast<BusinessService>(objects->getShared(i)));
   return services;
}

/**
 * Instance discovery poll. Used for automatic creation and deletion of business services
 */
void BusinessServicePrototype::instanceDiscoveryPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_instancePollState.complete(0);
      unlockProperties();
      return;
   }
   unlockProperties();

   poller->setStatus(_T("wait for lock"));
   pollerLock(instance);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   m_pollRequestId = rqId;

   sendPollerMsg(_T("Started instance discovery poll of business service prototype %s\r\n"), m_name);
   nxlog_debug_tag(DEBUG_TAG_BIZSVC, 5, _T("Started instance discovery poll of business service prototype %s [%u]"), m_name, m_id);
   unique_ptr<StringMap> instances = getInstances();
   if (instances != nullptr)
   {
      unique_ptr<SharedObjectArray<BusinessService>> services = getServices();
      nxlog_debug_tag(DEBUG_TAG_BIZSVC, 5, _T("BusinessServicePrototype::instanceDiscoveryPoll(%s [%u]): %d instances read, %d existing services"), m_name, m_id, instances->size(), services->size());

      SharedPtrIterator<BusinessService> it = services->begin();
      while (it.hasNext())
      {
         const shared_ptr<BusinessService>& service = it.next();
         if (instances->contains(service->getInstance()))
         {
            instances->remove(service->getInstance());
            it.remove();
         }
      }

      // At this point services array contains only services to be deleted
      for (const shared_ptr<BusinessService>& service : *services)
      {
         nxlog_debug_tag(DEBUG_TAG_BIZSVC, 4, _T("Business service \"%s\" [%u] removed by instance discovery from prototype \"%s\" [%u]"), service->getName(), service->getId(), m_name, m_id);
         sendPollerMsg(_T("   Business service \"%s\" removed\r\n"), service->getName());
         service->deleteObject();
      }

      for (auto instance : *instances)
      {
         auto service = make_shared<BusinessService>(*this, instance->value, instance->key);
         NetObjInsert(service, true, false); // Insert into indexes
         service->updateFromPrototype(*this);
         shared_ptr<NetObj> parent = getParents()->getShared(0);
         linkObjects(parent, service);
         parent->calculateCompoundStatus();
         service->unhide();
         nxlog_debug_tag(DEBUG_TAG_BIZSVC, 4, _T("Business service \"%s\" [%u] created by instance discovery from prototype \"%s\" [%u]"), service->getName(), service->getId(), m_name, m_id);
         sendPollerMsg(_T("   Business service \"%s\" created\r\n"), service->getName());
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_BIZSVC, 4, _T("BusinessServicePrototype::instanceDiscoveryPoll(%s [%u]): cannot read instances"), m_name, m_id);
      sendPollerMsg(POLLER_WARNING _T("   Cannot read list of instances\r\n"));
   }

   sendPollerMsg(_T("Finished instance discovery poll of business service prototype %s\r\n"), m_name);
   nxlog_debug_tag(DEBUG_TAG_BIZSVC, 5, _T("Finished instance discovery poll of business service prototype %s [%u]"), m_name, m_id);
   pollerUnlock();
}

/**
 * Lock object for instance discovery poll
 */
bool BusinessServicePrototype::lockForInstanceDiscoveryPoll()
{
   bool success = false;
   lockProperties();
   if (static_cast<uint32_t>(time(nullptr) - m_instancePollState.getLastCompleted()) > getCustomAttributeAsUInt32(_T("SysConfig:DataCollection.InstancePollingInterval"), g_instancePollingInterval))
   {
      success = m_instancePollState.schedule();
   }
   unlockProperties();
   return success;
}
