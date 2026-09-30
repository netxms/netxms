/*
** NetXMS - Network Management System
** NetXMS shell
** Copyright (C) 2025-2026 Raden Solutions
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
** File: script.cpp
**
**/

#include "nxshell.h"

/**
 * Interval between checks for operation cancellation while waiting for data from server, in milliseconds
 */
#define READ_POLL_INTERVAL    250

/**
 * Time to wait for script to stop after stop request, in milliseconds
 */
#define STOP_TIMEOUT          5000

/**
 * Print script result. Nothing is printed for null; arrays and hash maps are printed as JSON.
 */
static void PrintScriptResult(json_t *result)
{
   if ((result == nullptr) || json_is_null(result))
      return;

   std::string text;
   if (json_is_string(result))
   {
      text = json_string_value(result);
   }
   else
   {
      char *encoded = json_dumps(result, JSON_INDENT(2) | JSON_ENCODE_ANY);
      if (encoded == nullptr)
         return;
      text = encoded;
      MemFree(encoded);
   }
   text.append("\n");
   WriteOutput(text.c_str());
}

/**
 * Print NXSL error described by diagnostic document
 */
static void PrintScriptDiagnostic(const char *aliasName, const char *reason, json_t *diagnostic)
{
   // Compilation diagnostic has error information in nested object
   json_t *error = json_object_get(diagnostic, "error");
   if (!json_is_object(error))
      error = diagnostic;

   std::string text;
   if (aliasName != nullptr)
      text.append("alias \"").append(aliasName).append("\": ");
   text.append(reason);

   const char *message = json_object_get_string_utf8(error, "message", nullptr);
   if ((message != nullptr) && (*message != 0))
   {
      int lineNumber = json_object_get_int32(error, "lineNumber", 0);
      if (lineNumber > 0)
         text.append(" (line ").append(std::to_string(lineNumber)).append(")");
      text.append(": ").append(message);
   }
   PrintError("%s", text.c_str());
}

/**
 * Report failure of script execution request
 */
void Shell::reportScriptError(const char *aliasName)
{
   json_t *diagnostic = json_object_get(m_client->getErrorDocument(), "diagnostic");
   if (json_is_object(diagnostic))
   {
      PrintScriptDiagnostic(aliasName, m_client->getErrorText(), diagnostic);
      return;
   }

   switch(m_client->getHttpStatus())
   {
      case 403:
         if (m_location.empty())
            PrintError("access denied (script execution without current object requires access right to manage scripts; use \"cd\" to select an object)");
         else
            PrintError("access denied (no access right to execute scripts on %s)", m_location.back().name.c_str());
         break;
      case 404:
         if (m_location.empty())
         {
            PrintError("server does not support script execution without current object");
         }
         else
         {
            PrintError("object %s [%u] does not exist", m_location.back().name.c_str(), m_location.back().id);
            validateLocation();
         }
         break;
      default:
         PrintError("%s", m_client->getErrorText());
         break;
   }
}

/**
 * Execute NXSL script in context of current object (without object context if there is no current
 * object). Script output is displayed as it is produced, followed by script result.
 */
bool Shell::executeScript(const std::string& source, const std::vector<std::string>& parameters, const char *aliasName)
{
   json_t *request = json_object();
   json_object_set_new(request, "script", json_string(source.c_str()));
   json_object_set_new(request, "stream", json_true());
   json_t *parameterList = json_array();
   for(size_t i = 0; i < parameters.size(); i++)
      json_array_append_new(parameterList, json_string(parameters[i].c_str()));
   json_object_set_new(request, "parameters", parameterList);

   char path[64];
   if (!m_location.empty())
      snprintf(path, sizeof(path), "/v1/objects/%u/execute-script", m_location.back().id);
   else
      strcpy(path, "/v1/execute-script");

   json_t *response;
   bool success = m_client->call("POST", path, request, &response);
   json_decref(request);

   if (!success)
   {
      reportScriptError(aliasName);
      return false;
   }

   std::string outputPath = json_object_get_string_utf8(response, "wsUrl", "");
   if (outputPath.empty())
   {
      // Server without support for output streaming executes script immediately
      PrintScriptResult(json_object_get(response, "result"));
      json_decref(response);
      return true;
   }
   json_decref(response);

   WebSocketClient socket;
   if (!m_client->connectWebSocket(outputPath.c_str(), &socket))
   {
      PrintError("cannot connect to script output stream");
      return false;
   }

   success = false;
   bool completed = false;
   bool lineStart = true;
   int64_t stopRequestTime = 0;
   json_t *result = nullptr;
   while(!completed)
   {
      if (m_client->isCancelled())
      {
         if (stopRequestTime == 0)
         {
            socket.sendText("{\"type\":\"stop\"}");
            stopRequestTime = GetCurrentTimeMs();
         }
         else if (GetCurrentTimeMs() - stopRequestTime > STOP_TIMEOUT)
         {
            PrintError("script did not stop on request, connection closed");
            break;
         }
      }

      json_t *message;
      WebSocketReadResult readResult = ReadWebSocketMessage(&socket, READ_POLL_INTERVAL, &message);
      if (readResult == WebSocketReadResult::TIMEOUT)
         continue;

      if (readResult != WebSocketReadResult::MESSAGE)
      {
         json_decref(message);
         PrintError("connection to script output stream lost");
         break;
      }

      const char *type = json_object_get_string_utf8(message, "type", "");
      if (!strcmp(type, "output"))
      {
         const char *data = json_object_get_string_utf8(message, "data", "");
         if (*data != 0)
         {
            WriteOutput(data);
            lineStart = (data[strlen(data) - 1] == '\n');
         }
      }
      else if (!strcmp(type, "result"))
      {
         json_decref(result);
         result = json_incref(json_object_get(message, "data"));
      }
      else
      {
         if (!lineStart)
            WriteOutput("\n");

         if (!strcmp(type, "completed"))
         {
            PrintScriptResult(result);
            success = true;
         }
         else if (!strcmp(type, "stopped"))
         {
            PrintStatus("Script stopped");
         }
         else
         {
            PrintScriptDiagnostic(aliasName, json_object_get_string_utf8(message, "message", "Script execution failed"), json_object_get(message, "diagnostic"));
         }
         completed = true;
      }
      json_decref(message);
   }

   json_decref(result);
   socket.disconnect();
   return success;
}
