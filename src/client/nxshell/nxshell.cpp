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
#include <jni.h>

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

/**
 * Options
 */
static const char *s_optHost = "127.0.0.1";
static const char *s_optUser = "admin";
static const char *s_optPassword = "";
static const char *s_optJre = NULL;
static const char *s_optClassPath = NULL;

/**
 * Prototype for JNI_CreateJavaVM
 */
typedef jint (JNICALL *T_JNI_CreateJavaVM)(JavaVM **, void **, void *);

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
   classpath.append(_T("nxshell.jar"));
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

   char server[256], login[256], password[256];
   snprintf(server, 256, "-Dnetxms.server=%s", s_optHost);
   snprintf(login, 256, "-Dnetxms.login=%s", s_optUser);

   char clearPassword[128];
   DecryptPasswordA(s_optUser, s_optPassword, clearPassword, 128);
   snprintf(password, 256, "-Dnetxms.password=%s", clearPassword);

#ifdef UNICODE
   vmOptions[0].optionString = classpath.getUTF8String();
#else
   vmOptions[0].optionString = strdup(classpath);
#endif
   vmOptions[1].optionString = server;
   vmOptions[2].optionString = login;
   vmOptions[3].optionString = password;

   vmArgs.version = JNI_VERSION_1_6;
   vmArgs.options = vmOptions;
   vmArgs.nOptions = 4;
   vmArgs.ignoreUnrecognized = JNI_TRUE;

   int rc = 4;
   T_JNI_CreateJavaVM CreateJavaVM = (T_JNI_CreateJavaVM)DLGetSymbolAddr(jvmModule, "JNI_CreateJavaVM", NULL);
   if (CreateJavaVM != NULL)
   {
      JavaVM *jvm = NULL;
      JNIEnv *jniEnv;
      if (CreateJavaVM(&jvm, (void **)&jniEnv, &vmArgs) == JNI_OK)
      {
         jclass shell = jniEnv->FindClass("org/netxms/Shell");
         if (shell != NULL)
         {
            jmethodID shellMain = jniEnv->GetStaticMethodID(shell, "main", "([Ljava/lang/String;)V");
            if (shellMain != NULL)
            {
               jclass stringClass = jniEnv->FindClass("java/lang/String");
               jobjectArray jargs = jniEnv->NewObjectArray(argc, stringClass, NULL);
               for(int i = 0; i < argc; i++)
               {
                  jchar *tmp = (jchar *)UCS2StringFromMBString(argv[i]);
                  jstring js = jniEnv->NewString(tmp, (jsize)ucs2_strlen((UCS2CHAR *)tmp));
                  free(tmp);
                  if (js != NULL)
                  {
                     jniEnv->SetObjectArrayElement(jargs, i, js);
                     jniEnv->DeleteLocalRef(js);
                  }
               }
               jniEnv->CallStaticVoidMethod(shell, shellMain, jargs);
               rc = 0;
            }
            else
            {
               _tprintf(_T("Cannot find shell entry point\n"));
            }
         }
         else
         {
            _tprintf(_T("Cannot find shell class\n"));
         }
         jvm->DestroyJavaVM();
      }
      else
      {
         _tprintf(_T("CreateJavaVM failed\n"));
      }
   }
   else
   {
      _tprintf(_T("JNI_CreateJavaVM failed\n"));
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
	{ (char *)"classpath",      required_argument, NULL,        'C' },
	{ (char *)"help",           no_argument,       NULL,        'h' },
	{ (char *)"host",           required_argument, NULL,        'H' },
	{ (char *)"jre",            required_argument, NULL,        'j' },
	{ (char *)"password",       required_argument, NULL,        'P' },
	{ (char *)"user",           required_argument, NULL,        'u' },
	{ (char *)"version",        no_argument,       NULL,        'v' },
	{ NULL, 0, NULL, 0 }
};
#endif

#define SHORT_OPTIONS "C:hH:j:P:u:v"

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
      _T("  -h, --help                  Display this help message.\n")
      _T("  -H, --host <hostname>       Specify host name or IP address.\n")
      _T("  -j, --jre <path>            Specify JRE location.\n")
      _T("  -P, --password <password>   Specify user's password. Default is empty.\n")
      _T("  -u, --user <user>           Login to server as user. Default is \"admin\".\n")
      _T("  -v, --version               Display version information.\n\n")
#else
      _T("  -C <path>      Additional Java class path.\n")
      _T("  -h             Display this help message.\n")
      _T("  -H <hostname>  Specify host name or IP address.\n")
      _T("  -j <path>      Specify JRE location.\n")
      _T("  -P <password>  Specify user's password. Default is empty.\n")
      _T("  -u <user>      Login to server as user. Default is \"admin\".\n")
      _T("  -v             Display version information.\n\n")
#endif
      );
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
		   case 'h': // help
			   usage(true);
			   exit(0);
			   break;
		   case 'H': // host
			   s_optHost = optarg;
			   break;
		   case 'j': // host
			   s_optJre = optarg;
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
