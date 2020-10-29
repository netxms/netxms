/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
** File: websvc.cpp
**
**/

#include "nxcore.h"
#include <nxcore_websvc.h>

#define DEBUG_TAG _T("websvc")

/**
 * Create web service definition from NXCP message
 */
WebServiceDefinition::WebServiceDefinition(const NXCPMessage *msg)
{
   m_id = msg->getFieldAsUInt32(VID_WEBSVC_ID);
   if (m_id == 0)
      m_id = CreateUniqueId(IDG_WEBSVC_DEFINITION);
   m_guid = msg->getFieldAsGUID(VID_GUID);
   if (m_guid.isNull())
      m_guid = uuid::generate();
   m_name = msg->getFieldAsString(VID_NAME);
   m_description = msg->getFieldAsString(VID_DESCRIPTION);
   m_url = msg->getFieldAsString(VID_URL);
   m_authType = WebServiceAuthTypeFromInt(msg->getFieldAsInt16(VID_AUTH_TYPE));
   m_login = msg->getFieldAsString(VID_LOGIN_NAME);
   m_password = msg->getFieldAsString(VID_PASSWORD);
   m_cacheRetentionTime = msg->getFieldAsUInt32(VID_RETENTION_TIME);
   m_requestTimeout = msg->getFieldAsUInt32(VID_TIMEOUT);
   m_headers.loadMessage(msg, VID_NUM_HEADERS, VID_HEADERS_BASE);
   m_flags = msg->getFieldAsUInt32(VID_FLAGS);
}

/**
 * Create web service definition from config file
 */
WebServiceDefinition::WebServiceDefinition(const ConfigEntry *config, uint32_t id)
{
   m_id = id;
   if (m_id == 0)
      m_id = CreateUniqueId(IDG_WEBSVC_DEFINITION);
   m_guid = config->getSubEntryValueAsUUID(_T("guid"));
   if (m_guid.isNull())
      m_guid = uuid::generate();
   m_name = MemCopyString(config->getSubEntryValue(_T("name")));
   m_description = MemCopyString(config->getSubEntryValue(_T("description")));
   m_url = MemCopyString(config->getSubEntryValue(_T("url")));
   m_authType = WebServiceAuthTypeFromInt(config->getSubEntryValueAsInt(_T("authType")));
   m_login = MemCopyString(config->getSubEntryValue(_T("login")));
   m_password = MemCopyString(config->getSubEntryValue(_T("password")));
   m_cacheRetentionTime = config->getSubEntryValueAsInt(_T("retentionTime"));
   m_requestTimeout = config->getSubEntryValueAsInt(_T("timeout"));
   m_flags = config->getSubEntryValueAsInt(_T("flags"));

   ConfigEntry *headerRoot = config->findEntry(_T("headers"));
   if (headerRoot != nullptr)
   {
     ObjectArray<ConfigEntry> *headers = headerRoot->getSubEntries(_T("header*"));
     for(int i = 0; i < headers->size(); i++)
     {
        ConfigEntry *e = headers->get(i);
        m_headers.set(e->getSubEntryValue(_T("name")), e->getSubEntryValue(_T("value")));
     }
     delete headers;
   }

}

/**
 * Create web service definition from database record.
 * Expected field order:
 *    id,guid,name,url,auth_type,login,password,cache_retention_time,request_timeout,description,flags
 */
WebServiceDefinition::WebServiceDefinition(DB_HANDLE hdb, DB_RESULT hResult, int row)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   m_guid = DBGetFieldGUID(hResult, row, 1);
   m_name = DBGetField(hResult, row, 2, NULL, 0);
   m_url = DBGetField(hResult, row, 3, NULL, 0);
   m_authType = WebServiceAuthTypeFromInt(DBGetFieldLong(hResult, row, 4));
   m_login = DBGetField(hResult, row, 5, NULL, 0);
   m_password = DBGetField(hResult, row, 6, NULL, 0);
   m_cacheRetentionTime = DBGetFieldULong(hResult, row, 7);
   m_requestTimeout = DBGetFieldULong(hResult, row, 8);
   m_description = DBGetField(hResult, row, 9, NULL, 0);
   m_flags = DBGetFieldULong(hResult, row, 10);


   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT name,value FROM websvc_headers WHERE websvc_id=%u"), m_id);
   DB_RESULT headers = DBSelect(hdb, query);
   if (headers != NULL)
   {
      int count = DBGetNumRows(headers);
      for(int i = 0; i < count; i++)
      {
         TCHAR *name = DBGetField(headers, i, 0, NULL, 0);
         TCHAR *value = DBGetField(headers, i, 1, NULL, 0);
         if ((name != NULL) && (value != NULL) && (*name != 0))
         {
            m_headers.setPreallocated(name, value);
         }
         else
         {
            MemFree(name);
            MemFree(value);
         }
      }
      DBFreeResult(headers);
   }
}

