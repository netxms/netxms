/* 
** nxshell - launcher for main Java application
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

NETXMS_EXECUTABLE_HEADER(nxshell)

/**
 * Options
 */
static const char *s_optHost = "127.0.0.1";
static const char *s_optPort = "";
static const char *s_optUser = "admin";
static const char *s_optPassword = nullptr;
static const char *s_optToken = nullptr;
static const char *s_optJre = nullptr;
static const char *s_optClassPath = nullptr;
static const char *s_optPropertiesFile = nullptr;
static bool s_optSync = true;

/**
 * Start application
 */
static int StartApp(int argc, char *argv[])
{
   TCHAR jre[MAX_PATH];
   if (s_optJre != nullptr)
   {
#ifdef UNICODE
      MultiByteToWideCharSysLocale(s_optJre, jre, MAX_PATH);
      jre[MAX_PATH - 1] = 0;
#else
      strlcpy(jre, s_optJre, MAX_PATH);
#endif
   }
   else
   {
      if (FindJavaRuntime(jre, MAX_PATH) == nullptr)
      {
         _tprintf(_T("Cannot find suitable Java runtime environment\n"));
         return 2;
      }
   }
   nxlog_debug(1, _T("Using JRE: %s"), jre);

   StringList vmOptions;
   char buffer[256];
   snprintf(buffer, 256, "-Dnetxms.server=%s", s_optHost);
   vmOptions.addMBString(buffer);
   snprintf(buffer, 256, "-Dnetxms.port=%s", s_optPort);
   vmOptions.addMBString(buffer);
   snprintf(buffer, 256, "-Dnetxms.syncObjects=%s", s_optSync ? "true" : "false");
   vmOptions.addMBString(buffer);

   if (s_optToken != nullptr)
   {
      snprintf(buffer, 256, "-Dnetxms.token=%s", s_optToken);
      vmOptions.addMBString(buffer);
   }
   else
   {
      snprintf(buffer, 256, "-Dnetxms.login=%s", s_optUser);
      vmOptions.addMBString(buffer);

      if (s_optPassword != nullptr)
      {
         char clearPassword[128];
         DecryptPasswordA(s_optUser, s_optPassword, clearPassword, 128);
         snprintf(buffer, 256, "-Dnetxms.password=%s", clearPassword);
         vmOptions.addMBString(buffer);
      }
   }

   if (s_optPropertiesFile != nullptr)
   {
      snprintf(buffer, 256, "-Dnxshell.properties=%s", s_optPropertiesFile);
      vmOptions.addMBString(buffer);
      if (_access(s_optPropertiesFile, R_OK) != 0)
      {
         WriteToTerminalEx(_T("WARNING: property file \"%hs\" cannot be accessed (%s)\n"), s_optPropertiesFile, _tcserror(errno));
      }
   }

   if (_isatty(_fileno(stdin)) && _isatty(_fileno(stdout)))
   {
      vmOptions.addMBString("-Dnxshell.interactive=true");
   }

   int debugLevel = nxlog_get_debug_level();
   if (debugLevel > 0)
   {
      vmOptions.add(_T("-Dnxshell.debug=true"));
      if (debugLevel > 7)
      {
         vmOptions.add(_T("-verbose:jni"));
         vmOptions.add(_T("-verbose:class"));
      }
   }

   int rc = 0;

#ifdef UNICODE
   TCHAR *cp = WideStringFromMBStringSysLocale(s_optClassPath);
#else
#define cp s_optClassPath
#endif
   JNIEnv *env;
   static const TCHAR *syslibs[] =
   {
      _T("netxms-base-") NETXMS_JAR_VERSION _T(".jar"),
      _T("netxms-client-") NETXMS_JAR_VERSION _T(".jar"),  // should be listed explicitly, otherwise jython cannot do some imports correctly
      nullptr
   };
   JavaBridgeError err = CreateJavaVM(jre, _T("nxshell-") NETXMS_JAR_VERSION _T(".jar"), syslibs, cp, &vmOptions, &env);
   if (err == NXJAVA_SUCCESS)
   {
      nxlog_debug(5, _T("JVM created"));
      err = StartJavaApplication(env, "org/netxms/Shell", argc, argv);
      if (err != NXJAVA_SUCCESS)
      {
#ifdef _WIN32
         _setmode(_fileno(stdout), _O_U16TEXT);
#endif
         _tprintf(_T("Cannot start Java application (%s)\n"), GetJavaBridgeErrorMessage(err));
         rc = 4;
      }
      DestroyJavaVM();
   }
   else
   {
      _tprintf(_T("Unable to create Java VM (%s)\n"), GetJavaBridgeErrorMessage(err));
      rc = 3;
   }
   return rc;
}

/**
 * Command line options
 */
