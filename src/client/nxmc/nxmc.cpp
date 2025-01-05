/* 
** nxmc - launcher for main Java application
** Copyright (C) 2017-2025 Raden Solutions
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

#include <nms_common.h>
#include <nms_util.h>
#include <netxms_getopt.h>
#include <netxms-version.h>
#include <nxjava.h>

NETXMS_EXECUTABLE_HEADER(nxmc)

#ifdef _WIN32
#define NXMC_CHAR WCHAR
#define _NXMC_T(s) L##s
#define NXMC_OPTARG optargW
#define NXMC_SNPRINTF _snwprintf
#define NXMC_ADD_STRING(sb, s) sb.add(s)
#else
#define NXMC_CHAR char
#define _NXMC_T(s) s
#define NXMC_OPTARG optarg
#define NXMC_SNPRINTF snprintf
#define NXMC_ADD_STRING(sb, s) sb.addMBString(s)
#endif

/**
 * Options
 */
static bool s_autoLogin = false;
static const NXMC_CHAR *s_optHost = _NXMC_T("127.0.0.1");
static const NXMC_CHAR *s_optUser = nullptr;
static const NXMC_CHAR *s_optPassword = _NXMC_T("");
static const NXMC_CHAR *s_optToken = nullptr;
static const NXMC_CHAR *s_optJre = nullptr;
static const NXMC_CHAR *s_optMaxMem = nullptr;
static const NXMC_CHAR *s_optClassPath = nullptr;

/**
 * Display error message
 */
static void ShowErrorMessage(const TCHAR *format, ...)
{
   TCHAR message[1024];
   va_list args;
   va_start(args, format);
   _vsntprintf(message, 1024, format, args);
   va_end(args);
#ifdef _WIN32
   MessageBox(nullptr, message, _T("NetXMS Management Console - Startup Error"), MB_OK | MB_ICONERROR);
#else
   _tprintf(_T("%s\n"), message);
#endif
   nxlog_debug_tag(_T("nxmc"), 1, _T("%s"), message);
}

/**
 * Start application
 */
