/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
** File: java.cpp
**
**/

#include "nxcore.h"
#include <nxjava.h>

#define DEBUG_TAG_JAVA  _T("java")

/**
 * JVM path
 */
#ifdef _WIN32
TCHAR g_jvmPath[MAX_PATH] = _T("jvm.dll");
#else
TCHAR g_jvmPath[MAX_PATH] = _T("libjvm.so");
#endif

/**
 * JVM options
 */
TCHAR *g_jvmOptions = NULL;

/**
 * User classpath
 */
TCHAR *g_userClasspath = NULL;

/**
 * Global reference to bridge class
 */
static jclass s_bridgeClass = NULL;

/**
 * Register native methods for given class
 */
static bool RegisterNatives(JNIEnv *env, const char *className, JNINativeMethod *methods, jint length)
{
   jclass classRef = CreateJavaClassGlobalRef(env, className);
   if (classRef == NULL)
      return false;

   return env->RegisterNatives(classRef, methods, length) == 0;
}

/**
 * Class:     org.netxms.server.ServerConfiguration
 * Method:    read
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
static jstring JNICALL J_ServerConfiguration_read(JNIEnv *jenv, jclass jcls, jstring jname)
{
   if (jname == NULL)
      return NULL;

   TCHAR *name = CStringFromJavaString(jenv, jname);
   TCHAR buffer[MAX_DB_STRING];
   bool success = ConfigReadStr(name, buffer, MAX_DB_STRING, NULL);
   free(name);
   return success ? JavaStringFromCString(jenv, buffer) : NULL;
}

/**
 * Class:     org.netxms.server.ServerConfiguration
 * Method:    write
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
static void JNICALL J_ServerConfiguration_write(JNIEnv *jenv, jclass jcls, jstring jname, jstring jvalue)
{
   if ((jname == NULL) || (jvalue == NULL))
      return;

   TCHAR *name = CStringFromJavaString(jenv, jname);
   TCHAR *value = CStringFromJavaString(jenv, jvalue);
   ConfigWriteStr(name, value, true, true, false);
   free(name);
   free(value);
}

/**
 * Native methods for ServerConfiguration class
 */
static JNINativeMethod s_serverConfigurationNatives[] =
{
   { (char *)"read", (char *)"(Ljava/lang/String;)Ljava/lang/String;", (void *)J_ServerConfiguration_read },
   { (char *)"write", (char *)"(Ljava/lang/String;Ljava/lang/String;)V", (void *)J_ServerConfiguration_write }
};

/**
 * Initialize server Java components
 */
bool JavaCoreStart()
{
   nxlog_debug_tag(DEBUG_TAG_JAVA, 1, _T("Using JVM %s"), g_jvmPath);

   JNIEnv *env;
   JavaBridgeError err = CreateJavaVM(g_jvmPath, _T("netxms-server.jar"), NULL, g_userClasspath, NULL, &env);
   if (err != NXJAVA_SUCCESS)
   {
      nxlog_write(MSG_JVM_CREATION_FAILED, NXLOG_ERROR, "s", GetJavaBridgeErrorMessage(err));
      return false;
   }

   s_bridgeClass = CreateJavaClassGlobalRef(env, "org/netxms/server/ServerCore");
   if (s_bridgeClass == NULL)
   {
      nxlog_write(MSG_JVM_CREATION_FAILED, NXLOG_ERROR, "s", _T("Cannot find server main class"));
      return false;
   }

   jmethodID serverInit = env->GetStaticMethodID(s_bridgeClass, "initialize", "()Z");
   if (serverInit == NULL)
   {
      nxlog_write(MSG_JVM_CREATION_FAILED, NXLOG_ERROR, "s", _T("Cannot find required entry points in server main class"));
      return false;
   }

   jboolean success = env->CallStaticBooleanMethod(s_bridgeClass, serverInit);
   if (!success)
   {
      nxlog_write(MSG_JVM_CREATION_FAILED, NXLOG_ERROR, "s", _T("Java side initialization failure"));
      return false;
   }

   if (!RegisterNatives(env, "org/netxms/server/ServerConfiguration", s_serverConfigurationNatives, sizeof(s_serverConfigurationNatives) / sizeof(JNINativeMethod)))
   {
      nxlog_write(MSG_JVM_CREATION_FAILED, NXLOG_ERROR, "s", _T("Cannot register native methods for ServerConfiguration class"));
      return false;
   }

   nxlog_debug_tag(DEBUG_TAG_JAVA, 1, _T("Server JVM created"));
   return true;
}

/**
 * Initiate Java core shutdown
 */
void JavaCoreStop()
{
   JNIEnv *env = AttachThreadToJavaVM();
   jmethodID m = env->GetStaticMethodID(s_bridgeClass, "initiateShutdown", "()V");
   if (m != NULL)
   {
      env->CallStaticVoidMethod(s_bridgeClass, m);
   }
   DetachThreadFromJavaVM();
}

/**
 * Wait for Java core complete shutdown
 */
void JavaCoreWaitForShutdown()
{
   JNIEnv *env = AttachThreadToJavaVM();
   jmethodID m = env->GetStaticMethodID(s_bridgeClass, "shutdownCompleted", "()V");
   if (m != NULL)
   {
      env->CallStaticVoidMethod(s_bridgeClass, m);
   }
   DetachThreadFromJavaVM();
}
