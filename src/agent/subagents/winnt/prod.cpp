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
static bool ResolveSidToUsername(const wchar_t *sidString, wchar_t *userName, size_t userNameSize)
{
   PSID pSid = nullptr;
   if (!ConvertStringSidToSidW(sidString, &pSid))
      return false;

   wchar_t name[256] = L"";
   wchar_t domain[256] = L"";
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

   // Check if package already known (duplicate detection - same name, version, and user)
   for(int i = 0; i < table->getNumRows(); i++)
      if (!_tcsicmp(table->getAsString(i, 0), displayName) &&
          !_tcsicmp(table->getAsString(i, 1), version) &&
          !_tcsicmp(table->getAsString(i, 7, L""), CHECK_NULL_EX(userName)))
         return;

   table->addRow();
   table->set(0, displayName);   // NAME
   table->set(1, version);       // VERSION

   wchar_t publisher[256] = L"";
   size = sizeof(publisher);
   if (RegQueryValueExW(hKey, L"Publisher", nullptr, &type, reinterpret_cast<BYTE*>(publisher), &size) == ERROR_SUCCESS)
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
 * Enable or disable a process privilege
 */
static bool EnablePrivilege(const TCHAR *privilege, bool enable)
{
   HANDLE hToken;
   if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
      return false;

   LUID luid;
   if (!LookupPrivilegeValue(nullptr, privilege, &luid))
   {
      CloseHandle(hToken);
      return false;
   }

   TOKEN_PRIVILEGES tp;
   tp.PrivilegeCount = 1;
   tp.Privileges[0].Luid = luid;
   tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

   bool success = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr) && (GetLastError() != ERROR_NOT_ALL_ASSIGNED);
   CloseHandle(hToken);
   return success;
}

/**
 * Check if a user's registry hive is already loaded in HKEY_USERS
 */
