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
** File: tools.cpp
**/

#include "libnxjava.h"

/**
 * Create global reference to Java class
 *
 * @param env JNI environment for current thread
 * @param className Java class name
 * @return Java class object
 */
jclass LIBNXJAVA_EXPORTABLE CreateJavaClassGlobalRef(JNIEnv *env, const char *className)
{
   if (env == nullptr)
      return nullptr;

   jclass c = env->FindClass(className);
   if (c == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_JAVA_BRIDGE, _T("Could not find class %hs"), className);
      return nullptr;
   }

   jclass gc = static_cast<jclass>(env->NewGlobalRef(c));
   env->DeleteLocalRef(c);

   if (gc == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_JAVA_BRIDGE, _T("Could not create global reference of class %s"), className);
      return nullptr;
   }

   return gc;
}

/**
 * Convert Java string to C string.
 * Result string is dynamically allocated and must be disposed by calling free().
 */
TCHAR LIBNXJAVA_EXPORTABLE *CStringFromJavaString(JNIEnv *env, jstring jstr)
{
   if (jstr == nullptr)
      return nullptr;
   const jchar *chars = env->GetStringChars(jstr, nullptr);
   jsize len = env->GetStringLength(jstr);
   TCHAR *str = MemAllocString(len + 1);
#ifdef UNICODE
#if UNICODE_UCS4
   ucs2_to_ucs4(chars, len, str, len + 1);
#else
   memcpy(str, chars, len * sizeof(WCHAR));
#endif
#else
   ucs2_to_mb(chars, len, str, len + 1);
#endif
   env->ReleaseStringChars(jstr, chars);
   str[len] = 0;
   return str;
}

/**
 * Convert Java string to C string.
 * Result string is in static buffer
 */
TCHAR LIBNXJAVA_EXPORTABLE *CStringFromJavaString(JNIEnv *env, jstring jstr, TCHAR *buffer, size_t bufferLen)
{
   if (jstr == nullptr)
      return nullptr;
   const jchar *chars = env->GetStringChars(jstr, nullptr);
   jsize len = env->GetStringLength(jstr);
#ifdef UNICODE
#if UNICODE_UCS4
   ucs2_to_ucs4(chars, std::min((size_t)len, bufferLen - 1), buffer, bufferLen);
#else
   memcpy(buffer, chars, std::min((size_t)len, bufferLen) * sizeof(WCHAR));
#endif
#else
   ucs2_to_mb(chars, std::min((size_t)len, bufferLen - 1), buffer, bufferLen);
#endif
   env->ReleaseStringChars(jstr, chars);
   buffer[std::min((size_t)len, bufferLen - 1)] = 0;
   return buffer;
}

/**
 * Convert wide character C string to Java string.
 */
jstring LIBNXJAVA_EXPORTABLE JavaStringFromCStringW(JNIEnv *env, const WCHAR *str)
{
   if (str == nullptr)
      return nullptr;
   jsize len = (jsize)wcslen(str);
#if UNICODE_UCS4
   jchar *tmp = (jchar *)UCS2StringFromUCS4String(str);
   jstring js = env->NewString(tmp, len);
   MemFree(tmp);
#else
   jstring js = env->NewString((jchar *)str, len);
#endif
   return js;
}

/**
 * Convert C string in current code page to Java string.
 */
jstring LIBNXJAVA_EXPORTABLE JavaStringFromCStringA(JNIEnv *env, const char *str)
{
   if (str == nullptr)
      return nullptr;
   jsize len = (jsize)strlen(str);
   jchar *tmp = (jchar *)UCS2StringFromMBString(str);
   jstring js = env->NewString(tmp, len);
   MemFree(tmp);
   return js;
}

/**
 * Convert C string in system locale code page to Java string.
 */
jstring LIBNXJAVA_EXPORTABLE JavaStringFromCStringSysLocale(JNIEnv *env, const char *str)
{
   if (str == nullptr)
      return nullptr;
   jsize len = (jsize)strlen(str);
#if UNICODE_UCS4
   WCHAR *wtmp = WideStringFromMBStringSysLocale(str);
   jchar *tmp = (jchar *)UCS2StringFromUCS4String(wtmp);
#else
   jchar *tmp = (jchar *)WideStringFromMBStringSysLocale(str);
#endif
   jstring js = env->NewString(tmp, len);
   MemFree(tmp);
   return js;
}

/**
 * Create StringList from Java string array
 */
StringList LIBNXJAVA_EXPORTABLE *StringListFromJavaArray(JNIEnv *curEnv, jobjectArray a)
{
   StringList *list = new StringList();
   jsize count = curEnv->GetArrayLength(a);
   for(jsize i = 0; i < count; i++)
   {
      jstring s = reinterpret_cast<jstring>(curEnv->GetObjectArrayElement(a, i));
      list->addPreallocated(CStringFromJavaString(curEnv, s));
      curEnv->DeleteLocalRef(s);
   }
   return list;
}
