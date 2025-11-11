/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: netmap.cpp
**
**/

#include "nxcore.h"
#include "netxms-regex.h"

static PCRE *nodeRegexp = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("\"(?:nodeId|objectId|baseObjectId|rootObjectId|contextObjectId)\"\\s*:\\s*(\\d+)")), PCRE_COMMON_FLAGS | PCRE_CASELESS, nullptr, nullptr, nullptr);
static PCRE *dciRegexp = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("\"dciId\"\\s*:\\s*(\\d+)")), PCRE_COMMON_FLAGS | PCRE_CASELESS, nullptr, nullptr, nullptr);


/**
 * Default constructor
 */
DashboardBase::DashboardBase() : super(), Pollable(this, Pollable::AUTOBIND), AutoBindTarget(this), DelegateObject(this), m_elements(0, 16, Ownership::True)
{
   m_numColumns = 1;
   m_status = STATUS_NORMAL;
}

/**
 * Constructor for creating new dashboard object
 */
DashboardBase::DashboardBase(const TCHAR *name) : super(name), Pollable(this, Pollable::AUTOBIND), AutoBindTarget(this), DelegateObject(this), m_elements(0, 16, Ownership::True)
{
   m_numColumns = 1;
   m_status = STATUS_NORMAL;
}

/**
 * Constructor for creating new dashboard object
 */
DashboardBase::DashboardBase(const TCHAR *name, DashboardBase *source) : super(name), Pollable(this, Pollable::AUTOBIND), AutoBindTarget(this), DelegateObject(this), m_elements(0, 16, Ownership::True)
{
   for (auto e : source->m_elements)
   {
      m_elements.add(new DashboardElement(*e));
      updateObjectAndDciList(e);
   }
   m_numColumns = source->m_numColumns;
   m_flags = source->m_flags;
   setComments(source->getComments());
}

/**
 * Update dashboard instance from template
 */
void DashboardBase::updateFromTemplate(DashboardBase *source)
{
   lockProperties();
   m_elements.clear();
   for (auto e : source->m_elements)
   {
      m_elements.add(new DashboardElement(*e));
      updateObjectAndDciList(e);
   }
   m_numColumns = source->m_numColumns;
   unlockProperties();
   setModified(MODIFY_OTHER);
}

/**
 * Redefined status calculation
 */
void DashboardBase::calculateCompoundStatus(bool forcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Create object from database
 */
bool DashboardBase::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   if (!super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   if (!AutoBindTarget::loadFromDatabase(hdb, id))
      return false;

   if (!Pollable::loadFromDatabase(hdb, id))
      return false;

   m_status = STATUS_NORMAL;

   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT element_type,element_data,layout_data FROM dashboard_elements WHERE dashboard_id=%u ORDER BY element_id"), id);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult == nullptr)
      return false;

   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      DashboardElement *e = new DashboardElement;
      e->m_type = DBGetFieldInt32(hResult, i, 0);
      e->m_data = DBGetField(hResult, i, 1, nullptr, 0);
      e->m_layout = DBGetField(hResult, i, 2, nullptr, 0);
      m_elements.add(e);
      updateObjectAndDciList(e);
   }

   DBFreeResult(hResult);
   return true;
}

/**
 * Save object to database
 */
bool DashboardBase::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_OTHER))
      success = AutoBindTarget::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_OTHER))
   {
      success = executeQueryOnObject(hdb, L"DELETE FROM dashboard_elements WHERE dashboard_id=?");
      lockProperties();
      if (success && !m_elements.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, L"INSERT INTO dashboard_elements (dashboard_id,element_id,element_type,element_data,layout_data) VALUES (?,?,?,?,?)");
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; success && (i < m_elements.size()); i++)
            {
               DashboardElement *element = m_elements.get(i);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, i);
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, element->m_type);
               DBBind(hStmt, 4, DB_SQLTYPE_TEXT, element->m_data, DB_BIND_STATIC);
               DBBind(hStmt, 5, DB_SQLTYPE_TEXT, element->m_layout, DB_BIND_STATIC);
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
      unlockProperties();

   }
   return success;
}

/**
 * Delete object from database
 */
bool DashboardBase::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, L"DELETE FROM dashboard_elements WHERE dashboard_id=?");
   if (success)
      success = AutoBindTarget::deleteFromDatabase(hdb);
   return success;
}

/**
 * Create NXCP message with object's data
 */
