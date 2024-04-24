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
** File: notificationwnd.cpp
**/

#include "nxuseragent.h"

#define MSG_WINDOW_CLASS_NAME   _T("NetXMS_UA_MsgWindow")

/**
 * Notification timeout
 */
uint32_t g_notificationTimeout = 60000;

/**
 * Notification window object
 */
class NotificationWindow
{
private:
   HWND m_hWnd;
   UserAgentNotification *m_notification;
   Button *m_closeButton;
   int m_messageHeight;
   int m_vpos;
   POINT m_closeButtonSize;

   void createMessageControl(RECT &rect);
   void paint(HDC hdc);
   void paint()
   {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(m_hWnd, &ps);
      paint(hdc);
      EndPaint(m_hWnd, &ps);
   }

public:
   NotificationWindow(const UserAgentNotification *notification);
   NotificationWindow(const NotificationWindow& src) = delete;
   ~NotificationWindow();

   bool open(int baseY);
   void close();

   LRESULT windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

   const ServerObjectKey& getNotificationId() const { return m_notification->getId(); }
   int getVerticalPosition() const { return m_vpos; }
};

/**
 * Window list
 */
static ObjectArray<NotificationWindow> s_notificationWindows(16, 16, Ownership::True);

/**
 * Constructor
 */
NotificationWindow::NotificationWindow(const UserAgentNotification *notification)
{
   m_notification = new UserAgentNotification(notification);
   m_closeButton = nullptr;
   m_hWnd = NULL;
}

/**
 * Destructor
 */
NotificationWindow::~NotificationWindow()
{
   delete m_notification;
   delete m_closeButton;
}

/**
 * Erase background of notification window
 */
static void EraseBackground(HWND hWnd, HDC hdc)
{
   RECT rect;
   GetClientRect(hWnd, &rect);
   HBRUSH brush = CreateSolidBrush(GetApplicationColor(APP_COLOR_NOTIFICATION_BACKGROUND));
   FillRect(hdc, &rect, brush);
   DeleteObject(brush);
}

/**
 * Window procedure
 */
LRESULT NotificationWindow::windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_PAINT:
         paint();
         break;
      case WM_PRINTCLIENT:
         paint((HDC)wParam);
         break;
      case WM_ERASEBKGND:
         EraseBackground(hWnd, (HDC)wParam);
         break;
      case WM_KEYDOWN:
         if (wParam == VK_ESCAPE)
            close();
         break;
      case WM_TIMER:
         if (!AnimateWindow(m_hWnd, 1500, AW_HIDE | AW_BLEND))
         {
            TCHAR buffer[1024];
            nxlog_debug(7, _T("NotificationWindow::windowProc: AnimateWindow failed: %s"), GetSystemErrorText(GetLastError(), buffer, 1024));
         }
         close();
         break;
      case NM_CLICK:
      case WM_NOTIFY:
         if ((((LPNMHDR)lParam)->code == NM_CLICK) || (((LPNMHDR)lParam)->code == NM_RETURN))
         {
            ShellExecute(m_hWnd, _T("open"), ((PNMLINK)lParam)->item.szUrl, NULL, NULL, SW_SHOWDEFAULT);
         }
         break;
      default:
         return DefWindowProc(hWnd, uMsg, wParam, lParam);
   }
   return 0;
}

/**
 * Paint message window
 */
void NotificationWindow::paint(HDC hdc)
{
   SetBkColor(hdc, GetApplicationColor(APP_COLOR_BACKGROUND));

   RECT clientArea;
   GetClientRect(m_hWnd, &clientArea);

   // Draw border
   HPEN hPen = CreatePen(PS_SOLID, BORDER_WIDTH, GetApplicationColor(APP_COLOR_BORDER));
   SelectObject(hdc, hPen);
   SelectObject(hdc, GetStockObject(NULL_BRUSH));
   Rectangle(hdc, 0, 0, clientArea.right, clientArea.bottom);
   DeleteObject(hPen);

   DrawIconEx(hdc, clientArea.left + MARGIN_WIDTH, clientArea.top + MARGIN_HEIGHT, GetApplicationIcon(),
      APP_ICON_SIZE, APP_ICON_SIZE, 0, NULL, DI_NORMAL | DI_COMPAT);
}

/**
 * Close message window
 */
void NotificationWindow::close()
{
   MarkNotificationAsRead(m_notification->getId());
   DestroyWindow(m_hWnd);
}

/**
 * Create control displaying single message
 */
