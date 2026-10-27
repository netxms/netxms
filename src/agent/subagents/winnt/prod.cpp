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
 * Location of Store package repository within user's classes hive (UsrClass.dat)
 */
#define STORE_PACKAGES_PATH   L"Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppModel\\Repository\\Packages"

/**
 * Store package information
 */
struct StorePackage
{
   String fullName;
   String name;
   String version;
   String vendor;

   StorePackage(const wchar_t *_fullName, const wchar_t *_name, const wchar_t *_version, const wchar_t *_vendor) :
      fullName(_fullName), name(_name), version(_version), vendor(_vendor)
   {
   }

   bool equals(const StorePackage *p) const
   {
      return fullName.equals(p->fullName) && name.equals(p->name) && version.equals(p->version) && vendor.equals(p->vendor);
   }
};

/**
 * Last successfully read list of Store packages for user profile. Reported in place of live data when
 * profile's classes hive is not available (user logged off with roaming profile, FSLogix, UPD, etc.),
 * so that package list does not change with user's logon state.
 */
struct StoreProfileCacheEntry
{
   MutableString userName;
   ObjectArray<StorePackage> packages;

   StoreProfileCacheEntry(const wchar_t *_userName) : userName(_userName), packages(64, 64, Ownership::True)
   {
   }

   StoreProfileCacheEntry(const wchar_t *_userName, ObjectArray<StorePackage>&& _packages) : userName(_userName), packages(std::move(_packages))
   {
   }

   bool matches(const wchar_t *_userName, const ObjectArray<StorePackage>& _packages) const
   {
      if (wcscmp(userName, _userName) || (packages.size() != _packages.size()))
         return false;
      for(int i = 0; i < _packages.size(); i++)
      {
         const StorePackage *p = _packages.get(i);
         bool found = false;
         for(int j = 0; j < packages.size(); j++)
         {
            if (packages.get(j)->equals(p))
            {
               found = true;
               break;
            }
         }
         if (!found)
            return false;
      }
      return true;
   }
};

/**
 * Store package cache (profile SID -> last known package list)
 */
static StringObjectMap<StoreProfileCacheEntry> s_storeCache(Ownership::True);
static bool s_storeCacheLoaded = false;
static Mutex s_storeCacheLock;

/**
 * Load Store package cache from local database
 */
static void LoadStorePackageCache()
{
   DB_HANDLE hdb = AgentGetLocalDatabaseHandle();
   if (hdb == nullptr)
      return;

   DB_RESULT hResult = DBSelect(hdb, L"SELECT sid,user_name FROM appx_user_cache");
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         wchar_t sid[256], userName[512];
         DBGetField(hResult, i, 0, sid, 256);
         s_storeCache.set(sid, new StoreProfileCacheEntry(DBGetField(hResult, i, 1, userName, 512)));
      }
      DBFreeResult(hResult);
   }

   hResult = DBSelect(hdb, L"SELECT sid,package,name,version,vendor FROM appx_package_cache");
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         wchar_t sid[256];
         StoreProfileCacheEntry *entry = s_storeCache.get(DBGetField(hResult, i, 0, sid, 256));
         if (entry == nullptr)
            continue;

         wchar_t fullName[256], name[512], version[64], vendor[512];
         entry->packages.add(new StorePackage(DBGetField(hResult, i, 1, fullName, 256), DBGetField(hResult, i, 2, name, 512),
            DBGetField(hResult, i, 3, version, 64), DBGetField(hResult, i, 4, vendor, 512)));
      }
      DBFreeResult(hResult);
   }

   nxlog_debug_tag(DEBUG_TAG, 5, L"Store package cache loaded (%d user profiles)", s_storeCache.size());
}

/**
 * Execute query with profile SID as the only parameter
 */
static bool ExecuteQueryForProfile(DB_HANDLE hdb, const wchar_t *query, const wchar_t *sid)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, query);
   if (hStmt == nullptr)
      return false;
   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, sid, DB_BIND_STATIC);
   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Delete Store package cache entry from local database
 */
static void DeleteStorePackageCacheEntry(const wchar_t *sid)
{
   DB_HANDLE hdb = AgentGetLocalDatabaseHandle();
   if (hdb == nullptr)
      return;

   DBBegin(hdb);
   if (ExecuteQueryForProfile(hdb, L"DELETE FROM appx_package_cache WHERE sid=?", sid) &&
       ExecuteQueryForProfile(hdb, L"DELETE FROM appx_user_cache WHERE sid=?", sid))
      DBCommit(hdb);
   else
      DBRollback(hdb);
}

