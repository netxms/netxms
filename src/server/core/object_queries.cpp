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
** File: object_queries.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("obj.query")

/**
 * Index of predefined object queries
 */
static SharedPointerIndex<ObjectQuery> s_objectQueries;

/**
 * Create object query from NXCP message
 */
ObjectQuery::ObjectQuery(const NXCPMessage& msg) :
         m_description(msg.getFieldAsString(VID_DESCRIPTION), -1, Ownership::True),
         m_source(msg.getFieldAsString(VID_SCRIPT_CODE), -1, Ownership::True),
         m_inputFields(msg.getFieldAsInt32(VID_NUM_PARAMETERS))
{
   m_id = msg.getFieldAsUInt32(VID_QUERY_ID);
   if (m_id == 0)
      m_id = CreateUniqueId(IDG_OBJECT_QUERY);
   m_guid = msg.getFieldAsGUID(VID_GUID);
   if (m_guid.isNull())
      m_guid = uuid::generate();
   msg.getFieldAsString(VID_NAME, m_name, MAX_OBJECT_NAME);

   int count = msg.getFieldAsInt32(VID_NUM_PARAMETERS);
   uint32_t fieldId = VID_PARAM_LIST_BASE;
   for(int i = 0; i < count; i++)
   {
      InputField *f = m_inputFields.addPlaceholder();
      msg.getFieldAsString(fieldId++, f->name, 32);
      f->type = msg.getFieldAsInt16(fieldId++);
      msg.getFieldAsString(fieldId++, f->displayName, 128);
      f->flags = msg.getFieldAsUInt32(fieldId++);
      f->orderNumber = msg.getFieldAsInt16(fieldId++);
      fieldId += 5;
   }

   compile();
}

/**
 * Create object query from database.
 * Expected field order: id, guid, name, description, script
 */
ObjectQuery::ObjectQuery(DB_HANDLE hdb, DB_RESULT hResult, int row) :
         m_description(DBGetField(hResult, row, 3, nullptr, 0), -1, Ownership::True),
         m_source(DBGetField(hResult, row, 4, nullptr, 0), -1, Ownership::True)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   m_guid = DBGetFieldGUID(hResult, row, 1);
   DBGetField(hResult, row, 2, m_name, MAX_OBJECT_NAME);

   DB_RESULT hParams = DBSelectFormatted(hdb, _T("SELECT name,input_type,display_name,flags,sequence_num FROM input_fields WHERE category='Q' AND owner_id=%u ORDER BY sequence_num"), m_id);
   if (hParams != nullptr)
   {
      int count = DBGetNumRows(hParams);
      for (int i = 0; i < count; i++)
      {
         InputField *p = m_inputFields.addPlaceholder();
         DBGetField(hParams, i, 0, p->name, 32);
         p->type = static_cast<int16_t>(DBGetFieldLong(hResult, i, 1));
         DBGetField(hParams, i, 2, p->displayName, 128);
         p->flags = DBGetFieldULong(hResult, i, 3);
         p->orderNumber = static_cast<int16_t>(DBGetFieldLong(hResult, i, 4));
      }
      DBFreeResult(hParams);
   }

   compile();
}

/**
 * Compile query
 */
void ObjectQuery::compile()
{
   m_script = CompileServerScript(m_source, SCRIPT_CONTEXT_OBJECT_QUERY, nullptr, 0, _T("ObjectQuery::%s"), m_name);
   if (m_script == nullptr)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Failed to compile script for predefined object query %s [%u]"), m_name, m_id);
   }
}

/**
 * Save object query to database
 */
