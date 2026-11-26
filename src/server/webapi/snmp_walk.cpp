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
** File: snmp_walk.cpp
**
**/

#include "webapi.h"

#define DEBUG_TAG L"webapi.snmp"

/**
 * Default and maximum number of variables returned by synchronous walk
 */
#define DEFAULT_WALK_LIMIT 10000
#define MAX_WALK_LIMIT     100000

/**
 * Number of variables sent in single WebSocket message by streaming walk
 */
#define STREAM_BATCH_SIZE  50

/**
 * Create JSON representation of SNMP variable
 */
static json_t *SnmpVariableToJson(const SNMP_Variable *v)
{
   json_t *json = json_object();

   char oid[MAX_OID_LEN * 11 + 1];
   json_object_set_new(json, "oid", json_string(v->getName().toStringA(oid, sizeof(oid))));

   char typeName[64];
   json_object_set_new(json, "type", json_string(SnmpDataTypeNameA(v->getType(), typeName, sizeof(typeName))));

   wchar_t value[4096];
   json_object_set_new(json, "value", json_string_w(FormatSNMPValue(v, value, 4096)));

   // Numeric types and OIDs are stored decoded in host representation, all other types keep value bytes as received
   switch(v->getType())
   {
      case ASN_OBJECT_ID:
      case ASN_INTEGER:
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
      case ASN_COUNTER64:
      case ASN_INTEGER64:
      case ASN_UINTEGER64:
      case ASN_FLOAT:
      case ASN_DOUBLE:
         break;
      default:
         {
            size_t length = v->getValueLength();
            Buffer<char, 1024> rawValue(length * 2 + 1);
            json_object_set_new(json, "rawValue", json_string(BinToStrA(v->getValue(), length, rawValue)));
         }
         break;
   }
   return json;
}

/**
 * Convert SNMP error code to UTF-8 error message
 */
static const char *SnmpErrorTextUTF8(uint32_t rc, char *buffer, size_t size)
{
   size_t bytes = wchar_to_utf8(SnmpGetErrorText(rc), -1, buffer, size - 1);
   buffer[bytes] = 0;
   return buffer;
}

/**
 * Context for synchronous walk
 */
struct SyncWalkContext
{
   json_t *variables;
   int32_t limit;
};

/**
 * Callback for synchronous walk. Stops walk on first variable beyond the limit, so truncation is reported only
 * when there are more variables.
 */
static uint32_t SyncWalkCallback(SNMP_Variable *v, SNMP_Transport *transport, SyncWalkContext *context)
{
   if (json_array_size(context->variables) >= static_cast<size_t>(context->limit))
      return SNMP_ERR_SNAPSHOT_TOO_BIG;
   json_array_append_new(context->variables, SnmpVariableToJson(v));
   return SNMP_ERR_SUCCESS;
}

/**
 * Streaming walk request
 */
struct StreamingWalkRequest
{
   shared_ptr<Node> node;
   String snmpAgentName;
   String oid;
   int32_t limit;
   shared_ptr<WebSocketOutputSession> session;
   json_t *batch;
   int32_t count;

   StreamingWalkRequest(const shared_ptr<Node>& _node, const String& _snmpAgentName, const String& _oid, int32_t _limit, const shared_ptr<WebSocketOutputSession>& _session) :
      node(_node), snmpAgentName(_snmpAgentName), oid(_oid), session(_session)
   {
      limit = _limit;
      batch = json_array();
      count = 0;
   }

   ~StreamingWalkRequest()
   {
      json_decref(batch);
   }

   /**
    * Send accumulated variables to client
    */
   void flush()
   {
      if (json_array_size(batch) == 0)
         return;
      json_t *msg = json_object();
      json_object_set_new(msg, "type", json_string("data"));
      json_object_set_new(msg, "variables", batch);
      session->sendMessage(msg);
      json_decref(msg);
      batch = json_array();
   }
};

/**
 * Callback for streaming walk. Walk is aborted when client disconnects.
 */
static uint32_t StreamingWalkCallback(SNMP_Variable *v, SNMP_Transport *transport, StreamingWalkRequest *request)
{
   if (request->session->isDisconnected())
      return SNMP_ERR_ABORTED;
   if ((request->limit > 0) && (request->count >= request->limit))
      return SNMP_ERR_SNAPSHOT_TOO_BIG;

   json_array_append_new(request->batch, SnmpVariableToJson(v));
   request->count++;
   if (json_array_size(request->batch) >= STREAM_BATCH_SIZE)
      request->flush();
   return SNMP_ERR_SUCCESS;
}

/**
 * Execute streaming walk (client thread pool callback). Walk is started only after client connects.
 */