/**
 * Save Store package cache entry to local database
 */
static void SaveStorePackageCacheEntry(const wchar_t *sid, const StoreProfileCacheEntry *entry)
{
   DB_HANDLE hdb = AgentGetLocalDatabaseHandle();
   if (hdb == nullptr)
      return;

   DBBegin(hdb);
   bool success = ExecuteQueryForProfile(hdb, L"DELETE FROM appx_package_cache WHERE sid=?", sid) &&
                  ExecuteQueryForProfile(hdb, L"DELETE FROM appx_user_cache WHERE sid=?", sid);

   if (success)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, L"INSERT INTO appx_user_cache (sid,user_name) VALUES (?,?)");
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, sid, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, entry->userName, DB_BIND_STATIC);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   if (success && !entry->packages.isEmpty())
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, L"INSERT INTO appx_package_cache (sid,package,name,version,vendor) VALUES (?,?,?,?,?)", entry->packages.size() > 1);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, sid, DB_BIND_STATIC);
         for(int i = 0; success && (i < entry->packages.size()); i++)
         {
            const StorePackage *p = entry->packages.get(i);
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, p->fullName, DB_BIND_STATIC);
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, p->name, DB_BIND_STATIC);
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, p->version, DB_BIND_STATIC);
            DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, p->vendor, DB_BIND_STATIC);
            success = DBExecute(hStmt);
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   if (success)
   {
      DBCommit(hdb);
      nxlog_debug_tag(DEBUG_TAG, 6, L"Store package cache entry for profile %s saved (%d packages)", sid, entry->packages.size());
   }
   else
   {
      DBRollback(hdb);
   }
}

/**
 * Read string value from registry. Unlike plain RegQueryValueEx, guarantees null termination of the result.
 */
static bool ReadRegistryString(HKEY hKey, const wchar_t *valueName, wchar_t *buffer, size_t bufferSize)
{
   DWORD type, size = static_cast<DWORD>((bufferSize - 1) * sizeof(wchar_t));
   if ((RegQueryValueExW(hKey, valueName, nullptr, &type, reinterpret_cast<BYTE*>(buffer), &size) != ERROR_SUCCESS) ||
       ((type != REG_SZ) && (type != REG_EXPAND_SZ)))
      return false;
   buffer[size / sizeof(wchar_t)] = 0;
   return true;
}

/**
 * Resolve indirect string (e.g., @{PackageFullName?ms-resource://...}). Strings that are not indirect are left as is.
 * Returns false if indirect string cannot be resolved or resolves to empty string.
 */
static bool ResolveIndirectString(wchar_t *str, size_t size)
{
   if (str[0] != L'@')
      return true;

   wchar_t resolved[512];
   if ((SHLoadIndirectString(str, resolved, 512, nullptr) != S_OK) || (resolved[0] == 0))
      return false;

   wcslcpy(str, resolved, size);
   return true;
}

/**
 * Extract name and version from package full name (format: Name_Version_Architecture_ResourceId_PublisherId)
 */
static void ParsePackageFullName(const wchar_t *fullName, wchar_t *name, size_t nameSize, wchar_t *version, size_t versionSize)
{
   wcslcpy(name, fullName, nameSize);
   version[0] = 0;

   wchar_t *p = wcschr(name, L'_');
   if (p != nullptr)
   {
      *p++ = 0;
      wchar_t *v = wcschr(p, L'_');
      if (v != nullptr)
         *v = 0;
      wcslcpy(version, p, versionSize);
   }
}

/**
 * Check if two package full names refer to the same package name (version, architecture, and publisher may differ)
 */
static bool IsSamePackageName(const wchar_t *fullName1, const wchar_t *fullName2)
{
   const wchar_t *p = wcschr(fullName1, L'_');
   size_t len = (p != nullptr) ? p - fullName1 : wcslen(fullName1);
   return !wcsnicmp(fullName1, fullName2, len) && ((fullName2[len] == L'_') || (fullName2[len] == 0));
}