/**
 * Destructor
 */
WebServiceDefinition::~WebServiceDefinition()
{
   MemFree(m_name);
   MemFree(m_description);
   MemFree(m_url);
   MemFree(m_login);
   MemFree(m_password);
}

/**
 * Context for ExpandHeaders function
 */
struct ExpandHeadersContext
{
   StringMap *headers;
   DataCollectionTarget *object;
   const StringList *args;
};

/**
 * Expand headers
 */
static EnumerationCallbackResult ExpandHeaders(const TCHAR *key, const TCHAR *value, ExpandHeadersContext *context)
{
   context->headers->set(key, context->object->expandText(value, nullptr, nullptr, shared_ptr<DCObjectInfo>(), nullptr, nullptr, nullptr, context->args));
   return _CONTINUE;
}

/**
 * Query web service using this definition. Returns agent RCC.
 */
uint32_t WebServiceDefinition::query(DataCollectionTarget *object, WebServiceRequestType requestType, const TCHAR *path,
         const StringList& args, AgentConnection *conn, void *result) const
{
   StringBuffer url = object->expandText(m_url, nullptr, nullptr, shared_ptr<DCObjectInfo>(), nullptr, nullptr, nullptr, &args);

   StringMap headers;
   ExpandHeadersContext context;
   context.headers = &headers;
   context.object = object;
   context.args = &args;
   m_headers.forEach(ExpandHeaders, &context);

   StringList pathList;
   pathList.add(path);

   StringMap resultSet;
   uint32_t rcc = conn->queryWebService(requestType, url, m_requestTimeout, m_cacheRetentionTime, m_login, m_password, m_authType, headers, pathList,
         isVerifyCertificate(), isVerifyHost(), isForcePlainTextParser(), (requestType == WebServiceRequestType::PARAMETER) ? &resultSet : result);
   if ((rcc == ERR_SUCCESS) && (requestType == WebServiceRequestType::PARAMETER))
   {
      const TCHAR *value = resultSet.get(path);
      if (value != nullptr)
         ret_string(static_cast<TCHAR*>(result), value);
      else
         rcc = ERR_UNKNOWN_PARAMETER;
   }
   return rcc;
}

/**
 * Fill NXCP message
 */
void WebServiceDefinition::fillMessage(NXCPMessage *msg) const
{
   msg->setField(VID_WEBSVC_ID, m_id);
   msg->setField(VID_GUID, m_guid);
   msg->setField(VID_NAME, m_name);
   msg->setField(VID_DESCRIPTION, m_description);
   msg->setField(VID_URL, m_url);
   msg->setField(VID_AUTH_TYPE, static_cast<INT16>(m_authType));
   msg->setField(VID_LOGIN_NAME, m_login);
   msg->setField(VID_PASSWORD, m_password);
   msg->setField(VID_RETENTION_TIME, m_cacheRetentionTime);
   msg->setField(VID_TIMEOUT, m_requestTimeout);
   m_headers.fillMessage(msg, VID_NUM_HEADERS, VID_HEADERS_BASE);
   msg->setField(VID_FLAGS, m_flags);
}

/**
 * Create export record for single header
 */
static EnumerationCallbackResult CreateHeaderExportRecord(const TCHAR *key, const TCHAR *value, StringBuffer *xml)
{
   xml->append(_T("\t\t\t\t<header>\n\t\t\t\t\t<name>"));
   xml->append(EscapeStringForXML2(key));
   xml->append(_T("</name>\n\t\t\t\t\t<value>"));
   xml->append(EscapeStringForXML2(value));
   xml->append(_T("</value>\n\t\t\t\t</header>\n"));
   return _CONTINUE;
}

