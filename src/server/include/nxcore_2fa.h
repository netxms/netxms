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
** File: nxcore_2fa.h
**
**/

#ifndef _nxcore_2fa_h_
#define _nxcore_2fa_h_

#include "nms_common.h"

/**
 * Two-factor authentication token base class
 */
class TwoFactorAuthenticationToken
{
private:
   TCHAR m_methodName[MAX_OBJECT_NAME];

public:
   TwoFactorAuthenticationToken(const TCHAR* methodName)
   {
      _tcslcpy(m_methodName, methodName, MAX_OBJECT_NAME);
   }
   virtual ~TwoFactorAuthenticationToken()
   {
   }

   virtual const TCHAR *getChallenge() const { return nullptr; }

   const TCHAR *getMethodName() const { return m_methodName; };
};

/**
 * TOTP authentication token class
 */
class TOTPToken : public TwoFactorAuthenticationToken
{
private:
   void* m_secret;
   size_t m_secretLength;

public:
   TOTPToken(const TCHAR* methodName, void* secret, size_t secretLength) : TwoFactorAuthenticationToken(methodName)
   {
      m_secret = secret;
      m_secretLength = secretLength;
   }
   virtual ~TOTPToken()
   {
      MemFree(m_secret);
   };

   const void* getSecret() const { return m_secret; };
   size_t getSecretLength() const { return m_secretLength; };
};

/**
 * Message authentication token class
 */
class MessageToken : public TwoFactorAuthenticationToken
{
private:
   uint32_t m_secret;

public:
   MessageToken(const TCHAR* methodName, uint32_t secret) : TwoFactorAuthenticationToken(methodName), m_secret(secret)
   {
   }
   virtual ~MessageToken()
   {
   }

   uint32_t getSecret() const { return m_secret; };
};

void LoadTwoFactorAuthenticationMethods();
TwoFactorAuthenticationToken* Prepare2FAChallenge(const TCHAR *methodName, uint32_t userId);
bool Validate2FAResponse(TwoFactorAuthenticationToken *token, TCHAR *response);
void Get2FADrivers(NXCPMessage *msg);
void Get2FAMethods(NXCPMessage *msg);
void Get2FAMethodDetails(const TCHAR* methodInfo, NXCPMessage *msg);
uint32_t Modify2FAMethod(const TCHAR* name, const TCHAR* methodType, const TCHAR* description, const char *configuration);
uint32_t Rename2FAMethod(const TCHAR *oldName, const TCHAR *newName);
uint32_t Delete2FAMethod(const TCHAR* name);
bool Is2FAMethodExists(const TCHAR* name);

#endif   /* _nxcore_2fa_h_ */
