/*
** NetXMS - Network Management System
** Copyright (C) 2023-2025 Raden Solutions
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
** File: netxms-webapi.h
**
**/

#ifndef _netxms_webapi_h_
#define _netxms_webapi_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_core.h>
#include <microhttpd.h>

#include <string>

#if MHD_VERSION < 0x00097002
#define MHD_Result int
#endif

#define DEBUG_TAG_WEBAPI  _T("webapi")

#define AUTH_TOKEN_VALIDITY_TIME 86400

/* do undefs for Method enum values in case any of them defined in system headers */
#undef DELETE
#undef GET
#undef PATCH
#undef POST
#undef PUT
#undef UNKNOWN

/**
 * Request method
 */
enum class Method
{
   DELETE = 0,
   GET = 1,
   PATCH = 2,
   POST = 3,
   PUT = 4,
   UNKNOWN = 5
};

/**
 * Method code from symbolic name
 */
static inline Method MethodFromSymbolicName(const char *method)
{
   if (!strcmp(method, "GET"))
      return Method::GET;
   if (!strcmp(method, "POST"))
      return Method::POST;
   if (!strcmp(method, "DELETE"))
      return Method::DELETE;
   if (!strcmp(method, "PUT"))
      return Method::PUT;
   if (!strcmp(method, "PATCH"))
      return Method::PATCH;
   return Method::UNKNOWN;
}

/**
 * Get client IP address from connection
 */
static inline InetAddress GetClientAddress(MHD_Connection *connection)
{
   const MHD_ConnectionInfo *info = MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
   return InetAddress::createFromSockaddr(info->client_addr);
}

class Context;
typedef int (*RouteHandler)(Context *context);

/**
 * Web server route builder
 */
class NXCORE_EXPORTABLE RouteBuilder
{
private:
   const char *m_path;
   bool m_auth;
   RouteHandler m_handlers[5];

public:
   RouteBuilder(const char *path)
   {
      m_path = path;
      m_auth = true;
      memset(m_handlers, 0, sizeof(m_handlers));
   }

   RouteBuilder& GET(RouteHandler handler)
   {
      m_handlers[(int)Method::GET] = handler;
      return *this;
   }

   RouteBuilder& POST(RouteHandler handler)
   {
      m_handlers[(int)Method::POST] = handler;
      return *this;
   }

   RouteBuilder& PUT(RouteHandler handler)
   {
      m_handlers[(int)Method::PUT] = handler;
      return *this;
   }

   RouteBuilder& PATCH(RouteHandler handler)
   {
      m_handlers[(int)Method::PATCH] = handler;
      return *this;
   }

   RouteBuilder& DELETE(RouteHandler handler)
   {
      m_handlers[(int)Method::DELETE] = handler;
      return *this;
   }

   RouteBuilder& noauth()
   {
      m_auth = false;
      return *this;
   }

   void build();
};

/**
 * Request context
 */
class NXCORE_EXPORTABLE Context : public GenericClientSession
{
private:
   MHD_Connection *m_connection;
   Method m_method;
   std::string m_path;
   RouteHandler m_handler;
   ByteStream m_requestData;
   ByteStream m_responseData;
   char m_contentType[64];
   json_t *m_requestDocument;
   UserAuthenticationToken m_token;
   StringMap m_placeholderValues;
   StringMap *m_responseHeaders;

public:
   Context(MHD_Connection *connection, const char *path, Method method, RouteHandler handler, const UserAuthenticationToken& token, uint32_t userId,
      const TCHAR *userName, uint64_t systemAccessRights, StringMap&& placeholderValues)
      : m_path(path), m_requestData(0), m_responseData(0), m_token(token), m_placeholderValues(std::move(placeholderValues))
   {
      m_connection = connection;
      m_method = method;
      m_handler = handler;
      m_requestData.setAllocationStep(32768);
      m_responseData.setAllocationStep(32768);
      m_requestDocument = nullptr;
      m_userId = userId;
      _tcscpy(m_loginName, userName);
      m_systemAccessRights = systemAccessRights;
      m_responseHeaders = nullptr;
      GetClientAddress(connection).toString(m_workstation);
   }

   virtual ~Context()
   {
      json_decref(m_requestDocument);
      delete m_responseHeaders;
   }

   const UserAuthenticationToken& getAuthenticationToken() const
   {
      return m_token;
   }

