/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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

#include "aitools.h"
#include <nxcore_websvc.h>
#include <nms_users.h>

/**
 * Maximum size of response document (in characters) included into function output
 */
#define MAX_DOCUMENT_SIZE  65536

/**
 * Placeholder returned instead of secret values
 */
#define SECRET_PLACEHOLDER "***"

/**
 * Get HTTP request method name
 */
static const char *HttpRequestMethodName(HttpRequestMethod method)
{
   switch(method)
   {
      case HttpRequestMethod::_POST:
         return "POST";
      case HttpRequestMethod::_PUT:
         return "PUT";
      case HttpRequestMethod::_DELETE:
         return "DELETE";
      case HttpRequestMethod::_PATCH:
         return "PATCH";
      default:
         return "GET";
   }
}

/**
 * Convert HTTP request method name to integer value. Returns -1 if name is invalid.
 */
static int HttpRequestMethodFromName(const char *name)
{
   static const char *names[] = { "GET", "POST", "PUT", "DELETE", "PATCH", nullptr };
   for(int i = 0; names[i] != nullptr; i++)
      if (!stricmp(name, names[i]))
         return i;
   return -1;
}

/**
 * Get web service authentication type name
 */
static const char *WebServiceAuthTypeName(WebServiceAuthType type)
{
   switch(type)
   {
      case WebServiceAuthType::BASIC:
         return "basic";
      case WebServiceAuthType::DIGEST:
         return "digest";
      case WebServiceAuthType::NTLM:
         return "ntlm";
      case WebServiceAuthType::BEARER:
         return "bearer";
      case WebServiceAuthType::ANY:
         return "any";
      case WebServiceAuthType::ANYSAFE:
         return "anysafe";
      default:
         return "none";
   }
}

/**
 * Convert web service authentication type name to integer value. Returns -1 if name is invalid.
 */
static int WebServiceAuthTypeFromName(const char *name)
{
   static const char *names[] = { "none", "basic", "digest", "ntlm", "bearer", "any", "anysafe", nullptr };
   for(int i = 0; names[i] != nullptr; i++)
      if (!stricmp(name, names[i]))
         return i;
   return -1;
}

/**
 * Check if header value should be hidden from AI model
 */
static bool IsSensitiveHeader(const wchar_t *name)
{
   static const wchar_t *patterns[] = { L"auth", L"token", L"key", L"secret", L"password", L"cookie", nullptr };
   for(int i = 0; patterns[i] != nullptr; i++)
      if (wcsistr(name, patterns[i]) != nullptr)
         return true;
   return false;
}

/**
 * Replace secrets within web service definition JSON with placeholders and convert numeric codes into names. Everything
 * returned by this module is passed to external LLM provider, so credentials must never leave the server.
 */
static void SanitizeDefinitionJson(json_t *definition)
{
   const char *password = json_object_get_string_utf8(definition, "password", "");
   json_object_set_new(definition, "passwordConfigured", json_boolean(*password != 0));
   json_object_del(definition, "password");

   WebServiceAuthType authType = WebServiceAuthTypeFromInt(json_object_get_int32(definition, "authType", 0));
   json_object_set_new(definition, "authType", json_string(WebServiceAuthTypeName(authType)));
   if (authType == WebServiceAuthType::BEARER)
      json_object_set_new(definition, "login", json_string(SECRET_PLACEHOLDER));   // For bearer authentication login field holds access token

   json_object_set_new(definition, "httpRequestMethod",
            json_string(HttpRequestMethodName(HttpRequestMethodFromInt(json_object_get_int32(definition, "httpRequestMethod", 0)))));

   uint32_t flags = json_object_get_uint32(definition, "flags");
   json_object_del(definition, "flags");
   json_object_set_new(definition, "verifyCertificate", json_boolean((flags & WSF_VERIFY_CERTIFICATE) != 0));
   json_object_set_new(definition, "verifyHost", json_boolean((flags & WSF_VERIFY_HOST) != 0));
   json_object_set_new(definition, "followLocation", json_boolean((flags & WSF_FOLLOW_LOCATION) != 0));
   json_object_set_new(definition, "forcePlainTextParser", json_boolean((flags & WSF_FORCE_PLAIN_TEXT_PARSER) != 0));

   json_t *headers = json_object_get(definition, "headers");
   size_t index;
   json_t *header;
   json_array_foreach(headers, index, header)
   {
      wchar_t *name = json_object_get_string_w(header, "name", L"");
      if (IsSensitiveHeader(name))
         json_object_set_new(header, "value", json_string(SECRET_PLACEHOLDER));
      MemFree(name);
   }
}