#if HAVE_DECL_GETOPT_LONG
static struct option longOptions[] =
{
	{ (char *)"classpath",      required_argument, nullptr,        'C' },
	{ (char *)"debug",          no_argument,       nullptr,        'D' },
	{ (char *)"help",           no_argument,       nullptr,        'h' },
	{ (char *)"host",           required_argument, nullptr,        'H' },
	{ (char *)"jre",            required_argument, nullptr,        'j' },
	{ (char *)"no-sync",        required_argument, nullptr,        'n' },
	{ (char *)"password",       required_argument, nullptr,        'P' },
   { (char *)"port",           required_argument, nullptr,        'p' },
   { (char *)"properties",     required_argument, nullptr,        'r' },
   { (char *)"token",          required_argument, nullptr,        't' },
	{ (char *)"user",           required_argument, nullptr,        'u' },
	{ (char *)"version",        no_argument,       nullptr,        'v' },
	{ nullptr, 0, nullptr, 0 }
};
#endif

#define SHORT_OPTIONS "C:DhH:j:np:P:r:t:u:v"

/**
 * Print usage info
 */
static void ShowUsage(bool showVersion)
{
   if (showVersion)
   {
      _tprintf(
         _T("NetXMS Interactive Shell  Version ") NETXMS_VERSION_STRING _T("\n")
         _T("Copyright (c) 2006-2025 Raden Solutions\n\n"));
   }

   _tprintf(
      _T("Usage: nxshell [OPTIONS] [script]\n")
      _T("  \n")
      _T("Options:\n")
#if HAVE_GETOPT_LONG
      _T("  -C, --classpath <path>      Additional Java class path.\n")
      _T("  -D, --debug                 Show additional debug output (use twice for extra output).\n")
      _T("  -h, --help                  Display this help message.\n")
      _T("  -H, --host <hostname>       Specify host name or IP address. Could be in host:port form.\n")
      _T("  -j, --jre <path>            Specify JRE location.\n")
      _T("  -n, --no-sync               Do not synchronize objects on connect.\n")
      _T("  -p, --port <port>           Specify TCP port for connection. Default is 4701.\n")
      _T("  -P, --password <password>   Specify user's password. Default is empty.\n")
      _T("  -r, --properties <file>     File with additional Java properties.\n")
      _T("  -t, --token <token>         Login to server using given authentication token.\n")
      _T("  -u, --user <user>           Login to server as user. Default is \"admin\".\n")
      _T("  -v, --version               Display version information.\n\n")
#else
      _T("  -C <path>      Additional Java class path.\n")
      _T("  -D             Show additional debug output.\n")
      _T("  -h             Display this help message.\n")
      _T("  -H <hostname>  Specify host name or IP address. Could be in host:port form.\n")
      _T("  -j <path>      Specify JRE location.\n")
      _T("  -n             Do not synchronize objects on connect.\n")
      _T("  -p <port>      Specify TCP port for connection. Default is 4701.\n")
      _T("  -P <password>  Specify user's password. If not given, password will be read from terminal.\n")
      _T("  -r <file>      File with additinal Java properties.\n")
      _T("  -t <token>     Login to server using given authentication token.\n")
      _T("  -u <user>      Login to server as user. Default is \"admin\".\n")
      _T("  -v             Display version information.\n\n")
#endif
      );
}

/**
 * Debug writer
 */
static void DebugWriter(const TCHAR *tag, const TCHAR *format, va_list args)
{
#ifdef _WIN32
   int mode = _setmode(_fileno(stdout), _O_U16TEXT);
#endif
   _tprintf(_T("[%-19s] "), (tag != nullptr) ? tag : _T("nxshell"));
   _vtprintf(format, args);
   _fputtc(_T('\n'), stdout);
#ifdef _WIN32
   _setmode(_fileno(stdout), mode);
#endif
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true, true);

   opterr = 0;
   int c, debug = 0;
#if HAVE_DECL_GETOPT_LONG
   while ((c = getopt_long(argc, argv, SHORT_OPTIONS, longOptions, nullptr)) != -1)
#else
   while ((c = getopt(argc, argv, SHORT_OPTIONS)) != -1)
#endif
   {
		switch(c)
		{
		   case 'C': // classpath
			   s_optClassPath = optarg;
			   break;
         case 'D': // Additional debug
            debug++;
            nxlog_set_debug_writer(DebugWriter);
            nxlog_set_debug_level(debug == 1 ? 5 : 9);
            break;
		   case 'h': // help
		      ShowUsage(true);
			   exit(0);
			   break;
		   case 'H': // host
			   s_optHost = optarg;
			   break;
		   case 'j': // JRE
			   s_optJre = optarg;
			   break;
         case 'n': // no sync
            s_optSync = false;
            break;
         case 'p': // port
            s_optPort = optarg;
            break;
		   case 'P': // password
			   s_optPassword = optarg;
			   break;
         case 'r': // properties file
            s_optPropertiesFile = optarg;
            break;
		   case 't': // token
			   s_optToken = optarg;
			   break;
         case 'u': // user
            s_optUser = optarg;
            break;
		   case 'v': // version
            _tprintf(
               _T("NetXMS Interactive Shell  Version ") NETXMS_VERSION_STRING _T("\n")
               _T("Copyright (c) 2006-2025 Raden Solutions\n\n"));
			   exit(0);
			   break;
		   case '?':
			   exit(1);
			   break;
		}
	}

   return StartApp(argc - optind, &argv[optind]);
}
