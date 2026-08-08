/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
#include <netxms-webapi.h>

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
 * Get display name for authentication token type
 */
const wchar_t NXCORE_EXPORTABLE *AuthenticationTokenTypeName(AuthenticationTokenType type)
{
   switch(type)
   {
      case AuthenticationTokenType::PERSISTENT:
         return L"persistent";
      case AuthenticationTokenType::SERVICE:
         return L"service";
      case AuthenticationTokenType::SINGLE_USE:
         return L"single-use";
      default:
         return L"ephemeral";
   }
}

/**
 * Issue authentication token
 *
 * @param userId User ID for this token
 * @param validFor Initial validity period in seconds
 * @param type Token type (ephemeral, persistent, service, or single-use)
 * @param description Optional description
 * @param maxLifetime Maximum absolute lifetime in seconds (0 = use configured default for ephemeral, no limit for persistent)
 * @return descriptor of the issued token, or nullptr if the token cannot be issued (user ID refers to a group or to an
 *         unknown user, or a persistent token would be issued already expired). Callers must check the returned pointer.
 */
shared_ptr<AuthenticationTokenDescriptor> NXCORE_EXPORTABLE IssueAuthenticationToken(uint32_t userId, uint32_t validFor, AuthenticationTokenType type, const wchar_t *description, uint32_t maxLifetime)
{
   wchar_t userName[MAX_USER_NAME];
   if ((userId & GROUP_FLAG) || ResolveUserId(userId, userName) == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot issue authentication token for unknown user [%u]"), userId);
      return shared_ptr<AuthenticationTokenDescriptor>();
   }

   // Use configured default max lifetime for ephemeral tokens if not specified
   uint32_t effectiveMaxLifetime = maxLifetime;
   if ((effectiveMaxLifetime == 0) && (type != AuthenticationTokenType::PERSISTENT))
   {
      effectiveMaxLifetime = GetAuthTokenMaxLifetime();
   }

   auto descriptor = make_shared<AuthenticationTokenDescriptor>(userId, validFor, type, description, effectiveMaxLifetime);

   // Expiration time of a persistent token is stored in 32-bit database column and is clamped to
   // MAX_PERSISTENT_TOKEN_EXPIRATION_TIME, so past that point any persistent token would be issued
   // already expired. Check the constructed descriptor instead of the clock, so that clamping and
   // validation always see the same issuing time. Both request handlers reject an out of range
   // expiration time before reaching this point, so only internal callers can be clamped here.
   if (type == AuthenticationTokenType::PERSISTENT)
   {
      if (static_cast<int64_t>(descriptor->issuingTime) + validFor > MAX_PERSISTENT_TOKEN_EXPIRATION_TIME)
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Expiration time of persistent authentication token for user %s [%u] clamped to 2038-01-19T03:14:07Z", userName, userId);

      if (descriptor->expirationTime <= descriptor->issuingTime)
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Cannot issue persistent authentication token for user %s [%u]: token would be already expired", userName, userId);
         return shared_ptr<AuthenticationTokenDescriptor>();
      }
   }

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
         if (!DBExecute(hStmt))
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Failed to save persistent authentication token [%u] for user [%u] to database"), descriptor->tokenId, descriptor->userId);
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Failed to save persistent authentication token [%u] for user [%u] to database"), descriptor->tokenId, descriptor->userId);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("New %s authentication token issued for user %s [%u]"),
         AuthenticationTokenTypeName(type), userName, userId);
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
      nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token \"%s\" was revoked"), token.toMaskedString().cstr());
      if (descriptor->type == AuthenticationTokenType::PERSISTENT)
         DeleteAuthenticationTokenFromDB(descriptor->tokenId);
   }
}

/**
 * Revoke authentication token
 */
uint32_t NXCORE_EXPORTABLE RevokeAuthenticationToken(uint32_t tokenId, uint32_t userId)
{
   // Only persistent tokens are assigned an ID; ephemeral and service tokens all carry 0, so
   // a lookup by ID 0 would match an arbitrary one of them instead of the intended token
   if (tokenId == 0)
      return RCC_INVALID_TOKEN_ID;

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
   if (descriptor->type == AuthenticationTokenType::PERSISTENT)
      DeleteAuthenticationTokenFromDB(tokenId);
   return RCC_SUCCESS;
}

/**
 * Revoke all authentication tokens for a user
 */
