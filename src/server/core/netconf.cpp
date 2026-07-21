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
** File: netconf.cpp
**
**/

#include "nxcore.h"
#include <nxcore_netconf.h>

#define DEBUG_TAG L"netconf"

/**
 * Create NETCONF query definition from NXCP message
 */
NetconfQueryDefinition::NetconfQueryDefinition(const NXCPMessage& msg)
{
   m_id = msg.getFieldAsUInt32(VID_QUERY_ID);
   if (m_id == 0)
      m_id = CreateUniqueId(IDG_NETCONF_QUERY);
   m_guid = msg.getFieldAsGUID(VID_GUID);
   if (m_guid.isNull())
      m_guid = uuid::generate();
   m_name = msg.getFieldAsString(VID_NAME);
   m_description = msg.getFieldAsString(VID_DESCRIPTION);
   m_datastore = msg.getFieldAsInt16(VID_DATASTORE);
   m_filterType = msg.getFieldAsInt16(VID_FILTER_TYPE);
   m_filter = msg.getFieldAsString(VID_FILTER);
   m_cacheRetentionTime = msg.getFieldAsUInt32(VID_RETENTION_TIME);
   m_requestTimeout = msg.getFieldAsUInt32(VID_TIMEOUT);
   m_flags = msg.getFieldAsUInt32(VID_FLAGS);
}

/**
 * Create NETCONF query definition from database record
 * Expected column order: id,guid,name,description,datastore,filter_type,filter,cache_retention_time,request_timeout,flags
 */
NetconfQueryDefinition::NetconfQueryDefinition(DB_HANDLE hdb, DB_RESULT hResult, int row)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   m_guid = DBGetFieldGUID(hResult, row, 1);
   m_name = DBGetField(hResult, row, 2, nullptr, 0);
   m_description = DBGetField(hResult, row, 3, nullptr, 0);
   m_datastore = static_cast<int16_t>(DBGetFieldLong(hResult, row, 4));
   m_filterType = static_cast<int16_t>(DBGetFieldLong(hResult, row, 5));
   m_filter = DBGetField(hResult, row, 6, nullptr, 0);
   m_cacheRetentionTime = DBGetFieldULong(hResult, row, 7);
   m_requestTimeout = DBGetFieldULong(hResult, row, 8);
   m_flags = DBGetFieldULong(hResult, row, 9);
}

/**
 * NETCONF query definition destructor
 */
NetconfQueryDefinition::~NetconfQueryDefinition()
{
   MemFree(m_name);
   MemFree(m_description);
   MemFree(m_filter);
}

/**
 * Fill NXCP message with query definition data
 */
void NetconfQueryDefinition::fillMessage(NXCPMessage *msg) const
{
   msg->setField(VID_QUERY_ID, m_id);
   msg->setField(VID_GUID, m_guid);
   msg->setField(VID_NAME, m_name);
   msg->setField(VID_DESCRIPTION, m_description);
   msg->setField(VID_DATASTORE, m_datastore);
   msg->setField(VID_FILTER_TYPE, m_filterType);
   msg->setField(VID_FILTER, m_filter);
   msg->setField(VID_RETENTION_TIME, m_cacheRetentionTime);
   msg->setField(VID_TIMEOUT, m_requestTimeout);
   msg->setField(VID_FLAGS, m_flags);
}

/**
 * List of configured NETCONF query definitions
 */
static SharedObjectArray<NetconfQueryDefinition> s_netconfQueryDefinitions;
static Mutex s_netconfQueryDefinitionLock(MutexType::FAST);

/**
 * Load NETCONF query definitions from database
 */
void LoadNetconfQueryDefinitions()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_RESULT hResult = DBSelect(hdb, L"SELECT id,guid,name,description,datastore,filter_type,filter,cache_retention_time,request_timeout,flags FROM netconf_queries");
   if (hResult == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"NETCONF query definitions cannot be loaded due to database failure");
      return;
   }

   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
      s_netconfQueryDefinitions.add(make_shared<NetconfQueryDefinition>(hdb, hResult, i));

   DBFreeResult(hResult);
   DBConnectionPoolReleaseConnection(hdb);

   nxlog_debug_tag(DEBUG_TAG, 2, L"%d NETCONF query definitions loaded", count);
}

/**
 * Get all NETCONF query definitions
 */
SharedObjectArray<NetconfQueryDefinition> *GetNetconfQueryDefinitions()
{
   s_netconfQueryDefinitionLock.lock();
   auto definitions = new SharedObjectArray<NetconfQueryDefinition>(s_netconfQueryDefinitions);
   s_netconfQueryDefinitionLock.unlock();
   return definitions;
}

/**
 * Find NETCONF query definition by ID
 */
