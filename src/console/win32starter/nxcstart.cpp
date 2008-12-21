/* 
** NetXMS - Network Management System
** Win32 Console Starter
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
**/

#include <stdio.h>
#include <windows.h>
#include <process.h>
#include "resource.h"


//
// Static data
//

static HBITMAP hSplashBitmap = NULL;
static BITMAP bmSplash;


//
// Thread function which do all work
//

static void WorkThread(void *pArg)
{
   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   Sleep(1000);
   memset(&si, 0, sizeof(STARTUPINFO));
   si.cb = sizeof(STARTUPINFO);
   si.dwFlags = 0;
   if (CreateProcess(NULL, "nxcon.exe", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
   {
      Sleep(2000);
   }
   else
   {
      MessageBox((HWND)pArg, "Unable to create process", "NetXMS Console", MB_ICONSTOP);
   }
   PostMessage((HWND)pArg, WM_CLOSE, 0, 0);
}


//
// Windows procedure for splash window
//

static LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   HDC hdc, hbmdc;
   PAINTSTRUCT paintStruct;

   switch(uMsg)
   {
      case WM_CLOSE:
         DestroyWindow(hWnd);
         break;
      case WM_DESTROY:
         PostQuitMessage(0);
         break;
      case WM_PAINT:
         hdc = BeginPaint(hWnd, &paintStruct);
         hbmdc = CreateCompatibleDC(hdc);
         SelectObject(hbmdc, hSplashBitmap);
         BitBlt(hdc, 0, 0, bmSplash.bmWidth, bmSplash.bmHeight, hbmdc, 0, 0, SRCCOPY);
         DeleteDC(hbmdc);
         EndPaint(hWnd, &paintStruct);
         break;
      default:
         return DefWindowProc(hWnd, uMsg, wParam, lParam);
   }
   return 0;
}


//
// Entry point
//

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
   WNDCLASS wc;
   HWND hWnd;
   MSG msg;
   int cx, cy;

   // Register window class
   wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc = MainWindowProc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = hInstance;
   wc.hIcon = NULL;
   wc.hCursor = LoadCursor(NULL, IDC_WAIT);
   wc.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
   wc.lpszMenuName = NULL;
   wc.lpszClassName = "NetXMSStarterClass";

   if (!RegisterClass(&wc))
   {
      MessageBox(NULL, "Cannot register window class", "NetXMS Console", MB_ICONSTOP);
      return 0;
   }

   // Load splash screen bitmap from resources
   hSplashBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_SPLASH));
   GetObject(hSplashBitmap, sizeof(BITMAP), &bmSplash);

   // Calculate window position
   cx = (GetSystemMetrics(SM_CXMAXIMIZED) - bmSplash.bmWidth) / 2;
   cy = (GetSystemMetrics(SM_CYMAXIMIZED) - bmSplash.bmHeight) / 2;

   // Create window
   hWnd = CreateWindowEx(WS_EX_TOPMOST, "NetXMSStarterClass", "NetXMS Console", WS_POPUP,
                         cx, cy, bmSplash.bmWidth, bmSplash.bmHeight,
                         NULL, NULL, hInstance, NULL);
   if (hWnd == NULL)
   {
      MessageBox(NULL, "Cannot create window", "NetXMS Console", MB_ICONSTOP);
      return 0;
   }

   // Start worker thread
   _beginthread(WorkThread, 0, hWnd);

   // Everything is ready, show our window
   ShowWindow(hWnd, SW_SHOW);
   UpdateWindow(hWnd);

   // Main message loop
   while(GetMessage(&msg, NULL, 0, 0) > 0)
   {
      TranslateMessage(&msg); 
      DispatchMessage(&msg); 
   }

   // Cleanup
   DeleteObject(hSplashBitmap);
   return 0;
}
