/* 
** nxreportd - launcher for reporting server Java application
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
#include <jni.h>

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

/**
 * Options
 */
#ifdef _WIN32
static const TCHAR *s_optConfig = _T("C:\\nxreportd.conf");
#else
static const TCHAR *s_optConfig = _T("{search}");
#endif
static const char *s_optJre = NULL;
static const char *s_optClassPath = NULL;
static bool s_optDaemon = false;

/**
 * Prototype for JNI_CreateJavaVM
 */
typedef jint (JNICALL *T_JNI_CreateJavaVM)(JavaVM **, void **, void *);

/**
 * Start application
 */
static int StartApp()
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

   TCHAR errorText[256];
   HMODULE jvmModule = DLOpen(jre, errorText);
   if (jvmModule == NULL)
   {
      _tprintf(_T("Unable to load JVM: %s\n"), errorText);
      return 3;
   }

   JavaVMInitArgs vmArgs;
   JavaVMOption vmOptions[4];
   memset(vmOptions, 0, sizeof(vmOptions));

   TCHAR libdir[MAX_PATH];
   GetNetXMSDirectory(nxDirLib, libdir);

   String classpath = _T("-Djava.class.path=");
   classpath.append(libdir);
   classpath.append(FS_PATH_SEPARATOR_CHAR);
   classpath.append(_T("nxreportd.jar"));
   if (s_optClassPath != NULL)
   {
      classpath.append(JAVA_CLASSPATH_SEPARATOR);
#ifdef UNICODE
      WCHAR *cp = WideStringFromMBString(s_optClassPath);
      classpath.append(cp);
      free(cp);
#else
      classpath.append(s_optClassPath);
#endif
   }

#ifdef UNICODE
   vmOptions[0].optionString = classpath.getUTF8String();
#else
   vmOptions[0].optionString = strdup(classpath);
#endif

   vmArgs.version = JNI_VERSION_1_6;
   vmArgs.options = vmOptions;
   vmArgs.nOptions = 1;
   vmArgs.ignoreUnrecognized = JNI_TRUE;

   int rc = 4;
   T_JNI_CreateJavaVM CreateJavaVM = (T_JNI_CreateJavaVM)DLGetSymbolAddr(jvmModule, "JNI_CreateJavaVM", NULL);
   if (CreateJavaVM != NULL)
   {
      JavaVM *jvm = NULL;
      JNIEnv *jniEnv;
      if (CreateJavaVM(&jvm, (void **)&jniEnv, &vmArgs) == JNI_OK)
      {
         jclass shell = jniEnv->FindClass("org/netxms/reporting/Server");
         if (shell != NULL)
         {
            jmethodID startServer = jniEnv->GetStaticMethodID(shell, "startServer", "()V");
            if (startServer != NULL)
            {
               jniEnv->CallStaticVoidMethod(shell, startServer);
               rc = 0;
            }
            else
            {
               _tprintf(_T("Cannot find server startup method\n"));
            }
         }
         else
         {
            _tprintf(_T("Cannot find server class\n"));
         }
         jvm->DestroyJavaVM();
      }
      else
      {
         _tprintf(_T("JNI_CreateJavaVM failed\n"));
      }
   }
   else
   {
      _tprintf(_T("JNI_CreateJavaVM entry point not found\n"));
   }

   DLClose(jvmModule);
   return rc;
}

/**
 * Command line options
 */
#if HAVE_DECL_GETOPT_LONG
static struct option longOptions[] =
{
   { (char *)"config",         required_argument, NULL,        'c' },
   { (char *)"classpath",      required_argument, NULL,        'C' },
   { (char *)"daemon",         no_argument,       NULL,        'd' },
   { (char *)"help",           no_argument,       NULL,        'h' },
   { (char *)"jre",            required_argument, NULL,        'j' },
   { (char *)"version",        no_argument,       NULL,        'v' },
   { NULL, 0, NULL, 0 }
};
#endif

#define SHORT_OPTIONS "c:C:dhj:v"

/**
 * Print usage info
 */
static void usage(bool showVersion)
{
   if (showVersion)
   {
      _tprintf(
         _T("NetXMS Reporting Server  Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_VERSION_BUILD_STRING _T(" (") NETXMS_BUILD_TAG _T(")\n")
         _T("Copyright (c) 2017 Raden Solutions\n\n"));
   }

   _tprintf(
      _T("Usage: nxreportd [OPTIONS]\n")
      _T("  \n")
      _T("Options:\n")
#if HAVE_GETOPT_LONG
      _T("  -c, --config <file>         Configuration file to use.\n")
      _T("  -C, --classpath <path>      Additional Java class path.\n")
      _T("  -d, --daemon                Run as daemon (service).\n")
      _T("  -h, --help                  Display this help message.\n")
      _T("  -j, --jre <path>            Specify JRE location.\n")
      _T("  -v, --version               Display version information.\n\n")
#else
      _T("  -c <file>      Configuration file to use.\n")
      _T("  -C <path>      Additional Java class path.\n")
      _T("  -d             Run as daemon (service).\n")
      _T("  -h             Display this help message.\n")
      _T("  -j <path>      Specify JRE location.\n")
      _T("  -v             Display version information.\n\n")
#endif
      );
}

/**
 * Load configuration file
 */
static bool LoadConfig()
{
#if !defined(_WIN32)
   if (!_tcscmp(s_optConfig, _T("{search}")))
   {
      TCHAR path[MAX_PATH] = _T("");
      const TCHAR *homeDir = _tgetenv(_T("NETXMS_HOME"));
      if (homeDir != NULL)
      {
         _sntprintf(path, MAX_PATH, _T("%s/etc/netxmsd.conf"), homeDir);
      }
      if ((path[0] != 0) && (_taccess(path, 4) == 0))
      {
         s_optConfig = _tcsdup(path);
      }
      else if (_taccess(PREFIX _T("/etc/netxmsd.conf"), 4) == 0)
      {
         s_optConfig = PREFIX _T("/etc/netxmsd.conf");
      }
      else if (_taccess(_T("/usr/etc/netxmsd.conf"), 4) == 0)
      {
         s_optConfig = _T("/usr/etc/netxmsd.conf");
      }
      else
      {
         s_optConfig = _T("/etc/netxmsd.conf");
      }
   }
#endif


}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   int ret = 0;

   InitNetXMSProcess(false);

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
         case 'c': // config
#ifdef UNICODE
            s_optConfig = WideStringFromMBStringSysLocale(optarg);
#else
            s_optConfig = optarg;
#endif
            break;
         case 'C': // classpath
            s_optClassPath = optarg;
            break;
         case 'd': // daemon
            s_optDaemon = true;
            break;
         case 'h': // help
            usage(true);
            exit(0);
            break;
         case 'j': // JRE
            s_optJre = optarg;
            break;
         case 'v': // version
            _tprintf(
               _T("NetXMS Reporting Server  Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_VERSION_BUILD_STRING _T(" (") NETXMS_BUILD_TAG _T(")\n")
               _T("Copyright (c) 2017 Raden Solutions\n\n"));
            exit(0);
            break;
         case '?':
            exit(1);
            break;
      }
   }

   if (!LoadConfig())
      return 1;

   if (s_optDaemon)
   {
      return 127;
   }
   else
   {
      return StartApp();
   }
}
