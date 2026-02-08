/*
** NetXMS platform subagent for Windows
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
#include <sddl.h>
#include <shlwapi.h>

/**
 * Resolve indirect string (e.g., @{PackageFullName?ms-resource://...})
 */
static void ResolveIndirectString(TCHAR *str, size_t strSize)
{
   if (str[0] != _T('@'))
      return;

   TCHAR resolved[512];
   if (SHLoadIndirectString(str, resolved, 512, nullptr) == S_OK)
   {
      _tcslcpy(str, resolved, strSize);
      return;
   }

   // Cannot resolve, try to cut at first underscore - in many cases it will produce usable name
   wchar_t *p = wcschr(str + 2, L'_');
   if (p != nullptr)
   {
      *p = 0;
      memmove(str, str + 2, (wcslen(str) - 1) * sizeof(wchar_t));
   }
}

/**
 * Resolve SID string to username
 */
static bool ResolveSidToUsername(const TCHAR *sidString, TCHAR *userName, size_t userNameSize)
{
   PSID pSid = nullptr;
   if (!ConvertStringSidToSid(sidString, &pSid))
      return false;

   TCHAR name[256] = _T("");
   TCHAR domain[256] = _T("");
   DWORD nameSize = 256;
   DWORD domainSize = 256;
   SID_NAME_USE sidType;

   bool success = LookupAccountSid(nullptr, pSid, name, &nameSize, domain, &domainSize, &sidType) != 0;
   LocalFree(pSid);

   if (success)
   {
      if (domain[0] != 0)
         _sntprintf(userName, userNameSize, _T("%s\\%s"), domain, name);
      else
         _tcslcpy(userName, name, userNameSize);
   }
   return success;
}

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
 * Read Store package information from registry
 */
static void ReadStorePackageInfo(HKEY hKey, const TCHAR *packageFullName, const TCHAR *userName, Table *table)
{
   TCHAR displayName[512];
   DWORD type, size = sizeof(displayName);
   if (RegQueryValueEx(hKey, _T("DisplayName"), nullptr, &type, reinterpret_cast<BYTE*>(displayName), &size) != ERROR_SUCCESS)
      return;  // Skip packages without display name

   // Resolve indirect string references (e.g., @{PackageFullName?ms-resource://...})
   ResolveIndirectString(displayName, 512);

   // Extract version from PackageID (format: Name_Version_Arch__PublisherId)
   TCHAR packageId[512] = _T("");
   TCHAR version[128] = _T("");
   size = sizeof(packageId);
   if (RegQueryValueEx(hKey, _T("PackageID"), nullptr, &type, reinterpret_cast<BYTE*>(packageId), &size) == ERROR_SUCCESS)
   {
      TCHAR *p = _tcschr(packageId, _T('_'));
      if (p != nullptr)
      {
         p++;
         TCHAR *v = _tcschr(p, _T('_'));
         if (v != nullptr)
            *v = 0;
         _tcslcpy(version, p, 128);
      }
   }

   // Check if package already known (duplicate detection)
   for(int i = 0; i < table->getNumRows(); i++)
      if (!_tcsicmp(table->getAsString(i, 0), displayName) && !_tcsicmp(table->getAsString(i, 1), version))
         return;

   table->addRow();
   table->set(0, displayName);   // NAME
   table->set(1, version);       // VERSION

   TCHAR publisher[256] = _T("");
   size = sizeof(publisher);
   if (RegQueryValueEx(hKey, _T("Publisher"), nullptr, &type, reinterpret_cast<BYTE*>(publisher), &size) == ERROR_SUCCESS)
      ResolveIndirectString(publisher, 256);
   table->set(2, publisher);     // VENDOR

   // DATE, URL, DESCRIPTION - not available in registry

   // Use APPX: prefix to identify Store apps
   StringBuffer uninstallKey(_T("APPX:"));
   uninstallKey.append(packageFullName);
   table->set(6, uninstallKey);  // UNINSTALL_KEY

   // Set user name for user-specific packages
   if (userName != nullptr)
      table->set(7, userName);   // USER
}

/**
 * Read Windows Store packages from a specific registry root
 */