static int StartApp(int argc, NXMC_CHAR *argv[], const StringList& jreOptions)
{
   // Load application INI
   Config appConfig;
   TCHAR path[MAX_PATH];
#ifdef _WIN32
   GetModuleFileName(nullptr, path, MAX_PATH);
   TCHAR *p = _tcsrchr(path, _T('.'));
   if (p != nullptr)
      _tcscpy(p, _T(".ini"));
#else
   GetNetXMSDirectory(nxDirShare, path);
   _tcslcat(path, _T("/nxmc.ini"), MAX_PATH);
#endif
   if (_taccess(path, R_OK) == 0)
   {
      nxlog_debug_tag(_T("nxmc"), 1, _T("Loading application configuration file \"%s\""), path);
      appConfig.loadConfig(path, _T("nxmc"));
   }

   TCHAR jre[MAX_PATH];
   if (s_optJre != nullptr)
   {
#if !defined(_WIN32) && defined(UNICODE)
      MultiByteToWideCharSysLocale(s_optJre, jre, MAX_PATH);
      jre[MAX_PATH - 1] = 0;
#else
      _tcslcpy(jre, s_optJre, MAX_PATH);
#endif
   }
   else
   {
      if (FindJavaRuntime(jre, MAX_PATH) == nullptr)
      {
         ShowErrorMessage(_T("Cannot find suitable Java runtime environment"));
         return 2;
      }
   }
   nxlog_debug_tag(_T("nxmc"), 1, _T("Using JRE: %s"), jre);

   StringList vmOptions;
   if (s_autoLogin)
   {
      NXMC_ADD_STRING(vmOptions,  _NXMC_T("-Dnetxms.autologin=true"));
   }

   if ((s_optToken != nullptr) || (s_optUser != nullptr))
   {
      NXMC_CHAR buffer[256];
      NXMC_SNPRINTF(buffer, 256, _NXMC_T("-Dnetxms.server=%s"), s_optHost);
      NXMC_ADD_STRING(vmOptions, buffer);

      if (s_optToken != nullptr)
      {
         NXMC_SNPRINTF(buffer, 256, _NXMC_T("-Dnetxms.token=%s"), s_optToken);
         NXMC_ADD_STRING(vmOptions, buffer);
      }
      else if (s_optUser != nullptr)
      {
         NXMC_SNPRINTF(buffer, 256, _NXMC_T("-Dnetxms.login=%s"), s_optUser);
         NXMC_ADD_STRING(vmOptions, buffer);

         NXMC_CHAR clearPassword[128];
#ifdef _WIN32
         DecryptPasswordW(s_optUser, s_optPassword, clearPassword, 128);
#else
         DecryptPasswordA(s_optUser, s_optPassword, clearPassword, 128);
#endif
         NXMC_SNPRINTF(buffer, 256, _NXMC_T("-Dnetxms.password=%s"), clearPassword);
         NXMC_ADD_STRING(vmOptions, buffer);
      }
   }

   if (s_optMaxMem != nullptr)
   {
      NXMC_CHAR buffer[256];
      NXMC_SNPRINTF(buffer, 256, _NXMC_T("-Xmx%s"), s_optMaxMem);
      NXMC_ADD_STRING(vmOptions, buffer);
   }

   // Temporary directory for SWT libraries
   StringBuffer swtLibraryPath;
#ifdef _WIN32
   TCHAR temp[MAX_PATH];
   if (GetTempPath(MAX_PATH, temp) != 0)
   {
      swtLibraryPath.append(temp);
      swtLibraryPath.append(_T("nxmc."));
   }
   else
   {
      swtLibraryPath.append(_T("C:\\nxmc."));
   }
#else
   swtLibraryPath.append(_T("/tmp/nxmc."));
#endif
   swtLibraryPath.append(static_cast<uint32_t>(GetCurrentProcessId()));
   swtLibraryPath.append(_T("."));
   swtLibraryPath.append(GetCurrentTimeMs());
   if (CreateDirectoryTree(swtLibraryPath))
   {
      StringBuffer option(_T("-Dswt.library.path="));
      option.append(swtLibraryPath);
      vmOptions.add(option);
   }

   // Additional JVM options
   for (int i = 0; i < jreOptions.size(); i++)
   {
      StringBuffer option(_T("-"));
      option.append(jreOptions.get(i));
      vmOptions.add(option);
   }

   int rc = 0;

   const TCHAR *jar = appConfig.getValue(_T("/nxmc/loader"));
   if (jar == nullptr)
      jar = _T("nxmc-") NETXMS_JAR_VERSION _T(".jar");
   nxlog_debug_tag(_T("nxmc"), 1, _T("Using loader module %s"), jar);

#if !defined(_WIN32) && defined(UNICODE)
   TCHAR *cp = WideStringFromMBStringSysLocale(s_optClassPath);
#else
#define cp s_optClassPath
#endif
   JNIEnv *env;
   JavaBridgeError err = CreateJavaVM(jre, jar, nullptr, cp, &vmOptions, &env);
   if (err == NXJAVA_SUCCESS)
   {
      nxlog_debug_tag(_T("nxmc"), 5, _T("JVM created"));

      char startupClass[256];
      const TCHAR *c = appConfig.getValue(_T("/nxmc/class"));
      if (c != nullptr)
      {
#ifdef UNICODE
         wchar_to_utf8(c, -1, startupClass, 256);
#else
         strlcpy(startupClass, c, 256);
#endif
      }
      else
      {
         strcpy(startupClass, "org/netxms/nxmc/Startup");
      }

#ifdef _WIN32
      err = StartJavaApplication(env, startupClass, argc, nullptr, argv);
#else
      err = StartJavaApplication(env, startupClass, argc, argv, nullptr);
#endif
      if (err != NXJAVA_SUCCESS)
      {
         ShowErrorMessage(_T("Cannot start Java application (%s)"), GetJavaBridgeErrorMessage(err));
         rc = 4;
      }
      DestroyJavaVM();
   }
   else
   {
      ShowErrorMessage(_T("Unable to create Java VM (%s)"), GetJavaBridgeErrorMessage(err));
      rc = 3;
   }

   DeleteDirectoryTree(swtLibraryPath);
   return rc;
}