/**
 * Find web service definition by name or ID given in function arguments
 */
static shared_ptr<WebServiceDefinition> FindDefinition(json_t *arguments, const char *tag)
{
   json_t *value = json_object_get(arguments, tag);
   if (json_is_integer(value))
      return FindWebServiceDefinition(static_cast<uint32_t>(json_integer_value(value)));

   if (!json_is_string(value))
      return shared_ptr<WebServiceDefinition>();

   const char *text = json_string_value(value);
   if (*text == 0)
      return shared_ptr<WebServiceDefinition>();

   char *eptr;
   uint32_t id = strtoul(text, &eptr, 10);
   if ((id != 0) && (*eptr == 0))
   {
      shared_ptr<WebServiceDefinition> definition = FindWebServiceDefinition(id);
      if (definition != nullptr)
         return definition;
   }

   wchar_t *name = WideStringFromUTF8String(text);
   shared_ptr<WebServiceDefinition> definition = FindWebServiceDefinition(name);
   MemFree(name);
   return definition;
}

/**
 * Find data collection target by name or ID and check access rights
 */
static shared_ptr<DataCollectionTarget> FindAndValidateTarget(json_t *arguments, uint32_t userId, uint32_t accessRights, std::string *errorMessage)
{
   shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "object");
   if (object == nullptr)
   {
      *errorMessage = "Object not found";
      return shared_ptr<DataCollectionTarget>();
   }

   if (!object->isDataCollectionTarget())
   {
      *errorMessage = "Object cannot be used as web service query target";
      return shared_ptr<DataCollectionTarget>();
   }

   if (!object->checkAccessRights(userId, accessRights))
   {
      *errorMessage = "Access denied";
      return shared_ptr<DataCollectionTarget>();
   }

   return static_pointer_cast<DataCollectionTarget>(object);
}

/**
 * Acquire connection to effective web service proxy of given target
 */
static shared_ptr<AgentConnectionEx> AcquireProxyConnection(DataCollectionTarget *target, shared_ptr<Node> *proxyNode, std::string *errorMessage)
{
   uint32_t proxyId = target->getEffectiveWebServiceProxy();
   *proxyNode = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
   if (*proxyNode == nullptr)
   {
      *errorMessage = "Web service proxy node configured for this object does not exist";
      return shared_ptr<AgentConnectionEx>();
   }

   shared_ptr<AgentConnectionEx> conn = (*proxyNode)->acquireProxyConnection(WEB_SERVICE_PROXY);
   if (conn == nullptr)
      *errorMessage = "Cannot establish connection with agent on web service proxy node";
   return conn;
}

/**
 * Read optional list of macro expansion arguments
 */
static void ReadMacroArguments(json_t *arguments, StringList *args)
{
   json_t *value = json_object_get(arguments, "args");
   if (json_is_array(value))
   {
      size_t index;
      json_t *element;
      json_array_foreach(value, index, element)
      {
         if (json_is_string(element))
            args->addUTF8String(json_string_value(element));
      }
   }
   else if (json_is_string(value))
   {
      args->addUTF8String(json_string_value(value));
   }
}

/**
 * Add response document to output, truncating it if needed
 */
static void AddDocumentToJson(json_t *output, const wchar_t *document)
{
   if (document == nullptr)
   {
      json_object_set_new(output, "document", json_null());
      json_object_set_new(output, "documentSize", json_integer(0));
      return;
   }

   size_t size = wcslen(document);
   json_object_set_new(output, "documentSize", json_integer(static_cast<json_int_t>(size)));
   if (size > MAX_DOCUMENT_SIZE)
   {
      wchar_t *truncated = MemAllocStringW(MAX_DOCUMENT_SIZE + 1);
      memcpy(truncated, document, MAX_DOCUMENT_SIZE * sizeof(wchar_t));
      truncated[MAX_DOCUMENT_SIZE] = 0;
      json_object_set_new(output, "document", json_string_w(truncated));
      MemFree(truncated);
      json_object_set_new(output, "truncated", json_boolean(true));
   }
   else
   {
      json_object_set_new(output, "document", json_string_w(document));
      json_object_set_new(output, "truncated", json_boolean(false));
   }
}

/**
 * Get list of configured web service definitions
 */
