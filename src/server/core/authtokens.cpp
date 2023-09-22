/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Raden Solutions
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

#define ENCODED_USER_AUTHENTICATION_TOKEN_LENGTH   (USER_AUTHENTICATION_TOKEN_LENGTH * 8 / 5)

/**
 * Convert authentication token to string form (wide char version)
 */
WCHAR *UserAuthenticationToken::toStringW(WCHAR *buffer) const
{
   char encoded[ENCODED_USER_AUTHENTICATION_TOKEN_LENGTH + 1];
   base32_encode(reinterpret_cast<const char*>(m_value), USER_AUTHENTICATION_TOKEN_LENGTH, encoded, ENCODED_USER_AUTHENTICATION_TOKEN_LENGTH + 1);
   strlwr(encoded);
   ASCII_to_wchar(encoded, -1, buffer, ENCODED_USER_AUTHENTICATION_TOKEN_LENGTH + 1);
   return buffer;
}

/**
 * Convert authentication token to string form (ASCII version)
 */
char *UserAuthenticationToken::toStringA(char *buffer) const
{
   base32_encode(reinterpret_cast<const char*>(m_value), USER_AUTHENTICATION_TOKEN_LENGTH, buffer, ENCODED_USER_AUTHENTICATION_TOKEN_LENGTH + 1);
   strlwr(buffer);
   return buffer;
}

/**
 * Parse token
 */
static inline UserAuthenticationToken ParseToken(char *s)
{
   strupr(s);
   BYTE bytes[USER_AUTHENTICATION_TOKEN_LENGTH];
   memset(bytes, 0, sizeof(bytes));
   size_t size = USER_AUTHENTICATION_TOKEN_LENGTH;
   base32_decode(s, ENCODED_USER_AUTHENTICATION_TOKEN_LENGTH, reinterpret_cast<char*>(bytes), &size);
   return UserAuthenticationToken(bytes);
}

/**
 * Parse user authentication token (wide char version)
 */
UserAuthenticationToken UserAuthenticationToken::parseW(const WCHAR *s)
{
   if (_tcslen(s) != ENCODED_USER_AUTHENTICATION_TOKEN_LENGTH)
      return UserAuthenticationToken();

   char ts[ENCODED_USER_AUTHENTICATION_TOKEN_LENGTH];
   wchar_to_ASCII(s, -1, ts, sizeof(ts));
   return ParseToken(ts);
}

/**
 * Parse user authentication token (ASCII version)
 */
UserAuthenticationToken UserAuthenticationToken::parseA(const char *s)
{
   if (strlen(s) != ENCODED_USER_AUTHENTICATION_TOKEN_LENGTH)
      return UserAuthenticationToken();

   char ts[ENCODED_USER_AUTHENTICATION_TOKEN_LENGTH + 1];
   memcpy(ts, s, sizeof(ts));
   return ParseToken(ts);
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
UserAuthenticationToken NXCORE_EXPORTABLE IssueAuthenticationToken(uint32_t userId, uint32_t validFor)
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
void NXCORE_EXPORTABLE RevokeAuthenticationToken(const UserAuthenticationToken& token)
{
   s_tokens.remove(token);
   nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token \"%s\" was revoked"), token.toString().cstr());
}

/**
 * Authenticate user with token. If validFor is non-zero, token expiration time will be set to current time + provided value.
 */
bool NXCORE_EXPORTABLE ValidateAuthenticationToken(const UserAuthenticationToken& token, uint32_t *userId, uint32_t validFor)
{
   shared_ptr<AuthenticationTokenDescriptor> descriptor = s_tokens.getShared(token);
   if (descriptor == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token \"%s\" does not exist"), token.toString().cstr());
      return false;
   }
   time_t now = time(nullptr);
   if (descriptor->expirationTime <= now)
   {
      s_tokens.remove(token);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token \"%s\" has expired"), token.toString().cstr());
      return false;
   }
   if (validFor > 0)
   {
      descriptor->expirationTime = now + validFor;
   }
   *userId = descriptor->userId;
   return true;
}

/**
 * Callback for finding expired tokens
 */
EnumerationCallbackResult FindExpiredTokens(const UserAuthenticationToken& token, const shared_ptr<AuthenticationTokenDescriptor>& descriptor, SharedObjectArray<AuthenticationTokenDescriptor> *expiredTokens)
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
      nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token \"%s\" has expired"), token.toString().cstr());
   }
}

/**
 * Callback for printing tokens
 */
EnumerationCallbackResult PrintTokens(const UserAuthenticationToken& token, const shared_ptr<AuthenticationTokenDescriptor>& descriptor, ServerConsole *console)
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
