/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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

/**
 * JVM module
 */
static HMODULE s_jvmModule = NULL;

/**
 * JVM
 */
static JavaVM *s_javaVM = NULL;

/**
 * Prototype for JNI_CreateJavaVM
 */
typedef jint (JNICALL *T_JNI_CreateJavaVM)(JavaVM **, void **, void *);

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
   TCHAR errorText[256];
   s_jvmModule = DLOpen(jvmPath, errorText);
   if (s_jvmModule == NULL)
   {
      nxlog_write_generic(NXLOG_ERROR, _T("JavaBridge: Unable to load JVM: %s"), errorText);
      return NXJAVA_JVM_LOAD_FAILED;
   }

   TCHAR libdir[MAX_PATH];
   GetNetXMSDirectory(nxDirLib, libdir);

   String classpath = _T("-Djava.class.path=");
   classpath.append(libdir);
   classpath.append(FS_PATH_SEPARATOR_CHAR);
   classpath.append(_T("netxms-java-bridge.jar"));
   if (jar != NULL)
   {
      classpath.append(JAVA_CLASSPATH_SEPARATOR);
      classpath.append(libdir);
      classpath.append(FS_PATH_SEPARATOR_CHAR);
      classpath.append(jar);
   }
   if (syslibs != NULL)
   {
      for(int i = 0; syslibs[i] != NULL; i++)
      {
         classpath.append(JAVA_CLASSPATH_SEPARATOR);
         classpath.append(libdir);
         classpath.append(FS_PATH_SEPARATOR_CHAR);
         classpath.append(syslibs[i]);
      }
   }
   if (usercp != NULL)
   {
      classpath.append(JAVA_CLASSPATH_SEPARATOR);
      classpath.append(usercp);
   }

   JavaVMInitArgs vmArgs;
   JavaVMOption options[128];
   memset(options, 0, sizeof(options));

#ifdef UNICODE
   options[0].optionString = classpath.getUTF8String();
#else
   options[0].optionString = strdup(classpath);
#endif

   if (vmOptions != NULL)
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

   vmArgs.version = JNI_VERSION_1_6;
   vmArgs.options = options;
   vmArgs.nOptions = (vmOptions != NULL) ? vmOptions->size() + 1 : 1;
   vmArgs.ignoreUnrecognized = JNI_TRUE;

   nxlog_debug(6, _T("JVM options:"));
   for(int i = 0; i < vmArgs.nOptions; i++)
      nxlog_debug(6, _T("    %hs"), vmArgs.options[i].optionString);

   JavaBridgeError result;

   T_JNI_CreateJavaVM JNI_CreateJavaVM = (T_JNI_CreateJavaVM)DLGetSymbolAddr(s_jvmModule, "JNI_CreateJavaVM", errorText);
   if (JNI_CreateJavaVM != NULL)
   {
      if (JNI_CreateJavaVM(&s_javaVM, (void **)env, &vmArgs) == JNI_OK)
      {
         RegisterConfigHelperNatives(*env);
         RegisterPlatformNatives(*env);
         nxlog_debug(2, _T("JavaBridge: Java VM created"));
         result = NXJAVA_SUCCESS;
      }
      else
      {
         nxlog_write_generic(NXLOG_ERROR, _T("JavaBridge: cannot create Java VM"));
         result = NXJAVA_CANNOT_CREATE_JVM;
      }
   }
   else
   {
      nxlog_write_generic(NXLOG_ERROR, _T("JavaBridge: cannot find JVM entry point (%s)"));
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
   if (s_javaVM != NULL)
   {
      s_javaVM->DestroyJavaVM();
      s_javaVM = NULL;
   }

   if (s_jvmModule != NULL)
   {
      DLClose(s_jvmModule);
      s_jvmModule = NULL;
   }
}

/**
 * Attach current thread to Java VM
 *
 * @return JNI environment for current thread or NULL on failure
 */
JNIEnv LIBNXJAVA_EXPORTABLE *AttachThreadToJavaVM()
{
   if (s_javaVM == NULL)
      return NULL;

   JNIEnv *env;
   if (s_javaVM->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL) != JNI_OK)
      return NULL;
   return env;
}

/**
 * Detach current thread from Java VM
 */
void LIBNXJAVA_EXPORTABLE DetachThreadFromJavaVM()
{
   if (s_javaVM != NULL)
      s_javaVM->DetachCurrentThread();
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
 * @param argv pointers to arguments in (expected to be encoded using system locale code page)
 * @return NXJAVA_SUCCESS on success or appropriate error code
 */
JavaBridgeError LIBNXJAVA_EXPORTABLE StartJavaApplication(JNIEnv *env, const char *appClass, int argc, char **argv)
{
   jclass app = env->FindClass(appClass);
   if (app == NULL)
      return NXJAVA_APP_CLASS_NOT_FOUND;

   nxlog_debug(5, _T("Application class found"));
   jmethodID appMain = env->GetStaticMethodID(app, "main", "([Ljava/lang/String;)V");
   if (appMain == NULL)
      return NXJAVA_APP_ENTRY_POINT_NOT_FOUND;

   nxlog_debug(5, _T("Shell main method found"));
   jclass stringClass = env->FindClass("java/lang/String");
   jobjectArray jargs = env->NewObjectArray(argc, stringClass, NULL);
   for(int i = 0; i < argc; i++)
   {
      jstring js = JavaStringFromCStringSysLocale(env, argv[i]);
      if (js != NULL)
      {
         env->SetObjectArrayElement(jargs, i, js);
         env->DeleteLocalRef(js);
      }
   }
   env->CallStaticVoidMethod(app, appMain, jargs);
   env->DeleteLocalRef(jargs);
   return NXJAVA_SUCCESS;
}