void NotificationWindow::createMessageControl(RECT &rect)
{
   HWND hWnd = CreateWindowEx(0, WC_LINK,
      m_notification->getMessage(),
      WS_VISIBLE | WS_CHILD | WS_TABSTOP,
      rect.left, rect.top, rect.right - rect.left + 1, rect.bottom - rect.top + 1,
      m_hWnd, NULL, g_hInstance, NULL);
   if (hWnd == NULL)
   {
      TCHAR buffer[1024];
      nxlog_debug(1, _T("CreateWindowEx(%s) failed (%s)"), WC_LINK, GetSystemErrorText(GetLastError(), buffer, 1024));
      return;
   }

   SendMessage(hWnd, WM_SETFONT, (WPARAM)g_notificationFont, 0);

   SIZE idealSize;
   SendMessage(hWnd, LM_GETIDEALSIZE, rect.right - rect.left + 1, (LPARAM)&idealSize);
   m_messageHeight = (int)idealSize.cy;
   MoveWindow(hWnd, rect.left, rect.top, rect.right - rect.left + 1, m_messageHeight, FALSE);
   rect.bottom = rect.top + m_messageHeight - 1;
}

/**
 * Close button hanler
 */
static void CloseNotificationWindow(void *context)
{
   ((NotificationWindow*)context)->close();
}

/**
 * Open message window
 */
bool NotificationWindow::open(int baseY)
{
   HMONITOR monitor = MonitorFromPoint({ -1, -1 }, MONITOR_DEFAULTTOPRIMARY);

   MONITORINFO mi;
   mi.cbSize = sizeof(MONITORINFO);
   GetMonitorInfo(monitor, &mi);

   HDC hdc = GetDC(NULL);
   if (hdc == NULL)
   {
      nxlog_debug(5, _T("OpenMessageWindow: cannot get DC"));
      return false;
   }

   HGDIOBJ oldFont = SelectObject(hdc, g_symbolFont);
   m_closeButtonSize = GetTextExtent(hdc, _T("\x2716"));
   m_closeButtonSize.x += BUTTON_MARGIN_WIDTH * 2;
   m_closeButtonSize.y += BUTTON_MARGIN_HEIGHT * 2;

   SelectObject(hdc, g_messageFont);
   int textHeight = GetTextExtent(hdc, _T("XYZ")).y;

   SelectObject(hdc, oldFont);
   ReleaseDC(NULL, hdc);

   POINT size = { (mi.rcWork.right - mi.rcWork.left) / 3,  std::max(textHeight * 3, APP_ICON_SIZE) + MARGIN_HEIGHT * 2 };   // 3 lines of text by default
   int hpos = mi.rcWork.right - size.x - 5;
   m_vpos = ((baseY == 0x7FFFFFFF) ? mi.rcWork.bottom : baseY) - size.y - 5;
   if (m_vpos < 10)
      return false;  // No screen space available

   m_hWnd = CreateWindowEx(WS_EX_CONTROLPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST, MSG_WINDOW_CLASS_NAME,
      _T("NetXMS User Agent"),
      WS_POPUP,
      hpos, m_vpos, size.x, size.y, NULL, NULL, g_hInstance, NULL);
   SetWindowLongPtr(m_hWnd, 0, (LONG_PTR)this);

   RECT rect = { MARGIN_WIDTH + APP_ICON_SIZE + WINDOW_HORIZONTAL_SPACING, MARGIN_HEIGHT, size.x - MARGIN_WIDTH - WINDOW_HORIZONTAL_SPACING - m_closeButtonSize.x, 0 };
   createMessageControl(rect);
   if (size.y < m_messageHeight + MARGIN_HEIGHT * 2)
   {
      size.y = m_messageHeight + MARGIN_HEIGHT * 2;
      m_vpos = ((baseY == 0x7FFFFFFF) ? mi.rcWork.bottom : baseY) - size.y - 5;
      if (m_vpos < 10)  // cannot fit to screen after resize
      {
         DestroyWindow(m_hWnd);
         m_hWnd = NULL;
         return false;
      }
      else
      {
         MoveWindow(m_hWnd, hpos, m_vpos, size.x, size.y, FALSE);
      }
   }

   // Create close button
   rect.left = size.x - BORDER_WIDTH - 3 - m_closeButtonSize.x;
   rect.right = rect.left + m_closeButtonSize.x + 1;
   rect.top = BORDER_WIDTH + 1;
   rect.bottom = rect.top + m_closeButtonSize.y + 1;
   m_closeButton = new Button(m_hWnd, rect, _T("\x2716"), true, CloseNotificationWindow, this);
   m_closeButton->setBackgroundColor(GetApplicationColor(APP_COLOR_NOTIFICATION_BACKGROUND));
   m_closeButton->setForegroundColor(GetApplicationColor(APP_COLOR_NOTIFICATION_FOREGROUND));
   m_closeButton->setSelectionColor(GetApplicationColor(APP_COLOR_NOTIFICATION_SELECTED));
   m_closeButton->setHighlightColor(GetApplicationColor(APP_COLOR_NOTIFICATION_HIGHLIGHTED));

   if (!AnimateWindow(m_hWnd, 160, AW_HOR_NEGATIVE | AW_SLIDE))
   {
      TCHAR buffer[1024];
      nxlog_debug(7, _T("OpenMessageWindow: AnimateWindow failed: %s"), GetSystemErrorText(GetLastError(), buffer, 1024));
      ShowWindow(m_hWnd, SW_SHOW);
   }

   SetActiveWindow(m_hWnd);
   SetTimer(m_hWnd, 1, g_notificationTimeout, NULL);
   return true;
}

