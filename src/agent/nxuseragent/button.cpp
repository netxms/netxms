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
** File: button.cpp
**/

#include "nxuseragent.h"

#define BUTTON_CLASS_NAME   _T("NetXMS_UA_Button")

/**
 * Create new button
 */
Button::Button(HWND parent, RECT pos, const TCHAR *text, bool symbol, void(*handler)(void*), void *context)
{
   m_text = MemCopyString(text);
   m_symbol = symbol;
   m_handler = handler;
   m_context = context;

   m_backgroundColor = GetApplicationColor(APP_COLOR_BACKGROUND);
   m_foregroundColor = GetApplicationColor(APP_COLOR_FOREGROUND);
   m_selectionColor = GetApplicationColor(APP_COLOR_MENU_SELECTED);
   m_highlightColor = GetApplicationColor(APP_COLOR_MENU_HIGHLIGHTED);

   m_hWnd = CreateWindow(BUTTON_CLASS_NAME, text,
      WS_CHILD, pos.left, pos.top, pos.right - pos.left + 1, pos.bottom - pos.top + 1,
      parent, NULL, g_hInstance, NULL);
   SetWindowLongPtr(m_hWnd, 0, (LONG_PTR)this);
   ShowWindow(m_hWnd, SW_SHOW);
}

/**
 * Destructor
 */
Button::~Button()
{
   DestroyWindow(m_hWnd);
   MemFree(m_text);
}

/**
 * Execute associated command
 */
void Button::execute()
{
   m_selected = false;
   m_highlighted = false;
   if (m_handler != nullptr)
      m_handler(m_context);
}

/**
 * Draw button
 */
void Button::draw(HDC hdc) const
{
   RECT rect;
   GetClientRect(m_hWnd, &rect);
   SetBkColor(hdc, m_selected ? m_selectionColor : (m_highlighted ? m_highlightColor : m_backgroundColor));
   SetTextColor(hdc, m_foregroundColor);
   SelectObject(hdc, m_symbol ? g_symbolFont : g_menuFont);

   HBRUSH brush = CreateSolidBrush(GetBkColor(hdc));
   SelectObject(hdc, brush);
   SelectObject(hdc, GetStockObject(NULL_PEN));
   Rectangle(hdc, rect.left, rect.top, rect.right + 1, rect.bottom + 1);
   SelectObject(hdc, GetStockObject(NULL_BRUSH));
   DeleteObject(brush);

   InflateRect(&rect, -BUTTON_MARGIN_WIDTH * 2, -BUTTON_MARGIN_HEIGHT * 2);
   DrawText(hdc, m_text, (int)_tcslen(m_text), &rect, DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

/**
 * Set "selected" state
 */
void Button::setSelected(bool selected)
{
   m_highlighted = false;
   m_selected = selected;
   InvalidateRect(m_hWnd, NULL, FALSE);
}

/**
 * Enable mouse tracking
 */
void Button::trackMouseEvent()
{
   TRACKMOUSEEVENT t;
   t.cbSize = sizeof(TRACKMOUSEEVENT);
   t.dwFlags = TME_HOVER | TME_LEAVE;
   t.dwHoverTime = HOVER_DEFAULT;
   t.hwndTrack = m_hWnd;
   TrackMouseEvent(&t);
}

/**
 * Process Windows messages
 */
LRESULT Button::processMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   {
      case WM_PAINT:
         {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_hWnd, &ps);
            draw(hdc);
            EndPaint(m_hWnd, &ps);
         }
         break;
      case WM_PRINTCLIENT:
         draw((HDC)wParam);
         break;
      case WM_CREATE:
         trackMouseEvent();
         break;
      case WM_DESTROY:
         m_selected = false;
         m_highlighted = false;
         break;
      case WM_LBUTTONDOWN:
         m_highlighted = false;
         m_selected = true;
         InvalidateRect(m_hWnd, NULL, FALSE);
         trackMouseEvent();
         break;
      case WM_LBUTTONUP:
         if (m_selected)
         {
            execute();
         }
         break;
      case WM_MOUSEHOVER:
         trackMouseEvent();
         if (!m_highlighted && !m_selected)
         {
            m_highlighted = true;
            InvalidateRect(m_hWnd, NULL, FALSE);
         }
         break;
      case WM_MOUSEMOVE:
         if (!m_highlighted && !m_selected)
         {
            m_highlighted = true;
            InvalidateRect(m_hWnd, NULL, FALSE);
            trackMouseEvent();
         }
         break;
      case WM_MOUSELEAVE:
         if (m_selected || m_highlighted)
         {
            m_selected = false;
            m_highlighted = false;
            InvalidateRect(m_hWnd, NULL, FALSE);
         }
         break;
      default:
         return DefWindowProc(m_hWnd, msg, wParam, lParam);
   }
   return 0;
}

/**
 * Window procedure for menu item
 */
static LRESULT CALLBACK ButtonWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   Button *b = (Button*)GetWindowLongPtr(hWnd, 0);
   return (b != nullptr) ? b->processMessage(uMsg, wParam, lParam) : DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/**
 * Initialize buttons
 */
bool InitButtons()
{
   WNDCLASS wc;
   memset(&wc, 0, sizeof(WNDCLASS));
   wc.lpfnWndProc = ButtonWndProc;
   wc.hInstance = g_hInstance;
   wc.cbWndExtra = sizeof(Button*);
   wc.lpszClassName = BUTTON_CLASS_NAME;
   wc.hbrBackground = NULL;
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   if (RegisterClass(&wc) == 0)
   {
      TCHAR buffer[1024];
      nxlog_debug(1, _T("RegisterClass(%s) failed (%s)"), BUTTON_CLASS_NAME, GetSystemErrorText(GetLastError(), buffer, 1024));
      return false;
   }
   return true;
}