/**
 * Find display name previously reported for given package. Exact package match is preferred over other version of
 * the same package (several versions can be registered at once, and only some of them may be resolvable), and given
 * user profile is preferred over other profiles.
 */
static bool FindCachedDisplayName(const wchar_t *fullName, const StoreProfileCacheEntry *profileCacheEntry, wchar_t *displayName, size_t size)
{
   const wchar_t *cachedName = nullptr;
   for(int pass = 0; (pass < 2) && (cachedName == nullptr); pass++)
   {
      auto finder =
         [fullName, pass, &cachedName] (const wchar_t *sid, const StoreProfileCacheEntry *entry) -> EnumerationCallbackResult
         {
            for(int i = 0; i < entry->packages.size(); i++)
            {
               const StorePackage *p = entry->packages.get(i);
               if ((pass == 0) ? !wcsicmp(fullName, p->fullName) : IsSamePackageName(fullName, p->fullName))
               {
                  cachedName = p->name;
                  return _STOP;
               }
            }
            return _CONTINUE;
         };

      if (profileCacheEntry != nullptr)
         finder(nullptr, profileCacheEntry);
      if (cachedName == nullptr)
         s_storeCache.forEach(finder);
   }
   if (cachedName == nullptr)
      return false;

   wcslcpy(displayName, cachedName, size);
   return true;
}

/**
 * Get display name for Store package. When display name cannot be resolved or is empty, use name previously reported
 * for the same package, or name part of package full name if package was not seen before. This keeps package identity
 * on server (name and user) stable between polls when resource resolution fails intermittently.
 */
static bool GetStorePackageDisplayName(HKEY hKey, const wchar_t *fullName, const StoreProfileCacheEntry *profileCacheEntry, wchar_t *displayName, size_t size)
{
   if (!ReadRegistryString(hKey, L"DisplayName", displayName, size))
      return false;  // Skip packages without display name

   if ((displayName[0] != 0) && ResolveIndirectString(displayName, size))
      return true;

   nxlog_debug_tag(DEBUG_TAG, 6, L"Cannot resolve display name \"%s\" for Store package %s", displayName, fullName);
   if (!FindCachedDisplayName(fullName, profileCacheEntry, displayName, size))
   {
      wchar_t version[64];
      ParsePackageFullName(fullName, displayName, size, version, 64);
   }
   return true;
}

/**
 * Read Store package information from registry
 */
static void ReadStorePackageInfo(HKEY hKey, const wchar_t *fullName, const StoreProfileCacheEntry *profileCacheEntry, ObjectArray<StorePackage> *packages)
{
   wchar_t displayName[512];
   if (!GetStorePackageDisplayName(hKey, fullName, profileCacheEntry, displayName, 512))
      return;

   wchar_t name[256], version[64];
   ParsePackageFullName(fullName, name, 256, version, 64);

   // Skip duplicates (same package for different architectures)
   for(int i = 0; i < packages->size(); i++)
   {
      const StorePackage *p = packages->get(i);
      if (!wcsicmp(p->name, displayName) && !wcsicmp(p->version, version))
         return;
   }

   wchar_t vendor[512];
   if (!ReadRegistryString(hKey, L"Publisher", vendor, 512) || !ResolveIndirectString(vendor, 512))
      vendor[0] = 0;

   packages->add(new StorePackage(fullName, displayName, version, vendor));
}

/**
 * Read Store packages from user's classes hive. Returns ERROR_SUCCESS if package list was read completely.
 */
