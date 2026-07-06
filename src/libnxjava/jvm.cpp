/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: jvm.cpp
**/

#include "libnxjava.h"
#include <netxms-version.h>

/**
 * JVM module
 */
static HMODULE s_jvmModule = nullptr;

/**
 * JVM
 */
static JavaVM *s_javaVM = nullptr;

/**
 * Prototype for JNI_CreateJavaVM
 */
typedef jint (JNICALL *T_JNI_CreateJavaVM)(JavaVM **, void **, void *);

#ifdef _WIN32

/**
 * Get base directory for portable installation (executable directory containing lib\java with Java bridge JAR)
 */
static bool GetPortableBaseDirectory(TCHAR *base)
{
   DWORD len = GetModuleFileName(nullptr, base, MAX_PATH);
   if ((len == 0) || (len >= MAX_PATH))   // len == MAX_PATH indicates truncation
      return false;

   TCHAR *p = _tcsrchr(base, _T('\\'));
   if (p == nullptr)
      return false;
   *p = 0;

   TCHAR marker[MAX_PATH];
   int rc = _sntprintf(marker, MAX_PATH, _T("%s\\lib\\java\\netxms-java-bridge-") NETXMS_JAR_VERSION _T(".jar"), base);
   if ((rc < 0) || (rc >= MAX_PATH))   // swprintf returns negative value on truncation
      return false;   // truncated marker path could falsely match an existing file
   return _taccess(marker, F_OK) == 0;
}

#endif

/**
 * Create Java virtual machine
 *
 * @param jvmPath path to JVM library
 * @param jar application JAR - path should be relative to NetXMS library directory (can be NULL)
 * @param usercp user defined class path (can be NULL)
 * @param vmOptions additional VM options
 * @param env points where JNI environment for current thread will be stored
 * @return true if VM created successfully
 */
JavaBridgeError LIBNXJAVA_EXPORTABLE CreateJavaVM(const TCHAR *jvmPath, const TCHAR *jar, const TCHAR **syslibs, const TCHAR *usercp, StringList *vmOptions, JNIEnv **env)
{
#ifdef _WIN32
   StringBuffer path(jvmPath);
   if (path.endsWith(_T("bin\\server\\jvm.dll")))
   {
      path.shrink(15);
      nxlog_debug_tag(DEBUG_TAG_JAVA_RUNTIME, 4, _T("JavaBridge: set DLL load directory to \"%s\""), path.cstr());
      SetDllDirectory(path);
   }
#endif

   TCHAR errorText[256];
   s_jvmModule = DLOpen(jvmPath, errorText);
   if (s_jvmModule == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_JAVA_RUNTIME, _T("JavaBridge: Unable to load JVM: %s"), errorText);
      return NXJAVA_JVM_LOAD_FAILED;
   }

   TCHAR libdir[MAX_PATH];
#ifdef _WIN32
   TCHAR baseDir[MAX_PATH];
   bool portableLayout = GetPortableBaseDirectory(baseDir);
   if (portableLayout)
   {
      nxlog_debug_tag(DEBUG_TAG_JAVA_RUNTIME, 4, _T("JavaBridge: using base directory \"%s\" (from executable path)"), baseDir);
      _sntprintf(libdir, MAX_PATH, _T("%s\\lib\\java"), baseDir);
   }
   else
   {
      GetNetXMSDirectory(nxDirLib, libdir);
      _tcslcat(libdir, FS_PATH_SEPARATOR _T("java"), MAX_PATH);
   }
#else
   GetNetXMSDirectory(nxDirLib, libdir);
   _tcslcat(libdir, FS_PATH_SEPARATOR _T("java"), MAX_PATH);
#endif

   StringBuffer classpath(_T("-Djava.class.path="));
   classpath.append(libdir);
   classpath.append(FS_PATH_SEPARATOR _T("netxms-java-bridge-") NETXMS_JAR_VERSION _T(".jar"));
   if (jar != nullptr)
   {
      classpath.append(JAVA_CLASSPATH_SEPARATOR);
      classpath.append(libdir);
      classpath.append(FS_PATH_SEPARATOR_CHAR);
      classpath.append(jar);
   }
   if (syslibs != nullptr)
   {
      for(int i = 0; syslibs[i] != nullptr; i++)
      {
         classpath.append(JAVA_CLASSPATH_SEPARATOR);
         classpath.append(libdir);
         classpath.append(FS_PATH_SEPARATOR_CHAR);
         classpath.append(syslibs[i]);
      }
   }
   if (usercp != nullptr)
   {
      classpath.append(JAVA_CLASSPATH_SEPARATOR);
      classpath.append(usercp);
   }

