/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: oid.cpp
**
**/

#include "libnxsnmp.h"


//
// SNMP_ObjectId default constructor
//

SNMP_ObjectId::SNMP_ObjectId()
{
   m_dwLength = 0;
   m_pdwValue = NULL;
   m_pszTextValue = NULL;
}


//
// SNMP_ObjectId constructor from existing gvalue
//

SNMP_ObjectId::SNMP_ObjectId(DWORD dwLength, DWORD *pdwValue)
{
   m_dwLength = dwLength;
   m_pdwValue = (DWORD *)nx_memdup(pdwValue, sizeof(DWORD) * dwLength);
   m_pszTextValue = NULL;
   ConvertToText();
}


//
// SNMP_ObjectId destructor
//

SNMP_ObjectId::~SNMP_ObjectId()
{
   safe_free(m_pdwValue);
   safe_free(m_pszTextValue);
}


//
// Convert binary representation to text
//

void SNMP_ObjectId::ConvertToText(void)
{
   DWORD i, dwBufPos, dwNumChars;

   m_pszTextValue = (TCHAR *)realloc(m_pszTextValue, sizeof(TCHAR) * (m_dwLength * 6 + 1));
   m_pszTextValue[0] = 0;
   for(i = 0, dwBufPos = 0; i < m_dwLength; i++)
   {
      dwNumChars = _stprintf(&m_pszTextValue[dwBufPos], _T(".%d"), m_pdwValue[i]);
      dwBufPos += dwNumChars;
   }
}