std::string F_ListWebServices(json_t *arguments, uint32_t userId)
{
   if ((GetEffectiveSystemRights(userId) & SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS) == 0)
      return std::string("User does not have rights to read web service definitions");

   const char *filter = json_object_get_string_utf8(arguments, "filter", nullptr);

   json_t *definitions = GetWebServiceDefinitionsAsJson();
   json_t *output = json_array();
   size_t index;
   json_t *definition;
   json_array_foreach(definitions, index, definition)
   {
      if ((filter != nullptr) && (*filter != 0))
      {
         const char *name = json_object_get_string_utf8(definition, "name", "");
         const char *description = json_object_get_string_utf8(definition, "description", "");
         const char *url = json_object_get_string_utf8(definition, "url", "");
         if ((stristr(name, filter) == nullptr) && (stristr(description, filter) == nullptr) && (stristr(url, filter) == nullptr))
            continue;
      }
      SanitizeDefinitionJson(definition);
      json_array_append(output, definition);
   }
   json_decref(definitions);

   return JsonToString(output);
}

/**
 * Get single web service definition
 */
std::string F_GetWebService(json_t *arguments, uint32_t userId)
{
   if ((GetEffectiveSystemRights(userId) & SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS) == 0)
      return std::string("User does not have rights to read web service definitions");

   shared_ptr<WebServiceDefinition> definition = FindDefinition(arguments, "service");
   if (definition == nullptr)
      return std::string("Web service definition not found");

   json_t *output = definition->toJson();
   SanitizeDefinitionJson(output);

   // Metric name template for data collection items using this definition
   StringBuffer metricNameTemplate(definition->getName());
   metricNameTemplate.append(L":<path>");
   json_object_set_new(output, "dciMetricNameTemplate", json_string_t(metricNameTemplate));

   return JsonToString(output);
}

/**
 * Query web service and return raw response document
 */
std::string F_QueryWebService(json_t *arguments, uint32_t userId)
{
   std::string error;
   shared_ptr<DataCollectionTarget> target = FindAndValidateTarget(arguments, userId, OBJECT_ACCESS_QUERY_WEBSVC, &error);
   if (target == nullptr)
      return error;

   shared_ptr<WebServiceDefinition> definition = FindDefinition(arguments, "service");
   if (definition == nullptr)
      return std::string("Web service definition not found");

   shared_ptr<Node> proxyNode;
   shared_ptr<AgentConnectionEx> conn = AcquireProxyConnection(target.get(), &proxyNode, &error);
   if (conn == nullptr)
      return error;

   StringList args;
   ReadMacroArguments(arguments, &args);

   WebServiceCallResult result = definition->retrieveDocument(target.get(), args, conn.get());

   WriteAuditLog(AUDIT_OBJECTS, result.success, userId, nullptr, 0, target->getId(),
            L"AI assistant queried web service \"%s\" on object \"%s\" [%u]", definition->getName(), target->getName(), target->getId());

   json_t *output = json_object();
   json_object_set_new(output, "service", json_string_t(definition->getName()));
   json_object_set_new(output, "object", json_string_t(target->getName()));
   json_object_set_new(output, "proxyNode", json_string_t(proxyNode->getName()));
   json_object_set_new(output, "url",
            json_string_t(target->expandText(definition->getUrl(), nullptr, nullptr, shared_ptr<DCObjectInfo>(), nullptr, nullptr, nullptr, nullptr, &args)));
   json_object_set_new(output, "httpRequestMethod", json_string(HttpRequestMethodName(definition->getHttpRequestMethod())));
   json_object_set_new(output, "success", json_boolean(result.success));
   json_object_set_new(output, "httpResponseCode", json_integer(result.httpResponseCode));
   if (result.success)
   {
      AddDocumentToJson(output, result.document);
   }
   else
   {
      const wchar_t *errorText = AgentErrorCodeToText(result.agentErrorCode);
      json_object_set_new(output, "agentError", json_string_t(errorText));
      // Agent connection layer fills error message with error code text, additional message is reported only if it differs
      if ((result.errorMessage[0] != 0) && wcscmp(result.errorMessage, errorText))
         json_object_set_new(output, "errorMessage", json_string_w(result.errorMessage));
   }

   return JsonToString(output);
}

/**
 * Resolve data extraction path within web service response
 */
