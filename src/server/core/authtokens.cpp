/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Raden Solutions
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
** File: authtokens.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("auth")

/**
 * Parse user authentication token
 */
UserAuthenticationToken UserAuthenticationToken::parse(const TCHAR *s)
{
   if (_tcslen(s) != USER_AUTHENTICATION_TOKEN_LENGTH * 2)
      return UserAuthenticationToken();
   BYTE bytes[USER_AUTHENTICATION_TOKEN_LENGTH];
   StrToBin(s, bytes, USER_AUTHENTICATION_TOKEN_LENGTH);
   return UserAuthenticationToken(bytes);
}

/**
 * Authentication token descriptor
 */
struct AuthenticationTokenDescriptor
{
   UserAuthenticationToken token;
   uint32_t userId;
   time_t expirationTime;

   AuthenticationTokenDescriptor(uint32_t uid, uint32_t validFor)
   {
      BYTE bytes[USER_AUTHENTICATION_TOKEN_LENGTH];
      GenerateRandomBytes(bytes, USER_AUTHENTICATION_TOKEN_LENGTH);
      token = UserAuthenticationToken(bytes);
      userId = uid;
      expirationTime = time(nullptr) + validFor;
   }
};

/**
 * Registered authentication tokens
 */
static SynchronizedSharedHashMap<UserAuthenticationToken, AuthenticationTokenDescriptor> s_tokens;

/**
 * Issue authentication token
 */
UserAuthenticationToken IssueAuthenticationToken(uint32_t userId, uint32_t validFor)
{
   TCHAR userName[MAX_USER_NAME];
   if ((userId & GROUP_FLAG) || ResolveUserId(userId, userName) == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot issue authentication token for unknown user [%u]"), userId);
      return UserAuthenticationToken();
   }

   auto descriptor = make_shared<AuthenticationTokenDescriptor>(userId, validFor);
   s_tokens.set(descriptor->token, descriptor);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("New authentication token issued for user %s [%u]"), userName, userId);
   return descriptor->token;
}

/**
 * Revoke authentication token
 */
void RevokeAuthenticationToken(const UserAuthenticationToken& token)
{
   s_tokens.remove(token);
   nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token %s was revoked"), token.toString().cstr());
}

/**
 * Authenticate user with token
 */
bool ValidateAuthenticationToken(const UserAuthenticationToken& token, uint32_t *userId)
{
   shared_ptr<AuthenticationTokenDescriptor> descriptor = s_tokens.getShared(token);
   if (descriptor == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token %s does not exist"), token.toString().cstr());
      return false;
   }
   if (descriptor->expirationTime <= time(nullptr))
   {
      s_tokens.remove(token);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token %s has expired"), token.toString().cstr());
      return false;
   }
   *userId = descriptor->userId;
   return true;
}

/**
 * Callback for finding expired tokens
 */
static EnumerationCallbackResult FindExpiredTokens(const UserAuthenticationToken& token, const shared_ptr<AuthenticationTokenDescriptor>& descriptor, SharedObjectArray<AuthenticationTokenDescriptor> *expiredTokens)
{
   if (descriptor->expirationTime <= time(nullptr))
      expiredTokens->add(descriptor);
   return _CONTINUE;
}

/**
 * Check for expired authentication tokens
 */
void CheckUserAuthenticationTokens(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   SharedObjectArray<AuthenticationTokenDescriptor> expiredTokens;
   s_tokens.forEach(FindExpiredTokens, &expiredTokens);
   for(int i = 0; i < expiredTokens.size(); i++)
   {
      const UserAuthenticationToken& token = expiredTokens.get(i)->token;
      s_tokens.remove(token);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token %s has expired"), token.toString().cstr());
   }
}

/**
 * Callback for printing tokens
 */
static EnumerationCallbackResult PrintTokens(const UserAuthenticationToken& token, const shared_ptr<AuthenticationTokenDescriptor>& descriptor, ServerConsole *console)
{
   TCHAR userName[MAX_USER_NAME];
   console->printf(_T(" %-32s | %6u | %-24s | %s\n"), token.toString().cstr(), descriptor->userId, ResolveUserId(descriptor->userId, userName), FormatTimestamp(descriptor->expirationTime).cstr());
   return _CONTINUE;
}

/**
 * Print authentication tokens
 */
void ShowAuthenticationTokens(ServerConsole *console)
{
   console->printf(_T(" %-32s | %-6s | %-24s | %s\n"), _T("Token"), _T("UID"), _T("User name"), _T("Expires at"));
   console->printf(_T("----------------------------------+--------+--------------------------+----------------------\n"));
   s_tokens.forEach(PrintTokens, console);
}