bool ObjectQuery::saveToDatabase(DB_HANDLE hdb) const
{
   if (!DBBegin(hdb))
      return false;

   static const TCHAR *columns[] = { _T("guid"), _T("name"), _T("description"), _T("script"), nullptr };
   DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("object_queries"), _T("id"), m_id, columns);
   if (hStmt == nullptr)
   {
      DBRollback(hdb);
      return false;
   }

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_guid);
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC, 255);
   DBBind(hStmt, 4, DB_SQLTYPE_TEXT, m_source, DB_BIND_STATIC);
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_id);
   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);

   if (success)
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM input_fields WHERE category='Q' AND owner_id=?"));

   if (success && !m_inputFields.isEmpty())
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO input_fields (category,owner_id,name,display_name,input_type,flags,sequence_num) VALUES ('Q',?,?,?,?,?,?)"));
      if (hStmt != nullptr)
      {
         TCHAR type[2];
         type[1] = 0;

         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         for (int i = 0; (i < m_inputFields.size()) && success; i++)
         {
            InputField *p = m_inputFields.get(i);
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, p->name, DB_BIND_STATIC);
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, p->displayName, DB_BIND_STATIC);
            type[0] = p->type + '0';
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, type, DB_BIND_STATIC);
            DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, p->flags);
            DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, p->orderNumber);
            success = DBExecute(hStmt);
         }

         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   if (success)
      success = DBCommit(hdb);
   else
      DBRollback(hdb);
   return success;
}

/**
 * Delete query from database
 */
bool ObjectQuery::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM object_queries WHERE id=?"));
   if (success)
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM input_fields WHERE category='Q' AND owner_id=?"));
   return success;
}

/**
 * Fill message with object query data
 */
uint32_t ObjectQuery::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   uint32_t fieldId = baseId;
   msg->setField(fieldId++, m_id);
   msg->setField(fieldId++, m_guid);
   msg->setField(fieldId++, m_name);
   msg->setField(fieldId++, m_description);
   msg->setField(fieldId++, m_source);
   msg->setField(fieldId++, m_script != nullptr);
   fieldId += 3;
   msg->setField(fieldId++, m_inputFields.size());
   for(int i = 0; i < m_inputFields.size(); i++)
   {
      InputField *f = m_inputFields.get(i);
      msg->setField(fieldId++, f->name);
      msg->setField(fieldId++, f->type);
      msg->setField(fieldId++, f->displayName);
      msg->setField(fieldId++, f->flags);
      msg->setField(fieldId++, f->orderNumber);
      fieldId += 5;
   }
   return fieldId;
}

/**
 * Context for GetObjectQueriesCallback
 */
struct GetObjectQueriesCallbackContext
{
   NXCPMessage *msg;
   uint32_t fieldId;
   uint32_t count;
};

/**
 * Callback for getting configured object queries into NXCP message
 */
static EnumerationCallbackResult GetObjectQueriesCallback(ObjectQuery *query, GetObjectQueriesCallbackContext *context)
{
   context->fieldId = query->fillMessage(context->msg, context->fieldId);
   context->count++;
   return _CONTINUE;
}

/**
 * Get all configured object queries
 */
uint32_t GetObjectQueries(NXCPMessage *msg)
{
   GetObjectQueriesCallbackContext context;
   context.msg = msg;
   context.fieldId = VID_ELEMENT_LIST_BASE;
   context.count = 0;
   s_objectQueries.forEach(GetObjectQueriesCallback, &context);
   msg->setField(VID_NUM_ELEMENTS, context.count);
   return RCC_SUCCESS;
}

/**
 * Get all configured object queries
 */
json_t *GetObjectQueriesList()
{
   json_t *result = json_array();
   s_objectQueries.forEach([result](ObjectQuery *query) {
      json_t *queryJson = json_object();
      json_object_set_new(queryJson, "id", json_integer(query->getId()));
      json_object_set_new(queryJson, "name", json_string_t(query->getName()));
      json_array_append_new(result, queryJson);
      return _CONTINUE;
   });
   return result;
}

/**
 * Create or modify object query from message
 */
