/* 
** NetXMS - Network Management System
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
** $module: dcivalue.cpp
**
**/

#include "nxcore.h"


//
// Default constructor
//

ItemValue::ItemValue()
{
   m_szString[0] = 0;
   m_iInt32 = 0;
   m_iInt64 = 0;
   m_dwInt32 = 0;
   m_qwInt64 = 0;
   m_dFloat = 0;
}


//
// Construct value object from string value
//

ItemValue::ItemValue(const char *pszValue)
{
   strncpy(m_szString, pszValue, MAX_DB_STRING);
   m_iInt32 = strtol(m_szString, NULL, 0);
   m_iInt64 = strtoll(m_szString, NULL, 0);
   m_dwInt32 = strtoul(m_szString, NULL, 0);
   m_qwInt64 = strtoull(m_szString, NULL, 0);
   m_dFloat = strtod(m_szString, NULL);
}


//
// Construct value object from another ItemValue object
//

ItemValue::ItemValue(const ItemValue *pValue)
{
   strcpy(m_szString, pValue->m_szString);
   m_iInt32 = pValue->m_iInt32;
   m_iInt64 = pValue->m_iInt64;
   m_dwInt32 = pValue->m_dwInt32;
   m_qwInt64 = pValue->m_qwInt64;
   m_dFloat = pValue->m_dFloat;
}


//
// Destructor
//

ItemValue::~ItemValue()
{
}


//
// Assignment operators
//

const ItemValue& ItemValue::operator=(ItemValue &src)
{
   strcpy(m_szString, src.m_szString);
   m_iInt32 = src.m_iInt32;
   m_iInt64 = src.m_iInt64;
   m_dwInt32 = src.m_dwInt32;
   m_qwInt64 = src.m_qwInt64;
   m_dFloat = src.m_dFloat;
   return *this;
}

const ItemValue& ItemValue::operator=(double dFloat)
{
   m_dFloat = dFloat;
   sprintf(m_szString, "%f", m_dFloat);
   m_iInt32 = (long)m_dFloat;
   m_iInt64 = (INT64)m_dFloat;
   m_dwInt32 = (DWORD)m_dFloat;
   m_qwInt64 = (QWORD)m_dFloat;
   return *this;
}

const ItemValue& ItemValue::operator=(long iInt32)
{
   m_iInt32 = iInt32;
   sprintf(m_szString, "%ld", m_iInt32);
   m_dFloat = (double)m_iInt32;
   m_iInt64 = (INT64)m_iInt32;
   m_dwInt32 = (DWORD)m_iInt32;
   m_qwInt64 = (QWORD)m_iInt32;
   return *this;
}

const ItemValue& ItemValue::operator=(INT64 iInt64)
{
   m_iInt64 = iInt64;
#ifdef _WIN32
   sprintf(m_szString, "%I64d", m_iInt64);
#else    /* _WIN32 */
   sprintf(m_szString, "%lld", m_iInt64);
#endif
   m_dFloat = (double)m_iInt64;
   m_iInt32 = (long)m_iInt64;
   m_dwInt32 = (DWORD)m_iInt64;
   m_qwInt64 = (QWORD)m_iInt64;
   return *this;
}

const ItemValue& ItemValue::operator=(DWORD dwInt32)
{
   m_dwInt32 = dwInt32;
   sprintf(m_szString, "%lu", m_dwInt32);
   m_dFloat = (double)m_dwInt32;
   m_iInt32 = (long)m_dwInt32;
   m_iInt64 = (INT64)m_dwInt32;
   m_qwInt64 = (QWORD)m_dwInt32;
   return *this;
}

const ItemValue& ItemValue::operator=(QWORD qwInt64)
{
   m_qwInt64 = qwInt64;
#ifdef _WIN32
   sprintf(m_szString, "%I64u", m_qwInt64);
#else    /* _WIN32 */
   sprintf(m_szString, "%llu", m_qwInt64);
#endif
   m_dFloat = (double)((INT64)m_qwInt64);
   m_iInt32 = (long)m_qwInt64;
   m_iInt64 = (INT64)m_qwInt64;
   return *this;
}
