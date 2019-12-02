#include "nxuseragent.h"

#define MSG_WINDOW_CLASS_NAME   _T("NetXMS_UA_MsgWindow")

/**
 * Message window handle
 */
static HWND s_hWnd = NULL;

/**
 * Saved element sizes and positions
 */
static int s_headerSeparatorPos;

/**
 * Background brush
 */
static HBRUSH s_backgroundBrush = NULL;

/**
 * Paint message window
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
   MoveToEx(hdc, MENU_HIGHLIGHT_MARGIN, s_headerSeparatorPos, NULL);
   LineTo(hdc, clientArea.right - MENU_HIGHLIGHT_MARGIN, s_headerSeparatorPos);
   DeleteObject(hPen);
}

/**
* Paint message window
*/
static void PaintWindow(HWND hWnd)
{
   PAINTSTRUCT ps;
   HDC hdc = BeginPaint(hWnd, &ps);
   PaintWindow(hWnd, hdc);
   EndPaint(hWnd, &ps);
}

/**
 * Window procedure for message window
 */
static LRESULT CALLBACK MsgWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_PAINT:
         PaintWindow(hWnd);
         break;
      case WM_PRINTCLIENT:
         PaintWindow(hWnd, (HDC)wParam);
         break;
      case WM_CTLCOLORSTATIC:
      {
         HDC hdcStatic = (HDC)wParam;
         SetTextColor(hdcStatic, GetApplicationColor(APP_COLOR_FOREGROUND));
         SetBkColor(hdcStatic, GetApplicationColor(APP_COLOR_BACKGROUND));
         if (s_backgroundBrush == NULL)
            s_backgroundBrush = CreateSolidBrush(GetApplicationColor(APP_COLOR_BACKGROUND));
         return (INT_PTR)s_backgroundBrush;
      }
      default:
         return DefWindowProc(hWnd, uMsg, wParam, lParam);
   }
   return 0;
}

/**
 * Prepare message window
 */
bool PrepareMessageWindow()
{
   WNDCLASS wc;
   memset(&wc, 0, sizeof(WNDCLASS));
   wc.lpfnWndProc = MsgWndProc;
   wc.hInstance = g_hInstance;
   wc.cbWndExtra = 0;
   wc.lpszClassName = MSG_WINDOW_CLASS_NAME;
   wc.hbrBackground = CreateSolidBrush(GetApplicationColor(APP_COLOR_BACKGROUND));
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
 * Create control displaying single message
 */
static void CreateMessageControl(HWND parent, RECT &rect, UserAgentNotification *n)
{
   HWND hWnd = CreateWindowEx(0, WC_LINK,
      n->getMessage(),
      WS_VISIBLE | WS_CHILD | WS_TABSTOP,
      rect.left, rect.top, rect.right - rect.left + 1, rect.bottom - rect.top + 1,
      parent, NULL, g_hInstance, NULL);
   if (hWnd == NULL)
   {
      TCHAR buffer[1024];
      nxlog_debug(1, _T("CreateWindowEx(%s) failed (%s)"), WC_LINK, GetSystemErrorText(GetLastError(), buffer, 1024));
      return;
   }

   SendMessage(hWnd, WM_SETFONT, (WPARAM)g_messageFont, 0);

   int height = (int)SendMessage(hWnd, LM_GETIDEALHEIGHT, 0, 0);
   MoveWindow(hWnd, rect.left, rect.top, rect.right - rect.left + 1, height, FALSE);
   rect.bottom = rect.top + height - 1;
}

/**
 * Open message window
 */
void OpenMessageWindow()
{
   if (s_hWnd != NULL)
   {
      ShowWindow(s_hWnd, SW_SHOW);
      SetActiveWindow(s_hWnd);
      return;
   }

   ObjectArray<UserAgentNotification> *notifications = GetNotificationsForDisplay();
   if (notifications->isEmpty())
   {
      nxlog_debug(5, _T("OpenMessageWindow: no notifications to display"));
      delete notifications;
      return;
   }

   HMONITOR monitor = MonitorFromPoint({ -1, -1 }, MONITOR_DEFAULTTOPRIMARY);

   MONITORINFO mi;
   mi.cbSize = sizeof(MONITORINFO);
   GetMonitorInfo(monitor, &mi);

   POINT size = { (mi.rcWork.right - mi.rcWork.left) * 2 / 3,  (mi.rcWork.bottom - mi.rcWork.top) * 2 / 3 };
   int hpos = mi.rcWork.right - (mi.rcWork.right - mi.rcWork.left) / 2 - size.x / 2;
   int vpos = mi.rcWork.bottom - (mi.rcWork.bottom - mi.rcWork.top) / 2 - size.y / 2;

   HDC hdc = GetDC(NULL);
   if (hdc == NULL)
   {
      nxlog_debug(5, _T("OpenMessageWindow: cannot get DC"));
      delete notifications;
      return;
   }

   HGDIOBJ oldFont = SelectObject(hdc, g_messageFont);
   s_headerSeparatorPos = GetTextExtent(hdc, _T("XYZ")).y + WINDOW_VERTICAL_SPACING;

   s_hWnd = CreateWindowEx(WS_EX_CONTROLPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST, MSG_WINDOW_CLASS_NAME,
      _T("NetXMS User Agent"),
      WS_POPUP,
      hpos, vpos, size.x, size.y, NULL, NULL, g_hInstance, NULL);

   RECT rect = { MARGIN_WIDTH, s_headerSeparatorPos + WINDOW_VERTICAL_SPACING, size.x - MARGIN_WIDTH * 2, 0 };
   for (int i = 0; i < notifications->size(); i++)
   {
      CreateMessageControl(s_hWnd, rect, notifications->get(i));
      rect.top = rect.bottom + WINDOW_VERTICAL_SPACING;
   }

   SelectObject(hdc, oldFont);
   ReleaseDC(NULL, hdc);

   ShowWindow(s_hWnd, SW_SHOW);
}