void DashboardBase::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);

   msg->setField(VID_NUM_COLUMNS, static_cast<uint16_t>(m_numColumns));
   msg->setField(VID_NUM_ELEMENTS, static_cast<uint32_t>(m_elements.size()));

   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < m_elements.size(); i++)
   {
      DashboardElement *element = m_elements.get(i);
      msg->setField(fieldId++, static_cast<uint16_t>(element->m_type));
      msg->setField(fieldId++, CHECK_NULL_EX(element->m_data));
      msg->setField(fieldId++, CHECK_NULL_EX(element->m_layout));
      fieldId += 7;
   }
}

/**
 * Fill NXCP message with object's data - stage 2
 * Object's properties are not locked when this method is called. Should be
 * used only to fill data where properties lock is not enough (like data
 * collection configuration).
 */
void DashboardBase::fillMessageUnlocked(NXCPMessage *msg, uint32_t userId)
{
   AutoBindTarget::fillMessage(msg);
}

/**
 * Modify object from NXCP message
 */
uint32_t DashboardBase::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   AutoBindTarget::modifyFromMessage(msg);

   if (msg.isFieldExist(VID_NUM_COLUMNS))
      m_numColumns = msg.getFieldAsInt16(VID_NUM_COLUMNS);

   if (msg.isFieldExist(VID_NUM_ELEMENTS))
   {
      m_elements.clear();
      m_objectSet.clear();
      m_dciSet.clear();

      int count = (int)msg.getFieldAsUInt32(VID_NUM_ELEMENTS);
      uint32_t fieldId = VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         DashboardElement *e = new DashboardElement;
         e->m_type = msg.getFieldAsInt16(fieldId++);
         e->m_data = msg.getFieldAsString(fieldId++);
         e->m_layout = msg.getFieldAsString(fieldId++);
         fieldId += 7;
         m_elements.add(e);
         updateObjectAndDciList(e);
      }
   }

   return super::modifyFromMessageInternal(msg, session);
}

