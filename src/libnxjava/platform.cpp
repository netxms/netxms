/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
 * Signature: (Ljava/lang/String;ILjava/lang/String;)V
 */
static void JNICALL J_writeLog(JNIEnv *jenv, jclass jcls, jstring jtag, jint level, jstring jmessage)
{
   if (jmessage == nullptr)
      return;

   TCHAR *message = CStringFromJavaString(jenv, jmessage);
   if (jtag != nullptr)
   {
      TCHAR *tag = CStringFromJavaString(jenv, jtag);
      nxlog_write_tag(static_cast<int>(level), tag, _T("%s"), message);
      MemFree(tag);
   }
   else
   {
      nxlog_write_tag(static_cast<int>(level), _T("java"), _T("%s"), message);
   }
   MemFree(message);
}

/**
 * Class:     org.netxms.bridge.Platform
 * Method:    writeDebugLog
 * Signature: (Ljava/lang/String;ILjava/lang/String;)V
 */
static void JNICALL J_writeDebugLog(JNIEnv *jenv, jclass jcls, jstring jtag, jint level, jstring jmessage)
{
   if (jmessage == nullptr)
      return;

   TCHAR *message = CStringFromJavaString(jenv, jmessage);
   if (jtag != nullptr)
   {
      TCHAR *tag = CStringFromJavaString(jenv, jtag);
      nxlog_debug_tag(tag, static_cast<int>(level), _T("%s"), message);
      MemFree(tag);
   }
   else
   {
      nxlog_debug_tag(_T("java"), static_cast<int>(level), _T("%s"), message);
   }
   MemFree(message);
}

/**
 * Native methods
 */
static JNINativeMethod s_jniNativeMethods[] =
{
   { (char *)"getNetXMSDirectoryInternal", (char *)"(I)Ljava/lang/String;", (void *)J_getNetXMSDirectoryInternal },
   { (char *)"writeDebugLog", (char *)"(Ljava/lang/String;ILjava/lang/String;)V", (void *)J_writeDebugLog },
   { (char *)"writeLog", (char *)"(Ljava/lang/String;ILjava/lang/String;)V", (void *)J_writeLog }
};

/**
 * Register native methods for Platform classes
 */
bool RegisterPlatformNatives(JNIEnv *env)
{
   jclass platformClass = CreateJavaClassGlobalRef(env, "org/netxms/bridge/Platform");
   if (platformClass == nullptr)
      return false;

   if (env->RegisterNatives(platformClass, s_jniNativeMethods, (jint)(sizeof(s_jniNativeMethods) / sizeof (s_jniNativeMethods[0]))) != 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_JAVA_BRIDGE, _T("Failed to register native methods for platform class"));
      return false;
   }

   return true;
}
