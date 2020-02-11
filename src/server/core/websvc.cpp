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
   m_guid = msg->getFieldAsGUID(VID_GUID);
   m_name = msg->getFieldAsString(VID_NAME);
   m_description = msg->getFieldAsString(VID_DESCRIPTION);
   m_url = msg->getFieldAsString(VID_URL);
   m_authType = WebServiceAuthTypeFromInt(msg->getFieldAsInt16(VID_AUTH_TYPE));
   m_login = msg->getFieldAsString(VID_LOGIN_NAME);
   m_password = msg->getFieldAsString(VID_PASSWORD);
   m_cacheRetentionTime = msg->getFieldAsUInt32(VID_RETENTION_TIME);
   m_requestTimeout = msg->getFieldAsUInt32(VID_TIMEOUT);
   m_headers.loadMessage(msg, VID_NUM_HEADERS, VID_HEADERS_BASE);
}

/**
 * Create web service definition from database record.
 * Expected field order:
 *    id,guid,name,url,auth_type,login,password,cache_retention_time,request_timeout,description
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
   context->headers->set(key, context->object->expandText(value, NULL, NULL, NULL, NULL, NULL, context->args));
   return _CONTINUE;
}

/**
 * Query web service using this definition. Returns agent RCC.
 */
UINT32 WebServiceDefinition::query(DataCollectionTarget *object, const TCHAR *path,
         const StringList *args, AgentConnection *conn, TCHAR *result) const
{
   StringBuffer url = object->expandText(m_url, NULL, NULL, NULL, NULL, NULL, args);

   StringMap headers;
   ExpandHeadersContext context;
   context.headers = &headers;
   context.object = object;
   context.args = args;
   m_headers.forEach(ExpandHeaders, &context);

   StringList attributes;
   attributes.add(path);

   StringMap resultSet;
   UINT32 rcc = conn->queryWebService(url, m_cacheRetentionTime, m_login, m_password, m_authType, headers, attributes, false, &resultSet);
   if (rcc == ERR_SUCCESS)
   {
      const TCHAR *value = resultSet.get(path);
      if (value != NULL)
         ret_string(result, value);
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
   xml->append(_T("</value>\n\t\t\t\t</header>"));
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
   xml.append(_T("</url>\n\t\t\t<authType>"));
   xml.append(static_cast<int32_t>(m_authType));
   xml.append(_T("</authType>\n\t\t\t<login>"));
   xml.append(EscapeStringForXML2(m_login));
   xml.append(_T("</login>\n\t\t\t<password>"));
   xml.append(EscapeStringForXML2(m_password));
   xml.append(_T("</password>\n\t\t\t<retentionTime>"));
   xml.append(m_cacheRetentionTime);
   xml.append(_T("</retentionTime>\n\t\t\t<timeout>"));
   xml.append(m_requestTimeout);
   xml.append(_T("</timeout>\n\t\t\t<headers>"));
   m_headers.forEach(CreateHeaderExportRecord, &xml);
   xml.append(_T("</headers>\n\t\t</webServiceDefinition>"));
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

   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,guid,name,url,auth_type,login,password,cache_retention_time,request_timeout FROM websvc_definitions"));
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
 * Parse list of service call arguments
 */
static bool ParseCallArgumensList(TCHAR *input, StringList *args)
{
   TCHAR *p = input;

   TCHAR *s = p;
   int state = 1; // normal text
/*
   for(; state > 0; p++)
   {
      switch(*p)
      {
         case '"':
            if (state == 1)
            {
               state = 2;
               s = p + 1;
            }
            else
            {
               state = 3;
               *p = 0;
               args->add(vm->createValue(s));
            }
            break;
         case ',':
            if (state == 1)
            {
               *p = 0;
               Trim(s);
               args.add(vm->createValue(s));
               s = p + 1;
            }
            else if (state == 3)
            {
               state = 1;
               s = p + 1;
            }
            break;
         case 0:
            if (state == 1)
            {
               Trim(s);
               args.add(vm->createValue(s));
               state = 0;
            }
            else if (state == 3)
            {
               state = 0;
            }
            else
            {
               state = -1; // error
            }
            break;
         case ' ':
            break;
         case ')':
            if (state == 1)
            {
               *p = 0;
               Trim(s);
               args.add(vm->createValue(s));
               state = 0;
            }
            else if (state == 3)
            {
               state = 0;
            }
            break;
         case '\\':
            if (state == 2)
            {
               memmove(p, p + 1, _tcslen(p) * sizeof(TCHAR));
               switch(*p)
               {
                  case 'r':
                     *p = '\r';
                     break;
                  case 'n':
                     *p = '\n';
                     break;
                  case 't':
                     *p = '\t';
                     break;
                  default:
                     break;
               }
            }
            else if (state == 3)
            {
               state = -1;
            }
            break;
         default:
            if (state == 3)
               state = -1;
            break;
      }
   }
*/
   return (state != -1);
}


/**
 * Read data collection item value from web service.
 * Request is expected in form service:path or service(arguments):path
 */
DataCollectionError ReadWebServiceData(const TCHAR *request, TCHAR *buffer, size_t bsize)
{
   TCHAR name[1024];
   _tcslcpy(name, request, 1024);
   Trim(name);

   TCHAR *path = _tcsrchr(name, _T(':'));
   if (path == NULL)
      return DCE_NOT_SUPPORTED;
   *path = 0;
   path++;

   // Can be in form service(arg1, arg2, ... argN)
   StringList args;
   TCHAR *p = _tcschr(name, _T('('));
   if (p != NULL)
   {
      size_t l = _tcslen(name) - 1;
      if (name[l] != _T(')'))
         return DCE_NOT_SUPPORTED;
      name[l] = 0;
      *p = 0;
      p++;
      if (!ParseCallArgumensList(p, &args))
         return DCE_NOT_SUPPORTED;
   }

   return DCE_SUCCESS;
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
