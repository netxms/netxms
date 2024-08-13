/*
** NetXMS platform subagent for Windows
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
** File: prod.cpp
**
**/

#include "winnt_subagent.h"

/**
 * Read product property
 */
static void ReadProductProperty(HKEY hKey, const TCHAR *propName, Table *table, int column)
{
   TCHAR buffer[1024];
   DWORD type, size = sizeof(buffer);
   if (RegQueryValueEx(hKey, propName, nullptr, &type, reinterpret_cast<BYTE*>(buffer), &size) == ERROR_SUCCESS)
      table->set(column, buffer);
}

/**
 * Read product information
 */
static void ReadProductInfo(HKEY hKey, const TCHAR *keyName, Table *table)
{
   TCHAR buffer[1024];
   DWORD type, size = sizeof(buffer);
   if (RegQueryValueEx(hKey, _T("DisplayName"), nullptr, &type, reinterpret_cast<BYTE*>(buffer), &size) != ERROR_SUCCESS)
      _tcscpy(buffer, keyName);

   TCHAR displayVersion[256];
   size = sizeof(displayVersion);
   if (RegQueryValueEx(hKey, _T("DisplayVersion"), nullptr, &type, reinterpret_cast<BYTE*>(displayVersion), &size) != ERROR_SUCCESS)
      displayVersion[0] = 0;

   // Check if product already known
   for(int i = 0; i < table->getNumRows(); i++)
      if (!_tcsicmp(table->getAsString(i, 0), buffer) && !_tcsicmp(table->getAsString(i, 1), displayVersion))
         return;

   table->addRow();
   table->set(0, buffer);
   table->set(1, displayVersion);
   ReadProductProperty(hKey, _T("Publisher"), table, 2);
   ReadProductProperty(hKey, _T("URLInfoAbout"), table, 4);

   size = sizeof(buffer);
   if (RegQueryValueEx(hKey, _T("InstallDate"), nullptr, &type, reinterpret_cast<BYTE*>(buffer), &size) == ERROR_SUCCESS)
   {
      _tcscat(buffer, _T("000000"));
      time_t t = ParseDateTime(buffer, 0);
      if (t != 0)
      {
         table->set(3, static_cast<int64_t>(t));
      }
   }

   bool uninstallCommandAvailable = false;
   size = sizeof(buffer);
   if (RegQueryValueEx(hKey, _T("QuietUninstallString"), nullptr, &type, reinterpret_cast<BYTE*>(buffer), &size) == ERROR_SUCCESS)
   {
      uninstallCommandAvailable = true;
   }
   else
   {
      size = sizeof(buffer);
      if (RegQueryValueEx(hKey, _T("UninstallString"), nullptr, &type, reinterpret_cast<BYTE*>(buffer), &size) == ERROR_SUCCESS)
         uninstallCommandAvailable = true;
   }
   if (uninstallCommandAvailable)
      table->set(6, keyName);
}

/**
 * Read products from registry
 */
static bool ReadProductsFromRegistry(Table *table, bool reg32)
{
   HKEY hRoot;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"), 0, KEY_READ | (reg32 ? KEY_WOW64_32KEY : KEY_WOW64_64KEY), &hRoot) != ERROR_SUCCESS)
      return false;

   DWORD index = 0;
   while(true)
   {
      TCHAR prodKeyName[MAX_PATH];
      DWORD prodKeyNameLen = MAX_PATH;
      if (RegEnumKeyEx(hRoot, index++, prodKeyName, &prodKeyNameLen, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
         break;

      HKEY hKey;
      if (RegOpenKeyEx(hRoot, prodKeyName, 0, KEY_READ | (reg32 ? KEY_WOW64_32KEY : KEY_WOW64_64KEY), &hKey) == ERROR_SUCCESS)
      {
         ReadProductInfo(hKey, prodKeyName, table);
         RegCloseKey(hKey);
      }
   }
   RegCloseKey(hRoot);
   return true;
}

/**
 * Handler for System.InstalledProducts table
 */
LONG H_InstalledProducts(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *)
{
   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   value->addColumn(_T("VERSION"), DCI_DT_STRING, _T("Version"), true);
   value->addColumn(_T("VENDOR"), DCI_DT_STRING, _T("Vendor"));
   value->addColumn(_T("DATE"), DCI_DT_INT64, _T("Install Date"));
   value->addColumn(_T("URL"), DCI_DT_STRING, _T("URL"));
   value->addColumn(_T("DESCRIPTION"), DCI_DT_STRING, _T("Description"));
   value->addColumn(_T("UNINSTALL_KEY"), DCI_DT_STRING, _T("Uninstall key"));

   bool success64 = ReadProductsFromRegistry(value, false);
   bool success32 = ReadProductsFromRegistry(value, true);

   return (success64 || success32) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Get uninstall command for product
 */
static bool GetProductUninstallCommand(const TCHAR *uninstallKey, bool reg32, TCHAR *command)
{
   StringBuffer keyName(_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"));
   keyName.append(uninstallKey);

   HKEY hKey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyName, 0, KEY_READ | (reg32 ? KEY_WOW64_32KEY : KEY_WOW64_64KEY), &hKey) != ERROR_SUCCESS)
      return false;

   bool success = false;
   TCHAR buffer[1024];
   DWORD type, size = sizeof(buffer);
   if (RegQueryValueEx(hKey, _T("QuietUninstallString"), nullptr, &type, reinterpret_cast<BYTE*>(command), &size) == ERROR_SUCCESS)
   {
      success = true;
   }
   else
   {
      size = MAX_PATH * sizeof(TCHAR);
      if (RegQueryValueEx(hKey, _T("UninstallString"), nullptr, &type, reinterpret_cast<BYTE*>(command), &size) == ERROR_SUCCESS)
      {
         success = true;
      }
   }
   RegCloseKey(hKey);
   return success;
}

/**
 * Agent action for uninstalling product
 */
uint32_t H_UninstallProduct(const shared_ptr<ActionExecutionContext>& context)
{
   if (!context->hasArgs())
      return ERR_BAD_ARGUMENTS;

   const TCHAR *uninstallKey = context->getArg(0);
   if (uninstallKey[0] == 0)
      return ERR_BAD_ARGUMENTS;

   TCHAR command[MAX_PATH];
   if (!GetProductUninstallCommand(uninstallKey, false, command) && !GetProductUninstallCommand(uninstallKey, true, command))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot find uninstall command for product key \"%s\""), uninstallKey);
      return ERR_INTERNAL_ERROR;
   }

   nxlog_debug_tag(DEBUG_TAG, 4, _T("Executing uninstall command \"%s\" for product key \"%s\""), command, uninstallKey);
   ProcessExecutor executor(command);
   if (!executor.execute())
      return ERR_EXEC_FAILED;

   return executor.waitForCompletion(120000) ? ERR_SUCCESS : ERR_REQUEST_TIMEOUT;
}
