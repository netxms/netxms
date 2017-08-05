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
** File: jre.cpp
**/

#include "libnxjava.h"

#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#ifndef _WIN32

/**
 * Check potential JVM path
 */
static bool CheckJvmPath(const char *base, const char *libdir, const char *arch, char *jvm, const TCHAR *description)
{
   static const char *jreType[] = { "server", "default", "j9vm", "classic", NULL };

   for(int i = 0; jreType[i] != NULL; i++)
   {
      snprintf(jvm, MAX_PATH, "%s%s/lib/%s/%s/libjvm.so", base, libdir, arch, jreType[i]);
      nxlog_debug(7, _T("FindJavaRuntime: checking %hs (%s)"), jvm, description);
      if (_access(jvm, 0) == 0)
         return true;

      snprintf(jvm, MAX_PATH, "%s%s/jre/lib/%s/%s/libjvm.so", base, libdir, arch, jreType[i]);
      nxlog_debug(7, _T("FindJavaRuntime: checking %hs (%s)"), jvm, description);
      if (_access(jvm, 0) == 0)
         return true;
   }

   if (!strcmp(arch, "x86_64"))
      return CheckJvmPath(base, libdir, "amd64", jvm, description);

   if (!strcmp(arch, "i686"))
      return CheckJvmPath(base, libdir, "i386", jvm, description);

   return false;
}

#endif

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
TCHAR LIBNXJAVA_EXPORTABLE *FindJavaRuntime(TCHAR *buffer, size_t size)
{
#ifdef _WIN32
   TCHAR path[MAX_PATH];
   GetModuleFileName(NULL, path, MAX_PATH);
   TCHAR *s = _tcsrchr(path, _T('\\'));
   if (s != NULL)
   {
      s++;
      _tcscpy(s, _T("jre\\bin\\server\\jvm.dll"));
      nxlog_debug(7, _T("FindJavaRuntime: checking %s (executable path)"), path);
      if (_taccess(path, 0) == 0)
      {
         nx_strncpy(buffer, path, size);
         return buffer;
      }
   }
#endif

   char jvm[MAX_PATH] = "";

#ifndef _WIN32
   struct utsname un;
   uname(&un);
#endif

   // Use NETXMS_HOME
   const char *netxmsHome = getenv("NETXMS_HOME");
   if ((netxmsHome != NULL) && (*netxmsHome != 0))
   {
#ifdef _WIN32
      snprintf(jvm, MAX_PATH, "%s\\bin\\jre\\bin\\server\\jvm.dll", netxmsHome);
      nxlog_debug(7, _T("FindJavaRuntime: checking %hs (NetXMS home)"), jvm);
#else
      CheckJvmPath(netxmsHome, "/lib", un.machine, jvm, _T("NetXMS home"));
#endif
   }

#ifdef _WIN32
   if ((jvm[0] == 0) || (_access(jvm, 0) != 0))
   {
      HKEY hKey;
      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\JavaSoft\\Java Runtime Environment"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
      {
         TCHAR currVersion[64];
         DWORD size = sizeof(currVersion);
         if (RegQueryValueEx(hKey, _T("CurrentVersion"), NULL, NULL, (BYTE *)currVersion, &size) == ERROR_SUCCESS)
         {
            HKEY hSubKey;
            if (RegOpenKeyEx(hKey, currVersion, 0, KEY_QUERY_VALUE, &hSubKey) == ERROR_SUCCESS)
            {
               size = MAX_PATH - 20;
               if (RegQueryValueExA(hSubKey, "JavaHome", NULL, NULL, (BYTE *)jvm, &size) == ERROR_SUCCESS)
               {
                  strcat(jvm, "\\bin\\server\\jvm.dll");
                  nxlog_debug(7, _T("FindJavaRuntime: checking %hs (registry)"), jvm);
               }
               RegCloseKey(hSubKey);
            }
         }
         RegCloseKey(hKey);
      }
   }
#endif

   // Check JAVA_HOME
   if ((jvm[0] == 0) || (_access(jvm, 0) != 0))
   {
      const char *javaHome = getenv("JAVA_HOME");
      if ((javaHome != NULL) && (*javaHome != 0))
      {
#ifdef _WIN32
         snprintf(jvm, MAX_PATH, "%s\\bin\\server\\jvm.dll", javaHome);
         nxlog_debug(7, _T("FindJavaRuntime: checking %hs (Java home)"), jvm);
         if (_access(jvm, 0) != 0)
         {
            snprintf(jvm, MAX_PATH, "%s\\jre\\bin\\server\\jvm.dll", javaHome);
            nxlog_debug(7, _T("FindJavaRuntime: checking %hs (Java home)"), jvm);
         }
#else
         CheckJvmPath(javaHome, "", un.machine, jvm, _T("Java home"));
#endif
      }
   }

#ifdef JDK_LOCATION
   if ((jvm[0] == 0) || (_access(jvm, 0) != 0))
   {
#ifdef _WIN32
      snprintf(jvm, MAX_PATH, JDK_LOCATION "\\jre\\bin\\server\\jvm.dll");
      nxlog_debug(7, _T("FindJavaRuntime: checking %hs (JDK defined at compile time)"), jvm);
#else
      CheckJvmPath(JDK_LOCATION, "", un.machine, jvm, _T("JDK defined at compile time"));
#endif
   }
#endif

   if ((jvm[0] == 0) || (_access(jvm, 0) != 0))
      return NULL;

#ifdef UNICODE
   MultiByteToWideChar(CP_UTF8, 0, jvm, -1, buffer, (int)size);
   buffer[size - 1] = 0;
#else
   nx_strncpy(buffer, jvm, size);
#endif
   return buffer;
}
