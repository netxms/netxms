#include "nxuseragent.h"
#include <Psapi.h>

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
 * Shutdown condition
 */
CONDITION g_shutdownCondition = ConditionCreate(true);

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
   _tcscat(path, _T("\\nxuseragent\\log"));
   CreateFolder(path);
   
   _tcscat(path, _T("\\nxuseragent.log"));
   nxlog_open(path, 0);
   nxlog_set_debug_level(9);
}

/**
 * Load file store location
 */
static void LoadFileStoreLocation()
{
   TCHAR path[MAX_PATH];
   if (SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path) != S_OK)
      _tcscpy(path, _T("C:"));
   _tcscat(path, _T("\\nxuseragent\\filestore"));

   int fd = _topen(path, O_BINARY | O_RDONLY);
   if (fd != -1)
   {
      TCHAR fileStore[MAX_PATH];
      int bytes = _read(fd, fileStore, MAX_PATH * sizeof(TCHAR));
      _close(fd);
      if (bytes > 0)
      {
         fileStore[bytes / sizeof(TCHAR)] = 0;
         SetEnvironmentVariable(_T("NETXMS_FILE_STORE"), fileStore);
      }
   }
}

/**
 * Check if user agent already running in this session
 */
static void CheckIfRunning()
{
   DWORD sessionId;
   if (!ProcessIdToSessionId(GetCurrentProcessId(), &sessionId))
      return;

   TCHAR pipeName[128];
   _sntprintf(pipeName, 128, _T("\\\\.\\pipe\\nxuseragent.%u"), sessionId);

   DWORD response, bytes;
   if (!CallNamedPipe(pipeName, &sessionId, 4, &response, 4, &bytes, 1000))
      return;

   if ((bytes != 4) || (response != sessionId))
      return;

   ExitProcess(0);
}

/**
 * Echo listener
 */
static NamedPipeListener *s_echoListener = nullptr;

/**
 * Echo request handler
 */
static void EchoRequestHandler(NamedPipe *pipe, void *userArg)
{
   BYTE buffer[128];

   OVERLAPPED ov;
   memset(&ov, 0, sizeof(OVERLAPPED));
   ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
   DWORD bytes;
   if (ReadFile(pipe->handle(), buffer, 128, &bytes, &ov))
   {
      // completed immediately
      nxlog_debug(6, _T("EchoRequestHandler: echo request for %u bytes"), bytes);
      WriteFile(pipe->handle(), buffer, bytes, &bytes, NULL);
      CloseHandle(ov.hEvent);
      return;
   }

   if (GetLastError() != ERROR_IO_PENDING)
   {
      TCHAR errorText[1024];
      nxlog_debug(6, _T("EchoRequestHandler: ReadFile failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
      CloseHandle(ov.hEvent);
      return;
   }

   if (WaitForSingleObject(ov.hEvent, 1000) != WAIT_OBJECT_0)
   {
      CancelIo(pipe->handle());
      CloseHandle(ov.hEvent);
      return;
   }

   if (!GetOverlappedResult(pipe->handle(), &ov, &bytes, TRUE))
   {
      if (GetLastError() != ERROR_MORE_DATA)
      {
         CloseHandle(ov.hEvent);
         return;
      }
   }

   nxlog_debug(6, _T("EchoRequestHandler: echo request for %u bytes"), bytes);
   WriteFile(pipe->handle(), buffer, bytes, &bytes, NULL);
   CloseHandle(ov.hEvent);
}

/**
 * Start echo listener
 */
static bool StartEchoListener()
{
   DWORD sessionId;
   if (!ProcessIdToSessionId(GetCurrentProcessId(), &sessionId))
   {
      TCHAR errorText[1024];
      nxlog_write(NXLOG_ERROR, _T("StartEchoListener: ProcessIdToSessionId failed (%s)"),
            GetSystemErrorText(GetLastError(), errorText, 1024));
      return false;
   }

   TCHAR pipeName[128];
   _sntprintf(pipeName, 128, _T("nxuseragent.%u"), sessionId);
   s_echoListener = NamedPipeListener::create(pipeName, EchoRequestHandler, nullptr);
   if (s_echoListener == nullptr)
   {
      nxlog_write(NXLOG_ERROR, _T("StartEchoListener: cannot create named pipe"));
      return false;
   }

   s_echoListener->start();
   nxlog_write(NXLOG_INFO, _T("Echo listener started"));
   return true;
}

/**
 * Application entry point
 */
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
   CheckIfRunning();

   g_hInstance = hInstance;
   g_mainThreadId = GetCurrentThreadId();

   InitNetXMSProcess(false);
   InitLogging();
   LoadConfig();
   LoadFileStoreLocation();

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
       !PrepareApplicationWindow() || !PrepareNotificationWindows() || !SetupSessionEventHandler() ||
       !InitButtons() || !StartEchoListener())
   {
      nxlog_write(NXLOG_ERROR, _T("NetXMS User Agent initialization failed"));
      MessageBox(NULL, _T("NetXMS User Agent initialization failed"), _T("NetXMS User Agent"), MB_OK | MB_ICONSTOP);
      return 1;
   }
   UpdateAddressList();
   StartAgentConnector();
   nxlog_write(NXLOG_INFO, _T("NetXMS user agent started"));

   UpdateDesktopWallpaper();

   UINT modifiers;
   UINT keycode = GetHotKey(&modifiers);
   if (keycode != 0)
   {
      RegisterHotKey(NULL, 1, modifiers, keycode);
   }

   THREAD notificationManager = ThreadCreateEx(NotificationManager, 0, nullptr);

   bool startup = true;
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
      else if (msg.message == NXUA_MSG_SHOW_NOTIFICATIONS)
      {
         if (startup && (msg.wParam != 0))
         {
            ShowPendingNotifications(true);
            startup = false;
         }
         else
         {
            ShowPendingNotifications(false);
         }
      }
      else
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

   ConditionSet(g_shutdownCondition);

   if (s_echoListener != nullptr)
   {
      nxlog_debug(2, _T("Stopping echo listener"));
      s_echoListener->stop();
      delete s_echoListener;
   }
   StopAgentConnector();
   if (keycode != 0)
   {
      UnregisterHotKey(NULL, 1);
   }
   RemoveTrayIcon();
   ThreadJoin(notificationManager);

   nxlog_write(NXLOG_INFO, _T("NetXMS user agent stopped"));
   nxlog_close();
   return 0;
}
