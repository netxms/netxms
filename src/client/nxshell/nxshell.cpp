/* 
** nxshell - launcher for main Java application
** Copyright (C) 2017 Raden Solutions
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
#include <nxjava.h>

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

/**
 * Options
 */
static const char *s_optHost = "127.0.0.1";
static const char *s_optPort = "";
static const char *s_optUser = "admin";
static const char *s_optPassword = "";
static const char *s_optJre = NULL;
static const char *s_optClassPath = NULL;

/**
 * Start application
 */
static int StartApp(int argc, char *argv[])
{
   TCHAR jre[MAX_PATH];
   if (s_optJre != NULL)
   {
#ifdef UNICODE
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, s_optJre, -1, jre, MAX_PATH);
      jre[MAX_PATH - 1] = 0;
#else
      nx_strncpy(jre, s_optJre, MAX_PATH);
#endif
   }
   else
   {
      if (FindJavaRuntime(jre, MAX_PATH) == NULL)
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
   snprintf(buffer, 256, "-Dnetxms.login=%s", s_optUser);
   vmOptions.addMBString(buffer);

   char clearPassword[128];
   DecryptPasswordA(s_optUser, s_optPassword, clearPassword, 128);
   if (clearPassword[0] != 0)
   {
      snprintf(buffer, 256, "-Dnetxms.password=%s", clearPassword);
      vmOptions.addMBString(buffer);
   }

   bool verboseVM = (nxlog_get_debug_level() > 0);
   if (verboseVM)
   {
      vmOptions.add(_T("-verbose:jni"));
      vmOptions.add(_T("-verbose:class"));
   }

   int rc = 0;

#ifdef UNICODE
   TCHAR *cp = WideStringFromMBStringSysLocale(s_optClassPath);
#else
#define cp s_optClassPath
#endif
   JNIEnv *env;
   static const TCHAR *syslibs[] = { _T("netxms-base.jar"), _T("netxms-client.jar"), _T("simple-xml-2.7.1.jar"), NULL };
   JavaBridgeError err = CreateJavaVM(jre, _T("nxshell.jar"), syslibs, cp, &vmOptions, &env);
   if (err == NXJAVA_SUCCESS)
   {
      nxlog_debug(5, _T("JVM created"));
      err = StartJavaApplication(env, "org/netxms/Shell", argc, argv);
      if (err != NXJAVA_SUCCESS)
      {
         _tprintf(_T("Cannot start Java application (%s)"), GetJavaBridgeErrorMessage(err));
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
	{ (char *)"classpath",      required_argument, NULL,        'C' },
	{ (char *)"debug",          no_argument,       NULL,        'D' },
	{ (char *)"help",           no_argument,       NULL,        'h' },
	{ (char *)"host",           required_argument, NULL,        'H' },
	{ (char *)"jre",            required_argument, NULL,        'j' },
	{ (char *)"password",       required_argument, NULL,        'P' },
   { (char *)"port",           required_argument, NULL,        'p' },
	{ (char *)"user",           required_argument, NULL,        'u' },
	{ (char *)"version",        no_argument,       NULL,        'v' },
	{ NULL, 0, NULL, 0 }
};
#endif

#define SHORT_OPTIONS "C:DhH:j:p:P:u:v"

/**
 * Print usage info
 */
static void usage(bool showVersion)
{
   if (showVersion)
   {
      _tprintf(
         _T("NetXMS Interactive Shell  Version ") NETXMS_VERSION_STRING _T("\n")
         _T("Copyright (c) 2006-2017 Raden Solutions\n\n"));
   }

	_tprintf(
      _T("Usage: nxshell [OPTIONS] [script]\n")
      _T("  \n")
      _T("Options:\n")
#if HAVE_GETOPT_LONG
      _T("  -C, --classpath <path>      Additional Java class path.\n")
      _T("  -D, --debug                 Show additional debug output.\n")
      _T("  -h, --help                  Display this help message.\n")
      _T("  -H, --host <hostname>       Specify host name or IP address. Could be in host:port form.\n")
      _T("  -j, --jre <path>            Specify JRE location.\n")
      _T("  -p, --port <port>           Specify TCP port for connection. Default is 4701.\n")
      _T("  -P, --password <password>   Specify user's password. Default is empty.\n")
      _T("  -u, --user <user>           Login to server as user. Default is \"admin\".\n")
      _T("  -v, --version               Display version information.\n\n")
#else
      _T("  -C <path>      Additional Java class path.\n")
      _T("  -D             Show additional debug output.\n")
      _T("  -h             Display this help message.\n")
      _T("  -H <hostname>  Specify host name or IP address. Could be in host:port form.\n")
      _T("  -j <path>      Specify JRE location.\n")
      _T("  -p <port>      Specify TCP port for connection. Default is 4701.\n")
      _T("  -P <password>  Specify user's password. Default is empty.\n")
      _T("  -u <user>      Login to server as user. Default is \"admin\".\n")
      _T("  -v             Display version information.\n\n")
#endif
      );
}

/**
 * Debug writer
 */
static void DebugWriter(const TCHAR *tag, const TCHAR *msg)
{
   if (tag == NULL)      
      _tprintf(_T("DBG: %s\n"), msg);
   else
      _tprintf(_T("DBG: <%s> %s\n"), tag, msg);
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
	int ret = 0;

	InitNetXMSProcess(true);

	opterr = 0;
   int c;
#if HAVE_DECL_GETOPT_LONG
	while ((c = getopt_long(argc, argv, SHORT_OPTIONS, longOptions, NULL)) != -1)
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
            nxlog_set_debug_writer(DebugWriter);
            nxlog_set_debug_level(9);
            break;
		   case 'h': // help
			   usage(true);
			   exit(0);
			   break;
		   case 'H': // host
			   s_optHost = optarg;
			   break;
		   case 'j': // JRE
			   s_optJre = optarg;
			   break;
         case 'p': // port
            s_optPort = optarg;
            break;
		   case 'P': // password
			   s_optPassword = optarg;
			   break;
		   case 'u': // user
			   s_optUser = optarg;
			   break;
		   case 'v': // version
            _tprintf(
               _T("NetXMS Interactive Shell  Version ") NETXMS_VERSION_STRING _T("\n")
               _T("Copyright (c) 2006-2017 Raden Solutions\n\n"));
			   exit(0);
			   break;
		   case '?':
			   exit(1);
			   break;
		}
	}

   return StartApp(argc - optind, &argv[optind]);
}
