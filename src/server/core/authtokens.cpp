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
 * Authentication token hash
 */
typedef BYTE UserAuthenticationTokenHash[SHA256_DIGEST_SIZE];

/**
 * Authentication token descriptor
 */
struct AuthenticationTokenDescriptor
{
   UserAuthenticationToken token;
   UserAuthenticationTokenHash hash;
   uint32_t tokenId;
   uint32_t userId;
   time_t issuingTime;
   time_t expirationTime;
   bool persistent;
   bool validClearText;
   String description;

   /**
    * Create new token
    */
   AuthenticationTokenDescriptor(uint32_t uid, uint32_t validFor, bool _persistent, const TCHAR *_description) : description(_description)
   {
      BYTE bytes[USER_AUTHENTICATION_TOKEN_LENGTH];
      GenerateRandomBytes(bytes, USER_AUTHENTICATION_TOKEN_LENGTH);
      token = UserAuthenticationToken(bytes);
      CalculateSHA256Hash(bytes, USER_AUTHENTICATION_TOKEN_LENGTH, hash);
      tokenId = _persistent ? CreateUniqueId(IDG_AUTHTOKEN) : 0;
      userId = uid;
      issuingTime = time(nullptr);
      expirationTime = issuingTime + validFor;
      persistent = _persistent;
      validClearText = true;
   }

   /**
    * Create token from database record
    */
   AuthenticationTokenDescriptor(DB_RESULT hResult, int row) : description(DBGetFieldAsString(hResult, row, 4))
   {
      tokenId = DBGetFieldUInt32(hResult, row, 0);
      userId = DBGetFieldUInt32(hResult, row, 1);
      issuingTime = static_cast<time_t>(DBGetFieldInt64(hResult, row, 2));
      expirationTime = static_cast<time_t>(DBGetFieldInt64(hResult, row, 3));

      TCHAR text[128];
      DBGetField(hResult, row, 5, text, 128);
      StrToBin(text, hash, SHA256_DIGEST_SIZE);

      persistent = true;
      validClearText = false;
   }
};

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

   DB_RESULT hResult = DBSelect(hdb, _T("id,user_id,issuing_time,expiration_time,description,token_data FROM auth_tokens"));
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
UserAuthenticationToken NXCORE_EXPORTABLE IssueAuthenticationToken(uint32_t userId, uint32_t validFor, bool persistent, const TCHAR *description)
{
   TCHAR userName[MAX_USER_NAME];
   if ((userId & GROUP_FLAG) || ResolveUserId(userId, userName) == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot issue authentication token for unknown user [%u]"), userId);
      return UserAuthenticationToken();
   }

   auto descriptor = make_shared<AuthenticationTokenDescriptor>(userId, validFor, persistent, description);
   s_tokens.set(descriptor->hash, descriptor);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("New %s authentication token issued for user %s [%u]"), persistent ? _T("persistent") : _T("ephemeral"), userName, userId);

   if (persistent)
   {
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

   return descriptor->token;
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
 * Authenticate user with token. If validFor is non-zero, token expiration time will be set to current time + provided value.
 */
bool NXCORE_EXPORTABLE ValidateAuthenticationToken(const UserAuthenticationToken& token, uint32_t *userId, uint32_t validFor)
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
         console->printf(_T(" %10u | %c%c    | %-32s | %6u | %-24s | %s\n"),
            descriptor->tokenId,
            descriptor->persistent ? _T('P') : _T('-'), descriptor->validClearText ? _T('V') : _T('-'),
            descriptor->validClearText ? descriptor->token.toString().cstr() : _T("unavailable"),
            descriptor->userId, ResolveUserId(descriptor->userId, userName),
            FormatTimestamp(descriptor->expirationTime).cstr());
         return _CONTINUE;
      });
}
