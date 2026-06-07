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
** File: object_node_config.cpp
**
** Handlers for class-specific configuration property groups:
**   PATCH /v1/objects/:object-id/polling     (Node polling settings)
**   PATCH /v1/objects/:object-id/auto-bind   (AutoBindTarget classes)
**   PATCH /v1/objects/:object-id/snmp        (Node SNMP communication settings)
**   PATCH /v1/objects/:object-id/agent       (Node agent communication settings)
**
**/

#include "object_helpers.h"
#include <nms_script.h>

/**
 * Load the node addressed by URL placeholder "object-id" for a configuration
 * sub-resource, checking READ access. On failure returns nullptr and writes the HTTP
 * status code (400 / 403 / 404) to *httpCode; a non-node object yields 400. On success
 * *includeSensitiveData reflects whether the caller may see credentials.
 */
static shared_ptr<Node> LoadNodeForConfigRead(Context *context, const wchar_t *group, int *httpCode, bool *includeSensitiveData)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(L"object-id");
   shared_ptr<NetObj> object = (objectId != 0) ? FindObjectById(objectId) : shared_ptr<NetObj>();
   if ((object == nullptr) || object->isUnpublished() || object->isDeleted())
   {
      *httpCode = 404;
      return shared_ptr<Node>();
   }

   uint32_t userId = context->getUserId();
   if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      *httpCode = 403;
      return shared_ptr<Node>();
   }

   if (object->getObjectClass() != OBJECT_NODE)
   {
      wchar_t message[256];
      nx_swprintf(message, 256, L"Property group %s does not apply to object class %s", group, object->getObjectClassName());
      context->setErrorResponse(message);
      *httpCode = 400;
      return shared_ptr<Node>();
   }

   *includeSensitiveData = object->checkAccessRights(userId, OBJECT_ACCESS_MODIFY) || object->checkAccessRights(userId, OBJECT_ACCESS_READ_CREDENTIALS);
   return static_pointer_cast<Node>(object);
}

/**
 * Validate that the object addressed by URL placeholder "object-id" is a node the
 * caller may modify, for a configuration sub-resource PATCH. Returns the node on
 * success; on failure returns nullptr and writes the HTTP status code.
 */
static shared_ptr<NetObj> LoadNodeForConfigModify(Context *context, const wchar_t *group, int *httpCode)
{
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY, httpCode);
   if (object == nullptr)
      return object;

   if (object->getObjectClass() != OBJECT_NODE)
   {
      wchar_t message[256];
      nx_swprintf(message, 256, L"Property group %s does not apply to object class %s", group, object->getObjectClassName());
      context->setErrorResponse(message);
      *httpCode = 400;
      return shared_ptr<NetObj>();
   }
   return object;
}

/**
 * Handler for GET /v1/objects/:object-id/polling.
 * Returns the node polling property group (the same shape accepted by the matching
 * PATCH). Class-blind URL: returns 400 if the target object's class is not a node.
 */
int H_ObjectPollingGet(Context *context)
{
   int httpCode = 0;
   bool includeSensitiveData = false;
   shared_ptr<Node> node = LoadNodeForConfigRead(context, L"polling", &httpCode, &includeSensitiveData);
   if (node == nullptr)
      return httpCode;

   json_t *response = node->pollingConfigToJson();
   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/**
 * Handler for PATCH /v1/objects/:object-id/polling.
 * Applies the node polling property group (poller node, required poll count,
 * expected capabilities, poll-type flags). Class-blind URL: returns 400 if the
 * target object's class does not support polling configuration.
 */
int H_ObjectPollingUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadNodeForConfigModify(context, L"polling", &httpCode);
   if (object == nullptr)
      return httpCode;

   return ApplyJsonPatch(context, object.get(), "polling", L"Modified polling configuration of object %s [%u]");
}

/**
 * Handler for GET /v1/objects/:object-id/snmp.
 * Returns the node SNMP communication property group (the same shape accepted by the
 * matching PATCH). Credential passwords are included only when the caller has modify
 * or read-credentials access.
 */
int H_ObjectSNMPGet(Context *context)
{
   int httpCode = 0;
   bool includeSensitiveData = false;
   shared_ptr<Node> node = LoadNodeForConfigRead(context, L"snmp", &httpCode, &includeSensitiveData);
   if (node == nullptr)
      return httpCode;

   json_t *response = node->snmpConfigToJson(includeSensitiveData);
   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/**
 * Handler for PATCH /v1/objects/:object-id/snmp.
 * Applies the node SNMP communication property group (version, port, credentials,
 * context, codepage, proxy, settings lock, separate trap credentials). Class-blind
 * URL: returns 400 if the target object's class is not a node.
 */
int H_ObjectSNMPUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadNodeForConfigModify(context, L"snmp", &httpCode);
   if (object == nullptr)
      return httpCode;

   return ApplyJsonPatch(context, object.get(), "snmp", L"Modified SNMP configuration of object %s [%u]");
}

