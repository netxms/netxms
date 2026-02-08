/*
** Windows platform subagent
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: registry.cpp
**
**/

#include "winnt_subagent.h"

/**
 * Get registry top level key and subkey name from metric name
 */
static HKEY GetKeyAndSubkeyFromArgs(const wchar_t *metric, wchar_t *subkey)
{
   wchar_t path[MAX_PATH];
   if (!AgentGetMetricArg(metric, 1, path, MAX_PATH))
      return nullptr;

   wchar_t *s = wcschr(path, L'\\');
   if (s != nullptr)
   {
      *s = 0;
      wcscpy(subkey, s + 1);
   }
   else
   {
      subkey[0] = 0;
   }

   if (!wcsicmp(path, L"HKEY_CLASSES_ROOT"))
      return HKEY_CLASSES_ROOT;
   if (!wcsicmp(path, L"HKEY_CURRENT_CONFIG"))
      return HKEY_CURRENT_CONFIG;
   if (!wcsicmp(path, L"HKEY_CURRENT_USER"))
      return HKEY_CURRENT_USER;
   if (!wcsicmp(path, L"HKEY_LOCAL_MACHINE"))
      return HKEY_LOCAL_MACHINE;
   if (!wcsicmp(path, L"HKEY_USERS"))
      return HKEY_USERS;
   return nullptr;
}

/**
 * Get name for given root key
 */
static const wchar_t *RootKeyName(HKEY hKey)
{
   if (hKey == HKEY_CLASSES_ROOT)
      return L"HKEY_CLASSES_ROOT";
   if (hKey == HKEY_CURRENT_CONFIG)
      return L"HKEY_CURRENT_CONFIG";
   if (hKey == HKEY_CURRENT_USER)
      return L"HKEY_CURRENT_USER";
   if (hKey == HKEY_LOCAL_MACHINE)
      return L"HKEY_LOCAL_MACHINE";
   if (hKey == HKEY_USERS)
      return L"HKEY_USERS";
   return L"HKEY_UNKNOWN";
}

/**
 * Handler for System.Registry.Keys list
 */
LONG H_RegistryKeyList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   wchar_t subKeyName[MAX_PATH];
   HKEY hRoot = GetKeyAndSubkeyFromArgs(cmd, subKeyName);
   if (hRoot == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   HKEY hKey;
   LSTATUS status = RegOpenKeyExW(hRoot, subKeyName, 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
   if (status != ERROR_SUCCESS)
   {
      wchar_t errorText[1024];
      nxlog_debug_tag(DEBUG_TAG, 5, L"H_RegistryKeyList: cannot open root key \"%s\\%s\" (%s)", RootKeyName(hRoot), subKeyName, GetSystemErrorText(status, errorText, 1024));
      return SYSINFO_RC_ERROR;
   }

   DWORD index = 0;
   while (true)
   {
      wchar_t keyName[MAX_PATH];
      DWORD keyNameLen = MAX_PATH;
      status = RegEnumKeyExW(hKey, index++, keyName, &keyNameLen, nullptr, nullptr, nullptr, nullptr);
      if (status != ERROR_SUCCESS)
      {
         if (status != ERROR_NO_MORE_ITEMS)
         {
            wchar_t errorText[1024];
            nxlog_debug_tag(DEBUG_TAG, 5, L"H_RegistryKeyList: cannot read key list for \"%s\\%s\" (%s)", RootKeyName(hRoot), subKeyName, GetSystemErrorText(status, errorText, 1024));
            RegCloseKey(hKey);
            return SYSINFO_RC_ERROR;
         }
         break;
      }
      value->add(keyName);
   }

   RegCloseKey(hKey);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.Registry.Values list
 */
LONG H_RegistryValueList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   wchar_t subKeyName[MAX_PATH];
   HKEY hRoot = GetKeyAndSubkeyFromArgs(cmd, subKeyName);
   if (hRoot == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   HKEY hKey;
   LSTATUS status = RegOpenKeyExW(hRoot, subKeyName, 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
   if (status != ERROR_SUCCESS)
   {
      wchar_t errorText[1024];
      nxlog_debug_tag(DEBUG_TAG, 5, L"H_RegistryValueList: cannot open root key \"%s\\%s\" (%s)", RootKeyName(hRoot), subKeyName, GetSystemErrorText(status, errorText, 1024));
      return SYSINFO_RC_ERROR;
   }

   DWORD index = 0;
   while (true)
   {
      wchar_t valueName[MAX_PATH];
      DWORD valueNameLen = MAX_PATH;
      status = RegEnumValueW(hKey, index++, valueName, &valueNameLen, nullptr, nullptr, nullptr, nullptr);
      if (status != ERROR_SUCCESS)
      {
         if (status != ERROR_NO_MORE_ITEMS)
         {
            wchar_t errorText[1024];
            nxlog_debug_tag(DEBUG_TAG, 5, L"H_RegistryValueList: cannot read value list for \"%s\\%s\" (%s)", RootKeyName(hRoot), subKeyName, GetSystemErrorText(status, errorText, 1024));
            RegCloseKey(hKey);
            return SYSINFO_RC_ERROR;
         }
         break;
      }
      value->add(valueName);
   }

   RegCloseKey(hKey);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.Registry.Value metric
 */
LONG H_RegistryValue(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   wchar_t subKeyName[MAX_PATH];
   HKEY hRoot = GetKeyAndSubkeyFromArgs(cmd, subKeyName);
   if (hRoot == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   HKEY hKey;
   LSTATUS status = RegOpenKeyExW(hRoot, subKeyName, 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
   if (status != ERROR_SUCCESS)
   {
      wchar_t errorText[1024];
      nxlog_debug_tag(DEBUG_TAG, 5, L"H_RegistryValueList: cannot open root key \"%s\\%s\" (%s)", RootKeyName(hRoot), subKeyName, GetSystemErrorText(status, errorText, 1024));
      return SYSINFO_RC_ERROR;
   }

   LONG rc;
   wchar_t valueName[256] = L"";
   AgentGetMetricArgW(cmd, 2, valueName, 256);
   DWORD type, size = 1024;
   BYTE buffer[1024];
   status = RegGetValueW(hKey, nullptr, valueName, RRF_RT_ANY, &type, buffer, &size);
   if (status == ERROR_SUCCESS)
   {
      switch (type)
      {
         case REG_BINARY:
            BinToStrW(buffer, MIN(MAX_RESULT_LENGTH / 2 - 1, size), value);
            break;
         case REG_DWORD:
            ret_int(value, *reinterpret_cast<int32_t*>(buffer));
            break;
         case REG_DWORD_BIG_ENDIAN:
            ret_uint(value, ntohl(*reinterpret_cast<uint32_t*>(buffer)));
            break;
         case REG_EXPAND_SZ:
            ret_string(value, reinterpret_cast<wchar_t*>(buffer));
            break;
         case REG_QWORD:
            ret_int64(value, *reinterpret_cast<int64_t*>(buffer));
            break;
         case REG_SZ:
            ret_string(value, reinterpret_cast<wchar_t*>(buffer));
            break;
      }
      rc = SYSINFO_RC_SUCCESS;
   }
   else
   {
      rc = (status == ERROR_FILE_NOT_FOUND) ? SYSINFO_RC_NO_SUCH_INSTANCE : SYSINFO_RC_ERROR;
   }

   RegCloseKey(hKey);
   return rc;
}
