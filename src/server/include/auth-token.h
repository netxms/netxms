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
 * Authentication token types
 */
enum class AuthenticationTokenType
{
   EPHEMERAL = 0,    // Not saved to database, usually short-lived
   PERSISTENT = 1,   // Saved to database, usually with long expiration time
   SERVICE = 2       // Similar to ephemeral but will not trigger existing session disconnect
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
      TCHAR buffer[64];
      return String(toString(buffer));
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
   time_t issuingTime;
   time_t expirationTime;
   bool persistent;
   bool service;
   bool validClearText;
   String description;

   /**
    * Create new token
    */
   AuthenticationTokenDescriptor(uint32_t uid, uint32_t validFor, AuthenticationTokenType type, const TCHAR *_description) : description(_description)
   {
      BYTE bytes[USER_AUTHENTICATION_TOKEN_LENGTH];
      GenerateRandomBytes(bytes, USER_AUTHENTICATION_TOKEN_LENGTH);
      token = UserAuthenticationToken(bytes);
      CalculateSHA256Hash(bytes, USER_AUTHENTICATION_TOKEN_LENGTH, hash);
      tokenId = (type == AuthenticationTokenType::PERSISTENT) ? CreateUniqueId(IDG_AUTHTOKEN) : 0;
      userId = uid;
      issuingTime = time(nullptr);
      expirationTime = issuingTime + validFor;
      persistent = (type == AuthenticationTokenType::PERSISTENT);
      service = (type == AuthenticationTokenType::SERVICE);
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
      service = false;
      validClearText = false;
   }

   /**
    * Fill NXCP message
    */
   void fillMessage(NXCPMessage *msg, uint32_t baseId) const
   {
      msg->setField(baseId, tokenId);
      msg->setField(baseId + 1, userId);
      msg->setField(baseId + 2, persistent);
      msg->setField(baseId + 3, description);
      if (validClearText)
      {
         msg->setField(baseId + 4, token.toString());
      }
      msg->setFieldFromTime(baseId + 5, issuingTime);
      msg->setFieldFromTime(baseId + 6, expirationTime);
      msg->setField(baseId + 7, service);
   }
};

#ifdef _WIN32
template class NXCORE_TEMPLATE_EXPORTABLE shared_ptr<AuthenticationTokenDescriptor>;
#endif

#endif /* _auth_token_h_ */
