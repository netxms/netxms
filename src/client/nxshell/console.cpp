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
** File: console.cpp
**
**/

#include "nxshell.h"

/**
 * Interval between checks for operation cancellation while waiting for data from server, in milliseconds
 */
#define READ_POLL_INTERVAL    250

/**
 * Time to wait for server to accept console session, in milliseconds
 */
#define SESSION_OPEN_TIMEOUT  10000

/**
 * Read JSON message from WebSocket. Returns read result, *message is set only if complete message
 * with valid JSON document was received.
 */
WebSocketReadResult ReadWebSocketMessage(WebSocketClient *socket, uint32_t timeout, json_t **message)
{
   *message = nullptr;

   ByteStream data;
   WebSocketMessageType type;
   WebSocketReadResult result = socket->readMessage(&data, &type, timeout);
   if (result != WebSocketReadResult::MESSAGE)
      return result;

   json_error_t error;
   *message = json_loadb(reinterpret_cast<const char*>(data.buffer()), data.size(), 0, &error);
   return json_is_object(*message) ? WebSocketReadResult::MESSAGE : WebSocketReadResult::FAILURE;
}

/**
 * Open server debug console session
 */
bool Shell::openConsoleSession()
{
   closeConsoleSession();

   json_t *response;
   if (!m_client->call("POST", "/v1/server-console/session", nullptr, &response))
   {
      if (m_client->getHttpStatus() == 403)
         PrintError("access denied (server console access right is required)");
      else if (m_client->getHttpStatus() == 404)
         PrintError("server does not support debug console access via web API");
      else
         PrintError("%s", m_client->getErrorText());
      return false;
   }

   std::string path = json_object_get_string_utf8(response, "wsUrl", "");
   json_decref(response);

   m_consoleSocket = new WebSocketClient();
   if (path.empty() || !m_client->connectWebSocket(path.c_str(), m_consoleSocket))
   {
      PrintError("cannot connect to server console");
      closeConsoleSession();
      return false;
   }

   // Server sends "ready" when session is established
   json_t *message;
   bool ready = (ReadWebSocketMessage(m_consoleSocket, SESSION_OPEN_TIMEOUT, &message) == WebSocketReadResult::MESSAGE) &&
         !strcmp(json_object_get_string_utf8(message, "type", ""), "ready");
   json_decref(message);
   if (!ready)
   {
      PrintError("server did not accept console session");
      closeConsoleSession();
      return false;
   }
   return true;
}

/**
 * Close server debug console session
 */
void Shell::closeConsoleSession()
{
   if (m_consoleSocket == nullptr)
      return;

   m_consoleSocket->disconnect();
   delete m_consoleSocket;
   m_consoleSocket = nullptr;
}

/**
 * Execute server debug console command using request that returns complete output
 */
bool Shell::executeConsoleCommandBuffered(const std::string& command)
{
   json_t *request = json_object();
   json_object_set_new(request, "command", json_string(command.c_str()));

   json_t *response;
   bool success = m_client->call("POST", "/v1/server-console/execute", request, &response);
   json_decref(request);

   if (!success)
   {
      if (m_client->getHttpStatus() == 403)
         PrintError("access denied (server console access right is required)");
      else if (m_client->getHttpStatus() == 404)
         PrintError("server does not support debug console access via web API");
      else
         PrintError("%s", m_client->getErrorText());
      return false;
   }

   WriteOutput(json_object_get_string_utf8(response, "output", ""));
   json_decref(response);
   return true;
}

/**
 * Execute server debug console command. In interactive session output is displayed as it is
 * produced by server.
 */
bool Shell::executeConsoleCommand(const std::string& command)
{
   if (!m_interactive)
      return executeConsoleCommandBuffered(command);

   std::string name, arguments;
   SplitCommand(command, &name, &arguments);
   if ((name == "down") && !AskConfirmation("Shut down the server?"))
      return false;

   json_t *request = json_object();
   json_object_set_new(request, "type", json_string("command"));
   json_object_set_new(request, "command", json_string(command.c_str()));
   char *requestText = json_dumps(request, 0);
   json_decref(request);

   // Existing session can be closed by server, in that case new one is opened
   bool sent = (m_consoleSocket != nullptr) && m_consoleSocket->sendText(requestText);
   if (!sent)
      sent = openConsoleSession() && m_consoleSocket->sendText(requestText);
   MemFree(requestText);
   if (!sent)
   {
      if (m_mode == ShellMode::CONSOLE)
         leaveMode();
      return false;
   }

   bool success = true;
   bool sessionClosed = false;
   bool completed = false;
   while(!completed)
   {
      if (m_client->isCancelled())
      {
         // Command cannot be cancelled, so session is dropped; new one will be opened for next command
         closeConsoleSession();
         PrintStatus("Command interrupted, console session closed");
         return false;
      }

      json_t *message;
      WebSocketReadResult result = ReadWebSocketMessage(m_consoleSocket, READ_POLL_INTERVAL, &message);
      if (result == WebSocketReadResult::TIMEOUT)
         continue;

      if (result != WebSocketReadResult::MESSAGE)
      {
         json_decref(message);
         PrintError("connection to server console lost");
         success = false;
         sessionClosed = true;
         break;
      }

      const char *type = json_object_get_string_utf8(message, "type", "");
      if (!strcmp(type, "output"))
      {
         WriteOutput(json_object_get_string_utf8(message, "data", ""));
      }
      else if (!strcmp(type, "ready"))
      {
         completed = true;
      }
      else if (!strcmp(type, "error"))
      {
         // Server sends "ready" after error or closes the session
         PrintError("%s", json_object_get_string_utf8(message, "message", "command rejected by server"));
         success = false;
      }
      else if (!strcmp(type, "closed"))
      {
         if (!strcmp(json_object_get_string_utf8(message, "reason", ""), "shutdown"))
            PrintWarning("server shutdown initiated");
         sessionClosed = true;
         completed = true;
      }
      json_decref(message);
   }

   if (sessionClosed)
   {
      closeConsoleSession();
      if (m_mode == ShellMode::CONSOLE)
         leaveMode();
   }
   return success;
}