/**
 * Create export record
 */
void WebServiceDefinition::createExportRecord(StringBuffer &xml) const
{
   xml.append(_T("\t\t<webServiceDefinition id=\""));
   xml.append(m_id);
   xml.append(_T("\">\n\t\t\t<guid>"));
   xml.append(m_guid);
   xml.append(_T("</guid>\n\t\t\t<name>"));
   xml.append(EscapeStringForXML2(m_name));
   xml.append(_T("</name>\n\t\t\t<description>"));
   xml.append(EscapeStringForXML2(m_description));
   xml.append(_T("</description>\n\t\t\t<url>"));
   xml.append(EscapeStringForXML2(m_url));
   xml.append(_T("</url>\n\t\t\t<flags>"));
   xml.append(m_flags);
   xml.append(_T("</flags>\n\t\t\t<authType>"));
   xml.append(static_cast<int32_t>(m_authType));
   xml.append(_T("</authType>\n\t\t\t<login>"));
   xml.append(EscapeStringForXML2(m_login));
   xml.append(_T("</login>\n\t\t\t<password>"));
   xml.append(EscapeStringForXML2(m_password));
   xml.append(_T("</password>\n\t\t\t<retentionTime>"));
   xml.append(m_cacheRetentionTime);
   xml.append(_T("</retentionTime>\n\t\t\t<timeout>"));
   xml.append(m_requestTimeout);
   xml.append(_T("</timeout>\n\t\t\t<headers>\n"));
   m_headers.forEach(CreateHeaderExportRecord, &xml);
   xml.append(_T("\t\t\t</headers>\n\t\t</webServiceDefinition>\n"));
}

/**
 * Create JSON document from single header
 */
static EnumerationCallbackResult HeaderToJson(const TCHAR *key, const TCHAR *value, json_t *headers)
{
   json_t *header = json_object();
   json_object_set_new(header, "name", json_string_t(key));
   json_object_set_new(header, "value", json_string_t(value));
   json_array_append_new(headers, header);
   return _CONTINUE;
}

/**
 * Create JSON document
 */
json_t *WebServiceDefinition::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "guid", m_guid.toJson());
   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "description", json_string_t(m_description));
   json_object_set_new(root, "url", json_string_t(m_url));
   json_object_set_new(root, "flags", json_integer(m_flags));
   json_object_set_new(root, "authType", json_integer(static_cast<int32_t>(m_authType)));
   json_object_set_new(root, "login", json_string_t(m_login));
   json_object_set_new(root, "password", json_string_t(m_password));
   json_object_set_new(root, "cacheRetentionTime", json_integer(m_cacheRetentionTime));
   json_object_set_new(root, "requestTimeout", json_integer(m_requestTimeout));

   json_t *headers = json_array();
   m_headers.forEach(HeaderToJson, headers);
   json_object_set_new(root, "headers", headers);

   return root;
}

/**
 * List of configured web services
 */
static SharedObjectArray<WebServiceDefinition> s_webServiceDefinitions;
static Mutex s_webServiceDefinitionLock(true);

/**
 * Load web service definitions from database
 */
void LoadWebServiceDefinitions()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,guid,name,url,auth_type,login,password,cache_retention_time,request_timeout,description,flags FROM websvc_definitions"));
   if (hResult == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Web service definitions cannot be loaded due to database failure"));
      return;
   }

   DB_HANDLE cachedb = (g_flags & AF_CACHE_DB_ON_STARTUP) ? DBOpenInMemoryDatabase() : NULL;
   if (cachedb != NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Caching web service definition tables"));
      if (!DBCacheTable(cachedb, hdb, _T("websvc_headers"), _T("websvc_id,name"), _T("*")))
      {
         DBCloseInMemoryDatabase(cachedb);
         cachedb = NULL;
      }
   }

   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
      s_webServiceDefinitions.add(make_shared<WebServiceDefinition>((cachedb != NULL) ? cachedb : hdb, hResult, i));

   DBFreeResult(hResult);
   DBConnectionPoolReleaseConnection(hdb);

   if (cachedb != NULL)
      DBCloseInMemoryDatabase(cachedb);

   nxlog_debug_tag(DEBUG_TAG, 2, _T("%d web service definitions loaded"), count);
}

