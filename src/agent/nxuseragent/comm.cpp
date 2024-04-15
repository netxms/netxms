/*
** NetXMS user agent
** Copyright (C) 2009-2024 Raden Solutions
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
** File: comm.cpp
**/

#include "nxuseragent.h"

/**
 * Connection state
 */
bool g_connectedToAgent = false;

/**
 * Connection port
 */
static uint16_t s_port = 28180;

/**
 * Socket
 */
static SOCKET s_socket = INVALID_SOCKET;

/**
 * Socket lock
 */
static Mutex s_socketLock;

/**
 * Connect to master agent
 */
static bool ConnectToMasterAgent()
{
   nxlog_debug(7, _T("Connecting to master agent"));
   s_socket = CreateSocket(AF_INET, SOCK_STREAM, 0);
   if (s_socket == INVALID_SOCKET)
   {
      nxlog_debug(5, _T("Call to socket() failed\n"));
      return false;
   }

   // Fill in address structure
   struct sockaddr_in sa;
   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   sa.sin_addr.s_addr = htonl(0x7F000001);   // 127.0.0.1
   sa.sin_port = htons(s_port);

   // Connect to server
   if (ConnectEx(s_socket, (struct sockaddr *)&sa, sizeof(sa), 5000) == -1)
   {
      nxlog_debug(5, _T("Cannot establish connection with master agent"));
      closesocket(s_socket);
      s_socket = INVALID_SOCKET;
      return false;
   }

   return true;
}

/**
 * Send message to master agent
 */
static bool SendMessageToAgent(NXCPMessage *msg)
{
   if (s_socket == INVALID_SOCKET)
      return false;

   NXCP_MESSAGE *rawMsg = msg->serialize();
   bool success = (SendEx(s_socket, rawMsg, ntohl(rawMsg->size), 0, &s_socketLock) == ntohl(rawMsg->size));
   MemFree(rawMsg);
   return success;
}

/**
 * Send login message
 */
void SendLoginMessage()
{
   UserSession session;
   GetSessionInformation(&session);

   NXCPMessage msg(CMD_LOGIN, 0);
   msg.setField(VID_SESSION_ID, session.sid);
   msg.setField(VID_SESSION_STATE, static_cast<int16_t>(session.state));
   msg.setField(VID_NAME, session.name);
   msg.setField(VID_USER_NAME, session.user);
   msg.setField(VID_CLIENT_INFO, session.client);
   msg.setField(VID_USERAGENT, true);
   msg.setField(VID_PROCESS_ID, GetCurrentProcessId());
   SendMessageToAgent(&msg);
}

/**
 * Shutdown user agent
 */
