/*
** NetXMS multiplatform core agent
** Copyright (C) 2026 Raden Solutions
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
** File: session_fallback.cpp
**
** Session and user discovery for the case when WTS calls fail
** (Remote Desktop Services / Terminal Services is not running)
**
**/

#include "nxagentd.h"
#include <tlhelp32.h>

#define DEBUG_TAG _T("wts")

/**
 * Get active console session. Returns false if there is no active console session.
 */
bool GetConsoleSession(UserSession *session)
{
   DWORD consoleSessionId = WTSGetActiveConsoleSessionId();
   if (consoleSessionId == 0xFFFFFFFF)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("GetConsoleSession: no active console session"));
      return false;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("GetConsoleSession: using console session #%u"), consoleSessionId);
   session->id = consoleSessionId;
   session->state = WTSActive;
   _tcslcpy(session->name, _T("Console"), 64);
   session->console = true;
   return true;
}

/**
 * Check if given process token belongs to interactively logged on user (not to system or service account)
 */
static bool IsInteractiveUserToken(HANDLE token)
{
   union
   {
      TOKEN_USER tokenUser;
      BYTE buffer[sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE];
   } user;
   DWORD size;
   if (!GetTokenInformation(token, TokenUser, &user, sizeof(user), &size))
      return false;

   PSID userSid = user.tokenUser.User.Sid;
   if (IsWellKnownSid(userSid, WinLocalSystemSid) || IsWellKnownSid(userSid, WinLocalServiceSid) || IsWellKnownSid(userSid, WinNetworkServiceSid))
      return false;

   // Desktop Window Manager (S-1-5-90-*) and font driver host (S-1-5-96-*) virtual accounts are interactive but do not represent a user
   SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
   if (!memcmp(GetSidIdentifierAuthority(userSid), &ntAuthority, sizeof(SID_IDENTIFIER_AUTHORITY)) && (*GetSidSubAuthorityCount(userSid) > 0))
   {
      DWORD rid = *GetSidSubAuthority(userSid, 0);
      if ((rid == 90) || (rid == 96))
         return false;
   }

   GetTokenInformation(token, TokenGroups, nullptr, 0, &size);
   if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
      return false;

   TOKEN_GROUPS *groups = static_cast<TOKEN_GROUPS*>(MemAlloc(size));
   bool interactive = false;
   if (GetTokenInformation(token, TokenGroups, groups, size, &size))
   {
      for (DWORD i = 0; i < groups->GroupCount; i++)
      {
         if (IsWellKnownSid(groups->Groups[i].Sid, WinInteractiveSid))
         {
            interactive = true;
            break;
         }
      }
   }
   MemFree(groups);
   return interactive;
}

/**
 * Find token of interactively logged on user in given session by scanning processes running in that session.
 * Returned token handle should be closed by caller. Returns nullptr if there are no interactive user processes in the session.
 */
HANDLE FindInteractiveUserToken(DWORD sessionId)
{
   HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
   if (snapshot == INVALID_HANDLE_VALUE)
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(DEBUG_TAG, 6, _T("FindInteractiveUserToken: call to CreateToolhelp32Snapshot failed (%s)"),
            GetSystemErrorText(GetLastError(), buffer, 1024));
      return nullptr;
   }

   // Earliest created process is the one started by logon sequence for console user; processes started later
   // via runas or elevation may have logon SID without access to winsta0\default, so their token is unusable
   HANDLE userToken = nullptr;
   FILETIME userTokenProcessCreationTime;
   DWORD userTokenProcessId = 0;
   TCHAR userTokenProcessName[MAX_PATH];

   PROCESSENTRY32 pe;
   pe.dwSize = sizeof(PROCESSENTRY32);
   if (Process32First(snapshot, &pe))
   {
      do
      {
         DWORD processSessionId;
         if (!ProcessIdToSessionId(pe.th32ProcessID, &processSessionId) || (processSessionId != sessionId))
            continue;

         HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe.th32ProcessID);
         if (hProcess == nullptr)
            continue;

         HANDLE token;
         if (OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_DUPLICATE, &token))
         {
            FILETIME creationTime, exitTime, kernelTime, userTime;
            if (IsInteractiveUserToken(token) && GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime) &&
                ((userToken == nullptr) || (CompareFileTime(&creationTime, &userTokenProcessCreationTime) < 0)))
            {
               if (userToken != nullptr)
                  CloseHandle(userToken);
               userToken = token;
               userTokenProcessCreationTime = creationTime;
               userTokenProcessId = pe.th32ProcessID;
               _tcslcpy(userTokenProcessName, pe.szExeFile, MAX_PATH);
            }
            else
            {
               CloseHandle(token);
            }
         }
         CloseHandle(hProcess);
      } while (Process32Next(snapshot, &pe));
   }
   CloseHandle(snapshot);

   if (userToken != nullptr)
      nxlog_debug_tag(DEBUG_TAG, 6, _T("FindInteractiveUserToken: using token of process %s (PID %u) in session #%u"),
            userTokenProcessName, userTokenProcessId, sessionId);
   return userToken;
}

/**
 * Get name and domain of interactively logged on user in given session
 */
bool GetInteractiveUserName(DWORD sessionId, MutableString *user, MutableString *domain)
{
   HANDLE token = FindInteractiveUserToken(sessionId);
   if (token == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("GetInteractiveUserName: no interactive user process in session #%u"), sessionId);
      return false;
   }

   union
   {
      TOKEN_USER tokenUser;
      BYTE buffer[sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE];
   } tokenInfo;
   DWORD size;
   TCHAR userName[256], domainName[256];
   DWORD userNameSize = 256, domainNameSize = 256;
   SID_NAME_USE use;
   bool success = GetTokenInformation(token, TokenUser, &tokenInfo, sizeof(tokenInfo), &size) &&
         LookupAccountSid(nullptr, tokenInfo.tokenUser.User.Sid, userName, &userNameSize, domainName, &domainNameSize, &use);
   DWORD error = success ? ERROR_SUCCESS : GetLastError();
   CloseHandle(token);
   if (!success)
   {
      TCHAR errorText[1024];
      nxlog_debug_tag(DEBUG_TAG, 6, _T("GetInteractiveUserName: cannot get logged on user in session #%u (%s)"),
            sessionId, GetSystemErrorText(error, errorText, 1024));
      return false;
   }

   *user = userName;
   *domain = domainName;
   return true;
}
