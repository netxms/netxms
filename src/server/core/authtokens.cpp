/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Raden Solutions
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
#include <nms_users.h>

#define DEBUG_TAG _T("auth")

#define ENCODED_USER_AUTHENTICATION_TOKEN_LENGTH   (USER_AUTHENTICATION_TOKEN_LENGTH * 8 / 5)

/**
 * Convert authentication token to string form (wide char version)
 */
WCHAR *UserAuthenticationToken::toStringW(wchar_t *buffer) const
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
UserAuthenticationToken UserAuthenticationToken::parseW(const wchar_t *s)
{
   if (wcslen(s) != ENCODED_USER_AUTHENTICATION_TOKEN_LENGTH)
      return UserAuthenticationToken();

   char ts[ENCODED_USER_AUTHENTICATION_TOKEN_LENGTH + 1];
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
 * Registered authentication tokens
 */
static SynchronizedSharedHashMap<UserAuthenticationTokenHash, AuthenticationTokenDescriptor> s_tokens;

/**
 * Load persistent tokens
 */
void LoadAuthenticationTokens()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,user_id,issuing_time,expiration_time,description,token_data FROM auth_tokens"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         auto descriptor = make_shared<AuthenticationTokenDescriptor>(hResult, i);
         s_tokens.set(descriptor->hash, descriptor);
      }
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Issue authentication token
 */
shared_ptr<AuthenticationTokenDescriptor> NXCORE_EXPORTABLE IssueAuthenticationToken(uint32_t userId, uint32_t validFor, AuthenticationTokenType type, const wchar_t *description)
{
   wchar_t userName[MAX_USER_NAME];
   if ((userId & GROUP_FLAG) || ResolveUserId(userId, userName) == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot issue authentication token for unknown user [%u]"), userId);
      return shared_ptr<AuthenticationTokenDescriptor>();
   }

   auto descriptor = make_shared<AuthenticationTokenDescriptor>(userId, validFor, type, description);
   s_tokens.set(descriptor->hash, descriptor);

   if (type == AuthenticationTokenType::PERSISTENT)
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("New persistent authentication token issued for user %s [%u]"), userName, userId);

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO auth_tokens (id,user_id,issuing_time,expiration_time,description,token_data) VALUES (?,?,?,?,?,?)"));
      if (hStmt != nullptr)
      {
         TCHAR hashText[128];
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, descriptor->tokenId);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, descriptor->userId);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(descriptor->issuingTime));
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(descriptor->expirationTime));
         DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, descriptor->description, DB_BIND_STATIC);
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, BinToStr(descriptor->hash, SHA256_DIGEST_SIZE, hashText), DB_BIND_STATIC);
         DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("New %s authentication token issued for user %s [%u]"),
         (type == AuthenticationTokenType::SERVICE) ? L"service" : L"ephemeral", userName, userId);
   }

   return descriptor;
}

/**
 * Delete authentication token from database
 */
static void DeleteAuthenticationTokenFromDB(uint32_t tokenId)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   ExecuteQueryOnObject(hdb, tokenId, _T("DELETE FROM auth_tokens WHERE id=?"));
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Revoke authentication token
 */
void NXCORE_EXPORTABLE RevokeAuthenticationToken(const UserAuthenticationToken& token)
{
   UserAuthenticationTokenHash hash;
   CalculateSHA256Hash(token.value(), USER_AUTHENTICATION_TOKEN_LENGTH, hash);
   shared_ptr<AuthenticationTokenDescriptor> descriptor = s_tokens.getShared(hash);
   if (descriptor != nullptr)
   {
      s_tokens.remove(hash);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token \"%s\" was revoked"), token.toString().cstr());
      if (descriptor->persistent)
         DeleteAuthenticationTokenFromDB(descriptor->tokenId);
   }
}

/**
 * Revoke authentication token
 */
uint32_t NXCORE_EXPORTABLE RevokeAuthenticationToken(uint32_t tokenId, uint32_t userId)
{
   shared_ptr<AuthenticationTokenDescriptor> descriptor = s_tokens.findElement(
      [tokenId] (const UserAuthenticationTokenHash& key, const AuthenticationTokenDescriptor& descriptor) -> bool
      {
         return descriptor.tokenId == tokenId;
      });
   if (descriptor == nullptr)
      return RCC_INVALID_TOKEN_ID;

   if ((userId != 0) && (userId != descriptor->userId))
      return RCC_ACCESS_DENIED;

   s_tokens.remove(descriptor->hash);
   nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token [%u] was revoked"), tokenId);
   if (descriptor->persistent)
      DeleteAuthenticationTokenFromDB(tokenId);
   return RCC_SUCCESS;
}

