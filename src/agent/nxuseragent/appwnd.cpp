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
** File: appwnd.cpp
**/

#include "nxuseragent.h"
#include <netxms-version.h>

#define APP_WINDOW_CLASS_NAME   _T("NetXMS_UA_AppWindow")

/**
 * Fonts
 */
HFONT g_menuFont = NULL;
HFONT g_messageFont = NULL;
HFONT g_notificationFont = NULL;
HFONT g_symbolFont = NULL;
HFONT g_footerFont = NULL;

/**
 * Config reload flag
 */
bool g_reloadConfig = true;

/**
 * App window handle
 */
static HWND s_hWnd = NULL;

/**
 * Close button
 */
static Button *s_closeButton = NULL;

/**
 * Saved element sizes and positions
 */
static POINT s_menuStartPos;
static POINT s_sessionInfoPosLeft;
static POINT s_sessionInfoPosRight;
static POINT s_footerStartPos;
static POINT s_closeButtonSize;
static int s_infoLineHeight = 0;

/**
 * Get size of text
 */
POINT GetTextExtent(HDC hdc, const TCHAR *text, HFONT font)
{
   HGDIOBJ oldFont = (font != NULL) ? SelectObject(hdc, font) : NULL;

   RECT rect = { 0, 0, 0, 0 };
   DrawText(hdc, text, (int)_tcslen(text), &rect, DT_CALCRECT | DT_NOPREFIX);

   if (oldFont != NULL)
      SelectObject(hdc, oldFont);

   return { rect.right, rect.bottom };
}

/**
 * Print text at given location
 */
static void PrintText(HDC hdc, int x, int y, COLORREF color, const TCHAR *text)
{
   SetTextColor(hdc, color);
   TextOut(hdc, x, y, text, (int)_tcslen(text));
}

/**
 * Draw connection indicator
 */
static void DrawConnectionIndicator(HDC hdc, int x, int y)
{
   HPEN hPen = CreatePen(PS_SOLID, 1, g_connectedToAgent ? RGB(0, 224, 0) : RGB(224, 0, 0));
   SelectObject(hdc, hPen);

   HBRUSH hBrush = CreateSolidBrush(g_connectedToAgent ? RGB(0, 128, 0) : RGB(128, 0, 0));
   SelectObject(hdc, hBrush);

   Rectangle(hdc, x, y, x + 16, y + INDICATOR_HEIGHT);

   SelectObject(hdc, GetStockObject(NULL_PEN));
   SelectObject(hdc, GetStockObject(NULL_BRUSH));

   DeleteObject(hPen);
   DeleteObject(hBrush);
}

/**
 * Paint main window
 */
