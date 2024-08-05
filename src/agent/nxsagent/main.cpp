/*
** NetXMS Session Agent
** Copyright (C) 2003-2024 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: main.cpp
**
**/

#include "nxsagent.h"
#include <netxms_getopt.h>
#include <netxms-version.h>
#include <shlobj.h>
#include <shellapi.h>

#ifdef _WIN32
#include <Psapi.h>
#endif

NETXMS_EXECUTABLE_HEADER(nxsagent)

/**
 * Connection port
 */
static UINT16 s_port = 28180;

/**
 * Socket
 */
static SOCKET s_socket = INVALID_SOCKET;

/**
 * Socket lock
 */
static Mutex s_socketLock;

/**
 * "Hide console" flag
 */
static bool s_hideConsole = true;

/**
 * Initialize logging
 */
static void InitLogging()
{
   TCHAR path[MAX_PATH], *appDataPath;
   if (SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &appDataPath) == S_OK)
   {
      _tcslcpy(path, appDataPath, MAX_PATH);
      CoTaskMemFree(appDataPath);
   }
   else
   {
      _tcscpy(path, _T("C:"));
   }
   _tcscat(path, _T("\\nxsagent"));
   CreateDirectoryTree(path);

   _tcscat(path, _T("\\nxsagent.log"));
   nxlog_open(path, s_hideConsole ? 0 : NXLOG_PRINT_TO_STDOUT);
   nxlog_set_debug_level(9);
}

/**
 * Connect to master agent
 */
static bool ConnectToMasterAgent()
{
   nxlog_debug(1, _T("Connecting to master agent..."));
   s_socket = CreateSocket(AF_INET, SOCK_STREAM, 0);
   if (s_socket == INVALID_SOCKET)
   {
      nxlog_debug(1, _T("Call to socket() failed"));
      return false;
   }

   // Fill in address structure
   struct sockaddr_in sa;
   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   sa.sin_addr.s_addr = htonl(0x7F000001);
   sa.sin_port = htons(s_port);

   // Connect to server
	if (ConnectEx(s_socket, (struct sockaddr *)&sa, sizeof(sa), 5000) == -1)
   {
      nxlog_debug(1, _T("Cannot establish connection with master agent"));
      closesocket(s_socket);
      s_socket = INVALID_SOCKET;
      return false;
   }
   
   return true;
}

/**
 * Send message to master agent
 */
static bool SendMsg(const NXCPMessage& msg)
{
   if (s_socket == INVALID_SOCKET)
      return false;

   NXCP_MESSAGE *rawMsg = msg.serialize();
   bool success = (SendEx(s_socket, rawMsg, ntohl(rawMsg->size), 0, &s_socketLock) == ntohl(rawMsg->size));
   MemFree(rawMsg);
   return success;
}

/**
 * Send login message
 */