std::string F_TestWebServicePath(json_t *arguments, uint32_t userId)
{
   std::string error;
   shared_ptr<DataCollectionTarget> target = FindAndValidateTarget(arguments, userId, OBJECT_ACCESS_QUERY_WEBSVC, &error);
   if (target == nullptr)
      return error;

   shared_ptr<WebServiceDefinition> definition = FindDefinition(arguments, "service");
   if (definition == nullptr)
      return std::string("Web service definition not found");

   wchar_t *path = json_object_get_string_w(arguments, "path", nullptr);
   if ((path == nullptr) || (*path == 0))
   {
      MemFree(path);
      return std::string("Data extraction path must be provided");
   }

   const char *type = json_object_get_string_utf8(arguments, "type", "value");
   bool listRequest = !stricmp(type, "list");
   if (!listRequest && stricmp(type, "value"))
   {
      MemFree(path);
      return std::string("Invalid request type specified. Supported: value, list");
   }

   shared_ptr<Node> proxyNode;
   shared_ptr<AgentConnectionEx> conn = AcquireProxyConnection(target.get(), &proxyNode, &error);
   if (conn == nullptr)
   {
      MemFree(path);
      return error;
   }

   StringList args;
   ReadMacroArguments(arguments, &args);

   json_t *output = json_object();
   json_object_set_new(output, "service", json_string_t(definition->getName()));
   json_object_set_new(output, "object", json_string_t(target->getName()));
   json_object_set_new(output, "path", json_string_w(path));
   json_object_set_new(output, "type", json_string(listRequest ? "list" : "value"));

   uint32_t rcc;
   if (listRequest)
   {
      StringList values;
      rcc = definition->query(target.get(), WebServiceRequestType::LIST, path, args, conn.get(), &values);
      if (rcc == ERR_SUCCESS)
         json_object_set_new(output, "values", values.toJson());
   }
   else
   {
      wchar_t value[MAX_RESULT_LENGTH] = L"";
      rcc = definition->query(target.get(), WebServiceRequestType::PARAMETER, path, args, conn.get(), value);
      if (rcc == ERR_SUCCESS)
         json_object_set_new(output, "value", json_string_w(value));
   }

   json_object_set_new(output, "success", json_boolean(rcc == ERR_SUCCESS));
   if (rcc == ERR_UNKNOWN_METRIC)
      json_object_set_new(output, "error", json_string("Path did not match anything in web service response document"));
   else if (rcc != ERR_SUCCESS)
      json_object_set_new(output, "error", json_string_t(AgentErrorCodeToText(rcc)));

   // Metric name to use when creating data collection item for this path
   if (rcc == ERR_SUCCESS)
   {
      StringBuffer metricName(definition->getName());
      metricName.append(L':');
      metricName.append(path);
      json_object_set_new(output, "dciMetricName", json_string_t(metricName));
   }

   MemFree(path);
   return JsonToString(output);
}

/**
 * Read web service definition properties from function arguments into JSON configuration document.
 * Returns error message on validation failure or empty string on success.
 */
static std::string ReadDefinitionProperties(json_t *arguments, json_t *config)
{
   const char *name = json_object_get_string_utf8(arguments, "name", nullptr);
   if (name != nullptr)
      json_object_set_new(config, "name", json_string(name));

   const char *description = json_object_get_string_utf8(arguments, "description", nullptr);
   if (description != nullptr)
      json_object_set_new(config, "description", json_string(description));

   const char *url = json_object_get_string_utf8(arguments, "url", nullptr);
   if (url != nullptr)
      json_object_set_new(config, "url", json_string(url));

   const char *method = json_object_get_string_utf8(arguments, "http_request_method", nullptr);
   if (method != nullptr)
   {
      int m = HttpRequestMethodFromName(method);
      if (m == -1)
         return std::string("Invalid HTTP request method specified. Supported: GET, POST, PUT, DELETE, PATCH");
      json_object_set_new(config, "httpRequestMethod", json_integer(m));
   }

   const char *requestData = json_object_get_string_utf8(arguments, "request_data", nullptr);
   if (requestData != nullptr)
      json_object_set_new(config, "requestData", json_string(requestData));

   const char *authType = json_object_get_string_utf8(arguments, "auth_type", nullptr);
   if (authType != nullptr)
   {
      int t = WebServiceAuthTypeFromName(authType);
      if (t == -1)
         return std::string("Invalid authentication type specified. Supported: none, basic, digest, ntlm, bearer, any, anysafe");
      json_object_set_new(config, "authType", json_integer(t));
   }

   const char *login = json_object_get_string_utf8(arguments, "login", nullptr);
   if (login != nullptr)
      json_object_set_new(config, "login", json_string(login));

   const char *password = json_object_get_string_utf8(arguments, "password", nullptr);
   if (password != nullptr)
      json_object_set_new(config, "password", json_string(password));

   json_t *value = json_object_get(arguments, "request_timeout");
   if (json_is_integer(value))
      json_object_set_new(config, "requestTimeout", json_integer(json_integer_value(value)));

   value = json_object_get(arguments, "cache_retention_time");
   if (json_is_integer(value))
      json_object_set_new(config, "cacheRetentionTime", json_integer(json_integer_value(value)));

   uint32_t flags = json_object_get_uint32(config, "flags");
   static const std::pair<const char*, uint32_t> flagMap[] =
   {
      { "verify_certificate", WSF_VERIFY_CERTIFICATE },
      { "verify_host", WSF_VERIFY_HOST },
      { "follow_location", WSF_FOLLOW_LOCATION },
      { "force_plain_text_parser", WSF_FORCE_PLAIN_TEXT_PARSER }
   };
   for(const auto& f : flagMap)
   {
      value = json_object_get(arguments, f.first);
      if (json_is_boolean(value))
      {
         if (json_is_true(value))
            flags |= f.second;
         else
            flags &= ~f.second;
      }
   }
   json_object_set_new(config, "flags", json_integer(flags));

   json_t *headers = json_object_get(arguments, "headers");
   if (json_is_object(headers))
      json_object_set(config, "headers", headers);

   return std::string();
}

