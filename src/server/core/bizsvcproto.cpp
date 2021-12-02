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
bool BusinessServicePrototype::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   if (!super::loadFromDatabase(hdb, id))
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
   m_instanceSource = DBGetFieldULong(hResult, 0, 3);
   m_instanceDiscoveryFilter = DBGetField(hResult, 0, 4, nullptr, 0);
   m_objectStatusThreshhold = DBGetFieldULong(hResult, 0, 5);
   m_dciStatusThreshhold = DBGetFieldULong(hResult, 0, 6);

   DBFreeResult(hResult);
   DBFreeStatement(hStmt);

   compileInstanceDiscoveryFilterScript();
   return true;
}

/**
 * Save business service to database
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
      bool success = false;
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
   }
   return success;
}

/**
 * Fill message with business service data
 */
void BusinessServicePrototype::fillMessageInternal(NXCPMessage *msg, uint32_t userId)
{
   AutoBindTarget::fillMessage(msg);
   msg->setField(VID_INSTD_METHOD, m_instanceDiscoveryMethod);
   msg->setField(VID_INSTD_DATA, m_instanceDiscoveryData);
   msg->setField(VID_INSTD_FILTER, m_instanceDiscoveryFilter);
   msg->setField(VID_OBJECT_STATUS_THRESHOLD, m_objectStatusThreshhold);
   msg->setField(VID_DCI_STATUS_THRESHOLD, m_dciStatusThreshhold);
   msg->setField(VID_NODE_ID, m_instanceSource);
   return super::fillMessageInternal(msg, userId);
}

/**
 * Modify business service prototype from request
 */
uint32_t BusinessServicePrototype::modifyFromMessageInternal(const NXCPMessage& msg)
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

   return super::modifyFromMessageInternal(msg);
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

   delete m_compiledInstanceDiscoveryFilter;
   m_compiledInstanceDiscoveryFilter = NXSLCompile(m_instanceDiscoveryFilter, errorMsg, errorMsgLen, nullptr);
   if (m_compiledInstanceDiscoveryFilter == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_BIZSVC, 2, _T("Failed to compile filter script for service instance discovery %s [%u] (%s)"), m_name, m_id, errorMsg);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_BIZSVC, 4, _T("Compiled filter script for service instance discovery %s [%u] : (%s)"), m_name, m_id, m_instanceDiscoveryFilter);
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
      return unique_ptr<StringMap>();

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
      TCHAR buffer[1024];
      _sntprintf(buffer, 1024, _T("%s::%s::InstanceDiscovery"), getObjectClassName(), m_name);
      PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, vm->getErrorText(), 0);
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

   auto resultMap = make_unique<StringMap>();
   if ((m_compiledInstanceDiscoveryFilter != nullptr) && !m_compiledInstanceDiscoveryFilter->isEmpty())
   {
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
               _sntprintf(buffer, 1024, _T("%s::%s::InstanceDiscoveryFilter"), getObjectClassName(), m_name);
               PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, filter->getErrorText(), 0);
               nxlog_debug_tag(DEBUG_TAG_BIZSVC, 5, _T("Failed to execute instance discovery filter script for business service prototype %s [%u] (%s)"), m_name, m_id, filter->getErrorText());
            }
            delete filter;
         }
         else
         {
            TCHAR buffer[1024];
            _sntprintf(buffer, 1024, _T("%s::%s::InstanceDiscoveryFilter"), getObjectClassName(), m_name);
            PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, _T("Script load error"), 0);
            nxlog_debug_tag(DEBUG_TAG_BIZSVC, 5, _T("Failed to load instance discovery filter script for business service prototype %s [%u]"), m_name, m_id);
         }
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
      return (object->getObjectClass() == OBJECT_BUSINESS_SERVICE) && (static_cast<BusinessService*>(object)->getPrototypeId() == CAST_FROM_POINTER(context, uint32_t));
   }, CAST_TO_POINTER(m_id, void*));

   unique_ptr<SharedObjectArray<BusinessService>> services = make_unique<SharedObjectArray<BusinessService>>(objects->size());
   for (int i = 0; i < objects->size(); i++)
      services->add(static_pointer_cast<BusinessService>(objects->getShared(i)));
   return services;
}

/**
 * Instance discovery poll. Used for automatic creation and deletion of business services
 */
void BusinessServicePrototype::instanceDiscoveryPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
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
   unique_ptr<StringMap> instances = getInstances();
   if (instances != nullptr)
   {
      unique_ptr<SharedObjectArray<BusinessService>> services = getServices();

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
         nxlog_debug_tag(DEBUG_TAG_BIZSVC, 4, _T("Business service \"%s\" [%u] removed by instance discovery from prototype \"%s\" [%u]"),
                  service->getName(), service->getId(), m_name, m_id);
         sendPollerMsg(_T("   Business service \"%s\" removed\r\n"), service->getName());
         service->deleteObject();
         it.remove();
      }

      for (auto instance : *instances)
      {
         auto service = make_shared<BusinessService>(*this, instance->value, instance->key);
         NetObjInsert(service, true, false); // Insert into indexes
         shared_ptr<NetObj> parent = getParents()->getShared(0);
         parent->addChild(service);
         service->addParent(parent);
         parent->calculateCompoundStatus();
         service->unhide();
         nxlog_debug_tag(DEBUG_TAG_BIZSVC, 4, _T("Business service \"%s\" [%u] created by instance discovery from prototype \"%s\" [%u]"),
                  service->getName(), service->getId(), m_name, m_id);
         sendPollerMsg(_T("   Business service \"%s\" created\r\n"), service->getName());
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_BIZSVC, 4, _T("Cannot read instances for business service prototype \"%s\" [%u]"), m_name, m_id);
      sendPollerMsg(POLLER_WARNING _T("   Cannot read list of instances\r\n"));
   }

   sendPollerMsg(_T("Finished instance discovery poll of business service prototype %s\r\n"), m_name);
   pollerUnlock();
}

/**
 * Lock object for instance discovery poll
 */
bool BusinessServicePrototype::lockForInstanceDiscoveryPoll()
{
   bool success = false;
   lockProperties();
   if (static_cast<uint32_t>(time(nullptr) - m_instancePollState.getLastCompleted()) > g_instancePollingInterval)
   {
      success = m_instancePollState.schedule();
   }
   unlockProperties();
   return success;
}