static bool IsHiveLoaded(const TCHAR *sidString)
{
   HKEY hKey;
   if (RegOpenKeyEx(HKEY_USERS, sidString, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
   {
      RegCloseKey(hKey);
      return true;
   }
   return false;
}

/**
 * Read Windows Store packages from all user profiles.
 * Returns false if any profile was skipped because the user's session was in transition
 * (NTUSER.DAT mounted but UsrClass.dat hive not yet mounted) - in that case the snapshot
 * is incomplete and the caller should signal a collection error so the server preserves
 * the previous snapshot rather than treating missing packages as removals.
 */
static bool ReadStorePackagesFromAllUsers(Table *table)
{
   // Enumerate all user profiles from ProfileList (includes logged-off users)
   HKEY hProfileList;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
       _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"),
       0, KEY_READ, &hProfileList) != ERROR_SUCCESS)
      return true;

   // Enable SE_BACKUP_NAME and SE_RESTORE_NAME privileges needed for RegLoadKey/RegUnloadKey
   bool backupPrivEnabled = EnablePrivilege(SE_BACKUP_NAME, true);
   bool restorePrivEnabled = EnablePrivilege(SE_RESTORE_NAME, true);

   bool complete = true;
   DWORD index = 0;
   while(true)
   {
      wchar_t sidName[256];
      DWORD sidNameLen = 256;
      if (RegEnumKeyExW(hProfileList, index++, sidName, &sidNameLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
         break;

      // Skip well-known short SIDs (e.g., S-1-5-18, S-1-5-19, S-1-5-20)
      if (_tcslen(sidName) < 20)
         continue;

      // Resolve SID to username
      wchar_t userName[512];
      const wchar_t *userNamePtr = nullptr;
      if (ResolveSidToUsername(sidName, userName, 512))
         userNamePtr = userName;

      // Windows Store package data is in the UsrClass.dat hive (Software\Classes subtree),
      // not in NTUSER.DAT. When a user is logged in, Windows mounts UsrClass.dat automatically
      // as HKEY_USERS\<SID>_Classes. For logged-out users, we need to load it manually.
      StringBuffer classesKeyName(sidName);
      classesKeyName.append(_T("_Classes"));

      bool classesHiveLoadedByUs = false;
      if (!IsHiveLoaded(classesKeyName))
      {
         // If NTUSER.DAT is mounted, the user has an active session and Windows is
         // expected to mount UsrClass.dat as well. If it is not mounted yet, the session
         // is in transition (logon/logoff). Loading UsrClass.dat manually here would
         // exclusively lock the file and break AppX per-user package attachment, causing
         // Store apps to fail to launch. Skip this profile and signal incomplete data.
         if (IsHiveLoaded(sidName))
         {
            nxlog_debug_tag(DEBUG_TAG, 5, L"Skipping classes hive for user %s (%s) - active session, classes hive not mounted yet",
               (userNamePtr != nullptr) ? userNamePtr : L"unknown", sidName);
            complete = false;
            continue;
         }

         // Classes hive is not loaded - load UsrClass.dat from the user's profile directory
         HKEY hProfileKey;
         if (RegOpenKeyExW(hProfileList, sidName, 0, KEY_READ, &hProfileKey) == ERROR_SUCCESS)
         {
            wchar_t profilePath[MAX_PATH];
            DWORD type, size = sizeof(profilePath);
            if (RegQueryValueExW(hProfileKey, L"ProfileImagePath", nullptr, &type, reinterpret_cast<BYTE*>(profilePath), &size) == ERROR_SUCCESS)
            {
               // Expand environment variables in profile path
               wchar_t expandedPath[MAX_PATH];
               ExpandEnvironmentStringsW(profilePath, expandedPath, MAX_PATH);

               StringBuffer usrClassDat(expandedPath);
               usrClassDat.append(L"\\AppData\\Local\\Microsoft\\Windows\\UsrClass.dat");

               if (RegLoadKeyW(HKEY_USERS, classesKeyName, usrClassDat) == ERROR_SUCCESS)
               {
                  classesHiveLoadedByUs = true;
                  nxlog_debug_tag(DEBUG_TAG, 6, L"Loaded classes registry hive for user %s (%s)", (userNamePtr != nullptr) ? userNamePtr : L"unknown", sidName);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 6, L"Cannot load classes registry hive for %s from %s", sidName, usrClassDat.cstr());
               }
            }
            RegCloseKey(hProfileKey);
         }
      }

      StringBuffer path(classesKeyName);
      path.append(L"\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppModel\\Repository\\Packages");
      ReadStorePackagesFromRegistryPath(HKEY_USERS, path, userNamePtr, table);

      if (classesHiveLoadedByUs)
      {
         RegUnLoadKey(HKEY_USERS, classesKeyName);
         nxlog_debug_tag(DEBUG_TAG, 6, L"Unloaded classes registry hive for %s", sidName);
      }
   }

   // Restore privileges
   if (backupPrivEnabled)
      EnablePrivilege(SE_BACKUP_NAME, false);
   if (restorePrivEnabled)
      EnablePrivilege(SE_RESTORE_NAME, false);

   RegCloseKey(hProfileList);
   return complete;
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
 * Read Windows Store packages from registry (all sources).
 * Returns false if the per-user enumeration was incomplete due to a session in transition.
 */
static bool ReadStorePackagesFromRegistry(Table *table)
{
   // Read from all user profiles (HKEY_USERS\<SID>\...)
   bool complete = ReadStorePackagesFromAllUsers(table);

   // Read provisioned (system-wide) packages
   ReadProvisionedStorePackages(table);

   return complete;
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
   bool storeComplete = true;
   OSVERSIONINFO v;
   v.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
#pragma warning(push)
#pragma warning(disable : 4996)
   GetVersionEx(&v);
#pragma warning(pop)
   if (v.dwMajorVersion >= 10)
   {
      CoInitializeEx(nullptr, COINIT_MULTITHREADED);
      storeComplete = ReadStorePackagesFromRegistry(value);
      CoUninitialize();
   }

   // Report collection error if Store package enumeration was incomplete - otherwise
   // server would see missing per-user packages as removals and re-add them on the next
   // successful poll, generating spurious uninstall/install events.
   if (!storeComplete)
      return SYSINFO_RC_ERROR;

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