static void PaintWindow(HWND hWnd, HDC hdc)
{
   SetBkColor(hdc, GetApplicationColor(APP_COLOR_BACKGROUND));

   RECT clientArea;
   GetClientRect(hWnd, &clientArea);

   // Draw border and menu separator
   HPEN hPen = CreatePen(PS_SOLID, BORDER_WIDTH, GetApplicationColor(APP_COLOR_BORDER));
   SelectObject(hdc, hPen);
   SelectObject(hdc, GetStockObject(NULL_BRUSH));
   Rectangle(hdc, 0, 0, clientArea.right, clientArea.bottom);
   MoveToEx(hdc, MENU_HIGHLIGHT_MARGIN, s_menuStartPos.y - WINDOW_VERTICAL_SPACING, NULL);
   LineTo(hdc, clientArea.right - MENU_HIGHLIGHT_MARGIN, s_menuStartPos.y - WINDOW_VERTICAL_SPACING);
   MoveToEx(hdc, MENU_HIGHLIGHT_MARGIN, s_footerStartPos.y, NULL);
   DeleteObject(hPen);
   InflateRect(&clientArea, -(MARGIN_WIDTH + BORDER_WIDTH), -BORDER_WIDTH);

   DrawIconEx(hdc, clientArea.left, clientArea.top + HEADER_HEIGHT, GetApplicationIcon(),
      APP_ICON_SIZE, APP_ICON_SIZE, 0, NULL, DI_NORMAL | DI_COMPAT);

   // Draw welcome message
   SelectObject(hdc, g_messageFont);
   SetTextColor(hdc, GetApplicationColor(APP_COLOR_FOREGROUND));
   RECT textRect = { clientArea.left + APP_ICON_SIZE + APP_ICON_SPACING, clientArea.top + HEADER_HEIGHT, clientArea.right, s_sessionInfoPosLeft.y - WINDOW_VERTICAL_SPACING };
   const TCHAR *msg = GetWelcomeMessage();
   DrawText(hdc, msg, (int)_tcslen(msg), &textRect, DT_LEFT | DT_TOP | DT_NOPREFIX);

   // Draw computer info
   UserSession session;
   GetSessionInformation(&session);

   int y = s_sessionInfoPosLeft.y;
   PrintText(hdc, s_sessionInfoPosLeft.x, y, GetApplicationColor(APP_COLOR_HIGHLIGHT), _T("User"));
   PrintText(hdc, s_sessionInfoPosRight.x, y, GetApplicationColor(APP_COLOR_HIGHLIGHT), _T("Computer"));
   y += s_infoLineHeight;
   
   PrintText(hdc, s_sessionInfoPosLeft.x, y, GetApplicationColor(APP_COLOR_FOREGROUND), session.user);
   PrintText(hdc, s_sessionInfoPosRight.x, y, GetApplicationColor(APP_COLOR_FOREGROUND), session.computer);
   y += s_infoLineHeight + WINDOW_VERTICAL_SPACING;

   PrintText(hdc, s_sessionInfoPosLeft.x, y, GetApplicationColor(APP_COLOR_HIGHLIGHT), _T("Session"));
   PrintText(hdc, s_sessionInfoPosRight.x, y, GetApplicationColor(APP_COLOR_HIGHLIGHT), _T("Client name"));
   y += s_infoLineHeight;

   PrintText(hdc, s_sessionInfoPosLeft.x, y, GetApplicationColor(APP_COLOR_FOREGROUND), session.name);
   PrintText(hdc, s_sessionInfoPosRight.x, y, GetApplicationColor(APP_COLOR_FOREGROUND), session.client);
   y += s_infoLineHeight + WINDOW_VERTICAL_SPACING;

   PrintText(hdc, s_sessionInfoPosLeft.x, y, GetApplicationColor(APP_COLOR_HIGHLIGHT), _T("IP addresses"));
   y += s_infoLineHeight;
   unique_ptr<StructArray<InetAddress>> addrList = GetAddressList();
   for (int i = 0; i < addrList->size(); i++)
   {
      TCHAR buffer[64];
      PrintText(hdc, s_sessionInfoPosLeft.x, y, GetApplicationColor(APP_COLOR_FOREGROUND), addrList->get(i)->toString(buffer));
      y += s_infoLineHeight;
   }

   // Draw footer
   DrawConnectionIndicator(hdc, s_footerStartPos.x, s_footerStartPos.y + FOOTER_HEIGHT / 2 - INDICATOR_HEIGHT / 2);
   SelectObject(hdc, g_footerFont);
   SetTextColor(hdc, GetApplicationColor(APP_COLOR_FOREGROUND));
   textRect.left = s_footerStartPos.x + 32;
   textRect.top = s_footerStartPos.y + 1;
   textRect.right = clientArea.right;
   textRect.bottom = textRect.top + FOOTER_HEIGHT - 1;
   DrawText(hdc, NETXMS_VERSION_STRING, (int)_tcslen(NETXMS_VERSION_STRING), &textRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

/**
 * Paint main window
 */
static void PaintWindow(HWND hWnd)
{
   PAINTSTRUCT ps;
   HDC hdc = BeginPaint(hWnd, &ps);
   PaintWindow(hWnd, hdc);
   EndPaint(hWnd, &ps);
}

/**
 * Process key press
 */
static void ProcessKeyPress(HWND hWnd, WPARAM key)
{
   switch(key)
   {
      case VK_ESCAPE:
         CloseApplicationWindow();
         break;
      case VK_UP:
         MoveMenuCursor(true);
         break;
      case VK_DOWN:
         MoveMenuCursor(false);
         break;
      case VK_RETURN:
         ExecuteCurrentMenuItem();
         break;
      case VK_BACK:
         ExecuteReturnToParentMenu();
         break;
   }
}

/**
 * Erase background of application window
 */
static void EraseBackground(HWND hWnd, HDC hdc)
{
   RECT rect;
   GetClientRect(hWnd, &rect);
   HBRUSH brush = CreateSolidBrush(GetApplicationColor(APP_COLOR_BACKGROUND));
   FillRect(hdc, &rect, brush);
   DeleteObject(brush);
}

/**
 * Window procedure for application window
 */
static LRESULT CALLBACK AppWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_PAINT:
         PaintWindow(hWnd);
         break;
      case WM_PRINTCLIENT:
         PaintWindow(hWnd, (HDC)wParam);
         break;
      case WM_ERASEBKGND:
         EraseBackground(hWnd, (HDC)wParam);
         return 1;
      case WM_ACTIVATEAPP:
         if (wParam)
         {
            SetFocus(hWnd);
         }
         else if (g_closeOnDeactivate)
         {
            DestroyWindow(hWnd);
            s_hWnd = NULL;
         }
         break;
      case WM_KEYDOWN:
         ProcessKeyPress(hWnd, wParam);
         break;
      case WM_DESTROY:
         delete_and_null(s_closeButton);
         s_hWnd = NULL;
         break;
      default:
         return DefWindowProc(hWnd, uMsg, wParam, lParam);
   }
   return 0;
}