shared_ptr<NetconfQueryDefinition> NXCORE_EXPORTABLE FindNetconfQueryDefinition(uint32_t id)
{
   shared_ptr<NetconfQueryDefinition> result;
   s_netconfQueryDefinitionLock.lock();
   for(int i = 0; i < s_netconfQueryDefinitions.size(); i++)
   {
      auto d = s_netconfQueryDefinitions.getShared(i);
      if (d->getId() == id)
      {
         result = d;
         break;
      }
   }
   s_netconfQueryDefinitionLock.unlock();
   return result;
}

/**
 * Find NETCONF query definition by name
 */
shared_ptr<NetconfQueryDefinition> NXCORE_EXPORTABLE FindNetconfQueryDefinition(const TCHAR *name)
{
   shared_ptr<NetconfQueryDefinition> result;
   s_netconfQueryDefinitionLock.lock();
   for(int i = 0; i < s_netconfQueryDefinitions.size(); i++)
   {
      auto d = s_netconfQueryDefinitions.getShared(i);
      if (!wcsicmp(name, d->getName()))
      {
         result = d;
         break;
      }
   }
   s_netconfQueryDefinitionLock.unlock();
   return result;
}

/**
 * Modify NETCONF query definition. Returns client RCC.
 */
uint32_t NXCORE_EXPORTABLE ModifyNetconfQueryDefinition(shared_ptr<NetconfQueryDefinition> definition)
{
   uint32_t rcc;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   static const TCHAR *columns[] = { L"guid", L"name", L"description", L"datastore", L"filter_type",
            L"filter", L"cache_retention_time", L"request_timeout", L"flags", nullptr };
   DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"netconf_queries", L"id", definition->getId(), columns);
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, definition->getGuid());
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, definition->getName(), DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, definition->getDescription(), DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<int32_t>(definition->getDatastore()));
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<int32_t>(definition->getFilterType()));
      DBBind(hStmt, 6, DB_SQLTYPE_TEXT, definition->getFilter(), DB_BIND_STATIC);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, definition->getCacheRetentionTime());
      DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, definition->getRequestTimeout());
      DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, definition->getFlags());
      DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, definition->getId());
      if (DBExecute(hStmt))
      {
         rcc = RCC_SUCCESS;

         s_netconfQueryDefinitionLock.lock();
         bool found = false;
         for(int i = 0; i < s_netconfQueryDefinitions.size(); i++)
         {
            if (s_netconfQueryDefinitions.get(i)->getId() == definition->getId())
            {
               s_netconfQueryDefinitions.replace(i, definition);
               found = true;
               break;
            }
         }
         if (!found)
            s_netconfQueryDefinitions.add(definition);
         s_netconfQueryDefinitionLock.unlock();
      }
      else
      {
         rcc = RCC_DB_FAILURE;
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }
   DBConnectionPoolReleaseConnection(hdb);

   return rcc;
}

/**
 * Delete NETCONF query definition. Returns client RCC.
 */
uint32_t NXCORE_EXPORTABLE DeleteNetconfQueryDefinition(uint32_t id)
{
   uint32_t rcc = RCC_INVALID_OBJECT_ID;
   shared_ptr<NetconfQueryDefinition> definition;
   s_netconfQueryDefinitionLock.lock();
   for(int i = 0; i < s_netconfQueryDefinitions.size(); i++)
   {
      auto d = s_netconfQueryDefinitions.getShared(i);
      if (d->getId() == id)
      {
         definition = d;
         s_netconfQueryDefinitions.remove(i);
         rcc = RCC_SUCCESS;
         break;
      }
   }
   s_netconfQueryDefinitionLock.unlock();

   if (rcc == RCC_SUCCESS)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      if (!ExecuteQueryOnObject(hdb, id, L"DELETE FROM netconf_queries WHERE id=?"))
      {
         rcc = RCC_DB_FAILURE;

         // Re-insert query definition into list
         s_netconfQueryDefinitionLock.lock();
         s_netconfQueryDefinitions.add(definition);
         s_netconfQueryDefinitionLock.unlock();
      }
      DBConnectionPoolReleaseConnection(hdb);
   }

   return rcc;
}

/**
 * Check NETCONF connectivity via given proxy. On success optionally returns capability
 * list announced by the device in hello message.
 */
