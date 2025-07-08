/*
** NetXMS - Network Management System
** Server Core
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: jwt.h
**
**/

#ifndef _jwt_h_
#define _jwt_h_

#include <nms_util.h>

/**
 * JWT Token class
 */
class NXCORE_EXPORTABLE JwtToken
{
private:
   json_t *m_header;
   json_t *m_payload;
   BYTE *m_signature;
   size_t m_signatureLen;
   bool m_valid;

   bool verifySignature(const char *secret, const char *header, const char *payload);

public:
   JwtToken();
   ~JwtToken();

   static JwtToken *create(json_t *payload, const char *secret, time_t expiration = 0);
   static JwtToken *parse(const char *tokenString, const char *secret);

   String encode(const char *secret);
   
   String getClaim(const char *name);
   int64_t getClaimAsInt(const char *name, int64_t defaultValue = 0);
   
   bool isExpired();
   time_t getExpiration();
   
   String getJti();
   void setJti(const char *jti);
   
   bool isValid() const { return m_valid; }
};

/**
 * JWT Token Blocklist Manager
 */
class NXCORE_EXPORTABLE JwtTokenBlocklist
{
private:
   StringSet m_blockedTokens;
   Mutex m_mutex;
   THREAD m_cleanupThread;
   time_t m_lastCleanup;
   
   static void cleanupThread(void *arg);
   void cleanup();

public:
   JwtTokenBlocklist();
   ~JwtTokenBlocklist();

   void addToken(const char *jti, time_t expiration);
   bool isBlocked(const char *jti);
   void removeExpiredTokens();
   
   static JwtTokenBlocklist *getInstance();
};

#endif