/**
 * Command line options
 */
#if HAVE_DECL_GETOPT_LONG
static struct option longOptions[] =
{
   { (char *)"auto",           no_argument,       nullptr,        'a' },
	{ (char *)"classpath",      required_argument, nullptr,        'C' },
	{ (char *)"debug",          no_argument,       nullptr,        'D' },
	{ (char *)"help",           no_argument,       nullptr,        'h' },
	{ (char *)"host",           required_argument, nullptr,        'H' },
	{ (char *)"jre",            required_argument, nullptr,        'j' },
   { (char *)"maxmem",         required_argument, nullptr,        'm' },
   { (char *)"option",         required_argument, nullptr,        'o' },
   { (char *)"password",       required_argument, nullptr,        'P' },
   { (char *)"port",           required_argument, nullptr,        'p' },
   { (char *)"token",          required_argument, nullptr,        't' },
	{ (char *)"user",           required_argument, nullptr,        'u' },
	{ (char *)"version",        no_argument,       nullptr,        'v' },
	{ nullptr, 0, nullptr, 0 }
};
#endif

#define SHORT_OPTIONS "aC:DhH:j:m:o:p:P:t:u:v"

/**
 * Print usage info
 */
static void ShowUsage()
{
#ifdef _WIN32
   MessageBox(nullptr,
#else
   _tprintf(
#endif
      _T("NetXMS Management Console  Version ") NETXMS_VERSION_STRING _T("\n")
      _T("Copyright (c) 2006-2025 Raden Solutions\n\n")
      _T("Usage: nxmc [LAUNCHER-OPTIONS] [-- APP-OPTIONS]\n")
      _T("\n")
      _T("Launcher options:\n")
#if HAVE_GETOPT_LONG
      _T("  -a, --auto                  Login automatically.\n")
      _T("  -C, --classpath <path>      Additional Java class path.\n")
#ifdef _WIN32
      _T("  -D, --debug                 Write launcher debug log (use twice for extra verbose output).\n")
#else
      _T("  -D, --debug                 Show additional debug output (use twice for extra verbose output).\n")
#endif
      _T("  -h, --help                  Display this help message.\n")
      _T("  -H, --host <hostname>       Specify host name or IP address. Could be in host:port form.\n")
      _T("  -j, --jre <path>            Specify JRE location.\n")
      _T("  -m, --maxmem <size>         JVM max memory size (JVM option -Xmx).\n")
      _T("  -o, --option <option>       Additional JVM option.\n")
      _T("  -P, --password <password>   Specify user's password. Default is empty.\n")
      _T("  -t, --token <token>         Login to server using given authentication token.\n")
      _T("  -u, --user <user>           Login to server as given user.\n")
      _T("  -v, --version               Display version information.\n\n")
#else
      _T("  -a             Login automatically.\n")
      _T("  -C <path>      Additional Java class path.\n")
      _T("  -D             Show additional debug output.\n")
      _T("  -h             Display this help message.\n")
      _T("  -H <hostname>  Specify host name or IP address. Could be in host:port form.\n")
      _T("  -j <path>      Specify JRE location.\n")
      _T("  -m <size>      JVM max memory size (JVM option -Xmx).\n")
      _T("  -o <option>    Additional JVM option.\n")
      _T("  -P <password>  Specify user's password. If not given, password will be read from terminal.\n")
      _T("  -t <token>     Login to server using given authentication token.\n")
      _T("  -u <user>      Login to server as given user.\n")
      _T("  -v             Display version information.\n\n")
#endif
      _T("Application options:\n")
      _T("  -auto                    Login automatically.\n")
      _T("  -ignore-protocol-version Do not check if protocol version is compatible.\n")
      _T("  -dashboard=<dashboard>   Open dashboard with given name or ID after login.\n")
      _T("  -disable-compression     Disable protocol level compression.\n")
      _T("  -kiosk-mode              Start in kiosk mode (only show selected dashboard or map).\n")
      _T("  -language=<lang>         Set language.\n")
      _T("  -login=<user>            Login to server as given user.\n")
      _T("  -map=<map>               Open map with given name or ID after login.\n")
      _T("  -password=<password>     Specify user's password. Default is empty.\n")
      _T("  -server=<hostname>       Specify host name or IP address. Could be in host:port form.\n")
      _T("  -token=<token>           Login to server using given authentication token.\n\n")
#ifdef _WIN32
      , _T("NetXMS Management Console"), MB_OK | MB_ICONINFORMATION
#endif
      );
}

/**
 * Debug writer
 */
static void DebugWriter(const TCHAR *tag, const TCHAR *format, va_list args)
{
   _tprintf(_T("[%-19s] "), (tag != nullptr) ? tag : _T("nxmc"));
   _vtprintf(format, args);
   _fputtc(_T('\n'), stdout);
}

/**
 * Entry point
 */
#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
   InitNetXMSProcess(true, true);

   int argc;
   WCHAR **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
#else
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true, true);
#endif

   StringList jreOptions;

   int debug = 0;
   opterr = 0;
   int c;