static LSTATUS ReadStorePackagesFromHive(HKEY hHive, const StoreProfileCacheEntry *profileCacheEntry, ObjectArray<StorePackage> *packages)
{
   HKEY hRoot;
   LSTATUS status = RegOpenKeyExW(hHive, STORE_PACKAGES_PATH, 0, KEY_READ, &hRoot);
   if (status != ERROR_SUCCESS)
      return (status == ERROR_FILE_NOT_FOUND) ? ERROR_SUCCESS : status;  // No package repository means no Store packages for this user

   DWORD index = 0;
   while(true)
   {
      wchar_t fullName[256];
      DWORD fullNameLen = 256;
      status = RegEnumKeyExW(hRoot, index++, fullName, &fullNameLen, nullptr, nullptr, nullptr, nullptr);
      if (status != ERROR_SUCCESS)
         break;

      HKEY hKey;
      if (RegOpenKeyExW(hRoot, fullName, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
      {
         ReadStorePackageInfo(hKey, fullName, profileCacheEntry, packages);
         RegCloseKey(hKey);
      }
   }
   RegCloseKey(hRoot);
   return (status == ERROR_NO_MORE_ITEMS) ? ERROR_SUCCESS : status;
}

/**
 * Read Store packages for given user profile. Returns ERROR_SUCCESS if package list was read completely,
 * or error code if profile's classes hive is not available or cannot be read.
 */
static LSTATUS ReadProfileStorePackages(HKEY hProfileList, const wchar_t *sid, const StoreProfileCacheEntry *profileCacheEntry, ObjectArray<StorePackage> *packages)
{
   // Windows Store package data lives in the UsrClass.dat hive (Software\Classes subtree),
   // not in NTUSER.DAT. When a user is logged in, Windows mounts UsrClass.dat automatically
   // as HKEY_USERS\<SID>_Classes - we read directly from there. For logged-out users we
   // load UsrClass.dat manually via RegLoadAppKey, which uses a process-private namespace
   // and shared file access. RegLoadKey would have taken an exclusive file lock and
   // prevented Windows from attaching UsrClass.dat during a concurrent user logon,
   // causing Store apps to fail to launch (issue #3240). Hive root key is held open while
   // reading, so hive mounted by Windows cannot disappear in the middle of enumeration.
   StringBuffer classesKeyName(sid);
   classesKeyName.append(L"_Classes");

   HKEY hHive;
   if (RegOpenKeyExW(HKEY_USERS, classesKeyName, 0, KEY_READ, &hHive) == ERROR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"Reading Store packages for profile %s from mounted classes hive", sid);
   }
   else
   {
#if (_WIN32_WINNT >= 0x0600)
      // Locate UsrClass.dat for this profile
      wchar_t usrClassDat[MAX_PATH] = L"";
      HKEY hProfileKey;
      if (RegOpenKeyExW(hProfileList, sid, 0, KEY_READ, &hProfileKey) == ERROR_SUCCESS)
      {
         wchar_t profilePath[MAX_PATH];
         if (ReadRegistryString(hProfileKey, L"ProfileImagePath", profilePath, MAX_PATH))
         {
            wchar_t expandedPath[MAX_PATH];
            ExpandEnvironmentStringsW(profilePath, expandedPath, MAX_PATH);
            _sntprintf(usrClassDat, MAX_PATH, L"%s\\AppData\\Local\\Microsoft\\Windows\\UsrClass.dat", expandedPath);
         }
         RegCloseKey(hProfileKey);
      }

      if (usrClassDat[0] == 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"Cannot determine UsrClass.dat path for profile %s", sid);
         return ERROR_BAD_PATHNAME;
      }

      LSTATUS status = RegLoadAppKeyW(usrClassDat, &hHive, KEY_READ, 0, 0);
      if (status != ERROR_SUCCESS)
         return status;

      nxlog_debug_tag(DEBUG_TAG, 6, L"Reading Store packages for profile %s from classes hive %s loaded via RegLoadAppKey", sid, usrClassDat);
#else
      // RegLoadAppKey and per-user Windows Store package hives are Vista+/Win8+;
      // Windows XP has no Store data, so there is nothing to collect for this user.
      return ERROR_FILE_NOT_FOUND;
#endif
   }

   LSTATUS status = ReadStorePackagesFromHive(hHive, profileCacheEntry, packages);
   RegCloseKey(hHive);
   return status;
}

/**
 * Convert profile list key name to SID. Returns nullptr if key name is not a canonical SID string
 * (for example, "<SID>.bak" entries left by Windows after temporary profile incidents).
 * Returned SID should be freed with LocalFree.
 */
static PSID ParseProfileSid(const wchar_t *keyName)
{
   PSID sid;
   if (!ConvertStringSidToSidW(keyName, &sid))
      return nullptr;

   bool valid = false;
   wchar_t *canonicalForm;
   if (ConvertSidToStringSidW(sid, &canonicalForm))
   {
      valid = (wcsicmp(canonicalForm, keyName) == 0);
      LocalFree(canonicalForm);
   }
   if (!valid)
   {
      LocalFree(sid);
      return nullptr;
   }
   return sid;
}

/**
 * Resolve SID to user name
 */
