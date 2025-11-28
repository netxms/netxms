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
** File: webapi_router.cpp
**
**/

#include "nxcore.h"
#include <netxms-webapi.h>
#include <nms_users.h>

/**
 * Route
 */
struct Route
{
   char *path;
   size_t len;
   Route *next;
   Route *sub;
   RouteHandler handlers[5];
   bool auth;
};

/**
 * Registered routes
 */
static Route *s_root = nullptr;

/**
 * Add route
 */
void RouteBuilder::build()
{
   if (nxlog_get_debug_level_tag(DEBUG_TAG_WEBAPI) >= 4)
   {
      StringBuffer methods;
      if (m_handlers[(int)Method::GET] != nullptr)
         methods.append(_T("GET "));
      if (m_handlers[(int)Method::POST] != nullptr)
         methods.append(_T("POST "));
      if (m_handlers[(int)Method::PUT] != nullptr)
         methods.append(_T("PUT "));
      if (m_handlers[(int)Method::PATCH] != nullptr)
         methods.append(_T("PATCH "));
      if (m_handlers[(int)Method::DELETE] != nullptr)
         methods.append(_T("DELETE "));
      methods.trim();
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 4, _T("Registering endpoint /%hs [%s]%s"), m_path, methods.cstr(), m_auth ? _T("") : _T(" (no auth)"));
   }

   if (s_root == nullptr)
   {
      s_root = MemAllocStruct<Route>();
   }

   if (*m_path == 0)
   {
      memcpy(s_root->handlers, m_handlers, sizeof(m_handlers));
      s_root->auth = m_auth;
      return;
   }

   Route *curr = s_root;

   char temp[1024];
   strlcpy(temp, m_path, sizeof(temp));
   char *p = temp;
   while(true)
   {
      char *s = strchr(p, '/');
      if (s == nullptr)
         break;

      *s = 0;
      Route *r;
      for(r = curr->sub; r != nullptr; r = r->next)
         if (!strcmp(r->path, p))
            break;

      if (r == nullptr)
      {
         r = MemAllocStruct<Route>();
         r->path = MemCopyStringA(p);
         r->len = strlen(p);
         r->next = curr->sub;
         curr->sub = r;
      }
      curr = r;

      p = s + 1;
   }

   Route *r;
   for(r = curr->sub; r != nullptr; r = r->next)
      if (!strcmp(r->path, p))
         break;

   if (r == nullptr)
   {
      r = MemAllocStruct<Route>();
      r->path = MemCopyStringA(p);
      r->len = strlen(p);
      r->next = curr->sub;
      curr->sub = r;
   }

   memcpy(r->handlers, m_handlers, sizeof(m_handlers));
   r->auth = m_auth;
}

/**
 * Route request. Selects appropriate route handler and checks authorization if needed.
 * Returns request context on success and NULL on failure.
 */
Context *RouteRequest(MHD_Connection *connection, const char *path, const char *method, int *responseCode)
{
   if (path[0] != '/')
   {
      *responseCode = 404;
      return nullptr;
   }

   StringMap placeholderValues;

   const char *p = &path[1];
   const char *s = p;
   Route *curr = s_root;
   while((s != nullptr) && (*s != 0))
   {
      s = strchr(p, '/');
      size_t l = (s != nullptr) ? s - p : strlen(p);

      // Check exact match first
      Route *r;
      for(r = curr->sub; r != nullptr; r = r->next)
      {
         if ((r->len == l) && (r->path[0] != ':') && !memcmp(r->path, p, l))
            break;
      }

      // Check for placeholders
      if (r == nullptr)
      {
         for(r = curr->sub; r != nullptr; r = r->next)
         {
            if (r->path[0] == ':')
            {
               wchar_t key[256], value[256];
               utf8_to_wchar(&r->path[1], -1, key, 256);
               size_t chars = utf8_to_wchar(p, l, value, 255);
               value[chars] = 0;
               placeholderValues.set(key, value);
               break;
            }
         }
      }

      if (r == nullptr)
      {
         *responseCode = 404;
         return nullptr;
      }

      curr = r;
      p = s + 1;
   }

   Method methodId = MethodFromSymbolicName(method);
   if (methodId == Method::UNKNOWN)
   {
      *responseCode = 405; // method not allowed
      return nullptr;
   }

   RouteHandler handler = curr->handlers[(int)methodId];
   if (handler == nullptr)
   {
      *responseCode = 405; // method not allowed
      return nullptr;
   }

   // Check content type if content is expected
   if ((methodId == Method::POST) || (methodId == Method::PUT) || (methodId == Method::PATCH))
   {
      const char *contentType = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_TYPE);
      if ((contentType == nullptr) || (strcmp(contentType, "application/json") && strncmp(contentType, "application/json;", 17)))
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Unsupported media type \"%hs\""), CHECK_NULL_A(contentType));
         *responseCode = 415;  // Unsupported Media Type
         return nullptr;
      }
   }

   // Check authentication
   UserAuthenticationToken token;
   uint32_t userId = INVALID_UID;
   wchar_t loginName[MAX_USER_NAME] = L"";
   uint64_t systemAccessRights = 0;
   if (curr->auth)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, L"Selected route requires authentication");

      char encodedToken[64] = "";
      MHD_get_connection_values(connection, MHD_HEADER_KIND,
         [] (void *context, MHD_ValueKind kind, const char *key, const char *value)
         {
            if (!stricmp(key, "Authorization") && !strnicmp(value, "Bearer ", 7))
            {
               strlcpy(static_cast<char*>(context), &value[7], 64);
               return MHD_NO;
            }
            return MHD_YES;
         }, encodedToken);
      if (encodedToken[0] == 0)
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, L"Authentication token not provided in request");
         *responseCode = 401;  // Unauthorized
         return nullptr;
      }

      token = UserAuthenticationToken::parseA(encodedToken);
      if (token.isNull())
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, L"Authentication token \"%hs\" provided in request has invalid format", encodedToken);
         *responseCode = 401;  // Unauthorized
         return nullptr;
      }

      if (!ValidateAuthenticationToken(token, &userId, nullptr, AUTH_TOKEN_VALIDITY_TIME))
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, L"Authentication token \"%hs\" provided in request is invalid or expired", encodedToken);
         *responseCode = 401;  // Unauthorized
         return nullptr;
      }

      uint32_t rcc;
      if (!ValidateUserId(userId, loginName, &systemAccessRights, &rcc))
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, L"Authentication token \"%hs\" provided in request is valid but associated login is not (RCC=%u)", encodedToken);
         *responseCode = 403;  // Forbidden
         return nullptr;
      }

      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, L"Authentication token provided in request successfully passed validation (userId=%u)", userId);
   }

   return new Context(connection, path, methodId, handler, token, userId, loginName, systemAccessRights, std::move(placeholderValues));
}