void NXCORE_EXPORTABLE RevokeAuthenticationTokensForUser(uint32_t userId)
{
   SharedObjectArray<AuthenticationTokenDescriptor> tokensToRevoke;
   s_tokens.forEach(
      [userId, &tokensToRevoke] (const UserAuthenticationTokenHash& key, const shared_ptr<AuthenticationTokenDescriptor>& descriptor) -> EnumerationCallbackResult
      {
         if (descriptor->userId == userId)
            tokensToRevoke.add(descriptor);
         return _CONTINUE;
      });

   for (int i = 0; i < tokensToRevoke.size(); i++)
   {
      AuthenticationTokenDescriptor *d = tokensToRevoke.get(i);
      s_tokens.remove(d->hash);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Revoked authentication token [%u] for deleted user [%u]"), d->tokenId, userId);
      if (d->type == AuthenticationTokenType::PERSISTENT)
         DeleteAuthenticationTokenFromDB(d->tokenId);
   }
}

/**
 * Common implementation for authentication token validation and consumption. If validFor is non-zero, token expiration time
 * will be set to current time + provided value, but not exceeding the maximum expiration time (absolute token lifetime cap).
 *
 * @param token Authentication token to validate
 * @param consuming true if called from the designated spend point, in which case a single-use token is accepted and destroyed
 * @param userId Output: user ID associated with the token
 * @param tokenType Output: type of the token
 * @param validFor If non-zero, extend token expiration by this many seconds
 * @param expiresAt Output: token expiration time (for warning header calculation)
 * @param maxExpiresAt Output: maximum token expiration time (0 if no limit)
 * @return true if token is valid
 */
static bool ProcessAuthenticationToken(const UserAuthenticationToken& token, bool consuming, uint32_t *userId,
   AuthenticationTokenType *tokenType, uint32_t validFor, time_t *expiresAt, time_t *maxExpiresAt)
{
   UserAuthenticationTokenHash hash;
   CalculateSHA256Hash(token.value(), USER_AUTHENTICATION_TOKEN_LENGTH, hash);
   shared_ptr<AuthenticationTokenDescriptor> descriptor = s_tokens.getShared(hash);
   if (descriptor == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token \"%s\" does not exist"), token.toMaskedString().cstr());
      return false;
   }
   time_t now = time(nullptr);

   // Check against absolute maximum expiration time first (if set)
   if ((descriptor->maxExpirationTime > 0) && (descriptor->maxExpirationTime <= now))
   {
      s_tokens.remove(hash);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token [%u] \"%s\" has reached maximum lifetime"), descriptor->tokenId, token.toMaskedString().cstr());
      if (descriptor->type == AuthenticationTokenType::PERSISTENT)
         DeleteAuthenticationTokenFromDB(descriptor->tokenId);
      return false;
   }

   if (descriptor->expirationTime <= now)
   {
      s_tokens.remove(hash);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token [%u] \"%s\" has expired"), descriptor->tokenId, token.toMaskedString().cstr());
      if (descriptor->type == AuthenticationTokenType::PERSISTENT)
         DeleteAuthenticationTokenFromDB(descriptor->tokenId);
      return false;
   }

   if (descriptor->type == AuthenticationTokenType::SINGLE_USE)
   {
      if (!consuming)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"Single-use authentication token [%u] \"%s\" presented outside of login entry point",
            descriptor->tokenId, token.toMaskedString().cstr());
         return false;
      }

      // Token is burnt as soon as it is claimed, even if the login it was claimed for subsequently fails -
      // a one-shot credential that survives a failed attempt would be a retry oracle.
      if (InterlockedIncrement(&descriptor->claimed) != 1)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"Single-use authentication token [%u] \"%s\" was already consumed",
            descriptor->tokenId, token.toMaskedString().cstr());
         return false;
      }
      s_tokens.remove(hash);
      nxlog_debug_tag(DEBUG_TAG, 5, L"Single-use authentication token [%u] \"%s\" consumed", descriptor->tokenId, token.toMaskedString().cstr());
   }

   if (validFor > 0)
   {
      // Extend expiration time but do not exceed maximum expiration time (if set) and never shorten existing expiration
      time_t newExpiration = now + validFor;
      if ((descriptor->maxExpirationTime > 0) && (newExpiration > descriptor->maxExpirationTime))
         newExpiration = descriptor->maxExpirationTime;
      if (newExpiration > descriptor->expirationTime)
         descriptor->expirationTime = newExpiration;
   }
   *userId = descriptor->userId;
   if (tokenType != nullptr)
      *tokenType = descriptor->type;
   if (expiresAt != nullptr)
      *expiresAt = descriptor->expirationTime;
   if (maxExpiresAt != nullptr)
      *maxExpiresAt = descriptor->maxExpirationTime;
   return true;
}