/**
 * Find web service definition by UUID
 */
shared_ptr<WebServiceDefinition> FindWebServiceDefinition(const uuid guid)
{
   shared_ptr<WebServiceDefinition> result;
   s_webServiceDefinitionLock.lock();
   for(int i = 0; i < s_webServiceDefinitions.size(); i++)
   {
      auto d = s_webServiceDefinitions.getShared(i);
      if (guid.equals(d->getGuid()))
      {
         result = d;
         break;
      }
   }
   s_webServiceDefinitionLock.unlock();
   return result;
}

/**
 * Find web service definition by name
 */
shared_ptr<WebServiceDefinition> FindWebServiceDefinition(const TCHAR *name)
{
   shared_ptr<WebServiceDefinition> result;
   s_webServiceDefinitionLock.lock();
   for(int i = 0; i < s_webServiceDefinitions.size(); i++)
   {
      auto d = s_webServiceDefinitions.getShared(i);
      if (!_tcsicmp(name, d->getName()))
      {
         result = d;
         break;
      }
   }
   s_webServiceDefinitionLock.unlock();
   return result;
}

/**
 * Get all web service definitions
 */
SharedObjectArray<WebServiceDefinition> *GetWebServiceDefinitions()
{
   s_webServiceDefinitionLock.lock();
   auto definitions = s_webServiceDefinitions.clone();
   s_webServiceDefinitionLock.unlock();
   return definitions;
}

/**
 * Save single header to database
 */
static EnumerationCallbackResult SaveHeader(const TCHAR *key, const TCHAR *value, DB_STATEMENT hStmt)
{
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, key, DB_BIND_STATIC);
   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, value, DB_BIND_STATIC);
   return DBExecute(hStmt) ? _CONTINUE : _STOP;
}

/**
 * Modify web service definition. Returns client RCC.
 */
uint32_t ModifyWebServiceDefinition(shared_ptr<WebServiceDefinition> definition)
{
   uint32_t rcc;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (DBBegin(hdb))
   {
      bool success = false;

      static const TCHAR *columns[] = { _T("guid"), _T("name"), _T("description"), _T("url"),
               _T("auth_type"), _T("login"), _T("password"), _T("cache_retention_time"),
               _T("request_timeout"), _T("flags"), nullptr };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("websvc_definitions"), _T("id"), definition->getId(), columns);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, definition->getGuid());
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, definition->getName(), DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, definition->getDescription(), DB_BIND_STATIC);
         DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, definition->getUrl(), DB_BIND_STATIC);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<int32_t>(definition->getAuthType()));
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, definition->getLogin(), DB_BIND_STATIC);
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, definition->getPassword(), DB_BIND_STATIC);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, definition->getCacheRetentionTime());
         DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, definition->getRequestTimeout());
         DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, definition->getFlags());
         DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, definition->getId());
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }

      if (success)
         success = ExecuteQueryOnObject(hdb, definition->getId(), _T("DELETE FROM websvc_headers WHERE websvc_id=?"));

      const StringMap& headers = definition->getHeaders();
      if (success && !headers.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO websvc_headers (websvc_id,name,value) VALUES (?,?,?)"), headers.size() > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, definition->getId());
            success = (headers.forEach(SaveHeader, hStmt) == _CONTINUE);
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }

      if (success)
      {
         DBCommit(hdb);
         rcc = RCC_SUCCESS;

         s_webServiceDefinitionLock.lock();
         bool found = false;
         for(int i = 0; i < s_webServiceDefinitions.size(); i++)
         {
            if (s_webServiceDefinitions.get(i)->getId() == definition->getId())
            {
               s_webServiceDefinitions.replace(i, definition);
               found = true;
               break;
            }
         }
         if (!found)
            s_webServiceDefinitions.add(definition);
         s_webServiceDefinitionLock.unlock();
      }
      else
      {
         DBRollback(hdb);
         rcc = RCC_DB_FAILURE;
      }
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }
   DBConnectionPoolReleaseConnection(hdb);

   return rcc;
}

