/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: log_parsers.cpp
**
**/

#include "webapi.h"
#include <netxms-xml.h>

/**
 * Mapping of REST parser type to backing configuration CLOB variable
 */
struct LogParserType
{
   const wchar_t *type;
   const wchar_t *configKey;
};

static const LogParserType s_logParsers[] =
{
   { L"syslog",         L"SyslogParser" },
   { L"windows-event",  L"WindowsEventParser" },
   { L"opentelemetry",  L"OpenTelemetryLogParser" },
   { nullptr, nullptr }
};

/**
 * Resolve REST parser type to configuration variable name (nullptr if unknown)
 */
static const wchar_t *ResolveLogParserKey(const wchar_t *type)
{
   if (type == nullptr)
      return nullptr;
   for(int i = 0; s_logParsers[i].type != nullptr; i++)
   {
      if (!wcscmp(type, s_logParsers[i].type))
         return s_logParsers[i].configKey;
   }
   return nullptr;
}

/**
 * Handler for GET /v1/log-parsers/:parser-type
 */
int H_LogParser(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   const wchar_t *type = context->getPlaceholderValue(L"parser-type");
   const wchar_t *configKey = ResolveLogParserKey(type);
   if (configKey == nullptr)
      return 404;

   char *content = ConfigReadCLOBUTF8(configKey, "");

   json_t *output = json_object();
   json_object_set_new(output, "type", json_string_t(type));
   json_object_set_new(output, "content", json_string(content));
   context->setResponseData(output);
   json_decref(output);

   MemFree(content);
   return 200;
}

/**
 * Handler for PUT /v1/log-parsers/:parser-type
 */
int H_LogParserUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   const wchar_t *type = context->getPlaceholderValue(L"parser-type");
   const wchar_t *configKey = ResolveLogParserKey(type);
   if (configKey == nullptr)
      return 404;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonContent = json_object_get(request, "content");
   if (!json_is_string(jsonContent))
   {
      context->setErrorResponse("Missing or invalid required field (content)");
      return 400;
   }
   const char *content = json_string_value(jsonContent);

   // Validate that the supplied document is well-formed XML with a <parser> root,
   // so we never store a configuration the parser subsystem cannot load.
   if (content[0] != 0)
   {
      pugi::xml_document xml;
      if (!xml.load_buffer(content, strlen(content)))
      {
         context->setErrorResponse("Malformed parser XML");
         return 400;
      }
      if (!xml.child("parser"))
      {
         context->setErrorResponse("Parser document root element must be <parser>");
         return 400;
      }
   }

   // Writing the CLOB triggers OnConfigVariableChange, which reinitializes the live parser
   WCHAR *value = WideStringFromUTF8String(content);
   bool success = ConfigWriteCLOB(configKey, value, true);
   MemFree(value);
   if (!success)
      return 500;

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Log parser \"%s\" updated via REST API", type);
   return 204;
}
