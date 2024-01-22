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
   WCHAR *toStringW(WCHAR *buffer) const;
   TCHAR *toString(TCHAR *buffer) const
   {
#ifdef UNICODE
      return toStringW(buffer);
#else
      return toStringA(buffer);
#endif
   }
   String toString() const
   {
      TCHAR buffer[64];
      return String(toString(buffer));
   }

   static UserAuthenticationToken parseW(const WCHAR *s);
   static UserAuthenticationToken parseA(const char *s);
   static UserAuthenticationToken parse(const TCHAR *s)
   {
#ifdef UNICODE
      return parseW(s);
#else
      return parseA(s);
#endif
   }
};

#endif /* _auth_token_h_ */