static void Login()
{
   NXCPMessage msg(CMD_LOGIN, 0);

   DWORD sid;
   ProcessIdToSessionId(GetCurrentProcessId(), &sid);
   msg.setField(VID_SESSION_ID, sid);
   msg.setField(VID_PROCESS_ID, GetCurrentProcessId());

   DWORD size;
   WTS_CONNECTSTATE_CLASS *state;
   int16_t sessionState = USER_SESSION_OTHER;
   if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sid, WTSConnectState, (LPTSTR *)&state, &size))
   {
      switch(*state)
      {
         case WTSActive:
            sessionState = USER_SESSION_ACTIVE;
            break;
         case WTSConnected:
            sessionState = USER_SESSION_CONNECTED;
            break;
         case WTSDisconnected:
            sessionState = USER_SESSION_DISCONNECTED;
            break;
         case WTSIdle:
            sessionState = USER_SESSION_IDLE;
            break;
      }
      WTSFreeMemory(state);
   }
   else
   {
      TCHAR buffer[1024];
      nxlog_debug(1, _T("WTSQuerySessionInformation(WTSConnectState) failed (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
   }

   msg.setField(VID_SESSION_STATE, sessionState);

   TCHAR *sessionName;
   if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sid, WTSWinStationName, &sessionName, &size))
   {
      if (*sessionName != 0)
      {
         msg.setField(VID_NAME, sessionName);
      }
      else
      {
         TCHAR buffer[256];
         _sntprintf(buffer, 256, _T("%s-%d"), (sessionState == USER_SESSION_DISCONNECTED) ? _T("Disconnected") : ((sessionState == USER_SESSION_IDLE) ? _T("Idle") : _T("Session")), sid);
         msg.setField(VID_NAME, buffer);
      }
      WTSFreeMemory(sessionName);
   }
   else
   {
      TCHAR buffer[1024];
      nxlog_debug(1, _T("WTSQuerySessionInformation(WTSWinStationName) failed (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
      msg.setField(VID_NAME, _T("Console")); // assume console if session name cannot be read
   }

   TCHAR userName[256];
   size = 256;
   if (GetUserName(userName, &size))
   {
      msg.setField(VID_USER_NAME, userName);
   }

   SendMsg(msg);
}

/**
 * Shutdown session agent
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
         GetModuleFileName(NULL, path, MAX_PATH);
         command.append(path);
         command.append(_T('"'));
         if (s_hideConsole)
            command.append(_T(" -H"));

         PROCESS_INFORMATION pi;
         STARTUPINFO si;
         memset(&si, 0, sizeof(STARTUPINFO));
         si.cb = sizeof(STARTUPINFO);

         nxlog_debug(1, _T("Starting reload helper: %s"), command.cstr());
         if (CreateProcess(NULL, command.getBuffer(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
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
   ExitProcess(0);
}

/**
 * Get screen information for current session
 */
static void GetScreenInfo(NXCPMessage *msg)
{
   DEVMODE dm;
   if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm))
   {
      msg->setField(VID_SCREEN_WIDTH, static_cast<uint32_t>(dm.dmPelsWidth));
      msg->setField(VID_SCREEN_HEIGHT, static_cast<uint32_t>(dm.dmPelsHeight));
      msg->setField(VID_SCREEN_BPP, static_cast<uint32_t>(dm.dmBitsPerPel));
   }
   else
   {
      nxlog_debug(5, _T("Call to EnumDisplaySettings failed"));
   }
}

/**
 * Process request from master agent
 */
static void ProcessRequest(NXCPMessage *request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());

   switch(request->getCode())
   {
      case CMD_KEEPALIVE:
         response.setField(VID_RCC, ERR_SUCCESS);
         break;
      case CMD_TAKE_SCREENSHOT:
         TakeScreenshot(&response);
         break;
      case CMD_GET_SCREEN_INFO:
         GetScreenInfo(&response);
         break;
      case CMD_SHUTDOWN:
         ShutdownAgent(request->getFieldAsBoolean(VID_RESTART));
         break;
      default:
         response.setField(VID_RCC, ERR_UNKNOWN_COMMAND);
         break;
   }

   SendMsg(response);
}

/**
 * Message processing loop
 */
static void ProcessMessages()
{
   SocketMessageReceiver receiver(s_socket, 8192, 4 * 1024 * 1024);
   while(true)
   {
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(900000, &result);

      if ((result == MSGRECV_CLOSED) || (result == MSGRECV_COMM_FAILURE) || (result == MSGRECV_TIMEOUT))
      {
         nxlog_debug(1, _T("Error receiving message (%s)"), AbstractMessageReceiver::resultToText(result));
         break;
      }

      if (msg == nullptr)
         continue;   // Ignore other errors

      TCHAR msgCodeName[256];
      nxlog_debug(5, _T("Received message %s"), NXCPMessageCodeName(msg->getCode(), msgCodeName));
      ProcessRequest(msg);
      delete msg;
   }
}

#ifdef _WIN32

/**
 * Window proc for event handling window
 */
static LRESULT CALLBACK EventHandlerWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
      case WM_WTSSESSION_CHANGE:
         nxlog_debug(4, _T(">> session change: %d"), (int)wParam);
         if ((wParam == WTS_CONSOLE_CONNECT) || (wParam == WTS_CONSOLE_DISCONNECT) ||
             (wParam == WTS_REMOTE_CONNECT) || (wParam == WTS_REMOTE_DISCONNECT))
         {
            Login();
         }
         break;
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

/**
 * Event handling thread
 */
