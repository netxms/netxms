/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
** File: nxweblauncher.cpp
**
**/

#include "nxweblauncher.h"
#include <nxconfig.h>

/**
 * Installation directory
 */
WCHAR g_installDir[MAX_PATH] = L"";

/**
 * Java executable
 */
WCHAR g_javaExecutable[MAX_PATH] = L"";

/**
 * Configuration file
 */
WCHAR g_configFile[MAX_PATH] = L"";

/**
 * Find Java executable. Search algorithm is following:
 * 1. Check for bundled JRE in jre
 * 2. Check for bundled JRE in $NETXMS_HOME/bin/jre (Windows)
 * 3. Check JRE location in registry
 * 3. Check $JAVA_HOME
 * 4. Check $JAVA_HOME/jre
 *
 * @param buffer buffer for result
 * @param size buffer size in characters
 * @return true on success
 */
static bool FindJavaExecutable()
{
   wcscpy(g_javaExecutable, g_installDir);
   wcslcat(g_javaExecutable, L"\\jre\\bin\\java.exe", MAX_PATH);
   nxlog_debug_tag(DEBUG_TAG_JAVA_RUNTIME, 3, _T("FindJavaRuntime: checking %s (executable path)"), g_javaExecutable);
   if (_waccess(g_javaExecutable, F_OK) == 0)
      return true;

   WCHAR jvm[MAX_PATH] = L"";

   // Use NETXMS_HOME
   WCHAR netxmsHome[MAX_PATH];
   if (GetEnvironmentVariableW(L"NETXMS_HOME", netxmsHome, MAX_PATH) > 0)
   {
      _snwprintf(jvm, MAX_PATH, L"%s\\bin\\jre\\bin\\java.exe", netxmsHome);
      nxlog_debug_tag(DEBUG_TAG_JAVA_RUNTIME, 3, _T("FindJavaRuntime: checking %s (NetXMS home)"), jvm);
   }

   if ((jvm[0] == 0) || (_waccess(jvm, F_OK) != 0))
   {
      HKEY hKey;
      if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\JavaSoft\\Java Runtime Environment", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
      {
         WCHAR currVersion[64];
         DWORD size = sizeof(currVersion);
         if (RegQueryValueExW(hKey, L"CurrentVersion", nullptr, nullptr, (BYTE *)currVersion, &size) == ERROR_SUCCESS)
         {
            HKEY hSubKey;
            if (RegOpenKeyExW(hKey, currVersion, 0, KEY_QUERY_VALUE, &hSubKey) == ERROR_SUCCESS)
            {
               size = (MAX_PATH - 20) * sizeof(WCHAR);
               if (RegQueryValueExW(hSubKey, L"JavaHome", nullptr, nullptr, (BYTE *)jvm, &size) == ERROR_SUCCESS)
               {
                  wcscat(jvm, L"\\bin\\java.exe");
                  nxlog_debug_tag(DEBUG_TAG_JAVA_RUNTIME, 3, _T("FindJavaRuntime: checking %s (registry)"), jvm);
               }
               RegCloseKey(hSubKey);
            }
         }
         RegCloseKey(hKey);
      }
   }

   // Check JAVA_HOME
   if ((jvm[0] == 0) || (_waccess(jvm, F_OK) != 0))
   {
      WCHAR javaHome[MAX_PATH];
      if (GetEnvironmentVariableW(L"JAVA_HOME", javaHome, MAX_PATH) > 0)
      {
         _snwprintf(jvm, MAX_PATH, L"%s\\bin\\java.exe", javaHome);
         nxlog_debug_tag(DEBUG_TAG_JAVA_RUNTIME, 3, _T("FindJavaRuntime: checking %s (JAVA_HOME)"), jvm);
         if (_waccess(jvm, F_OK) != 0)
         {
            _snwprintf(jvm, MAX_PATH, L"%s\\jre\\bin\\java.exe", javaHome);
            nxlog_debug_tag(DEBUG_TAG_JAVA_RUNTIME, 3, _T("FindJavaRuntime: checking %s (JAVA_HOME)"), jvm);
         }
      }
      else
      {
         jvm[0] = 0;
      }
   }

   // Search in PATH
   if ((jvm[0] == 0) || (_waccess(jvm, F_OK) != 0))
   {
      if (SearchPathW(nullptr, L"java.exe", nullptr, MAX_PATH, jvm, nullptr) > 0)
      {
         nxlog_debug_tag(DEBUG_TAG_JAVA_RUNTIME, 3, _T("FindJavaRuntime: checking %s (PATH)"), jvm);
      }
      else
      {
         jvm[0] = 0;
      }
   }

   if ((jvm[0] == 0) || (_waccess(jvm, F_OK) != 0))
      return false;

   wcslcpy(g_javaExecutable, jvm, MAX_PATH);
   return true;
}