/**
 * Delete web service definition. Returns client RCC.
 */
uint32_t DeleteWebServiceDefinition(uint32_t id)
{
   uint32_t rcc = RCC_INVALID_WEB_SERVICE_ID;
   shared_ptr<WebServiceDefinition> definition;
   s_webServiceDefinitionLock.lock();
   for(int i = 0; i < s_webServiceDefinitions.size(); i++)
   {
      auto d = s_webServiceDefinitions.getShared(i);
      if (d->getId() == id)
      {
         definition = d;
         s_webServiceDefinitions.remove(i);
         rcc = RCC_SUCCESS;
         break;
      }
   }
   s_webServiceDefinitionLock.unlock();

   if (rcc == RCC_SUCCESS)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      if (DBBegin(hdb))
      {
         bool success = ExecuteQueryOnObject(hdb, id, _T("DELETE FROM websvc_definitions WHERE id=?"));
         if (success)
            success = ExecuteQueryOnObject(hdb, id, _T("DELETE FROM websvc_headers WHERE websvc_id=?"));
         if (success)
         {
            DBCommit(hdb);
         }
         else
         {
            DBRollback(hdb);
            rcc = RCC_DB_FAILURE;

            // Re-insert web service definition into list
            s_webServiceDefinitionLock.lock();
            s_webServiceDefinitions.add(definition);
            s_webServiceDefinitionLock.unlock();
         }
      }
      else
      {
         rcc = RCC_DB_FAILURE;
      }
      DBConnectionPoolReleaseConnection(hdb);
   }

   return rcc;
}

/**
 * Create XML web service
 */
void CreateWebServiceDefinitionExportRecord(StringBuffer &xml, uint32_t count, uint32_t *list)
{
   SharedObjectArray<WebServiceDefinition> *definitions = GetWebServiceDefinitions();
   for(uint32_t j = 0; j < count; j++)
   {
      for(int i = 0; i < definitions->size(); i++)
      {
         if (list[j] == definitions->get(i)->getId())
            definitions->get(i)->createExportRecord(xml);
      }
   }
   delete definitions;
}

/**
 * Import web service definition configuration
 */
bool ImportWebServiceDefinition(ConfigEntry *config, bool overwrite)
{
   if (config->getSubEntryValue(_T("name")) == NULL)
   {
      nxlog_debug_tag(_T("import"), 4, _T("ImportAction: no name specified"));
      return false;
   }

   uint32_t rcc = RCC_ALREADY_EXIST;
   const uuid guid = config->getSubEntryValueAsUUID(_T("guid"));
   auto service = FindWebServiceDefinition(guid);
   TCHAR guidText[64];
   if(service == nullptr)
   {
      // Check for duplicate name
      const TCHAR *name = config->getSubEntryValue(_T("name"));
      if (FindWebServiceDefinition(name) != nullptr)
      {
         nxlog_debug_tag(_T("import"), 4, _T("ImportWebServiceDefinition: name \"%s\" already exists"), name);
      }
      else
      {
         auto definition = make_shared<WebServiceDefinition>(config, 0);
         rcc = ModifyWebServiceDefinition(definition);
         if (rcc == RCC_SUCCESS)
            nxlog_debug_tag(_T("import"), 4, _T("ImportWebServiceDefinition: \"%s\" web service created"), name);
      }
   }
   else if (overwrite)
   {
      auto definition = make_shared<WebServiceDefinition>(config,service->getId());
      rcc = ModifyWebServiceDefinition(definition);
      nxlog_debug_tag(_T("import"), 4, _T("ImportWebServiceDefinition: found existing action \"%s\" with GUID %s (overwrite)"),
            service->getName(), guid.toString(guidText));
   }
   else
   {
      nxlog_debug_tag(_T("import"), 4, _T("ImportWebServiceDefinition: found existing action \"%s\" with GUID %s (skipping)"),
            service->getName(), guid.toString(guidText));
   }

   return rcc == RCC_SUCCESS;
}
