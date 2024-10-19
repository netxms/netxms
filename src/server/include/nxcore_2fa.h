/*
** NetXMS - Network Management System
** Copyright (C) 2021-2024 Raden Solutions
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
class NXCORE_EXPORTABLE TwoFactorAuthenticationToken
{
private:
   TCHAR m_methodName[MAX_OBJECT_NAME];

public:
   TwoFactorAuthenticationToken(const TCHAR *methodName)
   {
      _tcslcpy(m_methodName, methodName, MAX_OBJECT_NAME);
   }
   virtual ~TwoFactorAuthenticationToken() {}

   virtual const TCHAR *getChallenge() const { return nullptr; }
   virtual const TCHAR *getQRLabel() const { return nullptr; }

   const TCHAR *getMethodName() const { return m_methodName; };
};

/**
 * Secret length (in bytes) of TOTP secret
 */
#define TOTP_SECRET_LENGTH 64

/**
 * TOTP authentication token class
 */
class NXCORE_EXPORTABLE TOTPToken : public TwoFactorAuthenticationToken
{
private:
   BYTE m_secret[TOTP_SECRET_LENGTH];
   bool m_newSecret;
   TCHAR *m_uri;

public:
   TOTPToken(const TCHAR* methodName, const BYTE* secret, const TCHAR *userName, bool newSecret);
   virtual ~TOTPToken();

   virtual const TCHAR *getQRLabel() const { return m_uri; }

   const void* getSecret() const { return m_secret; };
   bool isNewSecret() const { return m_newSecret; }
};

/**
 * Message authentication token class
 */
class NXCORE_EXPORTABLE MessageToken : public TwoFactorAuthenticationToken
{
private:
   uint32_t m_secret;

public:
   MessageToken(const TCHAR* methodName, uint32_t secret) : TwoFactorAuthenticationToken(methodName), m_secret(secret)
   {
   }
   virtual ~MessageToken() {}

   uint32_t getSecret() const { return m_secret; };
};

void LoadTwoFactorAuthenticationMethods();
TwoFactorAuthenticationToken NXCORE_EXPORTABLE *Prepare2FAChallenge(const TCHAR *methodName, uint32_t userId);
bool NXCORE_EXPORTABLE Validate2FAResponse(TwoFactorAuthenticationToken *token, TCHAR *response, uint32_t userId, BYTE **trustedDeviceToken, size_t *trustedDeviceTokenSize);
bool NXCORE_EXPORTABLE Validate2FATrustedDeviceToken(const BYTE *token, size_t size, uint32_t userId);
void Get2FADrivers(NXCPMessage *msg);
void Get2FAMethods(NXCPMessage *msg);
uint32_t Modify2FAMethod(const TCHAR *name, const TCHAR *driver, const TCHAR *description, char *configuration);
uint32_t Rename2FAMethod(const TCHAR *oldName, const TCHAR *newName);
uint32_t Delete2FAMethod(const TCHAR *name);
bool Is2FAMethodExists(const TCHAR *name);
unique_ptr<StringMap> Extract2FAMethodBindingConfiguration(const TCHAR* methodName, const Config& binding);
bool Update2FAMethodBindingConfiguration(const TCHAR* methodName, Config *binding, const StringMap& updates);

#endif   /* _nxcore_2fa_h_ */
