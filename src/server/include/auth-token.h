/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: auth-token.h
**
**/

#ifndef _auth_token_h_
#define _auth_token_h_

#include <nms_util.h>

/**
 * User authentication token length
 */
#define USER_AUTHENTICATION_TOKEN_LENGTH  20

/**
 * Latest expiration time that can be assigned to a persistent authentication token. Expiration time
 * of a persistent token is stored in a 32-bit integer database column, so a token expiring beyond
 * this point cannot be written back and read as issued.
 */
#define MAX_PERSISTENT_TOKEN_EXPIRATION_TIME  _LL(0x7FFFFFFF)

/**
 * Size of authentication token description buffer, including terminating null character.
 * Matches width of auth_tokens.description database column.
 */
#define MAX_TOKEN_DESCRIPTION_LENGTH  128

/**
 * Authentication token types
 */
enum class AuthenticationTokenType
{
   EPHEMERAL = 0,    // Not saved to database, usually short-lived
   PERSISTENT = 1,   // Saved to database, usually with long expiration time
   SERVICE = 2,      // Similar to ephemeral but will not trigger existing session disconnect
   SINGLE_USE = 3    // Memory-only, destroyed by the login that spends it
};

/**
 * User authentication token
 */
class NXCORE_EXPORTABLE UserAuthenticationToken : public GenericId<USER_AUTHENTICATION_TOKEN_LENGTH>
{
public:
   UserAuthenticationToken() : GenericId<USER_AUTHENTICATION_TOKEN_LENGTH>(USER_AUTHENTICATION_TOKEN_LENGTH) { }
   UserAuthenticationToken(const BYTE *value) : GenericId<USER_AUTHENTICATION_TOKEN_LENGTH>(value, USER_AUTHENTICATION_TOKEN_LENGTH) { }
   UserAuthenticationToken(const UserAuthenticationToken& src) : GenericId<USER_AUTHENTICATION_TOKEN_LENGTH>(src) { }

   UserAuthenticationToken operator=(const UserAuthenticationToken& src)
   {
      GenericId<USER_AUTHENTICATION_TOKEN_LENGTH>::operator=(src);
      return *this;
   }

   bool equals(const UserAuthenticationToken &a) const
   {
      return GenericId<USER_AUTHENTICATION_TOKEN_LENGTH>::equals(a);
   }
   bool equals(const BYTE *value) const
   {
      return GenericId<USER_AUTHENTICATION_TOKEN_LENGTH>::equals(value, USER_AUTHENTICATION_TOKEN_LENGTH);
   }

   char *toStringA(char *buffer) const;
   wchar_t *toStringW(wchar_t *buffer) const;
   wchar_t *toString(wchar_t *buffer) const
   {
      return toStringW(buffer);
   }
   String toString() const
   {
      wchar_t buffer[64];
      return String(toString(buffer));
   }

   String toMaskedString() const
   {
      String full = toString();
      if (full.length() <= 8)
         return String(L"********");
      wchar_t buffer[64];
      nx_swprintf(buffer, 64, L"%.4s****%.4s", full.cstr(), full.cstr() + full.length() - 4);
      return String(buffer);
   }

   static UserAuthenticationToken parseW(const wchar_t *s);
   static UserAuthenticationToken parseA(const char *s);
   static UserAuthenticationToken parse(const wchar_t *s)
   {
      return parseW(s);
   }
};

/**
 * Authentication token hash
 */
typedef BYTE UserAuthenticationTokenHash[SHA256_DIGEST_SIZE];

/**
 * Authentication token descriptor
 */
struct NXCORE_EXPORTABLE AuthenticationTokenDescriptor
{
   UserAuthenticationToken token;
   UserAuthenticationTokenHash hash;
   uint32_t tokenId;
   uint32_t userId;
   AuthenticationTokenType type;
   time_t issuingTime;
   time_t expirationTime;
   time_t maxExpirationTime;  // Absolute maximum expiration time (cannot be extended beyond this)
   VolatileCounter claimed;   // Atomic claim guard for single-use tokens, 0 until consumed
   bool validClearText;       // Clear-text value is known (false for tokens restored from database)
   String description;