/**
 * Validate authentication token. Never consumes the token, and always rejects single-use tokens -
 * those can only be spent by ConsumeAuthenticationToken().
 */
bool NXCORE_EXPORTABLE ValidateAuthenticationToken(const UserAuthenticationToken& token, uint32_t *userId, uint32_t validFor, time_t *expiresAt, time_t *maxExpiresAt)
{
   return ProcessAuthenticationToken(token, false, userId, nullptr, validFor, expiresAt, maxExpiresAt);
}

/**
 * Validate authentication token and consume it if it is single-use. This is the single designated spend
 * point for single-use tokens and must be called from client session login only.
 *
 * Deliberately not marked as NXCORE_EXPORTABLE: the only caller lives in the same shared library, and
 * leaving the decoration off keeps the spend primitive out of reach of server modules.
 */
bool ConsumeAuthenticationToken(const UserAuthenticationToken& token, uint32_t *userId, AuthenticationTokenType *tokenType)
{
   return ProcessAuthenticationToken(token, true, userId, tokenType, 0, nullptr, nullptr);
}

/**
 * Check for expired authentication tokens
 */
void CheckUserAuthenticationTokens(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   SharedObjectArray<AuthenticationTokenDescriptor> expiredTokens;
   time_t now = time(nullptr);
   s_tokens.forEach(
      [&expiredTokens, now] (const UserAuthenticationTokenHash& key, const shared_ptr<AuthenticationTokenDescriptor>& descriptor) -> EnumerationCallbackResult
      {
         // Check both regular expiration and maximum absolute expiration (if max is set)
         bool maxExpired = (descriptor->maxExpirationTime > 0) && (descriptor->maxExpirationTime <= now);
         if ((descriptor->expirationTime <= now) || maxExpired)
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("User authentication token \"%s\" has expired%s"),
               descriptor->maskedValueText(),
               maxExpired ? _T(" (max lifetime reached)") : _T(""));
            expiredTokens.add(descriptor);
         }
         return _CONTINUE;
      });
   for(int i = 0; i < expiredTokens.size(); i++)
   {
      AuthenticationTokenDescriptor *d = expiredTokens.get(i);
      const UserAuthenticationTokenHash& hash = d->hash;
      s_tokens.remove(hash);
      if (d->type == AuthenticationTokenType::PERSISTENT)
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
 * Build JSON array with all authentication tokens for given user
 */
json_t NXCORE_EXPORTABLE *AuthenticationTokensToJson(uint32_t userId)
{
   json_t *array = json_array();
   s_tokens.forEach(
      [userId, array] (const UserAuthenticationTokenHash& key, const shared_ptr<AuthenticationTokenDescriptor>& descriptor) -> EnumerationCallbackResult
      {
         if (descriptor->userId == userId)
            json_array_append_new(array, descriptor->toJson());
         return _CONTINUE;
      });
   return array;
}

/**
 * Print authentication tokens
 */
void ShowAuthenticationTokens(ServerConsole *console)
{
   console->printf(L" %-10s | %-10s | %-12s | %-6s | %-24s | %s\n", L"ID", L"Type", L"Token", L"UID", L"User name", L"Expires at");
   console->printf(L"------------+------------+--------------+--------+--------------------------+----------------------\n");
   s_tokens.forEach(
      [console] (const UserAuthenticationTokenHash& key, const shared_ptr<AuthenticationTokenDescriptor>& descriptor) -> EnumerationCallbackResult
      {
         wchar_t userName[MAX_USER_NAME];
         console->printf(L" %10u | %-10s | %-12s | %6u | %-24s | %s\n",
            descriptor->tokenId,
            AuthenticationTokenTypeName(descriptor->type),
            descriptor->maskedValueText(),
            descriptor->userId, ResolveUserId(descriptor->userId, userName, true),
            FormatTimestamp(descriptor->expirationTime).cstr());
         return _CONTINUE;
      });
}