void DashboardBase::updateObjectAndDciList(DashboardElement *e)
{
   if (nodeRegexp != nullptr)
   {
      int cgcount = 0;
      int ofset = 0;
      do
      {
         int offsets[9];
         cgcount = _pcre_exec_t(nodeRegexp, nullptr, reinterpret_cast<const PCRE_TCHAR*>(e->m_data), static_cast<int>(_tcslen(e->m_data)), ofset, 0, offsets, 9);
         if (cgcount > 0)
         {
            int i = offsets[2] == -1 ? 2 : 1;
            TCHAR num[16];
            int len = MIN(offsets[i * 2 + 1] - offsets[i * 2], 15);
            memcpy(num, &e->m_data[offsets[i * 2]], len * sizeof(TCHAR));
            num[len] = 0;

            TCHAR *eptr;
            uint32_t n = _tcstoul(num, &eptr, 10);
            if (*eptr == 0)
            {
               m_objectSet.put(n);
            }
            ofset = offsets[1];
         }

      } while(cgcount > 0);
   }

   if (dciRegexp != nullptr)
   {
      int cgcount = 0;
      int ofset = 0;
      do
      {
         int offsets[9];
         cgcount = _pcre_exec_t(dciRegexp, nullptr, reinterpret_cast<const PCRE_TCHAR*>(e->m_data), static_cast<int>(_tcslen(e->m_data)), ofset, 0, offsets, 9);
         if (cgcount > 0)
         {
            TCHAR num[16];
            int i = offsets[2] == -1 ? 2 : 1;
            int len = MIN(offsets[i * 2 + 1] - offsets[i * 2], 15);
            memcpy(num, &e->m_data[offsets[i * 2]], len * sizeof(TCHAR));
            num[len] = 0;

            TCHAR *eptr;
            uint32_t n = _tcstoul(num, &eptr, 10);
            if (*eptr == 0)
            {
               m_dciSet.put(n);
            }
            ofset = offsets[1];
         }
      } while(cgcount > 0);
   }
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool DashboardBase::showThresholdSummary() const
{
   return false;
}

/**
 * Serialize object to JSON
 */
json_t *DashboardBase::toJson()
{
   json_t *root = super::toJson();
   AutoBindTarget::toJson(root);

   lockProperties();
   json_object_set_new(root, "numColumns", json_integer(m_numColumns));
   json_object_set_new(root, "elements", json_object_array(m_elements));
   unlockProperties();

   return root;
}

/**
 * Get element script
 */
String DashboardBase::getElementScript(int index) const
{
   StringBuffer script;
   lockProperties();
   if (m_elements.size() > index)
   {
      DashboardElement *e = m_elements.get(index);
      if (e->m_type == 30 || e->m_type == 31 || e->m_type == 6)
      {
         char *data = UTF8StringFromTString(e->m_data);
         json_error_t error;
         json_t *element = json_loads(data, 0, &error);
         if (element != nullptr)
         {
            const char *source = json_object_get_string_utf8(element, "script", "");
            script.appendUtf8String(source);
            json_decref(element);
         }
         else
         {
            nxlog_debug_tag(_T("dashboard"), 1, _T("Dashboard::getElementScript(%s [%u]): failed to load JSON for %d element (%hs)"), m_name, m_id, index, error.text);
         }
         MemFree(data);
      }
      else
      {
         nxlog_debug_tag(_T("dashboard"), 1, _T("Dashboard::getElementScript(%s [%u]): Incorrect %d element type %d"), m_name, m_id, index, e->m_type);
      }
   }
   else
   {
      nxlog_debug_tag(_T("dashboard"), 1, _T("Dashboard::getElementScript(%s [%u]): invalid element index %d"), m_name, m_id, index);
   }
   unlockProperties();
   return script;
}

/**
 * Get element script
 */
bool DashboardBase::isElementContextObject(int index, uint32_t contextObject) const
{
   bool isContextObject = false;
   lockProperties();
   if (m_elements.size() > index)
   {
      DashboardElement *e = m_elements.get(index);
      if (e->m_type == 30 || e->m_type == 31 || e->m_type == 6)
      {
         char *data = UTF8StringFromTString(e->m_data);
         json_error_t error;
         json_t *element = json_loads(data, 0, &error);
         if (element != nullptr)
         {
            uint32_t objectId = json_object_get_uint32(element, "objectId", 0);
            isContextObject = (objectId == contextObject);
            json_decref(element);
         }
         else
         {
            nxlog_debug_tag(_T("dashboard"), 1, _T("Dashboard::isElementContextObject(%s [%u]): failed to load JSON for %d element"), m_name, m_id, index);
         }
         MemFree(data);
      }
      else
      {
         nxlog_debug_tag(_T("dashboard"), 1, _T("Dashboard::isElementContextObject(%s [%u]): not supported %d element type %d"), m_name, m_id, index, e->m_type);
      }
   }
   else
   {
      nxlog_debug_tag(_T("dashboard"), 1, _T("Dashboard::isElementContextObject(%s [%u]): invalid element index %d"), m_name, m_id, index);
   }
   unlockProperties();
   return isContextObject;
}


/*****************************************************************
 * Dashboard class
 *****************************************************************/


/**
 * Default constructor
 */
Dashboard::Dashboard() : super()
{
   m_displayPriority = 0;
   m_forcedContextObjectId = 0;
}

/**
 * Constructor for creating new dashboard object
 */
Dashboard::Dashboard(const TCHAR *name) : super(name)
{
	m_displayPriority = 0;
	m_forcedContextObjectId = 0;
}

/**
 * Constructor for creating new dashboard object
 */
Dashboard::Dashboard(const TCHAR *name, uint32_t contextId, DashboardBase *source) : super(name, source)
{
   m_displayPriority = 0;
   m_forcedContextObjectId = contextId;
}

/**
 * Create object from database
 */
bool Dashboard::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
	if (!super::loadFromDatabase(hdb, id, preparedStatements))
		return false;

	TCHAR query[256];
	_sntprintf(query, 256, _T("SELECT num_columns,display_priority,forced_context_object_id FROM dashboards WHERE id=%u"), id);
	DB_RESULT hResult = DBSelect(hdb, query);
	if (hResult == nullptr)
		return false;
	if (DBGetNumRows(hResult) > 0)
	{
		m_numColumns = DBGetFieldLong(hResult, 0, 0);
      m_displayPriority = DBGetFieldLong(hResult, 0, 1);
      m_forcedContextObjectId = DBGetFieldULong(hResult, 0, 2);
	}
	DBFreeResult(hResult);
	return true;
}

/**
 * Save object to database
 */
bool Dashboard::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_OTHER))
   {
      static const wchar_t *columns[] = { L"num_columns", L"display_priority", L"forced_context_object_id", nullptr };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"dashboards", L"id", m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_numColumns);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_displayPriority);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_forcedContextObjectId);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_id);
         unlockProperties();
         success = DBExecute(hStmt);
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
bool Dashboard::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, L"DELETE FROM dashboards WHERE id=?");
   return success;
}