static bool ResolveSidToUsername(PSID sid, const wchar_t *sidString, wchar_t *userName, size_t userNameSize)
{
   wchar_t name[256] = L"";
   wchar_t domain[256] = L"";
   DWORD nameSize = 256;
   DWORD domainSize = 256;
   SID_NAME_USE sidType;
   if (!LookupAccountSidW(nullptr, sid, name, &nameSize, domain, &domainSize, &sidType))
   {
      uint32_t error = GetLastError();
      wchar_t errorText[256];
      nxlog_debug_tag(DEBUG_TAG, 5, L"Cannot resolve SID %s to user name (error %u: %s)", sidString, error, GetSystemErrorText(error, errorText, 256));
      return false;
   }

   if (domain[0] != 0)
      _sntprintf(userName, userNameSize, L"%s\\%s", domain, name);
   else
      wcslcpy(userName, name, userNameSize);
   return true;
}

/**
 * Add Store packages to result table
 */
static void AddStorePackagesToTable(const ObjectArray<StorePackage>& packages, const wchar_t *userName, Table *table)
{
   for(int i = 0; i < packages.size(); i++)
   {
      const StorePackage *p = packages.get(i);
      table->addRow();
      table->set(0, p->name);       // NAME
      table->set(1, p->version);    // VERSION
      table->set(2, p->vendor);     // VENDOR

      // DATE, URL, DESCRIPTION - not available in registry

      // Use APPX: prefix to identify Store apps
      StringBuffer uninstallKey(L"APPX:");
      uninstallKey.append(p->fullName);
      table->set(6, uninstallKey);  // UNINSTALL_KEY
      table->set(7, userName);      // USER
   }
}

/**
 * Read Windows Store packages from all user profiles. Profiles whose classes hive is not available are
 * reported from cache of last successful read. Returns false if snapshot is incomplete - in that case
 * the caller should signal a collection error so the server preserves the previous snapshot rather
 * than treating missing packages as removals.
 */