/**
 * Handler for GET /v1/objects/:object-id/agent.
 * Returns the node agent communication property group (the same shape accepted by the
 * matching PATCH). The shared secret is included only when the caller has modify or
 * read-credentials access.
 */
int H_ObjectAgentGet(Context *context)
{
   int httpCode = 0;
   bool includeSensitiveData = false;
   shared_ptr<Node> node = LoadNodeForConfigRead(context, L"agent", &httpCode, &includeSensitiveData);
   if (node == nullptr)
      return httpCode;

   json_t *response = node->agentConfigToJson(includeSensitiveData);
   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/**
 * Handler for PATCH /v1/objects/:object-id/agent.
 * Applies the node agent communication property group (port, proxy, shared secret,
 * encryption/tunnel flags, cache and compression modes). Class-blind URL: returns
 * 400 if the target object's class is not a node.
 */
int H_ObjectAgentUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadNodeForConfigModify(context, L"agent", &httpCode);
   if (object == nullptr)
      return httpCode;

   return ApplyJsonPatch(context, object.get(), "agent", L"Modified agent configuration of object %s [%u]");
}

/**
 * Validate auto-bind filter scripts present in the request body. Compiles each
 * non-null "source" string and, on the first compilation failure, writes a 400
 * error response (with diagnostic) to the context and returns false.
 */
static bool ValidateAutoBindScripts(json_t *request, Context *context)
{
   json_t *filters = json_object_get(request, "autoBindFilters");
   if (!json_is_array(filters))
      return true;   // nothing to validate; shape errors are reported by modifyFromJSON

   size_t index;
   json_t *entry;
   json_array_foreach(filters, index, entry)
   {
      json_t *source = json_object_get(entry, "source");
      if (!json_is_string(source))
         continue;

      wchar_t *script = WideStringFromUTF8String(json_string_value(source));
      NXSL_CompilationDiagnostic diag;
      NXSL_VM *vm = NXSLCompileAndCreateVM(script, new NXSL_ServerEnv(), &diag);
      MemFree(script);
      if (vm == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, L"H_ObjectAutoBindUpdate: filter %d compilation error (%s)",
            static_cast<int>(index), diag.errorText.cstr());
         json_t *response = json_object();
         json_object_set_new(response, "error", json_string("Auto-bind filter script compilation failed"));
         json_object_set_new(response, "filterIndex", json_integer(index));
         json_object_set_new(response, "diagnostic", diag.toJson());
         context->setResponseData(response);
         json_decref(response);
         return false;
      }
      delete vm;
   }
   return true;
}

/**
 * Handler for PATCH /v1/objects/:object-id/auto-bind.
 * Applies the auto-bind property group to any AutoBindTarget-derived object.
 * Filter scripts are validated by default (compiled); pass ?validate=false to
 * store work-in-progress / intentionally broken scripts (matches NXCP leniency).
 */
int H_ObjectAutoBindUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<NetObj> object = LoadObjectForModify(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (object == nullptr)
      return httpCode;

   AutoBindTarget *autoBindTarget = GetObjectAsAutoBindTarget(object.get());
   if (autoBindTarget == nullptr)
   {
      wchar_t message[256];
      nx_swprintf(message, 256, L"Property group auto-bind does not apply to object class %s", object->getObjectClassName());
      context->setErrorResponse(message);
      return 400;
   }

   json_t *request = context->getRequestDocument();
   if ((request == nullptr) || !json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return 400;
   }

   if (context->getQueryParameterAsBoolean("validate", true) && !ValidateAutoBindScripts(request, context))
      return 400;

   json_t *oldSnapshot = object->toJson(false);
   uint32_t rcc = autoBindTarget->modifyFromJSON(request);
   if (rcc != RCC_SUCCESS)
   {
      json_decref(oldSnapshot);
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, L"H_ObjectAutoBindUpdate: modifyFromJSON failed for object %s [%u] with RCC %u",
         object->getName(), object->getId(), rcc);
      context->setErrorResponse("Invalid property values in request");
      return 400;
   }
   object->markAsModified(MODIFY_ALL);

   json_t *newSnapshot = object->toJson(false);
   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, object->getId(), oldSnapshot, newSnapshot,
      L"Modified auto-bind configuration of object %s [%u]", object->getName(), object->getId());
   json_decref(oldSnapshot);

   context->setResponseData(newSnapshot);
   json_decref(newSnapshot);
   return 200;
}