/**
 * Background brush
 */
static HBRUSH s_backgroundBrush = NULL;

/**
 * Window procedure for notification window
 */
static LRESULT CALLBACK MsgWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   if (uMsg == WM_CTLCOLORSTATIC)
   {
      HDC hdcStatic = (HDC)wParam;
      SetTextColor(hdcStatic, GetApplicationColor(APP_COLOR_NOTIFICATION_FOREGROUND));
      SetBkColor(hdcStatic, GetApplicationColor(APP_COLOR_NOTIFICATION_BACKGROUND));
      if (s_backgroundBrush == NULL)
         s_backgroundBrush = CreateSolidBrush(GetApplicationColor(APP_COLOR_NOTIFICATION_BACKGROUND));
      return (INT_PTR)s_backgroundBrush;
   }

   NotificationWindow *wnd = (NotificationWindow *)GetWindowLongPtr(hWnd, 0);
   if (wnd == nullptr)
      return DefWindowProc(hWnd, uMsg, wParam, lParam);

   if (uMsg == WM_DESTROY)
   {
      SetWindowLongPtr(hWnd, 0, 0);
      s_notificationWindows.remove(wnd);
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
   }

   return wnd->windowProc(hWnd, uMsg, wParam, lParam);
}

/**
 * Prepare notification windows
 */
bool PrepareNotificationWindows()
{
   WNDCLASS wc;
   memset(&wc, 0, sizeof(WNDCLASS));
   wc.lpfnWndProc = MsgWndProc;
   wc.hInstance = g_hInstance;
   wc.cbWndExtra = sizeof(LONG_PTR);
   wc.lpszClassName = MSG_WINDOW_CLASS_NAME;
   wc.hbrBackground = CreateSolidBrush(GetApplicationColor(APP_COLOR_NOTIFICATION_BACKGROUND));
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   if (RegisterClass(&wc) == 0)
   {
      TCHAR buffer[1024];
      nxlog_debug(1, _T("RegisterClass(%s) failed (%s)"), MSG_WINDOW_CLASS_NAME, GetSystemErrorText(GetLastError(), buffer, 1024));
      return false;
   }
   return true;
}

/**
 * Show pending notifications
 */
void ShowPendingNotifications(bool startup)
{
   if (g_reloadConfig)
   {
      nxlog_debug(3, _T("Configuration reload flag is set"));
      LoadConfig();
      UpdateDesktopWallpaper();
      if (s_backgroundBrush != NULL)
      {
         DeleteObject(s_backgroundBrush);
         s_backgroundBrush = NULL;
      }
      g_reloadConfig = false;
   }

   ObjectArray<UserAgentNotification> *notifications = GetNotificationsForDisplay(startup);
   nxlog_debug(5, _T("%d notificatons to display"), notifications->size());
   for (int i = 0; i < notifications->size(); i++)
   {
      UserAgentNotification *n = notifications->get(i);
      bool displayed = false;
      int baseY = 0x7FFFFFFF;
      for (int j = 0; j < s_notificationWindows.size(); j++)
      {
         NotificationWindow *w = s_notificationWindows.get(j);
         if (w->getNotificationId().equals(n->getId()))
         {
            displayed = true;
         }
         if (baseY > w->getVerticalPosition() - 5)
            baseY = w->getVerticalPosition() - 5;
      }
      if (displayed)
         continue;

      NotificationWindow *wnd = new NotificationWindow(n);
      if (wnd->open(baseY))
      {
         s_notificationWindows.add(wnd);
      }
      else
      {
         delete wnd;
      }
   }
   delete notifications;
}