/**
 * Run server
 */
int RunServer()
{
   Config config;
   config.loadConfig(g_configFile, L"Launcher");

   const WCHAR *logFileName = config.getValue(L"/Launcher/LauncherLogFile", L"%NXWEBUI_HOME%\\logs\\launcher.log");
   WCHAR expandedLogFileName[MAX_PATH];
   ExpandEnvironmentStringsW(logFileName, expandedLogFileName, MAX_PATH);
   nxlog_open(expandedLogFileName, 0);
   nxlog_set_debug_level(5);

   if (!FindJavaExecutable())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_JAVA_RUNTIME, L"Cannot find suitable Java runtime environment");
      nxlog_close();
      return 1;
   }
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_JAVA_RUNTIME, L"Using Java executable \"%s\"", g_javaExecutable);

   StringBuffer command;

   command.append(L"-Djetty.home=\"");
   command.append(g_installDir);
   command.append(L"\\jetty-home\"");

   command.append(L" -Djetty.base=\"");
   command.append(g_installDir);
   command.append(L"\\jetty-base\"");

   logFileName = config.getValue(L"/Launcher/JettyLogFile", L"%NXWEBUI_HOME%\\logs\\jetty.log");
   ExpandEnvironmentStringsW(logFileName, expandedLogFileName, MAX_PATH);
   command.append(L" -Djetty.logfile=\"");
   command.append(expandedLogFileName);
   command.append(L"\"");

   logFileName = config.getValue(L"/Launcher/ApplicationLogFile", L"%NXWEBUI_HOME%\\logs\\nxmc.log");
   ExpandEnvironmentStringsW(logFileName, expandedLogFileName, MAX_PATH);
   command.append(L" -Dnxmc.logfile=\"");
   command.append(expandedLogFileName);
   command.append(L"\"");

   const WCHAR *maxMem = config.getValue(L"/Launcher/JavaMaxMemory");
   if (maxMem != nullptr)
   {
      command.append(L" -Xmx");
      command.append(maxMem);
   }

   command.append(L" -jar \"");
   command.append(g_installDir);
   command.append(L"\\jetty-home\\start.jar\" jetty.server.stopAtShutdown=true stop.port=17003 stop.key=nxmc$jetty$key");

   nxlog_debug_tag(DEBUG_TAG_STARTUP, 1, _T("Java command line: %s"), command.cstr());

   int exitCode = RunJetty(command.getBuffer()) ? 0 : 2;

   nxlog_close();
   return exitCode;
}

/**
 * Entry point
 */
int wmain(int argc, WCHAR *argv[])
{
   HKEY hKey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\WebUI"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      DWORD size = MAX_PATH * sizeof(TCHAR);
      RegQueryValueEx(hKey, _T("LauncherConfigFile"), nullptr, nullptr, (BYTE *)g_configFile, &size);
      
      size = MAX_PATH * sizeof(TCHAR);
      RegQueryValueEx(hKey, _T("InstallDir"), nullptr, nullptr, (BYTE *)g_installDir, &size);
      RegCloseKey(hKey);
   }

   if (g_installDir[0] == 0)
   {
      GetModuleFileNameW(nullptr, g_installDir, MAX_PATH);
      WCHAR *fs = wcsrchr(g_installDir, L'\\');
      if (fs != nullptr)
         *fs = 0;
   }

   if (g_installDir[wcslen(g_installDir) - 1] == L'\\')
      g_installDir[wcslen(g_installDir) - 1] = 0;

   SetEnvironmentVariableW(L"NXWEBUI_HOME", g_installDir);

   if (g_configFile[0] == 0)
   {
      wcscpy(g_configFile, g_installDir);
      wcslcat(g_configFile, L"\\etc\\launcher.conf", MAX_PATH);
   }

   if (argc == 1)
      return RunServer();

   if (!wcsicmp(argv[1], L"service"))
   {
      InitService();
   }
   else if (!wcsicmp(argv[1], L"install"))
   {
      InstallService();
   }
   else if (!wcsicmp(argv[1], L"remove"))
   {
      RemoveService();
   }
   else if (!wcsicmp(argv[1], L"start"))
   {
      StartWebUIService();
   }
   else if (!wcsicmp(argv[1], L"stop"))
   {
      StopWebUIService();
   }
   else
   {
      wprintf(L"Invalid command line option \"%s\"\n", argv[1]);
      return 3;
   }
   return 0;
}