static void EventHandler()
{
   HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(nullptr);

	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc = EventHandlerWndProc;
	wc.hInstance = hInstance;
	wc.cbWndExtra = 0;
	wc.lpszClassName = _T("NetXMS_SessionAgent_Wnd");
	if (RegisterClass(&wc) == 0)
   {
      nxlog_debug(1, _T("Call to RegisterClass() failed"));
      return;
   }
   nxlog_debug(1, _T("Event handler window class registered"));

	HWND hWnd = CreateWindow(_T("NetXMS_SessionAgent_Wnd"), _T("NetXMS Session Agent"), 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, nullptr);
	if (hWnd == nullptr)
	{
      nxlog_debug(1, _T("Cannot create window: %s"), GetSystemErrorText(GetLastError(), nullptr, 0));
		return;
	}
   nxlog_debug(1, _T("Event handler window created"));

   WTSRegisterSessionNotification(hWnd, NOTIFY_FOR_THIS_SESSION);
   nxlog_debug(1, _T("Event loop started"));

   MSG msg;
	while(GetMessage(&msg, nullptr, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

/**
 * Check if user agent already running in this session
 */
static void CheckIfRunning()
{
   _tprintf(_T("Checking if session agent instance is already running\n"));

   DWORD sessionId;
   if (!ProcessIdToSessionId(GetCurrentProcessId(), &sessionId))
      return;

   TCHAR name[256];
   if (GetModuleBaseName(GetCurrentProcess(), NULL, name, 256) == 0)
      return;

   if (!CheckProcessPresenseInSession(sessionId, name))
      return;  // Not running

   _tprintf(_T("Another instance already running, exiting\n"));
   ExitProcess(0);
}

#endif

/**
 * Entry point
 */
#ifdef _WIN32
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
   InitNetXMSProcess(false);

   int argc;
   WCHAR **argv = CommandLineToArgvW(GetCommandLineW(), &argc);

#else
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

#endif

   int ch;
   while((ch = getoptW(argc, argv, "fHv")) != -1)
   {
		switch(ch)
		{
         case 'f':   // run in foreground
            s_hideConsole = false;
            break;
         case 'H':   // hide console (for compatibility)
            s_hideConsole = true;
            break;
		   case 'v':   // version
            _tprintf(_T("NetXMS Session Agent Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_BUILD_TAG _T("\n"));
            exit(0);
			   break;
		   case '?':
			   return 3;
		}
   }

#ifdef _WIN32
   CheckIfRunning();
   if (!s_hideConsole)
      AllocConsole();
#endif

   InitLogging();
   nxlog_write(NXLOG_INFO, _T("NetXMS Session Agent Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_BUILD_TAG);

#ifdef _WIN32
   nxlog_debug(1, _T("Hide console set to %s"), BooleanToString(s_hideConsole));

   WSADATA wsaData;
	int wrc = WSAStartup(MAKEWORD(2, 2), &wsaData);
   if (wrc != 0)
   {
      nxlog_debug(1, _T("WSAStartup() failed"));
      return 1;
   }
   nxlog_debug(1, _T("WSAStartup() completed"));

   auto __SetProcessDpiAwarenessContext = reinterpret_cast<BOOL (WINAPI *)(DPI_AWARENESS_CONTEXT)>(GetProcAddress(GetModuleHandle(_T("user32.dll")), "SetProcessDpiAwarenessContext"));
   if (__SetProcessDpiAwarenessContext != nullptr)
   {
      nxlog_debug(1, _T("SetProcessDpiAwarenessContext is available"));
      __SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
   }
   else
   {
      auto __SetProcessDPIAware = reinterpret_cast<BOOL(WINAPI *)()>(GetProcAddress(GetModuleHandle(_T("user32.dll")), "SetProcessDPIAware"));
      if (__SetProcessDPIAware != nullptr)
      {
         nxlog_debug(1, _T("SetProcessDPIAware is available"));
         __SetProcessDPIAware();
      }
      else
      {
         nxlog_debug(1, _T("Neither SetProcessDpiAwarenessContext nor SetProcessDPIAware are available"));
      }
   }

   ThreadCreate(EventHandler);
   nxlog_debug(1, _T("Event handler started"));
#endif

   while(true)
   {
      if (!ConnectToMasterAgent())
      {
         ThreadSleep(30);
         continue;
      }

      nxlog_debug(1, _T("*** Connected to master agent ***"));
      Login();
      ProcessMessages();
   }
   return 0;
}
