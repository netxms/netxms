/*
** NetXMS platform subagent for Windows
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
   if (RegQueryValueEx(hKey, propName, NULL, &type, (BYTE *)buffer, &size) == ERROR_SUCCESS)
      table->set(column, buffer);
}

/**
 * Read product information
 */
static void ReadProductInfo(HKEY hKey, const TCHAR *keyName, Table *table)
{
   TCHAR buffer[1024];
   DWORD type, size = sizeof(buffer);
   if (RegQueryValueEx(hKey, _T("DisplayName"), NULL, &type, (BYTE *)buffer, &size) != ERROR_SUCCESS)
      _tcscpy(buffer, keyName);

   // Check if product already known
   for(int i = 0; i < table->getNumRows(); i++)
      if (!_tcsicmp(table->getAsString(i, 0), buffer))
         return;   // Product found

   table->addRow();
   table->set(0, buffer);
   ReadProductProperty(hKey, _T("DisplayVersion"), table, 1);
   ReadProductProperty(hKey, _T("Publisher"), table, 2);
   ReadProductProperty(hKey, _T("URLInfoAbout"), table, 4);

   size = sizeof(buffer);
   if (RegQueryValueEx(hKey, _T("InstallDate"), NULL, &type, (BYTE *)buffer, &size) == ERROR_SUCCESS)
   {
      _tcscat(buffer, _T("000000"));
      time_t t = ParseDateTime(buffer, 0);
      if (t != 0)
      {
         table->set(3, (INT64)t);
      }
   }
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
	value->addColumn(_T("NAME"));
	value->addColumn(_T("VERSION"));
	value->addColumn(_T("VENDOR"));
	value->addColumn(_T("DATE"));
	value->addColumn(_T("URL"));
	value->addColumn(_T("DESCRIPTION"));

   bool success64 = ReadProductsFromRegistry(value, false);
   bool success32 = ReadProductsFromRegistry(value, true);

   return (success64 || success32) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}
