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
** File: tray.cpp
**/

#include "nxuseragent.h"

#define TRAY_WINDOW_CLASS_NAME   _T("NetXMS_UA_TrayWindow")

/**
 * Tray window handle
 */
static HWND s_hWnd = nullptr;

/**
 * Tray icon data
 */
static NOTIFYICONDATA s_trayIcon;

/**
 * Tray icon image
 */
static HICON s_trayIconImage = nullptr;

/**
 * Show tray contet menu
 */
static void ShowContextMenu(HWND hWnd, POINT pt)
{
   HMENU hMenu = CreatePopupMenu();
   AppendMenu(hMenu, MF_BYCOMMAND | MF_STRING, 1, _T("&Open"));
   AppendMenu(hMenu, MF_BYCOMMAND | MF_STRING, 2, _T("&About..."));
   AppendMenu(hMenu, MF_BYCOMMAND | MF_STRING, 3, _T("E&xit"));

   SetForegroundWindow(hWnd);
   int command = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
   switch (command)
   {
      case 1:
         OpenApplicationWindow(pt, false);
         break;
      case 3:
         PostQuitMessage(0);
         break;
   }
   PostMessage(hWnd, WM_NULL, 0, 0);
}

/**
 * Tray notification handler
 */
static LRESULT CALLBACK TrayEventHandlerWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   if (uMsg != NXUA_MSG_TOOLTIP_NOTIFY)
      return DefWindowProc(hWnd, uMsg, wParam, lParam);

   POINT pt;
   GetCursorPos(&pt);

   switch (LOWORD(lParam))
   {
      case WM_CONTEXTMENU:
         ShowContextMenu(hWnd, pt);
         break;
      case WM_LBUTTONDBLCLK:
      case WM_LBUTTONUP:
         SetForegroundWindow(hWnd);
         OpenApplicationWindow(pt, false);
         break;
      case NIN_KEYSELECT:
         pt.x = GET_X_LPARAM(wParam);
         pt.y = GET_Y_LPARAM(wParam);
         SetForegroundWindow(hWnd);
         OpenApplicationWindow(pt, false);
         break;
      default:
         break;
   }

   return 0;
}

/**
 * Setup tray icon
 */
bool SetupTrayIcon()
{
   // Register window class for processing tray icon notifications
   WNDCLASS wc;
   memset(&wc, 0, sizeof(WNDCLASS));
   wc.lpfnWndProc = TrayEventHandlerWndProc;
   wc.hInstance = g_hInstance;
   wc.cbWndExtra = 0;
   wc.lpszClassName = TRAY_WINDOW_CLASS_NAME;
   if (RegisterClass(&wc) == 0)
      return false;

   s_hWnd = CreateWindow(TRAY_WINDOW_CLASS_NAME, _T("NetXMS User Agent"), 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, g_hInstance, nullptr);
   if (s_hWnd == nullptr)
      return false;

   memset(&s_trayIcon, 0, sizeof(NOTIFYICONDATA));
   s_trayIcon.cbSize = sizeof(NOTIFYICONDATA);
   s_trayIcon.uVersion = NOTIFYICON_VERSION_4;
   s_trayIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
   s_trayIcon.hWnd = s_hWnd;
   s_trayIcon.uCallbackMessage = NXUA_MSG_TOOLTIP_NOTIFY;
   _tcslcpy(s_trayIcon.szTip, GetTooltipMessage(), sizeof(s_trayIcon.szTip) / sizeof(TCHAR));
   s_trayIcon.hIcon = (s_trayIconImage != nullptr) ? s_trayIconImage : LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_APP));

   Shell_NotifyIcon(NIM_ADD, &s_trayIcon);
   Shell_NotifyIcon(NIM_SETVERSION, &s_trayIcon);

   return true;
}

/**
 * Remove tray icon
 */
void RemoveTrayIcon()
{
   Shell_NotifyIcon(NIM_DELETE, &s_trayIcon);
}

/**
 * Update tray icon from file
 */
void UpdateTrayIcon(const TCHAR *file)
{
   if (s_trayIconImage != nullptr)
      DestroyIcon(s_trayIconImage);
   s_trayIconImage = (HICON)LoadImage(nullptr, file, IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_LOADFROMFILE);
   if (s_trayIconImage == nullptr)
      nxlog_debug(2, _T("Cannot load tray icon from %s"), file);

   s_trayIcon.hIcon = (s_trayIconImage != nullptr) ? s_trayIconImage : LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_APP));
   Shell_NotifyIcon(NIM_MODIFY, &s_trayIcon);
}

/**
 * Reset tray icon
 */
void ResetTrayIcon()
{
   if (s_trayIconImage != nullptr)
      DestroyIcon(s_trayIconImage);
   s_trayIconImage = nullptr;
   s_trayIcon.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_APP));
   Shell_NotifyIcon(NIM_MODIFY, &s_trayIcon);
}

/**
 * Update tray tooltip
 */
void UpdateTrayTooltip()
{
   _tcslcpy(s_trayIcon.szTip, GetTooltipMessage(), sizeof(s_trayIcon.szTip) / sizeof(TCHAR));
   Shell_NotifyIcon(NIM_MODIFY, &s_trayIcon);
}

/**
 * Get tray window handle
 */
HWND GetTrayWindow()
{
   return s_hWnd;
}

/**
 * Show balloon notification
 */
void ShowTrayNotification(const TCHAR *text)
{
   NOTIFYICONDATA nid;
   nid.cbSize = sizeof(nid);
   nid.uVersion = NOTIFYICON_VERSION_4;
   nid.hWnd = s_trayIcon.hWnd;
   nid.uID = s_trayIcon.uID;
   nid.uFlags = NIF_INFO;
   nid.dwInfoFlags = NIIF_INFO;
   _tcscpy_s(nid.szInfoTitle, _T("NetXMS User Support Application"));
   _tcslcpy(nid.szInfo, text, 256);
   Shell_NotifyIcon(NIM_MODIFY, &nid);
}
