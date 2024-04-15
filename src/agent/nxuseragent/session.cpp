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
** File: session.cpp
**/

#include "nxuseragent.h"

#define EVENT_HANDLER_WINDOW_CLASS_NAME   _T("NetXMS_UA_EventHandler")

/**
 * Current session information
 */
static UserSession s_session;
Mutex s_sessionLock;

/**
 * Update user session information
 */
static void UpdateSessionInformation()
{
   UserSession session;
   memset(&session, 0, sizeof(UserSession));
   session.state = USER_SESSION_OTHER;

   ProcessIdToSessionId(GetCurrentProcessId(), &session.sid);

   DWORD size;
   WTS_CONNECTSTATE_CLASS *state;
   if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, session.sid, WTSConnectState, (LPTSTR *)&state, &size))
   {
      switch (*state)
      {
         case WTSActive:
            session.state = USER_SESSION_ACTIVE;
            break;
         case WTSConnected:
            session.state = USER_SESSION_CONNECTED;
            break;
         case WTSDisconnected:
            session.state = USER_SESSION_DISCONNECTED;
            break;
         case WTSIdle:
            session.state = USER_SESSION_IDLE;
            break;
      }
      WTSFreeMemory(state);
   }
   else
   {
      TCHAR buffer[1024];
      nxlog_debug(5, _T("WTSQuerySessionInformation(WTSConnectState) failed (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
   }

   TCHAR *value;
   if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, session.sid, WTSWinStationName, &value, &size))
   {
      if (*value != 0)
      {
         _tcslcpy(session.name, value, 256);
      }
      else
      {
         _sntprintf(session.name, 256, _T("%s-%d"), (session.state == USER_SESSION_DISCONNECTED) ? _T("Disconnected") : ((session.state == USER_SESSION_IDLE) ? _T("Idle") : _T("Session")), session.sid);
      }
      WTSFreeMemory(value);
   }
   else
   {
      TCHAR buffer[1024];
      nxlog_debug(5, _T("WTSQuerySessionInformation(WTSWinStationName) failed (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
      _tcscpy(session.name, _T("Console")); // assume console if session name cannot be read
   }

   size = 256;
   GetUserNameEx(NameSamCompatible, session.user, &size);

   size = 256;
   if (!GetComputerObjectName(NameSamCompatible, session.computer, &size))
   {
      size = 256;
      GetComputerName(session.computer, &size);
   }

   if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, session.sid, WTSClientName, &value, &size))
   {
      if (*value != 0)
      {
         _tcslcpy(session.client, value, 256);
      }
      else
      {
         _tcscpy(session.client, session.computer); // assume local host if client name cannot be read
      }
      WTSFreeMemory(value);
   }
   else
   {
      TCHAR buffer[1024];
      nxlog_debug(5, _T("WTSQuerySessionInformation(WTSClientName) failed (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
      _tcscpy(session.client, session.computer); // assume local host if client name cannot be read
   }

   s_sessionLock.lock();
   memcpy(&s_session, &session, sizeof(UserSession));
   s_sessionLock.unlock();
}

/**
 * Window proc for event handling window
 */
static LRESULT CALLBACK EventHandlerWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static const TCHAR *eventNames[] = {
      _T("WTS_CONSOLE_CONNECT"), _T("WTS_CONSOLE_DISCONNECT"), _T("WTS_REMOTE_CONNECT"),
      _T("WTS_REMOTE_DISCONNECT"), _T("WTS_SESSION_LOGON"), _T("WTS_SESSION_LOGOFF"),
      _T("WTS_SESSION_LOCK"), _T("WTS_SESSION_UNLOCK"), _T("WTS_SESSION_REMOTE_CONTROL"),
      _T("WTS_SESSION_CREATE"), _T("WTS_SESSION_TERMINATE")
   };
   switch (uMsg)
   {
      case WM_WTSSESSION_CHANGE:
         nxlog_debug(3, _T("User session change (%s)"), 
            ((wParam >= 1) && (wParam <= 11)) ? eventNames[wParam - 1] : _T("UNKNOWN"));
         if ((wParam == WTS_CONSOLE_CONNECT) || (wParam == WTS_CONSOLE_DISCONNECT) ||
             (wParam == WTS_REMOTE_CONNECT) || (wParam == WTS_REMOTE_DISCONNECT))
         {
            UpdateSessionInformation();
            SendLoginMessage();
         }
         break;
      default:
         return DefWindowProc(hWnd, uMsg, wParam, lParam);
   }
   return 0;
}

/**
 * Setup user session event handler
 */
bool SetupSessionEventHandler()
{
   UpdateSessionInformation();

   UserSession session;
   GetSessionInformation(&session);
   nxlog_write(NXLOG_INFO, _T("Running in session %s as %s"), session.name, session.user);

   WNDCLASS wc;
   memset(&wc, 0, sizeof(WNDCLASS));
   wc.lpfnWndProc = EventHandlerWndProc;
   wc.hInstance = g_hInstance;
   wc.cbWndExtra = 0;
   wc.lpszClassName = EVENT_HANDLER_WINDOW_CLASS_NAME;
   if (RegisterClass(&wc) == 0)
   {
      TCHAR buffer[1024];
      nxlog_debug(1, _T("RegisterClass(%s) failed (%s)"), EVENT_HANDLER_WINDOW_CLASS_NAME, GetSystemErrorText(GetLastError(), buffer, 1024));
      return false;
   }

   HWND hWnd = CreateWindow(EVENT_HANDLER_WINDOW_CLASS_NAME, _T("NetXMS User Agent"), 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, g_hInstance, NULL);
   if (hWnd == NULL)
   {
      TCHAR buffer[1024];
      nxlog_debug(1, _T("Cannot create event handler window (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
      return THREAD_OK;
   }

   WTSRegisterSessionNotification(hWnd, NOTIFY_FOR_THIS_SESSION);
   return true;
}

/**
 * Get user session information
 */
void GetSessionInformation(UserSession *buffer)
{
   s_sessionLock.lock();
   memcpy(buffer, &s_session, sizeof(UserSession));
   s_sessionLock.unlock();
}
