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
** File: platform.cpp
**/

#include "libnxjava.h"

/**
 * Class:     org.netxms.bridge.Platform
 * Method:    getNetXMSDirectoryInternal
 * Signature: (I)Ljava/lang/String;
 */
static jstring JNICALL J_getNetXMSDirectoryInternal(JNIEnv *jenv, jclass jcls, jint type)
{
   TCHAR buffer[MAX_PATH];
   GetNetXMSDirectory(static_cast<nxDirectoryType>(type), buffer);
   return JavaStringFromCString(jenv, buffer);
}

/**
 * Class:     org.netxms.bridge.Platform
 * Method:    writeLog
 * Signature: (ILjava/lang/String;)V
 */
static void JNICALL J_writeLog(JNIEnv *jenv, jclass jcls, jint level, jstring jmessage)
{
   if (jmessage == NULL)
      return;

   TCHAR *message = CStringFromJavaString(jenv, jmessage);
   nxlog_write_generic((int)level, _T("%s"), message);
   free(message);
}

/**
 * Class:     org.netxms.bridge.Platform
 * Method:    writeDebugLog
 * Signature: (ILjava/lang/String;)V
 */
static void JNICALL J_writeDebugLog(JNIEnv *jenv, jclass jcls, jint level, jstring jmessage)
{
   if (jmessage == NULL)
      return;

   TCHAR *message = CStringFromJavaString(jenv, jmessage);
   nxlog_debug((int)level, _T("%s"), message);
   free(message);
}

/**
 * Native methods
 */
static JNINativeMethod s_jniNativeMethods[] =
{
   { (char *)"getNetXMSDirectoryInternal", (char *)"(I)Ljava/lang/String;", (void *)J_getNetXMSDirectoryInternal },
   { (char *)"writeDebugLog", (char *)"(ILjava/lang/String;)V", (void *)J_writeDebugLog },
   { (char *)"writeLog", (char *)"(ILjava/lang/String;)V", (void *)J_writeLog }
};

/**
 * Register native methods for Platform classes
 */
bool RegisterPlatformNatives(JNIEnv *env)
{
   jclass platformClass = CreateJavaClassGlobalRef(env, "org/netxms/bridge/Platform");
   if (platformClass == NULL)
      return false;

   if (env->RegisterNatives(platformClass, s_jniNativeMethods, (jint)(sizeof(s_jniNativeMethods) / sizeof (s_jniNativeMethods[0]))) != 0)
   {
      nxlog_write_generic(NXLOG_ERROR, _T("JavaBridge: Failed to register native methods for platform class"));
      return false;
   }

   return true;
}