/**
 * Create NXCP message with object's data
 */
void Dashboard::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);

   msg->setField(VID_DISPLAY_PRIORITY, m_displayPriority);
   msg->setField(VID_FORCED_CONTEXT_OBJECT, m_forcedContextObjectId);
}

/**
 * Modify object from NXCP message
 */
uint32_t Dashboard::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   if (msg.isFieldExist(VID_DISPLAY_PRIORITY))
      m_displayPriority = msg.getFieldAsInt32(VID_DISPLAY_PRIORITY);

   if (msg.isFieldExist(VID_FORCED_CONTEXT_OBJECT))
      m_forcedContextObjectId = msg.getFieldAsUInt32(VID_FORCED_CONTEXT_OBJECT);

	return super::modifyFromMessageInternal(msg, session);
}

/**
 * Serialize object to JSON
 */
json_t *Dashboard::toJson()
{
   json_t *root = super::toJson();

   lockProperties();
   json_object_set_new(root, "displayPriority", json_integer(m_displayPriority));
   json_object_set_new(root, "forcedContextObjectId", json_integer(m_forcedContextObjectId));
   unlockProperties();

   return root;
}

/**
 * Perform automatic assignment to objects
 */
void Dashboard::autobindPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   poller->setStatus(_T("wait for lock"));
   pollerLock(autobind);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   m_pollRequestId = rqId;
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Starting autobind poll of dashboard %s [%u]"), m_name, m_id);
   poller->setStatus(_T("checking objects"));

   if (!isAutoBindEnabled())
   {
      sendPollerMsg(_T("Automatic object binding is disabled\r\n"));
      nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of dashboard %s [%u])"), m_name, m_id);
      pollerUnlock();
      return;
   }

   NXSL_VM *cachedFilterVM = nullptr;
   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects(
      [] (NetObj *object, void *context) -> bool
      {
         if (object->isDataCollectionTarget())
            return true;
         int objectClass = object->getObjectClass();
         return (objectClass == OBJECT_NETWORK) || (objectClass == OBJECT_SERVICEROOT) || (objectClass == OBJECT_SUBNET) || (objectClass == OBJECT_ZONE) || (objectClass == OBJECT_CONDITION) || (objectClass == OBJECT_CONTAINER) || (objectClass == OBJECT_COLLECTOR) || (objectClass == OBJECT_RACK);
      }, nullptr);

   for (int i = 0; i < objects->size(); i++)
   {
      shared_ptr<NetObj> object = objects->getShared(i);

      AutoBindDecision decision = isApplicable(&cachedFilterVM, object, this);
      if ((decision == AutoBindDecision_Ignore) || ((decision == AutoBindDecision_Unbind) && !isAutoUnbindEnabled()))
         continue;   // Decision cannot affect checks

      if (decision == AutoBindDecision_Bind)
      {
         if (object->addDashboard(m_id))
         {
            StringBuffer cn(object->getObjectClassName());
            cn.toLowercase();
            sendPollerMsg(_T("   Adding to %s %s\r\n"), cn.cstr(), object->getName());
            nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("Dashboard::autobindPoll(): adding dashboard \"%s\" [%u] to %s \"%s\" [%u]"), m_name, m_id, cn.cstr(), object->getName(), object->getId());
         }
      }
      else if (decision == AutoBindDecision_Unbind)
      {
         if (object->removeDashboard(m_id))
         {
            StringBuffer cn(object->getObjectClassName());
            cn.toLowercase();
            nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("Dashboard::autobindPoll(): removing dashboard \"%s\" [%u] from %s \"%s\" [%u]"), m_name, m_id, cn.cstr(), object->getName(), object->getId());
            sendPollerMsg(_T("   Removing from %s %s\r\n"), cn.cstr(), object->getName());
         }
      }
   }
   delete cachedFilterVM;

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of dashboard %s [%u])"), m_name, m_id);
}

/*****************************************************************
 * DashboardTemplate class
 *****************************************************************/

/**
 * Default constructor
 */
DashboardTemplate::DashboardTemplate() : super()
{
}

/**
 * Constructor for creating new dashboard object
 */
DashboardTemplate::DashboardTemplate(const TCHAR *name) : super(name)
{
}

/**
 * Create object from database
 */