#ifdef _WIN32
   if (portableLayout)
      _sntprintf(libdir, MAX_PATH, _T("%s\\etc"), baseDir);
   else
      GetNetXMSDirectory(nxDirEtc, libdir);
#else
   GetNetXMSDirectory(nxDirEtc, libdir);
#endif
   classpath.append(JAVA_CLASSPATH_SEPARATOR);
   classpath.append(libdir);

   JavaVMOption options[128];
   memset(options, 0, sizeof(options));
#ifdef UNICODE
   options[0].optionString = classpath.getUTF8String();
#else
   options[0].optionString = MemCopyStringA(classpath);
#endif

   if (vmOptions != nullptr)
   {
      for(int i = 0; i < vmOptions->size(); i++)
      {
#ifdef UNICODE
         options[i + 1].optionString = UTF8StringFromWideString(vmOptions->get(i));
#else
         options[i + 1].optionString = UTF8StringFromMBString(vmOptions->get(i));
#endif
      }
   }

   JavaVMInitArgs vmArgs;
   vmArgs.version = JNI_VERSION_1_8;
   vmArgs.options = options;
   vmArgs.nOptions = (vmOptions != nullptr) ? vmOptions->size() + 1 : 1;
   vmArgs.ignoreUnrecognized = JNI_TRUE;

   nxlog_debug_tag(DEBUG_TAG_JAVA_RUNTIME, 6, _T("JVM options:"));
   for(int i = 0; i < vmArgs.nOptions; i++)
      nxlog_debug_tag(DEBUG_TAG_JAVA_RUNTIME, 6, _T("    %hs"), vmArgs.options[i].optionString);

   JavaBridgeError result;

   T_JNI_CreateJavaVM JNI_CreateJavaVM = (T_JNI_CreateJavaVM)DLGetSymbolAddr(s_jvmModule, "JNI_CreateJavaVM", errorText);
   if (JNI_CreateJavaVM != nullptr)
   {
      if (JNI_CreateJavaVM(&s_javaVM, (void **)env, &vmArgs) == JNI_OK)
      {
         RegisterConfigHelperNatives(*env);
         RegisterPlatformNatives(*env);
         nxlog_debug_tag(DEBUG_TAG_JAVA_RUNTIME, 2, _T("JavaBridge: Java VM created"));
         result = NXJAVA_SUCCESS;
      }
      else
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_JAVA_RUNTIME, _T("JavaBridge: cannot create Java VM"));
         result = NXJAVA_CANNOT_CREATE_JVM;
      }
   }
   else
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_JAVA_RUNTIME, _T("JavaBridge: cannot find JVM entry point (%s)"), errorText);
      result = NXJAVA_NO_ENTRY_POINT;
   }

   if (result != NXJAVA_SUCCESS)
      DestroyJavaVM();
   return result;
}

/**
 * Destroy Java virtual machine
 */
void LIBNXJAVA_EXPORTABLE DestroyJavaVM()
{
   if (s_javaVM != nullptr)
   {
      s_javaVM->DestroyJavaVM();
      s_javaVM = nullptr;
   }

   if (s_jvmModule != nullptr)
   {
      DLClose(s_jvmModule);
      s_jvmModule = nullptr;
   }
}

/**
 * Attach current thread to Java VM
 *
 * @return JNI environment for current thread or NULL on failure
 */
JNIEnv LIBNXJAVA_EXPORTABLE *AttachThreadToJavaVM()
{
   if (s_javaVM == nullptr)
      return nullptr;

   JNIEnv *env;
   if (s_javaVM->AttachCurrentThread(reinterpret_cast<void **>(&env), nullptr) != JNI_OK)
      return nullptr;
   return env;
}