static void ReadStorePackagesFromRegistryPath(HKEY hRootKey, const TCHAR *basePath, const TCHAR *userName, Table *table)
{
   HKEY hRoot;
   if (RegOpenKeyEx(hRootKey, basePath, 0, KEY_READ, &hRoot) != ERROR_SUCCESS)
      return;

   DWORD index = 0;
   while(true)
   {
      TCHAR packageName[512];
      DWORD packageNameLen = 512;
      if (RegEnumKeyEx(hRoot, index++, packageName, &packageNameLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
         break;

      HKEY hKey;
      if (RegOpenKeyEx(hRoot, packageName, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
      {
         ReadStorePackageInfo(hKey, packageName, userName, table);
         RegCloseKey(hKey);
      }
   }
   RegCloseKey(hRoot);
}

/**
 * Read Windows Store packages from all user profiles
 */
static void ReadStorePackagesFromAllUsers(Table *table)
{
   HKEY hUsers;
   if (RegOpenKeyEx(HKEY_USERS, nullptr, 0, KEY_READ, &hUsers) != ERROR_SUCCESS)
      return;

   DWORD index = 0;
   while(true)
   {
      TCHAR sidName[256];
      DWORD sidNameLen = 256;
      if (RegEnumKeyEx(hUsers, index++, sidName, &sidNameLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
         break;

      // Skip _Classes keys and well-known short SIDs
      if (_tcsstr(sidName, _T("_Classes")) != nullptr)
         continue;
      if (_tcslen(sidName) < 20)  // Real user SIDs are longer (e.g., S-1-5-21-...)
         continue;

      // Resolve SID to username
      TCHAR userName[512];
      const TCHAR *userNamePtr = nullptr;
      if (ResolveSidToUsername(sidName, userName, 512))
         userNamePtr = userName;

      StringBuffer path(sidName);
      path.append(_T("\\Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppModel\\Repository\\Packages"));
      ReadStorePackagesFromRegistryPath(HKEY_USERS, path, userNamePtr, table);
   }
   RegCloseKey(hUsers);
}

/**
 * Read provisioned (system-wide) Windows Store packages
 */
static void ReadProvisionedStorePackages(Table *table)
{
   HKEY hRoot;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
       _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Appx\\AppxAllUserStore\\Applications"),
       0, KEY_READ, &hRoot) != ERROR_SUCCESS)
      return;

   DWORD index = 0;
   while(true)
   {
      TCHAR packageName[512];
      DWORD packageNameLen = 512;
      if (RegEnumKeyEx(hRoot, index++, packageName, &packageNameLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
         break;

      // Provisioned packages use package name as key, extract name and version from it
      // Format: PackageFamilyName_version_arch__publisherId
      // Check for duplicate by package name
      bool duplicate = false;
      for(int i = 0; i < table->getNumRows(); i++)
      {
         const TCHAR *uninstallKey = table->getAsString(i, 6);
         if (uninstallKey != nullptr && _tcsstr(uninstallKey, packageName) != nullptr)
         {
            duplicate = true;
            break;
         }
      }
      if (duplicate)
         continue;

      // Parse package name to extract display name and version
      // Example: Microsoft.WindowsCalculator_11.2210.0.0_x64__8wekyb3d8bbwe
      TCHAR displayName[256];
      TCHAR version[64] = _T("");
      _tcslcpy(displayName, packageName, 256);

      TCHAR *p = _tcschr(displayName, _T('_'));
      if (p != nullptr)
      {
         *p = 0;
         p++;
         TCHAR *v = _tcschr(p, _T('_'));
         if (v != nullptr)
            *v = 0;
         _tcslcpy(version, p, 64);
      }

      table->addRow();
      table->set(0, displayName);  // NAME
      table->set(1, version);      // VERSION
      // VENDOR not available in provisioned package registry

      StringBuffer uninstallKey(_T("APPX:"));
      uninstallKey.append(packageName);
      table->set(6, uninstallKey);  // UNINSTALL_KEY
   }
   RegCloseKey(hRoot);
}

/**
 * Read Windows Store packages from registry (all sources)
 */
static void ReadStorePackagesFromRegistry(Table *table)
{
   // Read from all user profiles (HKEY_USERS\<SID>\...)
   ReadStorePackagesFromAllUsers(table);

   // Read provisioned (system-wide) packages
   ReadProvisionedStorePackages(table);
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
   value->addColumn(_T("USER"), DCI_DT_STRING, _T("User"));

   bool success64 = ReadProductsFromRegistry(value, false);
   bool success32 = ReadProductsFromRegistry(value, true);

   // Add Windows Store apps (Windows 10+) from all user profiles and provisioned packages
   OSVERSIONINFO v;
   v.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
#pragma warning(push)
#pragma warning(disable : 4996)
   GetVersionEx(&v);
#pragma warning(pop)
   if (v.dwMajorVersion >= 10)
   {
      CoInitializeEx(nullptr, COINIT_MULTITHREADED);
      ReadStorePackagesFromRegistry(value);
      CoUninitialize();
   }

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

   // Check for Windows Store app (cannot be uninstalled silently)
   if (_tcsncmp(uninstallKey, _T("APPX:"), 5) == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot uninstall Windows Store app \"%s\" - requires user interaction"), uninstallKey + 5);
      return ERR_FUNCTION_NOT_SUPPORTED;
   }

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