bool DashboardTemplate::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   if (!super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT num_columns,name_template FROM dashboard_templates WHERE id=%u"), id);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult == nullptr)
      return false;
   if (DBGetNumRows(hResult) > 0)
   {
      m_numColumns = DBGetFieldLong(hResult, 0, 0);
      m_nameTemplate = DBGetFieldAsSharedString(hResult, 0, 1);
   }
   DBFreeResult(hResult);

   _sntprintf(query, 256, _T("SELECT instance_object_id,instance_dashboard_id FROM dashboard_template_instances WHERE dashboard_template_id=%u"), id);
   hResult = DBSelect(hdb, query);
   if (hResult == nullptr)
      return false;
   int count = DBGetNumRows(hResult);
   if (count > 0)
   {
      for (int i = 0; i < count; i++)
         m_instanceDashboards[DBGetFieldLong(hResult, i, 0)] = DBGetFieldLong(hResult, i, 1);
   }
   DBFreeResult(hResult);

   return true;
}

/**
 * Save object to database
 */
bool DashboardTemplate::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_OTHER))
   {
      static const wchar_t *columns[] = { L"num_columns", L"name_template", nullptr };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"dashboard_templates", L"id", m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_numColumns);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_nameTemplate, DB_BIND_STATIC, 255);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_id);
         unlockProperties();
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }

      if (success)
      {
         success = executeQueryOnObject(hdb, L"DELETE FROM dashboard_template_instances WHERE dashboard_template_id=?");
         lockProperties();
         if (success && (m_instanceDashboards.size() != 0))
         {
            DB_STATEMENT hStmt = DBPrepare(hdb, L"INSERT INTO dashboard_template_instances (dashboard_template_id,instance_object_id,instance_dashboard_id) VALUES (?,?,?)", m_instanceDashboards.size() > 1);
            if (hStmt != nullptr)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
               for (std::pair<uint32_t, uint32_t> instance : m_instanceDashboards)
               {
                  DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, instance.first);
                  DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, instance.second);
                  success = DBExecute(hStmt);
                  if (!success)
                     break;
               }
               DBFreeStatement(hStmt);
            }
            else
            {
               success = false;
            }
         }
         unlockProperties();
      }
   }
   return success;
}

/**
 * Delete object from database
 */
bool DashboardTemplate::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, L"DELETE FROM dashboard_templates WHERE id=?");
   if (success)
      success = executeQueryOnObject(hdb, L"DELETE FROM dashboard_template_instances WHERE dashboard_template_id=?");
   return success;
}

/**
 * Create NXCP message with object's data
 */
void DashboardTemplate::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);

   msg->setField(VID_DASHBOARD_NAME_TEMPLATE, m_nameTemplate);
}

/**
 * Modify object from NXCP message
 */
uint32_t DashboardTemplate::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   if (msg.isFieldExist(VID_DASHBOARD_NAME_TEMPLATE))
      m_nameTemplate = msg.getFieldAsSharedString(VID_DASHBOARD_NAME_TEMPLATE);

   uint32_t result = super::modifyFromMessageInternal(msg, session);

   if (msg.isFieldExist(VID_NUM_ELEMENTS) || msg.isFieldExist(VID_NUM_COLUMNS))
   {
      // If elements or layout are changed - need to update all instances
      for (std::pair<uint32_t, uint32_t> instance : m_instanceDashboards)
      {
         shared_ptr<Dashboard> dashboard = static_pointer_cast<Dashboard>(FindObjectById(instance.second, OBJECT_DASHBOARD));
         if (dashboard != nullptr)
         {
            dashboard->updateFromTemplate(this);
         }
      }
   }
   return result;
}

/**
 * Serialize object to JSON
 */
json_t *DashboardTemplate::toJson()
{
   json_t *root = super::toJson();

   lockProperties();
   json_object_set_new(root, "nameTemplate", json_string_t(m_nameTemplate.cstr()));
   unlockProperties();

   return root;
}

/**
 * Perform automatic assignment to objects
 */