/**
 * Detach current thread from Java VM
 */
void LIBNXJAVA_EXPORTABLE DetachThreadFromJavaVM()
{
   if (s_javaVM != nullptr)
      s_javaVM->DetachCurrentThread();
}

/**
 * Log pending Java exception and clear exception state
 */
static void LogPendingJavaException(JNIEnv *env)
{
   jthrowable exception = env->ExceptionOccurred();
   if (exception == nullptr)
      return;

   env->ExceptionClear();

   bool logged = false;
   jclass throwableClass = env->FindClass("java/lang/Throwable");
   if (throwableClass != nullptr)
   {
      jmethodID toString = env->GetMethodID(throwableClass, "toString", "()Ljava/lang/String;");
      if (toString != nullptr)
      {
         jstring message = static_cast<jstring>(env->CallObjectMethod(exception, toString));
         if (env->ExceptionCheck())
         {
            // toString() itself thrown - discard secondary exception and use fallback message
            env->ExceptionClear();
         }
         else if (message != nullptr)
         {
            TCHAR buffer[1024];
            CStringFromJavaString(env, message, buffer, 1024);
            if (env->ExceptionCheck())
            {
               // string conversion failed (likely OutOfMemoryError) - discard and use fallback message
               env->ExceptionClear();
            }
            else
            {
               nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_JAVA_RUNTIME, _T("Java exception: %s"), buffer);
               logged = true;
            }
            env->DeleteLocalRef(message);
         }
      }
      env->DeleteLocalRef(throwableClass);
   }

   if (!logged)
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_JAVA_RUNTIME, _T("Java exception (unable to obtain description)"));

   // FindClass()/GetMethodID() may leave a pending exception on failure - honor the "clear exception state" contract
   env->ExceptionClear();
   env->DeleteLocalRef(exception);
}

/**
 * Start Java application. This is helper method for starting Java applications
 * from command line wrapper. Application class should contain entry point with
 * the following signature:
 *
 * public static void main(String[])
 *
 * @param env JNI environment for current thread
 * @param appClass application class name
 * @param argc number of command line arguments
 * @param argv pointers to arguments (expected to be encoded using system locale code page) or nullptr if wide character version is used
 * @param wargv pointers to arguments encoded as wide character strings
 * @return NXJAVA_SUCCESS on success or appropriate error code
 */
JavaBridgeError LIBNXJAVA_EXPORTABLE StartJavaApplication(JNIEnv *env, const char *appClass, int argc, char **argv, WCHAR **wargv)
{
   jclass app = env->FindClass(appClass);
   if (app == nullptr)
   {
      LogPendingJavaException(env);
      return NXJAVA_APP_CLASS_NOT_FOUND;
   }

   nxlog_debug_tag(DEBUG_TAG_JAVA_RUNTIME, 5, _T("Application class found"));
   jmethodID appMain = env->GetStaticMethodID(app, "main", "([Ljava/lang/String;)V");
   if (appMain == nullptr)
   {
      LogPendingJavaException(env);
      return NXJAVA_APP_ENTRY_POINT_NOT_FOUND;
   }

   nxlog_debug_tag(DEBUG_TAG_JAVA_RUNTIME, 5, _T("Application main method found"));
   jclass stringClass = env->FindClass("java/lang/String");
   jobjectArray jargs = env->NewObjectArray(argc, stringClass, nullptr);
   for(int i = 0; i < argc; i++)
   {
      jstring js = (argv != nullptr) ? JavaStringFromCStringSysLocale(env, argv[i]) : JavaStringFromCStringW(env, wargv[i]);
      if (js != nullptr)
      {
         env->SetObjectArrayElement(jargs, i, js);
         env->DeleteLocalRef(js);
      }
   }
   env->CallStaticVoidMethod(app, appMain, jargs);
   JavaBridgeError result = NXJAVA_SUCCESS;
   if (env->ExceptionCheck())
   {
      LogPendingJavaException(env);
      result = NXJAVA_APP_EXECUTION_FAILED;
   }
   env->DeleteLocalRef(jargs);
   nxlog_debug_tag(DEBUG_TAG_JAVA_RUNTIME, 5, _T("Application main method exited"));
   return result;
}