static void ExecuteStreamingWalk(StreamingWalkRequest *request)
{
   if (!request->session->waitForConnection())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"SNMP walk of %s on node %s [%u] cancelled: client did not connect", request->oid.cstr(), request->node->getName(), request->node->getId());
      delete request;
      return;
   }

   uint32_t rc = request->node->callSnmpEnumerate(request->snmpAgentName.isEmpty() ? nullptr : request->snmpAgentName.cstr(), request->oid, StreamingWalkCallback, request);
   request->flush();
   if ((rc == SNMP_ERR_SUCCESS) || (rc == SNMP_ERR_SNAPSHOT_TOO_BIG))
   {
      json_t *msg = json_object();
      json_object_set_new(msg, "type", json_string("completed"));
      json_object_set_new(msg, "count", json_integer(request->count));
      json_object_set_new(msg, "truncated", json_boolean(rc == SNMP_ERR_SNAPSHOT_TOO_BIG));
      request->session->finish(msg);
      json_decref(msg);
   }
   else
   {
      char errorText[256];
      request->session->sendError(SnmpErrorTextUTF8(rc, errorText, sizeof(errorText)));
   }
   nxlog_debug_tag(DEBUG_TAG, 5, L"SNMP walk of %s on node %s [%u] completed (%d variables, rc=%u)", request->oid.cstr(), request->node->getName(), request->node->getId(), request->count, rc);
   delete request;
}

/**
 * Handler for POST /v1/objects/:object-id/snmp-walk
 */
int H_ObjectSnmpWalk(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(L"object-id");
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (object->getObjectClass() != OBJECT_NODE)
   {
      context->setErrorResponse("Object is not a node");
      return 400;
   }

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ_SNMP))
      return 403;

   shared_ptr<Node> node = static_pointer_cast<Node>(object);
   json_t *request = context->getRequestDocument();

   const char *oidText = json_object_get_string_utf8(request, "oid", nullptr);
   if ((oidText == nullptr) || (*oidText == 0))
   {
      context->setErrorResponse("SNMP OID must be provided");
      return 400;
   }
   SNMP_ObjectId oid = SNMP_ObjectId::parseA(oidText);
   if (oid.length() == 0)
   {
      context->setErrorResponse("Invalid SNMP OID");
      return 400;
   }

   String snmpAgentName = json_object_get_string(request, "snmpAgentName", nullptr);
   if (snmpAgentName.isEmpty())
   {
      if (!node->isSNMPSupported())
      {
         context->setErrorResponse("SNMP is not supported on this node");
         return 400;
      }
   }
   else
   {
      AdditionalSnmpAgent *agent = node->getAdditionalSnmpAgent(snmpAgentName);
      if (agent == nullptr)
      {
         context->setErrorResponse("Unknown additional SNMP agent");
         return 400;
      }
      delete agent;
   }

   bool stream = json_object_get_boolean(request, "stream", false);
   int32_t limit = json_object_get_int32(request, "limit", stream ? 0 : DEFAULT_WALK_LIMIT);
   if ((limit < 0) || (!stream && ((limit == 0) || (limit > MAX_WALK_LIMIT))))
   {
      context->setErrorResponse("Invalid limit");
      return 400;
   }

   String oidString = oid.toString();
   nxlog_debug_tag(DEBUG_TAG, 4, L"SNMP walk of %s on node %s [%u] requested by user %s [%u] (agent=\"%s\", stream=%s, limit=%d)",
      oidString.cstr(), node->getName(), node->getId(), context->getLoginName(), context->getUserId(), snmpAgentName.cstr(), stream ? L"true" : L"false", limit);

   if (stream)
   {
      auto session = make_shared<WebSocketOutputSession>();
      uuid token = RegisterWebSocketOutputSession(session);
      ThreadPoolExecute(g_clientThreadPool, ExecuteStreamingWalk, new StreamingWalkRequest(node, snmpAgentName, oidString, limit, session));
      json_t *response = CreateWebSocketOutputResponse(token, "/v1/snmp-walks/output");
      json_object_set_new(response, "oid", json_string_w(oidString));
      context->setResponseData(response);
      json_decref(response);
      return 202;
   }

   SyncWalkContext walkContext;
   walkContext.variables = json_array();
   walkContext.limit = limit;
   uint32_t rc = node->callSnmpEnumerate(snmpAgentName.isEmpty() ? nullptr : snmpAgentName.cstr(), oidString, SyncWalkCallback, &walkContext);
   nxlog_debug_tag(DEBUG_TAG, 5, L"SNMP walk of %s on node %s [%u] completed (%d variables, rc=%u)", oidString.cstr(), node->getName(), node->getId(),
      static_cast<int>(json_array_size(walkContext.variables)), rc);

   if ((rc != SNMP_ERR_SUCCESS) && (rc != SNMP_ERR_SNAPSHOT_TOO_BIG) && (json_array_size(walkContext.variables) == 0))
   {
      json_decref(walkContext.variables);
      context->setErrorResponse(SnmpGetErrorText(rc));
      return 502;
   }

   json_t *response = json_object();
   json_object_set_new(response, "oid", json_string_w(oidString));
   json_object_set_new(response, "variables", walkContext.variables);
   json_object_set_new(response, "truncated", json_boolean(rc == SNMP_ERR_SNAPSHOT_TOO_BIG));
   if ((rc != SNMP_ERR_SUCCESS) && (rc != SNMP_ERR_SNAPSHOT_TOO_BIG))
      json_object_set_new(response, "error", json_string_w(SnmpGetErrorText(rc)));
   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/**
 * WebSocket upgrade handler for SNMP walk output streaming
 */
void WS_SnmpWalkOutputConnect(void *cls, MHD_Connection *connection, void *con_cls,
                              const char *extra_in, size_t extra_in_size, MHD_socket sock,
                              MHD_UpgradeResponseHandle *responseHandle)
{
   ConnectWebSocketOutputSession(static_cast<Context*>(cls), sock, responseHandle);
}
