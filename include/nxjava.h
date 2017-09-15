/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: nxjava.h
**
**/

#ifndef _nxjava_h_
#define _nxjava_h_

#ifdef _WIN32
#ifdef LIBNXJAVA_EXPORTS
#define LIBNXJAVA_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXJAVA_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXJAVA_EXPORTABLE
#endif

#include <nms_common.h>
#include <nms_util.h>
#include <nxconfig.h>
#include <jni.h>
#include <unicode.h>

/**
 * Error codes
 */
enum JavaBridgeError
{
   NXJAVA_SUCCESS = 0,
   NXJAVA_JVM_LOAD_FAILED = 1,
   NXJAVA_NO_ENTRY_POINT = 2,
   NXJAVA_CANNOT_CREATE_JVM = 3,
   NXJAVA_APP_CLASS_NOT_FOUND = 4,
   NXJAVA_APP_ENTRY_POINT_NOT_FOUND = 5
};

/**
 * Get error message from error code
 */
const TCHAR LIBNXJAVA_EXPORTABLE *GetJavaBridgeErrorMessage(JavaBridgeError error);

/**
 * Convert Java string to C string.
 * Result string is dynamically allocated and must be disposed by calling free().
 */
TCHAR LIBNXJAVA_EXPORTABLE *CStringFromJavaString(JNIEnv *env, jstring jstr);

/**
 * Convert Java string to C string.
 * Result string is in static buffer
 */
TCHAR LIBNXJAVA_EXPORTABLE *CStringFromJavaString(JNIEnv *env, jstring jstr, TCHAR *buffer, size_t bufferLen);

/**
 * Convert wide character C string to Java string.
 */
jstring LIBNXJAVA_EXPORTABLE JavaStringFromCStringW(JNIEnv *env, const WCHAR *str);

/**
 * Convert C string in current code page to Java string.
 */
jstring LIBNXJAVA_EXPORTABLE JavaStringFromCStringA(JNIEnv *env, const char *str);

/**
 * Convert C string to Java string.
 */
#ifdef UNICODE
#define JavaStringFromCString JavaStringFromCStringW
#else
#define JavaStringFromCString JavaStringFromCStringA
#endif

/**
 * Convert C string in system locale code page to Java string.
 */
jstring LIBNXJAVA_EXPORTABLE JavaStringFromCStringSysLocale(JNIEnv *env, const char *str);

/**
 * Create StringList from Java string array
 */
StringList LIBNXJAVA_EXPORTABLE *StringListFromJavaArray(JNIEnv *curEnv, jobjectArray a);

/**
 * Find Java runtime module. Search algorithm is following:
 * 1. Windows only - check for bundled JRE in bin\jre
 * 2. Check for bundled JRE in $NETXMS_HOME/bin/jre (Windows) or $NETXMS_HOME/lib/jre (non-Windows)
 * 3. Windows only - check JRE location in registry
 * 3. Check $JAVA_HOME
 * 4. Check $JAVA_HOME/jre
 * 5. Check JDK location specified at compile time
 *
 * @param buffer buffer for result
 * @param size buffer size in characters
 * @return buffer on success or NULL on failure
 */
TCHAR LIBNXJAVA_EXPORTABLE *FindJavaRuntime(TCHAR *buffer, size_t size);

/**
 * Create Java virtual machine
 *
 * @param jvmPath path to JVM library
 * @param jar application JAR - path should be relative to NetXMS library directory (can be NULL)
 * @param syslibs list of system jar files (should be terminated with NULL pointer, can be NULL)
 * @param usercp user defined class path (can be NULL)
 * @param vmOptions additional VM options
 * @param env points where JNI environment for current thread will be stored
 * @return NXJAVA_SUCCESS if VM created successfully or appropriate error code
 */
JavaBridgeError LIBNXJAVA_EXPORTABLE CreateJavaVM(const TCHAR *jvmPath, const TCHAR *jar, const TCHAR **syslibs, const TCHAR *usercp, StringList *vmOptions, JNIEnv **env);

/**
 * Destroy Java virtual machine
 */
void LIBNXJAVA_EXPORTABLE DestroyJavaVM();

/**
 * Attach current thread to Java VM
 *
 * @return JNI environment for current thread or NULL on failure
 */
JNIEnv LIBNXJAVA_EXPORTABLE *AttachThreadToJavaVM();

/**
 * Detach current thread from Java VM
 */
void LIBNXJAVA_EXPORTABLE DetachThreadFromJavaVM();

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
JavaBridgeError LIBNXJAVA_EXPORTABLE StartJavaApplication(JNIEnv *env, const char *appClass, int argc, char **argv);

/**
 * Create global reference to Java class
 *
 * @param env JNI environment for current thread
 * @param className Java class name
 * @return Java class object
 */
jclass LIBNXJAVA_EXPORTABLE CreateJavaClassGlobalRef(JNIEnv *env, const char *className);

/**
 * Create Java Config object (wrapper around C++ Config class)
 *
 * @param env JNI environment for current thread
 * @param config C++ Config object
 * @return Java wrapper object or NULL on failure
 */
jobject LIBNXJAVA_EXPORTABLE CreateConfigJavaInstance(JNIEnv *env, Config *config);

#endif   /* _nxjava_h_ */
