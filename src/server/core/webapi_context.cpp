/*
** NetXMS - Network Management System
** Copyright (C) 2023 Raden Solutions
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
** File: webapi_context.cpp
**
**/

#include "nxcore.h"
#include <netxms-webapi.h>

/**
 * Get request JSON document.
 */
json_t *Context::getRequestDocument()
{
   if (!hasRequestData())
      return nullptr;
   if (m_requestDocument == nullptr)
   {
      json_error_t error;
      m_requestDocument = json_loads(getRequestData(), 0, &error);
      if (m_requestDocument == nullptr)
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Cannot parse request document (%hs)"), error.text);
   }
   return m_requestDocument;
}

/**
 * Get query parameter as time value
 */
time_t Context::getQueryParameterAsTime(const char *name, time_t defaultValue) const
{
   const char *v = getQueryParameter(name);
   if (v == nullptr)
      return defaultValue;

   char *eptr;
   int64_t n = strtoll(v, &eptr, 10);
   if (*eptr == 0)
      return static_cast<time_t>(n);   // Assume UNIX timestamp

   struct tm t;
   if (strptime(v, "%Y-%m-%dT%H:%M:%SZ", &t) == nullptr)
      return defaultValue;

   return timegm(&t);
}

/**
 * Set error response from agent error
 */
void Context::setAgentErrorResponse(uint32_t agentErrorCode)
{
   json_t *response = json_object();
   json_object_set_new(response, "reason", json_string("Agent error"));
   json_object_set_new(response, "agentErrorCode", json_integer(agentErrorCode));
   json_object_set_new(response, "agentErrorText", json_string_t(AgentErrorCodeToText(agentErrorCode)));
   setResponseData(response);
   json_decref(response);
}

/**
 * Write audit log
 */
void Context::writeAuditLog(const TCHAR *subsys, bool success, uint32_t objectId, const TCHAR *format, ...) const
{
   va_list args;
   va_start(args, format);
   WriteAuditLog2(subsys, success, m_userId, m_workstation, 0, objectId, format, args);
   va_end(args);
}

/**
 * Write audit log with old and new values for changed entity
 */
void Context::writeAuditLogWithValues(const TCHAR *subsys, bool success, uint32_t objectId, const TCHAR *oldValue, const TCHAR *newValue, char valueType, const TCHAR *format, ...) const
{
   va_list args;
   va_start(args, format);
   WriteAuditLogWithValues2(subsys, success, m_userId, m_workstation, 0, objectId, oldValue, newValue, valueType, format, args);
   va_end(args);
}

/**
 * Write audit log with old and new values for changed entity
 */
void Context::writeAuditLogWithValues(const TCHAR *subsys, bool success, uint32_t objectId, json_t *oldValue, json_t *newValue, const TCHAR *format, ...) const
{
   va_list args;
   va_start(args, format);
   WriteAuditLogWithJsonValues2(subsys, success, m_userId, m_workstation, 0, objectId, oldValue, newValue, format, args);
   va_end(args);
}
