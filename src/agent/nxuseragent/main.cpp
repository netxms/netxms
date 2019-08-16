#include "nxuseragent.h"

NETXMS_EXECUTABLE_HEADER(nxuseragent)

/**
 * Enable Common Controls version 6
 */
#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

/**
 * Application instance handle
 */
HINSTANCE g_hInstance;

/**
 * Main thread ID
 */
DWORD g_mainThreadId = 0;

/**
 * Initialize logging
 */
static void InitLogging()
{
   TCHAR path[MAX_PATH];
   if (SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path) != S_OK)
      _tcscpy(path, _T("C:"));
   _tcscat(path, _T("\\nxuseragent\\log"));
   CreateFolder(path);
   
   _tcscat(path, _T("\\nxuseragent.log"));
   nxlog_open(path, 0);
   nxlog_set_debug_level(9);
}

/**
 * Application entry point
 */
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
   g_hInstance = hInstance;
   g_mainThreadId = GetCurrentThreadId();

   InitNetXMSProcess(false);
   InitLogging();
   LoadConfig();

   WSADATA wsaData;
   int wrc = WSAStartup(MAKEWORD(2, 2), &wsaData);
   if (wrc != 0)
   {
      TCHAR buffer[1024];
      nxlog_write(NXLOG_ERROR, _T("WSAStartup() failed (%s)"), GetSystemErrorText(WSAGetLastError(), buffer, 1024));
   }

   INITCOMMONCONTROLSEX icc;
   icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
   icc.dwICC = ICC_LINK_CLASS;

   if ((wrc != 0) || !InitCommonControlsEx(&icc) || !InitMenu() || !SetupTrayIcon() ||
       !PrepareApplicationWindow() || !PrepareMessageWindow() || !SetupSessionEventHandler())
   {
      nxlog_write(NXLOG_ERROR, _T("NetXMS User Agent initialization failed"));
      MessageBox(NULL, _T("NetXMS User Agent initialization failed"), _T("NetXMS User Agent"), MB_OK | MB_ICONSTOP);
      return 1;
   }
   UpdateAddressList();
   StartAgentConnector();
   nxlog_write(NXLOG_INFO, _T("NetXMS user agent started"));

   UINT modifiers;
   UINT keycode = GetHotKey(&modifiers);
   if (keycode != 0)
   {
      RegisterHotKey(NULL, 1, modifiers, keycode);
   }

   MSG msg;
   while(GetMessage(&msg, NULL, 0, 0))
   {
      if (msg.message == WM_HOTKEY)
      {
         POINT pt;
         GetCursorPos(&pt);
         SetForegroundWindow(GetTrayWindow());
         OpenApplicationWindow(pt, true);
      }
      else if (msg.message == NXUA_MSG_OPEN_MESSAGE_WINDOW)
      {
         OpenMessageWindow();
      }
      else
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

   StopAgentConnector();
   if (keycode != 0)
   {
      UnregisterHotKey(NULL, 1);
   }
   RemoveTrayIcon();
   nxlog_write(NXLOG_INFO, _T("NetXMS user agent stopped"));
   nxlog_close();
   return 0;
}