   Method getMethod() const
   {
      return m_method;
   }

   const char *getPath() const
   {
      return m_path.c_str();
   }

   const char *getQueryParameter(const char *name) const
   {
      return MHD_lookup_connection_value(m_connection, MHD_GET_ARGUMENT_KIND, name);
   }

   int32_t getQueryParameterAsInt32(const char *name, int32_t defaultValue = 0) const
   {
      const char *v = getQueryParameter(name);
      return (v != nullptr) ? strtol(v, nullptr, 0) : defaultValue;
   }

   uint32_t getQueryParameterAsUInt32(const char *name, uint32_t defaultValue = 0) const
   {
      const char *v = getQueryParameter(name);
      return (v != nullptr) ? strtoul(v, nullptr, 0) : defaultValue;
   }

   bool getQueryParameterAsBoolean(const char *name, bool defaultValue = false) const
   {
      const char *v = getQueryParameter(name);
      if (v == nullptr)
         return defaultValue;
      return !stricmp(v, "true") || !stricmp(v, "yes") || strtoul(v, nullptr, 0);
   }

   time_t getQueryParameterAsTime(const char *name, time_t defaultValue = 0) const;

   const TCHAR *getPlaceholderValue(const TCHAR *name) const
   {
      return m_placeholderValues.get(name);
   }

   const uint32_t getPlaceholderValueAsUInt32(const TCHAR *name, uint32_t defaultValue = 0) const
   {
      const TCHAR *v = m_placeholderValues.get(name);
      return (v != nullptr) ? _tcstoul(v, nullptr, 0) : defaultValue;
   }

   bool isDataRequired() const
   {
      return (m_method == Method::POST) || (m_method == Method::PATCH) || (m_method == Method::PUT);
   }

   bool hasRequestData() const
   {
      return m_requestData.size() > 0;
   }

   const char *getRequestData() const
   {
      return reinterpret_cast<const char*>(m_requestData.buffer());
   }

   json_t *getRequestDocument();

   const StringMap *getResponseHeaders() const
   {
      return m_responseHeaders;
   }

   bool hasResponseData() const
   {
      return m_responseData.size() > 0;
   }

   void *getResponseData()
   {
      return (void*)m_responseData.buffer();
   }

   size_t getResponseDataSize()
   {
      return m_responseData.size();
   }

   void onUploadData(const char *data, size_t size)
   {
      m_requestData.write(data, size);
   }

   void onUploadComplete()
   {
      if (m_requestData.size() > 0)
      {
         m_requestData.write('\0');
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Web API request data: %hs"), m_requestData.buffer());
      }
   }

   int invokeHandler()
   {
      return m_handler(this);
   }

   void setResponseData(const void *data, size_t size, const char *contentType)
   {
      strlcpy(m_contentType, contentType, sizeof(m_contentType));
      m_responseData.clear();
      m_responseData.write(data, size);
   }

   void setResponseData(json_t *data)
   {
      char *s = json_dumps(data, 0);
      setResponseData(s, strlen(s), "application/json");
      free(s);
   }

   void setErrorResponse(const char *reason)
   {
      json_t *response = json_object();
      json_object_set_new(response, "reason", json_string(reason));
      setResponseData(response);
      json_decref(response);
   }

   void setErrorResponse(const wchar_t *reason)
   {
      json_t *response = json_object();
      json_object_set_new(response, "reason", json_string_w(reason));
      setResponseData(response);
      json_decref(response);
   }

   void setAgentErrorResponse(uint32_t agentErrorCode);

   void setResponseHeader(const TCHAR *header, const TCHAR *content)
   {
      if (m_responseHeaders == nullptr)
         m_responseHeaders = new StringMap();
      m_responseHeaders->set(header, content);
   }

   virtual void writeAuditLog(const TCHAR *subsys, bool success, uint32_t objectId, const TCHAR *format, ...) const override;
   virtual void writeAuditLogWithValues(const TCHAR *subsys, bool success, uint32_t objectId, const TCHAR *oldValue, const TCHAR *newValue, char valueType, const TCHAR *format, ...)  const override;
   virtual void writeAuditLogWithValues(const TCHAR *subsys, bool success, uint32_t objectId, json_t *oldValue, json_t *newValue, const TCHAR *format, ...)  const override;
};

#endif