/**
 * Create or update web service definition. Definition ID 0 means creation of new definition.
 */
static std::string ModifyDefinition(json_t *config, uint32_t definitionId, uint32_t userId)
{
   wchar_t *name = json_object_get_string_w(config, "name", L"");
   if (*name == 0)
   {
      MemFree(name);
      return std::string("Web service definition name must be provided and cannot be empty");
   }

   shared_ptr<WebServiceDefinition> existing = FindWebServiceDefinition(name);
   MemFree(name);
   if ((existing != nullptr) && (existing->getId() != definitionId))
      return std::string("Web service definition with this name already exists");

   auto definition = make_shared<WebServiceDefinition>(config, definitionId);
   uint32_t rcc = ModifyWebServiceDefinition(definition);
   if (rcc != RCC_SUCCESS)
   {
      WriteAuditLog(AUDIT_SYSCFG, false, userId, nullptr, 0, 0, L"AI assistant failed to modify web service definition \"%s\"", definition->getName());
      char message[256];
      snprintf(message, 256, "Cannot save web service definition (error %u)", rcc);
      return std::string(message);
   }

   WriteAuditLog(AUDIT_SYSCFG, true, userId, nullptr, 0, 0, L"AI assistant %s web service definition \"%s\" [%u]",
            (definitionId == 0) ? L"created" : L"modified", definition->getName(), definition->getId());

   json_t *output = definition->toJson();
   SanitizeDefinitionJson(output);
   return JsonToString(output);
}

/**
 * Create new web service definition
 */
std::string F_CreateWebService(json_t *arguments, uint32_t userId)
{
   if ((GetEffectiveSystemRights(userId) & SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS) == 0)
      return std::string("User does not have rights to configure web service definitions");

   json_t *config = json_object();
   json_object_set_new(config, "requestTimeout", json_integer(30000));
   std::string error = ReadDefinitionProperties(arguments, config);
   if (!error.empty())
   {
      json_decref(config);
      return error;
   }

   if (json_object_get_string_utf8(config, "url", nullptr) == nullptr)
   {
      json_decref(config);
      return std::string("Web service URL must be provided");
   }

   std::string result = ModifyDefinition(config, 0, userId);
   json_decref(config);
   return result;
}

/**
 * Update existing web service definition
 */
std::string F_UpdateWebService(json_t *arguments, uint32_t userId)
{
   if ((GetEffectiveSystemRights(userId) & SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS) == 0)
      return std::string("User does not have rights to configure web service definitions");

   shared_ptr<WebServiceDefinition> definition = FindDefinition(arguments, "service");
   if (definition == nullptr)
      return std::string("Web service definition not found");

   json_t *config = definition->toJson();
   std::string error = ReadDefinitionProperties(arguments, config);
   if (!error.empty())
   {
      json_decref(config);
      return error;
   }

   std::string result = ModifyDefinition(config, definition->getId(), userId);
   json_decref(config);
   return result;
}