static bool ReadStorePackagesFromAllUsers(Table *table)
{
   // Enumerate all user profiles from ProfileList (includes logged-off users)
   HKEY hProfileList;
   LSTATUS status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList", 0, KEY_READ, &hProfileList);
   if (status != ERROR_SUCCESS)
   {
      wchar_t errorText[256];
      nxlog_debug_tag(DEBUG_TAG, 5, L"Cannot open profile list (error %u: %s) - marking snapshot incomplete", static_cast<uint32_t>(status), GetSystemErrorText(status, errorText, 256));
      return false;
   }

   LockGuard lockGuard(s_storeCacheLock);
   if (!s_storeCacheLoaded)
   {
      LoadStorePackageCache();
      s_storeCacheLoaded = true;
   }

   bool complete = true;
   StringSet profiles;
   DWORD index = 0;
   while(true)
   {
      wchar_t sid[256];
      DWORD sidLen = 256;
      status = RegEnumKeyExW(hProfileList, index++, sid, &sidLen, nullptr, nullptr, nullptr, nullptr);
      if (status != ERROR_SUCCESS)
         break;

      // Skip well-known short SIDs (e.g., S-1-5-18, S-1-5-19, S-1-5-20)
      if (wcslen(sid) < 20)
         continue;

      PSID pSid = ParseProfileSid(sid);
      if (pSid == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, L"Profile list entry %s skipped (not a valid SID)", sid);
         continue;
      }
      profiles.add(sid);

      StoreProfileCacheEntry *cacheEntry = s_storeCache.get(sid);

      // Use last known user name (or SID if user name was never resolved) when lookup fails,
      // so that failed lookup does not change package identity on server
      wchar_t userName[512];
      if (!ResolveSidToUsername(pSid, sid, userName, 512))
         wcslcpy(userName, (cacheEntry != nullptr) ? cacheEntry->userName.cstr() : sid, 512);
      LocalFree(pSid);

      ObjectArray<StorePackage> packages(64, 64, Ownership::True);
      LSTATUS readStatus = ReadProfileStorePackages(hProfileList, sid, cacheEntry, &packages);
      if (readStatus == ERROR_SUCCESS)
      {
         AddStorePackagesToTable(packages, userName, table);
         if ((cacheEntry == nullptr) || !cacheEntry->matches(userName, packages))
         {
            cacheEntry = new StoreProfileCacheEntry(userName, std::move(packages));
            s_storeCache.set(sid, cacheEntry);
            SaveStorePackageCacheEntry(sid, cacheEntry);
         }
         continue;
      }

      wchar_t errorText[256];
      GetSystemErrorText(readStatus, errorText, 256);
      if (cacheEntry != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"Store package data for user %s (%s) not available (error %u: %s) - reporting %d cached packages",
            userName, sid, static_cast<uint32_t>(readStatus), errorText, cacheEntry->packages.size());
         AddStorePackagesToTable(cacheEntry->packages, userName, table);
         if (wcscmp(cacheEntry->userName, userName))
         {
            cacheEntry->userName = userName;
            SaveStorePackageCacheEntry(sid, cacheEntry);
         }
      }
      else if ((readStatus == ERROR_FILE_NOT_FOUND) || (readStatus == ERROR_PATH_NOT_FOUND))
      {
         // No cached data and no hive on disk (e.g. profile was never used for Store apps) - nothing to report
         nxlog_debug_tag(DEBUG_TAG, 6, L"UsrClass.dat not present for user %s (%s)", userName, sid);
      }
      else
      {
         // Hive exists but cannot be read (sharing violation, access denied, etc.) and there is no
         // cached data - signal collection error so missing packages are not reported as removals
         nxlog_debug_tag(DEBUG_TAG, 5, L"Cannot read Store package data for user %s (%s) (error %u: %s) - marking snapshot incomplete",
            userName, sid, static_cast<uint32_t>(readStatus), errorText);
         complete = false;
      }
   }
   RegCloseKey(hProfileList);

   if (status == ERROR_NO_MORE_ITEMS)
   {
      // Drop cached data for profiles that were removed from the system
      StringList cachedProfiles = s_storeCache.keys();
      for(int i = 0; i < cachedProfiles.size(); i++)
      {
         const wchar_t *sid = cachedProfiles.get(i);
         if (!profiles.contains(sid))
         {
            nxlog_debug_tag(DEBUG_TAG, 5, L"Profile %s no longer exists, removing it from Store package cache", sid);
            s_storeCache.remove(sid);
            DeleteStorePackageCacheEntry(sid);
         }
      }
   }
   else
   {
      wchar_t errorText[256];
      nxlog_debug_tag(DEBUG_TAG, 5, L"Profile list enumeration failed (error %u: %s) - marking snapshot incomplete", static_cast<uint32_t>(status), GetSystemErrorText(status, errorText, 256));
      complete = false;
   }

   return complete;
}

/**
 * Read provisioned (system-wide) Windows Store packages. They are always reported as separate rows without user name,
 * independently of per-user packages, so that their presence does not depend on availability of user profiles.
 */
static void ReadProvisionedStorePackages(Table *table)
{
   HKEY hRoot;
   if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Appx\\AppxAllUserStore\\Applications", 0, KEY_READ, &hRoot) != ERROR_SUCCESS)
      return;

   StringSet knownPackages;
   DWORD index = 0;
   while(true)
   {
      wchar_t fullName[256];
      DWORD fullNameLen = 256;
      if (RegEnumKeyExW(hRoot, index++, fullName, &fullNameLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
         break;

      // Provisioned packages use package full name as key, extract name and version from it
      // Example: Microsoft.WindowsCalculator_11.2210.0.0_x64__8wekyb3d8bbwe
      wchar_t name[256], version[64];
      ParsePackageFullName(fullName, name, 256, version, 64);

      // Skip duplicates (same package for different architectures)
      StringBuffer packageKey(name);
      packageKey.append(L'_');
      packageKey.append(version);
      if (knownPackages.contains(packageKey))
         continue;
      knownPackages.add(packageKey);

      table->addRow();
      table->set(0, name);       // NAME
      table->set(1, version);    // VERSION
      // VENDOR not available in provisioned package registry

      StringBuffer uninstallKey(L"APPX:");
      uninstallKey.append(fullName);
      table->set(6, uninstallKey);  // UNINSTALL_KEY
   }
   RegCloseKey(hRoot);
}

/**
 * Read Windows Store packages from registry (all sources).
 * Returns false if the per-user enumeration was incomplete.
 */
static bool ReadStorePackagesFromRegistry(Table *table)
{
   // Read from all user profiles
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