/**
 * Authenticate user with token. If validFor is non-zero, token expiration time will be set to current time + provided value.
 */
bool NXCORE_EXPORTABLE ValidateAuthenticationToken(const UserAuthenticationToken& token, uint32_t *userId, bool *serviceToken, uint32_t validFor)
{
   UserAuthenticationTokenHash hash;
   CalculateSHA256Hash(token.value(), USER_AUTHENTICATION_TOKEN_LENGTH, hash);
   shared_ptr<AuthenticationTokenDescriptor> descriptor = s_tokens.getShared(hash);
   if (descriptor == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token \"%s\" does not exist"), token.toString().cstr());
      return false;
   }
   time_t now = time(nullptr);
   if (descriptor->expirationTime <= now)
   {
      s_tokens.remove(hash);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token [%u] \"%s\" has expired"), descriptor->tokenId, token.toString().cstr());
      if (descriptor->persistent)
         DeleteAuthenticationTokenFromDB(descriptor->tokenId);
      return false;
   }
   if (validFor > 0)
   {
      descriptor->expirationTime = now + validFor;
   }
   *userId = descriptor->userId;
   if (serviceToken != nullptr)
      *serviceToken = descriptor->service;
   return true;
}

/**
 * Check for expired authentication tokens
 */
void CheckUserAuthenticationTokens(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   SharedObjectArray<AuthenticationTokenDescriptor> expiredTokens;
   s_tokens.forEach(
      [&expiredTokens] (const UserAuthenticationTokenHash& key, const shared_ptr<AuthenticationTokenDescriptor>& descriptor) -> EnumerationCallbackResult
      {
         if (descriptor->expirationTime <= time(nullptr))
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token \"%s\" has expired"), descriptor->token.toString().cstr());
            expiredTokens.add(descriptor);
         }
         return _CONTINUE;
      });
   for(int i = 0; i < expiredTokens.size(); i++)
   {
      AuthenticationTokenDescriptor *d = expiredTokens.get(i);
      const UserAuthenticationTokenHash& hash = d->hash;
      s_tokens.remove(hash);
      if (d->persistent)
         DeleteAuthenticationTokenFromDB(d->tokenId);
   }
}

/**
 * Fill NXCP message with list of authentication tokens for given user
 */
void AuthenticationTokensToMessage(uint32_t userId, NXCPMessage *msg)
{
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   uint32_t count = 0;
   s_tokens.forEach(
      [userId, msg, &fieldId, &count] (const UserAuthenticationTokenHash& key, const shared_ptr<AuthenticationTokenDescriptor>& descriptor) -> EnumerationCallbackResult
      {
         if (descriptor->userId == userId)
         {
            descriptor->fillMessage(msg, fieldId);
            fieldId += 10;
            count++;
         }
         return _CONTINUE;
      });
   msg->setField(VID_NUM_ELEMENTS, count);
}

/**
 * Print authentication tokens
 */
void ShowAuthenticationTokens(ServerConsole *console)
{
   console->printf(_T(" %-10s | %s | %-32s | %-6s | %-24s | %s\n"), _T("ID"), _T("Flags"), _T("Token"), _T("UID"), _T("User name"), _T("Expires at"));
   console->printf(_T("------------+-------+----------------------------------+--------+--------------------------+----------------------\n"));
   s_tokens.forEach(
      [console] (const UserAuthenticationTokenHash& key, const shared_ptr<AuthenticationTokenDescriptor>& descriptor) -> EnumerationCallbackResult
      {
         TCHAR userName[MAX_USER_NAME];
         console->printf(_T(" %10u | %c%c%c   | %-32s | %6u | %-24s | %s\n"),
            descriptor->tokenId,
            descriptor->persistent ? _T('P') : _T('-'), descriptor->service ? _T('S') : _T('-'),
            descriptor->validClearText ? _T('V') : _T('-'),
            descriptor->validClearText ? descriptor->token.toString().cstr() : _T("unavailable"),
            descriptor->userId, ResolveUserId(descriptor->userId, userName),
            FormatTimestamp(descriptor->expirationTime).cstr());
         return _CONTINUE;
      });
}