static void ShutdownAgent(bool restart)
{
   nxlog_debug(1, _T("Shutdown request with restart option %s"), restart ? _T("ON") : _T("OFF"));

   if (restart)
   {
      TCHAR path[MAX_PATH];
      GetNetXMSDirectory(nxDirBin, path);

      TCHAR exe[MAX_PATH];
      _tcscpy(exe, path);
      _tcslcat(exe, _T("\\nxreload.exe"), MAX_PATH);
      if (VerifyFileSignature(exe))
      {
         StringBuffer command;
         command.append(_T('"'));
         command.append(exe);
         command.append(_T("\" -- \""));
         GetModuleFileName(nullptr, path, MAX_PATH);
         command.append(path);
         command.append(_T('"'));

         PROCESS_INFORMATION pi;
         STARTUPINFO si;
         memset(&si, 0, sizeof(STARTUPINFO));
         si.cb = sizeof(STARTUPINFO);

         nxlog_debug(3, _T("Starting reload helper:"));
         nxlog_debug(3, _T("%s"), command.cstr());
         if (CreateProcess(nullptr, command.getBuffer(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
         {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
         }
      }
      else
      {
         nxlog_debug(1, _T("Cannot verify signature of reload helper %s"), exe);
      }
   }

   PostThreadMessage(g_mainThreadId, WM_QUIT, 0, 0);
}

/**
 * Get screen information for current session
 */
static void GetScreenInfo(NXCPMessage *msg)
{
   DEVMODE dm;
   if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm))
   {
      msg->setField(VID_SCREEN_WIDTH, dm.dmPelsWidth);
      msg->setField(VID_SCREEN_HEIGHT, dm.dmPelsHeight);
      msg->setField(VID_SCREEN_BPP, dm.dmBitsPerPel);
   }
   else
   {
      nxlog_debug(5, _T("Call to EnumDisplaySettings failed"));
   }
}

/**
 * Update environment from master agent
 */
static void UpdateEnvironment(const NXCPMessage *request)
{
   int count = request->getFieldAsInt32(VID_NUM_ELEMENTS);
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   for (int i = 0; i < count; i++)
   {
      TCHAR *name = request->getFieldAsString(fieldId++);
      TCHAR *value = request->getFieldAsString(fieldId++);
      SetEnvironmentVariable(name, value);
      nxlog_debug(4, _T("SetEnvironmentVariable: %s = %s"), name, value);
      MemFree(name);
      MemFree(value);
   }
}

/** 
 * Process request from master agent
 */
static void ProcessRequest(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   switch (request->getCode())
   {
      case CMD_KEEPALIVE:
         msg.setField(VID_RCC, ERR_SUCCESS);
         break;
      case CMD_TAKE_SCREENSHOT:
         TakeScreenshot(&msg);
         break;
      case CMD_GET_SCREEN_INFO:
         GetScreenInfo(&msg);
         break;
      case CMD_WRITE_AGENT_CONFIG_FILE:
         UpdateConfig(request);
         break;
      case CMD_UPDATE_ENVIRONMENT:
         UpdateEnvironment(request);
         break;
      case CMD_UPDATE_UA_NOTIFICATIONS:
         UpdateNotifications(request);
         break;
      case CMD_ADD_UA_NOTIFICATION:
         AddNotification(request);
         break;
      case CMD_RECALL_UA_NOTIFICATION:
         RemoveNotification(request);
         break;
      case CMD_SHUTDOWN:
         ShutdownAgent(request->getFieldAsBoolean(VID_RESTART));
         break;
      default:
         msg.setField(VID_RCC, ERR_UNKNOWN_COMMAND);
         break;
   }

   SendMessageToAgent(&msg);
}

/**
 * Message processing loop
 */
static void ProcessMessages()
{
   SocketMessageReceiver receiver(s_socket, 4096, 256 * 1024);
   while (true)
   {
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(900000, &result);

      // Check for decryption error
      if (result == MSGRECV_DECRYPTION_FAILURE)
      {
         nxlog_debug(4, _T("Unable to decrypt received message"));
         continue;
      }

      // Receive error
      if (msg == nullptr)
      {
         if (result == MSGRECV_CLOSED)
            nxlog_debug(5, _T("Connection with master agent closed"));
         else
            nxlog_debug(5, _T("Message receiving error (%s)"), AbstractMessageReceiver::resultToText(result));
         break;
      }

      if (!msg->isBinary())
      {
         TCHAR msgCodeName[256];
         nxlog_debug(5, _T("Received message %s"), NXCPMessageCodeName(msg->getCode(), msgCodeName));
         ProcessRequest(msg);
      }
      delete msg;
   }
}

/**
 * Stop flag
 */
static bool s_stop = false;

/**
 * Communication thread
 */
static void CommThread()
{
   nxlog_debug(1, _T("Communication thread started"));

   // Change DPI awareness for this thread (required for corect screenshots)
   auto __SetThreadDpiAwarenessContext = reinterpret_cast<DPI_AWARENESS_CONTEXT (WINAPI *)(DPI_AWARENESS_CONTEXT)>(GetProcAddress(GetModuleHandle(_T("user32.dll")), "SetThreadDpiAwarenessContext"));
   if (__SetThreadDpiAwarenessContext != nullptr)
      __SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
   else
      nxlog_write(NXLOG_WARNING, _T("SetThreadDpiAwarenessContext is not available"));

   while(!s_stop)
   {
      if (!ConnectToMasterAgent())
      {
         ThreadSleep(30);
         continue;
      }

      nxlog_debug(1, _T("Connected to master agent"));
      g_connectedToAgent = true;
      SendLoginMessage();
      ProcessMessages();
      g_connectedToAgent = false;
   }
   nxlog_debug(1, _T("Communication thread stopped"));
}

/**
 * Start agent connector
 */
void StartAgentConnector()
{
   ThreadCreate(CommThread);
}

/**
 * Stop agent connector
 */
void StopAgentConnector()
{
   s_stop = true;
   closesocket(s_socket);
}