bool NetconfCheckConnection(uint32_t proxyId, const InetAddress& addr, uint16_t port, const TCHAR *login, const TCHAR *password, uint32_t keyId, StringList *capabilities)
{
   shared_ptr<NetObj> proxyNode = FindObjectById(proxyId, OBJECT_NODE);
   if (proxyNode == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"NetconfCheckConnection(%s:%u): invalid proxy node ID %u", addr.toString().cstr(), port, proxyId);
      return false;
   }

   shared_ptr<AgentConnectionEx> conn = static_cast<Node&>(*proxyNode).acquireProxyConnection(NETCONF_PROXY);
   if (conn == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"NetconfCheckConnection(%s:%u): cannot acquire connection to proxy agent", addr.toString().cstr(), port);
      return false;
   }

   NXCPMessage request(CMD_NETCONF_QUERY, conn->generateRequestId(), conn->getProtocolVersion());
   request.setField(VID_IP_ADDRESS, addr);
   request.setField(VID_PORT, port);
   request.setField(VID_USER_NAME, login);
   request.setField(VID_PASSWORD, password);
   request.setField(VID_SSH_KEY_ID, keyId);
   request.setField(VID_REQUEST_TYPE, static_cast<uint16_t>(NetconfRequestType::CAPABILITIES));

   bool success = false;
   NXCPMessage *response = conn->customRequest(&request);
   if (response != nullptr)
   {
      uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
      if (rcc == ERR_SUCCESS)
      {
         success = true;
         if (capabilities != nullptr)
            *capabilities = StringList(*response, VID_ELEMENT_LIST_BASE, VID_NUM_ELEMENTS);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, L"NetconfCheckConnection(%s:%u): agent error %u", addr.toString().cstr(), port, rcc);
      }
      delete response;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"NetconfCheckConnection(%s:%u): timeout waiting for proxy agent response", addr.toString().cstr(), port);
   }
   return success;
}

/**
 * Get copy of NETCONF capability list detected on this node
 */
StringList Node::getNetconfCapabilities() const
{
   lockProperties();
   StringList capabilities(m_netconfCapabilities);
   unlockProperties();
   return capabilities;
}

/**
 * Get metric value via NETCONF. Metric name format: <query-definition-name>:<xpath>,
 * where XPath expression is evaluated against document cached on proxy agent and
 * addressed as /data/...
 */
DataCollectionError Node::getMetricFromNetconf(const TCHAR *metric, TCHAR *buffer, size_t size)
{
   const TCHAR *separator = wcschr(metric, L':');
   if ((separator == nullptr) || (separator == metric) || (separator[1] == 0))
   {
      nxlog_debug_tag(DEBUG_TAG_DC_NETCONF, 6, L"Node::getMetricFromNetconf(%s [%u]): invalid metric name \"%s\"", m_name, m_id, metric);
      return DCE_NOT_SUPPORTED;
   }

   String definitionName(metric, separator - metric);
   const TCHAR *xpath = separator + 1;

   shared_ptr<NetconfQueryDefinition> definition = FindNetconfQueryDefinition(definitionName);
   if (definition == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_NETCONF, 6, L"Node::getMetricFromNetconf(%s [%u]): unknown query definition \"%s\"", m_name, m_id, definitionName.cstr());
      return DCE_NOT_SUPPORTED;
   }

   shared_ptr<NetObj> proxyNode = FindObjectById(getEffectiveNetconfProxy(), OBJECT_NODE);
   if (proxyNode == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_NETCONF, 6, L"Node::getMetricFromNetconf(%s [%u]): invalid NETCONF proxy node", m_name, m_id);
      return DCE_COMM_ERROR;
   }

   shared_ptr<AgentConnectionEx> conn = static_cast<Node&>(*proxyNode).acquireProxyConnection(NETCONF_PROXY);
   if (conn == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_NETCONF, 6, L"Node::getMetricFromNetconf(%s [%u]): cannot acquire connection to proxy agent", m_name, m_id);
      return DCE_COMM_ERROR;
   }

   NXCPMessage request(CMD_NETCONF_QUERY, conn->generateRequestId(), conn->getProtocolVersion());
   request.setField(VID_IP_ADDRESS, m_ipAddress);
   request.setField(VID_PORT, m_netconfPort);
   request.setField(VID_USER_NAME, getSshLogin());
   request.setField(VID_PASSWORD, getSshPassword());
   request.setField(VID_SSH_KEY_ID, m_sshKeyId);
   request.setField(VID_TIMEOUT, definition->getRequestTimeout());
   request.setField(VID_RETENTION_TIME, definition->getCacheRetentionTime());
   request.setField(VID_DATASTORE, definition->getDatastore());
   request.setField(VID_FILTER_TYPE, definition->getFilterType());
   request.setField(VID_FILTER, definition->getFilter());
   request.setField(VID_NUM_PARAMETERS, static_cast<int32_t>(1));
   request.setField(VID_PARAM_LIST_BASE, xpath);

   NXCPMessage *response = conn->customRequest(&request);
   if (response == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_NETCONF, 6, L"Node::getMetricFromNetconf(%s [%u]): timeout waiting for proxy agent response", m_name, m_id);
      return DCE_COMM_ERROR;
   }

   DataCollectionError rc;
   uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
   if (rcc == ERR_SUCCESS)
   {
      if (response->getFieldAsInt32(VID_NUM_PARAMETERS) > 0)
      {
         response->getFieldAsString(VID_PARAM_LIST_BASE + 1, buffer, size);
         rc = DCE_SUCCESS;
      }
      else
      {
         rc = DCE_NO_SUCH_INSTANCE;
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DC_NETCONF, 6, L"Node::getMetricFromNetconf(%s [%u]): agent error %u", m_name, m_id, rcc);
      rc = DCE_COMM_ERROR;
   }
   delete response;
   return rc;
}
