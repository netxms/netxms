/* 
 ** Java-Bridge NetXMS subagent
 ** Copyright (C) 2013 TEMPEST a.s.
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
 ** File: java_subagent.h
 **
 **/

#ifndef _java_subagent_h
#define _java_subagent_h

#include <nms_util.h>
#include <nms_agent.h>
#include <jni.h>
#include <unicode.h>

/**
 * Convert Java string to C string.
 * Result string is dynamically allocated and must be disposed by calling free().
 */
inline TCHAR *CStringFromJavaString(JNIEnv *env, jstring jstr)
{
   const jchar *chars = env->GetStringChars(jstr, NULL);
   jsize len = env->GetStringLength(jstr);
   TCHAR *str = (TCHAR *)malloc((len + 1) * sizeof(TCHAR));
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
inline TCHAR *CStringFromJavaString(JNIEnv *env, jstring jstr, TCHAR *buffer, size_t bufferLen)
{
   const jchar *chars = env->GetStringChars(jstr, NULL);
   jsize len = env->GetStringLength(jstr);
#ifdef UNICODE
#if UNICODE_UCS4
   ucs2_to_ucs4(chars, min(len, bufferLen - 1), buffer, bufferLen);
#else
   memcpy(buffer, chars, min((size_t)len, bufferLen) * sizeof(WCHAR));
#endif
#else
   ucs2_to_mb(chars, min(len, bufferLen - 1), buffer, bufferLen);
#endif
   env->ReleaseStringChars(jstr, chars);
   buffer[min((size_t)len, bufferLen - 1)] = 0;
   return buffer;
}

/**
 * Convert C string to Java string.
 */
inline jstring JavaStringFromCString(JNIEnv *env, const TCHAR *str)
{
   jsize len = (jsize)_tcslen(str);
#ifdef UNICODE
#if UNICODE_UCS4
   jchar *tmp = (jchar *)UCS2StringFromUCS4String(str);
   jstring js = env->NewString(tmp, len);
   free(tmp);
#else
   jstring js = env->NewString((jchar *)str, len);
#endif
#else
   jchar *tmp = (jchar *)UCS2StringFromMBString(str);
   jstring js = env->NewString(tmp, len);
   free(tmp);
#endif
   return js;
}

/**
 * Create StringList from Java string array
 */
inline StringList *StringListFromJavaArray(JNIEnv *curEnv, jobjectArray a)
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

/**
 * Functions
 */
jclass CreateClassGlobalRef(JNIEnv *curEnv, const char *className);
bool RegisterConfigHelperNatives(JNIEnv *curEnv);
jobject CreateConfigInstance(JNIEnv *curEnv, Config *config);

#endif