uint32_t ModifyObjectQuery(const NXCPMessage& msg, uint32_t *queryId)
{
   auto query = make_shared<ObjectQuery>(msg);
   uint32_t rcc;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (query->saveToDatabase(hdb))
   {
      s_objectQueries.put(query->getId(), query);
      *queryId = query->getId();
      rcc = RCC_SUCCESS;
      NotifyClientSessions(NX_NOTIFY_OBJECT_QUERY_UPDATED, query->getId());
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }
   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Delete object query
 */
uint32_t DeleteObjectQuery(uint32_t queryId)
{
   shared_ptr<ObjectQuery> query = s_objectQueries.get(queryId);
   if (query == nullptr)
      return RCC_INVALID_OBJECT_QUERY_ID;

   uint32_t rcc;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (query->deleteFromDatabase(hdb))
   {
      s_objectQueries.remove(queryId);
      rcc = RCC_SUCCESS;
      NotifyClientSessions(NX_NOTIFY_OBJECT_QUERY_DELETED, queryId);
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }
   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Load object queries from database
 */
void LoadObjectQueries()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,guid,name,description,script FROM object_queries"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         auto query = make_shared<ObjectQuery>(hdb, hResult, i);
         s_objectQueries.put(query->getId(), query);
      }
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Filter object
 */
static int FilterObject(NXSL_VM *vm, const shared_ptr<NetObj>& object, const shared_ptr<NetObj>& context, const StringMap *inputFields, NXSL_VariableSystem **globalVariables)
{
   // Remove global variables that may be set by previous script execution
   vm->removeGlobalVariable("$node");
   vm->removeGlobalVariable("$map");

   SetupServerScriptVM(vm, object, shared_ptr<DCObjectInfo>());
   vm->setContextObject(object->createNXSLObject(vm));
   if (inputFields != nullptr)
   {
      vm->setGlobalVariable("$INPUT", vm->createValue(new NXSL_HashMap(vm, inputFields)));
   }
   if (context != nullptr)
   {
      // This is context provided by caller (usually dashboard context), not script's context object
      vm->setGlobalVariable("$context", context->createNXSLObject(vm));
   }

   NXSL_VariableSystem *expressionVariables = nullptr;
   ObjectRefArray<NXSL_Value> args(0);
   if (!vm->run(args, globalVariables, &expressionVariables))
   {
      delete expressionVariables;
      return -1;
   }

   if (globalVariables != nullptr)
   {
      if  (expressionVariables != nullptr)
         (*globalVariables)->merge(expressionVariables);

      if (vm->getResult()->isHashMap())
      {
         NXSL_HashMap *map = vm->getResult()->getValueAsHashMap();
         StringList keys = map->getKeysAsList();
         for (int i = 0; i < keys.size(); i++)
         {
            const wchar_t *k = keys.get(i);
            (*globalVariables)->create(k, vm->createValue(map->get(k)));
         }
      }
   }

   delete expressionVariables;
   return vm->getResult()->isTrue() ? 1 : 0;
}

/**
 * Query result comparator
 */
static int ObjectQueryComparator(StringList *orderBy, const ObjectQueryResult **object1, const ObjectQueryResult **object2)
{
   for(int i = 0; i < orderBy->size(); i++)
   {
      bool descending = false;
      const TCHAR *attr = orderBy->get(i);
      if (*attr == _T('-'))
      {
         descending = true;
         attr++;
      }
      else if (*attr == _T('+'))
      {
         attr++;
      }

      const TCHAR *v1 = (*object1)->values->get(attr);
      const TCHAR *v2 = (*object2)->values->get(attr);

      // Try to compare as numbers
      if ((v1 != nullptr) && (v2 != nullptr))
      {
         TCHAR *eptr;
         double d1 = _tcstod(v1, &eptr);
         if (*eptr == 0)
         {
            double d2 = _tcstod(v2, &eptr);
            if (*eptr == 0)
            {
               if (d1 < d2)
                  return descending ? 1 : -1;
               if (d1 > d2)
                  return descending ? -1 : 1;
               continue;
            }
         }
      }

      // Compare as text if at least one value is not a number
      int rc = _tcsicmp(CHECK_NULL_EX(v1), CHECK_NULL_EX(v2));
      if (rc != 0)
         return descending ? -rc : rc;
   }
   return 0;
}

/**
 * Get metadata entry for given variable
 */
static inline const wchar_t *GetVariableMetadata(NXSL_VM *vm, const NXSL_Identifier& varName, const wchar_t *suffix)
{
   wchar_t key[256];
   size_t l = utf8_to_wchar(varName.value, -1, key, 256);
   wcslcpy(&key[l - 1], suffix, 256 - l);
   return vm->getMetadataEntry(key);
}

/**
 * Get all configured object queries
 */
unique_ptr<ObjectArray<ObjectQueryResult>> NXCORE_EXPORTABLE FindAndExecuteObjectQueries(uint32_t queryId, uint32_t rootObjectId, uint32_t userId, wchar_t *errorMessage, size_t errorMessageLen,
   std::function<void(int)> progressCallback, bool readAllComputedFields, const StringList *fields, const StringList *orderBy,
   const StringMap *inputFields, uint32_t contextObjectId, uint32_t limit)
{
   shared_ptr<ObjectQuery> query = s_objectQueries.get(queryId);
   if (query == nullptr)
   {
      _tcslcpy(errorMessage, _T("Invalid object query ID"), errorMessageLen);
      return unique_ptr<ObjectArray<ObjectQueryResult>>();
   }
   String script = query->getScript();
   return QueryObjects(script, rootObjectId, userId, errorMessage, errorMessageLen, nullptr, readAllComputedFields, fields, orderBy, inputFields, contextObjectId, limit);
}

/**
 * Query objects
 */
unique_ptr<ObjectArray<ObjectQueryResult>> NXCORE_EXPORTABLE QueryObjects(const wchar_t *query, uint32_t rootObjectId, uint32_t userId, wchar_t *errorMessage, size_t errorMessageLen,
      std::function<void(int)> progressCallback, bool readAllComputedFields, const StringList *fields, const StringList *orderBy,
      const StringMap *inputFields, uint32_t contextObjectId, uint32_t limit, StringMap *metadata)
{
   NXSL_CompilationDiagnostic diag;
   NXSL_VM *vm = NXSLCompileAndCreateVM(query, new NXSL_ServerEnv(), &diag);
   if (vm == nullptr)
   {
      wcslcpy(errorMessage, diag.errorText, errorMessageLen);
      return unique_ptr<ObjectArray<ObjectQueryResult>>();
   }

   // Return all metadata set at compile time
   if (metadata != nullptr)
      metadata->addAll(vm->getMetadata());

   bool readFields = readAllComputedFields || (fields != nullptr);

   // Set class constants
   vm->addConstant("ACCESSPOINT", vm->createValue(OBJECT_ACCESSPOINT));
   vm->addConstant("ASSET", vm->createValue(OBJECT_ASSET));
   vm->addConstant("ASSETGROUP", vm->createValue(OBJECT_ASSETGROUP));
   vm->addConstant("ASSETROOT", vm->createValue(OBJECT_ASSETROOT));
   vm->addConstant("BUSINESSSERVICE", vm->createValue(OBJECT_BUSINESSSERVICE));
   vm->addConstant("BUSINESSSERVICEPROTOTYPE", vm->createValue(OBJECT_BUSINESSSERVICEPROTO));
   vm->addConstant("BUSINESSSERVICEROOT", vm->createValue(OBJECT_BUSINESSSERVICEROOT));
   vm->addConstant("CHASSIS", vm->createValue(OBJECT_CHASSIS));
   vm->addConstant("CLUSTER", vm->createValue(OBJECT_CLUSTER));
   vm->addConstant("COLLECTOR", vm->createValue(OBJECT_COLLECTOR));
   vm->addConstant("CONDITION", vm->createValue(OBJECT_CONDITION));
   vm->addConstant("CONTAINER", vm->createValue(OBJECT_CONTAINER));
   vm->addConstant("DASHBOARD", vm->createValue(OBJECT_DASHBOARD));
   vm->addConstant("DASHBOARDGROUP", vm->createValue(OBJECT_DASHBOARDGROUP));
   vm->addConstant("DASHBOARDROOT", vm->createValue(OBJECT_DASHBOARDROOT));
   vm->addConstant("DASHBOARDTEMPLATE", vm->createValue(OBJECT_DASHBOARDTEMPLATE));
   vm->addConstant("INTERFACE", vm->createValue(OBJECT_INTERFACE));
   vm->addConstant("MOBILEDEVICE", vm->createValue(OBJECT_MOBILEDEVICE));
   vm->addConstant("NETWORK", vm->createValue(OBJECT_NETWORK));
   vm->addConstant("NETWORKMAP", vm->createValue(OBJECT_NETWORKMAP));
   vm->addConstant("NETWORKMAPGROUP", vm->createValue(OBJECT_NETWORKMAPGROUP));
   vm->addConstant("NETWORKMAPROOT", vm->createValue(OBJECT_NETWORKMAPROOT));
   vm->addConstant("NETWORKSERVICE", vm->createValue(OBJECT_NETWORKSERVICE));
   vm->addConstant("NODE", vm->createValue(OBJECT_NODE));
   vm->addConstant("RACK", vm->createValue(OBJECT_RACK));
   vm->addConstant("SENSOR", vm->createValue(OBJECT_SENSOR));
   vm->addConstant("SERVICEROOT", vm->createValue(OBJECT_SERVICEROOT));
   vm->addConstant("SUBNET", vm->createValue(OBJECT_SUBNET));
   vm->addConstant("TEMPLATE", vm->createValue(OBJECT_TEMPLATE));
   vm->addConstant("TEMPLATEGROUP", vm->createValue(OBJECT_TEMPLATEGROUP));
   vm->addConstant("TEMPLATEROOT", vm->createValue(OBJECT_TEMPLATEROOT));
   vm->addConstant("VPNCONNECTOR", vm->createValue(OBJECT_VPNCONNECTOR));
   vm->addConstant("WIRELESSDOMAIN", vm->createValue(OBJECT_WIRELESSDOMAIN));
   vm->addConstant("ZONE", vm->createValue(OBJECT_ZONE));

   shared_ptr<NetObj> context = (contextObjectId != 0) ? FindObjectById(contextObjectId) : shared_ptr<NetObj>();

   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects(
      [userId, rootObjectId] (NetObj *object) -> bool
      {
         return ((rootObjectId == 0) || object->isParent(rootObjectId)) && object->checkAccessRights(userId, OBJECT_ACCESS_READ);
      });
   auto resultSet = new ObjectArray<ObjectQueryResult>(64, 64, Ownership::True);
   StringMap displayNameMapping;
   bool firstResult = true;
   int total = objects->size();
   int completed = 0;
   for(int i = 0; i < total; i++)
   {
      shared_ptr<NetObj> curr = objects->getShared(i);

      if (progressCallback != nullptr)
      {
         int p = i * 100 / total;
         if (p > completed)
         {
            completed = p;
            progressCallback(completed);
         }
      }

      NXSL_VariableSystem *globals = nullptr;
      int rc = FilterObject(vm, curr, context, inputFields, readFields ? &globals : nullptr);
      if (rc < 0)
      {
         _tcslcpy(errorMessage, vm->getErrorText(), errorMessageLen);
         delete_and_null(resultSet);
         delete globals;
         if (progressCallback != nullptr)
            progressCallback(100);
         break;
      }

      if (rc > 0)
      {
         StringMap *objectData;
         if (readFields)
         {
            objectData = new StringMap();

            if (fields != nullptr)
            {
               NXSL_Value *objectValue = curr->createNXSLObject(vm);
               NXSL_Object *object = objectValue->getValueAsObject();
               for(int j = 0; j < fields->size(); j++)
               {
                  const TCHAR *fieldName = fields->get(j);
                  NXSL_Variable *v = globals->find(fieldName);
                  if (v != nullptr)
                  {
                     objectData->set(fieldName, v->getValue()->getValueAsCString());
                  }
                  else
                  {
                     char attr[MAX_IDENTIFIER_LENGTH];
                     wchar_to_utf8(fields->get(j), -1, attr, MAX_IDENTIFIER_LENGTH - 1);
                     attr[MAX_IDENTIFIER_LENGTH - 1] = 0;
                     NXSL_Value *av = object->getClass()->getAttr(object, attr);
                     if (av != nullptr)
                     {
                        objectData->set(fieldName, av->getValueAsCString());
                        vm->destroyValue(av);
                     }
                     else
                     {
                        objectData->set(fieldName, _T(""));
                     }
                  }
               }
               vm->destroyValue(objectValue);
            }

            if (readAllComputedFields)
            {
               globals->forEach(
                  [vm, objectData, firstResult, &displayNameMapping] (const NXSL_Identifier& name, NXSL_Value *value) -> void
                  {
                     if (name.value[0] == '$')  // Ignore global variables set by system
                        return;

                     const wchar_t *visible = GetVariableMetadata(vm, name, L".visible");
                     bool hidden = (visible != nullptr) && (!wcsicmp(visible, L"false") || !wcscmp(visible, L"0"));
                     if (hidden)
                     {
                        if (GetVariableMetadata(vm, name, _T(".order")) == nullptr)
                           return;  // Visibility attribute set to FALSE and not used for ordering
                     }

                     const wchar_t *displayName = hidden ? nullptr : GetVariableMetadata(vm, name, L".name");
                     if (displayName != nullptr)
                     {
                        objectData->set(displayName, value->getValueAsCString());
                        if (firstResult)
                        {
                           wchar_t wname[MAX_IDENTIFIER_LENGTH];
                           utf8_to_wchar(name.value, -1, wname, MAX_IDENTIFIER_LENGTH);
                           displayNameMapping.set(displayName, wname);
                        }
                     }
                     else
                     {
                        objectData->setPreallocated(WideStringFromUTF8String(name.value), MemCopyString(value->getValueAsCString()));
                     }
                  });
            }
         }
         else
         {
            objectData = nullptr;
         }

         resultSet->add(new ObjectQueryResult(curr, objectData));
         firstResult = false;
      }
      delete globals;
   }

   // Sort result set, apply limit, remove hidden columns
   if (readFields && (resultSet != nullptr) && !resultSet->isEmpty())
   {
      StringList realOrderBy;
      if (orderBy != nullptr)
         realOrderBy.addAll(orderBy);

      StringList columns = resultSet->get(0)->values->keys();
      for(int i = 0; i < columns.size(); i++)
      {
         const wchar_t *columnName = columns.get(i);
         const wchar_t *originalName = displayNameMapping.get(columnName);
         wchar_t key[256];
         _sntprintf(key, 256, _T("%s.order"), (originalName != nullptr) ? originalName : columnName);
         const wchar_t *order = vm->getMetadataEntry(key);
         if (order != nullptr)
         {
            if (!_tcsicmp(order, _T("asc")) || !_tcsicmp(order, _T("ascending")))
            {
               realOrderBy.add(columnName);
            }
            else if (!_tcsicmp(order, _T("desc")) || !_tcsicmp(order, _T("descending")))
            {
               wchar_t c[256];
               c[0] = L'-';
               wcscpy(&c[1], columnName);
               realOrderBy.add(c);
            }
         }
      }

      if (!realOrderBy.isEmpty())
      {
         resultSet->sort(ObjectQueryComparator, &realOrderBy);
      }
      if (limit > 0)
      {
         resultSet->shrinkTo((int)limit);
      }

      for(int i = 0; i < columns.size(); i++)
      {
         wchar_t key[256];
         _sntprintf(key, 256, _T("%s.visible"), columns.get(i));
         const wchar_t *visible = vm->getMetadataEntry(key);
         if ((visible != nullptr) && (!wcscmp(visible, L"false") || !wcscmp(visible, L"0")))
         {
            for(int j = 0; j < resultSet->size(); j++)
               resultSet->get(j)->values->remove(columns.get(i));
            continue;
         }
      }
   }

   if (!readFields && (resultSet != nullptr) && (limit > 0))
   {
      resultSet->shrinkTo((int)limit);
   }

   delete vm;

   return unique_ptr<ObjectArray<ObjectQueryResult>>(resultSet);
}
