/*
** NetXMS - Network Management System
** Copyright (C) 2021 Raden Solutions
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
** File: authentification.h
**
**/

#ifndef _authentication_h_
#define _authentication_h_

#include "nms_common.h"

/**
 * Authentification token base class
 */
class AuthentificationToken
{
private:
   TCHAR m_methodName[MAX_OBJECT_NAME];

public:
   AuthentificationToken(const TCHAR* methodName)
   {
      _tcslcpy(m_methodName, methodName, MAX_OBJECT_NAME);
   }
   virtual ~AuthentificationToken()
   {
   }

   virtual const TCHAR *getChallenge() const { return nullptr; }

   const TCHAR *getMethodName() const { return m_methodName; };
};

/**
 * TOTP authentification token class
 */
class TOTPToken : public AuthentificationToken
{
private:
   uint8_t* m_secret;
   uint32_t m_secretLength;

public:
   TOTPToken(const TCHAR* methodName, uint8_t* secret, uint32_t secretLength)
      : AuthentificationToken(methodName)
   {
      m_secret = secret;
      m_secretLength = secretLength;
   }
   virtual ~TOTPToken()
   {
      MemFree(m_secret);
   };

   const uint8_t* getSecret() const { return m_secret; };
   uint32_t getSecretLength() const { return m_secretLength; };
};

/**
 * Message authentification token class
 */
class MessageToken : public AuthentificationToken
{
private:
   uint32_t m_secret;

public:
   MessageToken(const TCHAR* methodName, uint32_t secret)
      : AuthentificationToken(methodName), m_secret(secret)
   {
   }
   virtual ~MessageToken()
   {
   }

   uint32_t getSecret() const { return m_secret; };
};

void LoadAuthentificationMethods();
AuthentificationToken* Prepare2FAChallenge( const TCHAR* methodName, uint32_t userId);
bool Validate2FAChallenge(AuthentificationToken* token, TCHAR *challenge);
void Get2FAMethodInfo(const TCHAR* methodInfo, NXCPMessage& msg);
void Get2FAMethods(NXCPMessage& msg);
uint32_t Modify2FAMethod(const TCHAR* name, const TCHAR* methodType, const TCHAR* description, char* configuration);
uint32_t Delete2FAMethod(const TCHAR* name);

#endif   /* _authentication_h_ */