#ifdef _WIN32
   while ((c = getopt_longW(argc, argv, SHORT_OPTIONS, longOptions, nullptr)) != -1)
#else
#if HAVE_DECL_GETOPT_LONG
   while ((c = getopt_long(argc, argv, SHORT_OPTIONS, longOptions, nullptr)) != -1)
#else
   while ((c = getopt(argc, argv, SHORT_OPTIONS)) != -1)
#endif
#endif
   {
		switch(c)
		{
         case 'a': // Auto login
            s_autoLogin = true;
            break;
		   case 'C': // classpath
			   s_optClassPath = NXMC_OPTARG;
			   break;
         case 'D': // Additional debug
            debug++;
            break;
		   case 'h': // help
		      ShowUsage();
			   exit(0);
			   break;
		   case 'H': // host
			   s_optHost = NXMC_OPTARG;
			   break;
		   case 'j': // JRE
			   s_optJre = NXMC_OPTARG;
			   break;
         case 'm': // maxmem
            s_optMaxMem = NXMC_OPTARG;
            break;
         case 'o': // JRE option
#if defined(_WIN32) || !defined(UNICODE)
            jreOptions.add(NXMC_OPTARG);
#else
            jreOptions.addPreallocated(WideStringFromMBStringSysLocale(NXMC_OPTARG));
#endif
            break;
		   case 'P': // password
			   s_optPassword = NXMC_OPTARG;
			   break;
		   case 't': // token
			   s_optToken = NXMC_OPTARG;
			   break;
         case 'u': // user
            s_optUser = NXMC_OPTARG;
            break;
		   case 'v': // version
#ifdef _WIN32
            MessageBox(nullptr,
               _T("NetXMS Management Console  Version ") NETXMS_VERSION_STRING _T("\n")
               _T("Copyright (c) 2006-2025 Raden Solutions\n\n"), _T("NetXMS Management Console"), MB_OK | MB_ICONINFORMATION);
#else
            _tprintf(
               _T("NetXMS Management Console  Version ") NETXMS_VERSION_STRING _T("\n")
               _T("Copyright (c) 2006-2025 Raden Solutions\n\n"));
#endif
			   exit(0);
			   break;
		   case '?':
#ifdef _WIN32
            MessageBox(nullptr, _T("Invalid command line option"), _T("NetXMS Management Console"), MB_OK | MB_ICONERROR);
#else
            _tprintf(_T("Invalid command line option '%c'\n"), c);
#endif
            exit(1);
			   break;
		}
	}

   if (debug > 0)
   {
#ifdef _WIN32
      nxlog_open(_T("nxmc-debug.log"), NXLOG_DEBUG_MODE);
#else
      nxlog_set_debug_writer(DebugWriter);
#endif
      nxlog_set_debug_level(debug == 1 ? 5 : 9);
   }

   int rc = StartApp(argc - optind, &argv[optind], jreOptions);

#ifdef _WIN32
   if (debug > 0)
      nxlog_close();
#endif

   return rc;
}