   /**
    * Create new token
    *
    * @param uid User ID for this token
    * @param validFor Initial validity period in seconds
    * @param _type Token type (ephemeral, persistent, service, or single-use)
    * @param _description Optional description
    * @param maxLifetime Maximum absolute lifetime in seconds (0 = no limit, only for non-persistent tokens)
    */
   AuthenticationTokenDescriptor(uint32_t uid, uint32_t validFor, AuthenticationTokenType _type, const wchar_t *_description, uint32_t maxLifetime = 0) : description(_description)
   {
      BYTE bytes[USER_AUTHENTICATION_TOKEN_LENGTH];
      GenerateRandomBytes(bytes, USER_AUTHENTICATION_TOKEN_LENGTH);
      token = UserAuthenticationToken(bytes);
      CalculateSHA256Hash(bytes, USER_AUTHENTICATION_TOKEN_LENGTH, hash);
      tokenId = (_type == AuthenticationTokenType::PERSISTENT) ? CreateUniqueId(IDG_AUTHTOKEN) : 0;
      userId = uid;
      type = _type;
      issuingTime = time(nullptr);
      expirationTime = issuingTime + validFor;
      // For persistent tokens, max expiration is the original expiration (no refresh extension allowed)
      // For other token types, max expiration is configurable (0 = no limit)
      if (_type == AuthenticationTokenType::PERSISTENT)
      {
         // Clamp to what the database column can hold, so that the token is always stored
         // and read back exactly as reported to the caller
         if (expirationTime > MAX_PERSISTENT_TOKEN_EXPIRATION_TIME)
            expirationTime = MAX_PERSISTENT_TOKEN_EXPIRATION_TIME;
         maxExpirationTime = expirationTime;
      }
      else if (maxLifetime > 0)
      {
         maxExpirationTime = issuingTime + maxLifetime;
         // Token stops authenticating at the absolute cap, so reporting a later expiration
         // time to the caller would promise validity the token will never have
         if (expirationTime > maxExpirationTime)
            expirationTime = maxExpirationTime;
      }
      else
         maxExpirationTime = 0;  // No absolute limit
      claimed = 0;
      validClearText = true;
   }

   /**
    * Create token from database record
    */
   AuthenticationTokenDescriptor(DB_RESULT hResult, int row) : description(DBGetFieldAsString(hResult, row, 4))
   {
      tokenId = DBGetFieldUInt32(hResult, row, 0);
      userId = DBGetFieldUInt32(hResult, row, 1);
      type = AuthenticationTokenType::PERSISTENT;
      issuingTime = static_cast<time_t>(DBGetFieldInt64(hResult, row, 2));
      expirationTime = static_cast<time_t>(DBGetFieldInt64(hResult, row, 3));
      maxExpirationTime = expirationTime;  // Persistent tokens cannot be extended beyond original expiration

      wchar_t text[128];
      DBGetField(hResult, row, 5, text, 128);
      StrToBin(text, hash, SHA256_DIGEST_SIZE);

      claimed = 0;
      validClearText = false;
   }

   /**
    * Fill NXCP message
    */
   void fillMessage(NXCPMessage *msg, uint32_t baseId) const
   {
      msg->setField(baseId, tokenId);
      msg->setField(baseId + 1, userId);
      msg->setField(baseId + 2, type == AuthenticationTokenType::PERSISTENT);
      msg->setField(baseId + 3, description);
      if (validClearText)
      {
         msg->setField(baseId + 4, token.toString());
      }
      msg->setFieldFromTime(baseId + 5, issuingTime);
      msg->setFieldFromTime(baseId + 6, expirationTime);
      msg->setField(baseId + 7, type == AuthenticationTokenType::SERVICE);
      msg->setField(baseId + 8, type == AuthenticationTokenType::SINGLE_USE);
   }

   /**
    * Serialize to JSON. The clear-text token value is included for as long as the descriptor
    * is held in memory, and is therefore returned by listings as well as by the issue response.
    * Persistent tokens reloaded from the database after a restart no longer carry it.
    */
   json_t *toJson() const
   {
      json_t *root = json_object();
      json_object_set_new(root, "id", json_integer(tokenId));
      json_object_set_new(root, "userId", json_integer(userId));
      json_object_set_new(root, "persistent", json_boolean(type == AuthenticationTokenType::PERSISTENT));
      json_object_set_new(root, "service", json_boolean(type == AuthenticationTokenType::SERVICE));
      json_object_set_new(root, "singleUse", json_boolean(type == AuthenticationTokenType::SINGLE_USE));
      json_object_set_new(root, "description", json_string_t(description.cstr()));
      json_object_set_new(root, "issuingTime", json_integer(static_cast<json_int_t>(issuingTime)));
      json_object_set_new(root, "expirationTime", json_integer(static_cast<json_int_t>(expirationTime)));
      if (validClearText)
         json_object_set_new(root, "value", json_string_t(token.toString().cstr()));
      return root;
   }
};

#endif /* _auth_token_h_ */