void DashboardTemplate::autobindPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   poller->setStatus(_T("wait for lock"));
   pollerLock(autobind);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   m_pollRequestId = rqId;
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Starting autobind poll of dashboard template %s [%u]"), m_name, m_id);
   poller->setStatus(_T("checking objects"));

   if (!isAutoBindEnabled())
   {
      sendPollerMsg(_T("Automatic object binding is disabled\r\n"));
      nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of dashboard template %s [%u])"), m_name, m_id);
      pollerUnlock();
      return;
   }

   NXSL_VM *cachedFilterVM = nullptr;
   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects(
      [] (NetObj *object, void *context) -> bool
      {
         if (object->isDataCollectionTarget())
            return true;
         int objectClass = object->getObjectClass();
         return (objectClass == OBJECT_NETWORK) || (objectClass == OBJECT_SERVICEROOT) || (objectClass == OBJECT_SUBNET) || (objectClass == OBJECT_ZONE) || (objectClass == OBJECT_CONDITION) || (objectClass == OBJECT_CONTAINER) || (objectClass == OBJECT_COLLECTOR) || (objectClass == OBJECT_RACK);
      }, nullptr);

   bool modified = false;
   IntegerArray<uint32_t> discoveredInstances;
   for (int i = 0; i < objects->size(); i++)
   {
      shared_ptr<NetObj> object = objects->getShared(i);

      AutoBindDecision decision = isApplicable(&cachedFilterVM, object, this);
      if ((decision == AutoBindDecision_Ignore) || ((decision == AutoBindDecision_Unbind) && !isAutoUnbindEnabled()))
         continue;

      uint32_t dashboardId = m_instanceDashboards[object->getId()];
      if (decision == AutoBindDecision_Bind)
      {
         StringBuffer objectName = object->expandText(m_nameTemplate);
         if (objectName.isEmpty())
            objectName = object->getName();

         bool found = (dashboardId != 0);
         if (dashboardId != 0)
         {
            shared_ptr<Dashboard> dashboard = static_pointer_cast<Dashboard>(FindObjectById(dashboardId, OBJECT_DASHBOARD));
            if (dashboard == nullptr)
            {
               // Instance was deleted, need to create a new one
               found = false;
            }
            else if (!objectName.equals(dashboard->getName()))
            {
               sendPollerMsg(_T("   Renaming dashboard instance %s to %s for object %s\r\n"), dashboard->getName(), objectName.cstr(), object->getName());
               nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("DashboardTemplate::autobindPoll(): renaming dashboard instance \"%s\" [%u] to \"%s\" for object \"%s\" [%u]"), dashboard->getName(), dashboard->getId(), objectName.cstr(), object->getName(), object->getId());
               dashboard->setName(objectName);
            }

         }
         if (!found)
         {
            shared_ptr<Dashboard> dashboard = make_shared<Dashboard>(objectName, object->getId(), this);
            NetObjInsert(dashboard, true, false);
            unique_ptr<SharedObjectArray<NetObj>> parents = this->getParents();
            if (parents->size() == 0)
            {
               parents->add(g_dashboardRoot);
            }
            NetObj::linkObjects(parents->getShared(0), dashboard);
            dashboard->unhide();
            dashboard->calculateCompoundStatus();
            m_instanceDashboards[object->getId()] = dashboard->getId();
            modified = true;

            sendPollerMsg(_T("   Creating dashboard instance %s for object %s\r\n"), objectName.cstr(), object->getName());
            nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("DashboardTemplate::autobindPoll(): creating dashboard instance \"%s\" for object \"%s\" [%u]"), objectName.cstr(), object->getName(), object->getId());
         }
      }
      else if (decision == AutoBindDecision_Unbind)
      {
         m_instanceDashboards.erase(object->getId());
         modified = true;
         if (dashboardId != 0)
         {
            shared_ptr<Dashboard> dashboard = static_pointer_cast<Dashboard>(FindObjectById(dashboardId, OBJECT_DASHBOARD));
            if (dashboard != nullptr)
            {
               sendPollerMsg(_T("   Deleting dashboard instance %s for object %s\r\n"), dashboard->getName(), object->getName());
               nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("DashboardTemplate::autobindPoll(): deleting dashboard instance \"%s\" [%u] for object \"%s\" [%u]"), dashboard->getName(), dashboard->getId(), object->getName(), object->getId());
               ThreadPoolExecute(g_mainThreadPool, [dashboard] () { dashboard->deleteObject(); });
            }
         }
      }
   }
   delete cachedFilterVM;

   if (modified)
      setModified(MODIFY_OTHER);

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of dashboard template %s [%u])"), m_name, m_id);
}

/*****************************************************************
 * DashboardGroup class
 *****************************************************************/

/**
 * Redefined status calculation for dashboard group
 */
void DashboardGroup::calculateCompoundStatus(bool forcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool DashboardGroup::showThresholdSummary() const
{
   return false;
}