/**
 * Prepare appliaction window
 */
bool PrepareApplicationWindow()
{
   WNDCLASS wc;
   memset(&wc, 0, sizeof(WNDCLASS));
   wc.lpfnWndProc = AppWndProc;
   wc.hInstance = g_hInstance;
   wc.cbWndExtra = 0;
   wc.lpszClassName = APP_WINDOW_CLASS_NAME;
   wc.hbrBackground = NULL;
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   if (RegisterClass(&wc) == 0)
   {
      TCHAR buffer[1024];
      nxlog_debug(1, _T("RegisterClass(%s) failed (%s)"), APP_WINDOW_CLASS_NAME, GetSystemErrorText(GetLastError(), buffer, 1024));
      return false;
   }

   HDC hdc = GetDC(NULL);
   if (hdc == NULL)
      return false;
   g_menuFont = CreateFont(-MulDiv(FONT_SIZE_MENU, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0, 0, 0,
      FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
      PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Calibri"));
   g_messageFont = CreateFont(-MulDiv(FONT_SIZE_MESSAGE, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0, 0, 0,
      FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
      PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Calibri"));
   g_notificationFont = CreateFont(-MulDiv(FONT_SIZE_NOTIFICATION, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0, 0, 0,
      FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
      PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Calibri"));
   g_symbolFont = CreateFont(-MulDiv(FONT_SIZE_SYMBOLS, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0, 0, 0,
      FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
      PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Segoe UI Symbol"));
   g_footerFont = CreateFont(-MulDiv(FONT_SIZE_FOOTER, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0, 0, 0,
      FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
      PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Lucida Console"));
   ReleaseDC(NULL, hdc);

   return true;
}

/**
 * Update size of single information area
 */
inline void UpdateInfoAreaSize(HDC hdc, POINT& size, const TCHAR *name, const TCHAR *value)
{
   POINT pt = GetTextExtent(hdc, name);
   if (pt.x > size.x)
      size.x = pt.x;
   size.y += pt.y;

   pt = GetTextExtent(hdc, value);
   if (pt.x > size.x)
      size.x = pt.x;
   size.y += pt.y + WINDOW_VERTICAL_SPACING;
}

/**
 * Calculate required window size
 */
static POINT CalculateRequiredSize()
{
   POINT size = CalculateMenuSize(NULL);

   HDC hdc = GetDC(NULL);
   if (hdc != NULL)
   {
      HGDIOBJ oldFont = SelectObject(hdc, g_symbolFont);

      s_closeButtonSize = GetTextExtent(hdc, _T("\x2716"));
      s_closeButtonSize.x += BUTTON_MARGIN_WIDTH * 2;
      s_closeButtonSize.y += BUTTON_MARGIN_HEIGHT * 2;

      SelectObject(hdc, g_messageFont);
      POINT pt = GetTextExtent(hdc, GetWelcomeMessage());

      pt.x += APP_ICON_SIZE + APP_ICON_SPACING * 2 + s_closeButtonSize.x; // icon size and close button size
      nxlog_debug(6, _T("Calculated welcome message box: w=%d, h=%d"), pt.x, pt.y);
      if (pt.x > size.x)
         size.x = pt.x;
      if (pt.y > APP_ICON_SIZE + WINDOW_VERTICAL_SPACING)
         size.y += pt.y - APP_ICON_SIZE - WINDOW_VERTICAL_SPACING;

      s_sessionInfoPosLeft.x = MARGIN_WIDTH + BORDER_WIDTH;
      s_sessionInfoPosLeft.y = BORDER_WIDTH + HEADER_HEIGHT + std::max(pt.y, (LONG)APP_ICON_SIZE) + WINDOW_VERTICAL_SPACING;

      // Information area
      s_infoLineHeight = GetTextExtent(hdc, _T("X")).y;

      UserSession session;
      GetSessionInformation(&session);

      POINT infoAreaSizeLeft = { 0, 0 };
      POINT infoAreaSizeRight = { 0, 0 };

      UpdateInfoAreaSize(hdc, infoAreaSizeLeft, _T("User"), session.user);
      UpdateInfoAreaSize(hdc, infoAreaSizeRight, _T("Computer"), session.computer);
      UpdateInfoAreaSize(hdc, infoAreaSizeLeft, _T("Session"), session.name);
      UpdateInfoAreaSize(hdc, infoAreaSizeRight, _T("Client"), session.client);

      POINT infoAreaSize;
      infoAreaSize.x = infoAreaSizeLeft.x + infoAreaSizeRight.x + INFO_AREA_SPACING;
      if (size.x < infoAreaSize.x)
         size.x = infoAreaSize.x;

      infoAreaSize.y = std::max(infoAreaSizeLeft.y, infoAreaSizeRight.y) + (GetAddressListSize() + 1) * s_infoLineHeight + WINDOW_VERTICAL_SPACING;

      s_sessionInfoPosRight.x = s_sessionInfoPosLeft.x + infoAreaSizeLeft.x + INFO_AREA_SPACING;
      s_sessionInfoPosRight.y = s_sessionInfoPosLeft.y;

      size.y += infoAreaSize.y + WINDOW_VERTICAL_SPACING;

      s_menuStartPos.x = BORDER_WIDTH + MENU_HIGHLIGHT_MARGIN;
      s_menuStartPos.y = s_sessionInfoPosLeft.y + infoAreaSize.y + WINDOW_VERTICAL_SPACING;

      SelectObject(hdc, oldFont);
      ReleaseDC(NULL, hdc);
   }

   size.x += BORDER_WIDTH * 2 + MARGIN_WIDTH * 2;
   size.y += BORDER_WIDTH * 2 + APP_ICON_SIZE + HEADER_HEIGHT + FOOTER_HEIGHT + WINDOW_VERTICAL_SPACING * 2;
   nxlog_debug(6, _T("Calculated app window size: w=%d, h=%d"), size.x, size.y);

   s_footerStartPos.x = BORDER_WIDTH + MARGIN_WIDTH;
   s_footerStartPos.y = size.y - BORDER_WIDTH - FOOTER_HEIGHT - 1;
   return size;
}

/**
 * Calculate application window position
 */
static POINT CalculateAppWindowPosition(MainWindowPosition position, POINT size, POINT pt, DWORD *animateFlags)
{
   HMONITOR monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);

   MONITORINFO mi;
   mi.cbSize = sizeof(MONITORINFO);
   GetMonitorInfo(monitor, &mi);

   POINT wpos;
   if (position == MainWindowPosition::AUTOMATIC)
   {
      wpos.x = (mi.rcWork.right - size.x - 5 >= pt.x) ? pt.x : mi.rcWork.right - size.x - 5;
      wpos.y = mi.rcWork.bottom - size.y - 5;
      *animateFlags = (wpos.x >= pt.x) ? AW_VER_NEGATIVE | AW_HOR_POSITIVE : AW_VER_NEGATIVE;
   }
   else
   {
      *animateFlags = (position == MainWindowPosition::MIDDLE_CENTER) ? AW_CENTER : 0;
      switch (static_cast<int>(position) & 15)
      {
         case 1:  // Left
            wpos.x = 5;
            *animateFlags = *animateFlags | AW_HOR_POSITIVE;
            break;
         case 2:  // Center
            wpos.x = mi.rcWork.right - (mi.rcWork.right - mi.rcWork.left) / 2 - size.x / 2;
            break;
         case 3:  // Right
            wpos.x = mi.rcWork.right - size.x - 5;
            *animateFlags = *animateFlags | AW_HOR_NEGATIVE;
            break;
      }
      switch (static_cast<int>(position) >> 4)
      {
         case 1:  // Top
            wpos.y = 5;
            *animateFlags = *animateFlags | AW_VER_POSITIVE;
            break;
         case 2:  // Middle
            wpos.y = mi.rcWork.bottom - (mi.rcWork.bottom - mi.rcWork.top) / 2 - size.y / 2;
            break;
         case 3:  // Bottom
            wpos.y = mi.rcWork.bottom - size.y - 5;
            *animateFlags = *animateFlags | AW_VER_NEGATIVE;
            break;
      }
   }
   return wpos;
}

/**
 * Open application window
 */
void OpenApplicationWindow(POINT pt, bool hotkeyOpen)
{
   if (s_hWnd != NULL)
   {
      ShowWindow(s_hWnd, SW_SHOW);
      SetActiveWindow(s_hWnd);
      return;
   }

   if (g_reloadConfig)
   {
      nxlog_debug(3, _T("Configuration reload flag is set"));
      LoadConfig();
      UpdateDesktopWallpaper();
      g_reloadConfig = false;
   }

   POINT size = CalculateRequiredSize();
   DWORD animateFlags;
   POINT pos = CalculateAppWindowPosition((hotkeyOpen && (g_mainWindowPosition == MainWindowPosition::AUTOMATIC)) ? MainWindowPosition::MIDDLE_CENTER : g_mainWindowPosition, size, pt, &animateFlags);
   s_hWnd = CreateWindowEx(WS_EX_CONTROLPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST, APP_WINDOW_CLASS_NAME,
      _T("NetXMS User Agent"),
      WS_POPUP,
      pos.x, pos.y, size.x, size.y, NULL, NULL, g_hInstance, NULL);

   // Create close button. It will be destroyed by WM_DESTROY handler of main window
   RECT rect;
   rect.left = size.x - BORDER_WIDTH - 3 - s_closeButtonSize.x;
   rect.right = rect.left + s_closeButtonSize.x + 1;
   rect.top = BORDER_WIDTH + 1;
   rect.bottom = rect.top + s_closeButtonSize.y + 1;
   s_closeButton = new Button(s_hWnd, rect, _T("\x2716"), true, CloseApplicationWindow);

   ActivateMenuItem(s_hWnd, NULL);
   if (!AnimateWindow(s_hWnd, 160, animateFlags))
   {
      TCHAR buffer[1024];
      nxlog_debug(7, _T("OpenApplicationWindow(%d,%d): AnimateWindow failed: %s"), pt.x, pt.y, GetSystemErrorText(GetLastError(), buffer, 1024));
      ShowWindow(s_hWnd, SW_SHOW);
   }
   SetActiveWindow(s_hWnd);
}

/**
 * Close application window
 */
void CloseApplicationWindow(void *context)
{
   ResetMenuCursor();
   if (s_hWnd != NULL)
   {
      DestroyWindow(s_hWnd);
      s_hWnd = NULL;
   }
}

/**
 * Get start position for menu elements
 */
POINT GetMenuPosition()
{
   return s_menuStartPos;
}
