/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2005 Victor Kirhenshtein
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
** $module: value.cpp
**
**/

#include "libnxsl.h"


//
// Constructors
//

NXSL_Value::NXSL_Value(void)
{
   m_dwFlags = VALUE_IS_NULL;
   m_pszValStr = NULL;
}

NXSL_Value::NXSL_Value(NXSL_Value *pValue)
{
   if (pValue != NULL)
   {
      m_dwFlags = pValue->m_dwFlags;
      if (pValue->m_dwFlags & VALUE_STRING_IS_VALID)
      {
         m_dwStrLen = pValue->m_dwStrLen;
         m_pszValStr = (char *)nx_memdup(pValue->m_pszValStr, m_dwStrLen + 1);
      }
      else
      {
         m_pszValStr = NULL;
      }
      m_iValInt = pValue->m_iValInt;
      m_dValFloat = pValue->m_dValFloat;
   }
   else
   {
      m_dwFlags = VALUE_IS_NULL;
      m_pszValStr = NULL;
   }
}

NXSL_Value::NXSL_Value(int nValue)
{
   m_dwFlags = VALUE_IS_NUMERIC;
   m_pszValStr = NULL;
   m_iValInt = nValue;
   m_dValFloat = nValue;
}

NXSL_Value::NXSL_Value(double dValue)
{
   m_pszValStr = NULL;
   m_iValInt = (int)dValue;
   m_dValFloat = dValue;
   m_dwFlags = VALUE_IS_NUMERIC;
   if ((double)m_iValInt != m_dValFloat)
      m_dwFlags |= VALUE_IS_REAL;
}

NXSL_Value::NXSL_Value(char *pszValue)
{
   m_dwStrLen = strlen(pszValue);
   m_pszValStr = strdup(pszValue);
   UpdateNumbers();
}


//
// Destructor
//

NXSL_Value::~NXSL_Value()
{
   safe_free(m_pszValStr);
}


//
// Update flags and values after string change
//

void NXSL_Value::UpdateNumbers(void)
{
   char *eptr;

   m_dwFlags = VALUE_STRING_IS_VALID;
   m_iValInt = strtol(m_pszValStr, &eptr, 0);
   if ((*eptr == 0) && ((DWORD)(eptr - m_pszValStr) == m_dwStrLen))
   {
      m_dwFlags |= VALUE_IS_NUMERIC;
      m_dValFloat = m_iValInt;
   }
   else
   {
      m_dValFloat = strtod(m_pszValStr, &eptr);
      if ((*eptr == 0) && ((DWORD)(eptr - m_pszValStr) == m_dwStrLen))
      {
         m_dwFlags |= VALUE_IS_NUMERIC;
         m_iValInt = (int)m_dValFloat;
      }
   }
   if ((double)m_iValInt != m_dValFloat)
      m_dwFlags |= VALUE_IS_REAL;
}


//
// Update string value
//

void NXSL_Value::UpdateString(void)
{
   char szBuffer[64];

   safe_free(m_pszValStr);
   if (m_dwFlags & VALUE_IS_REAL)
      sprintf(szBuffer, "%f", m_dValFloat);
   else if (m_dwFlags & VALUE_IS_NUMERIC)
      sprintf(szBuffer, "%d", m_iValInt);
   else
      szBuffer[0] = 0;
   m_dwStrLen = strlen(szBuffer);
   m_pszValStr = strdup(szBuffer);
   m_dwFlags |= VALUE_STRING_IS_VALID;
}


//
// Get value as ASCIIZ string
//

char *NXSL_Value::GetValueAsCString(void)
{
   if (m_dwFlags & VALUE_IS_NULL)
      return NULL;

   if (!(m_dwFlags & VALUE_STRING_IS_VALID))
      UpdateString();
   return m_pszValStr;
}


//
// Get value as string
//

char *NXSL_Value::GetValueAsString(DWORD *pdwLen)
{
   if (m_dwFlags & VALUE_IS_NULL)
      return NULL;

   if (!(m_dwFlags & VALUE_STRING_IS_VALID))
      UpdateString();
   *pdwLen = m_dwStrLen;
   return m_pszValStr;
}


//
// Get value as integer
//

int NXSL_Value::GetValueAsInt(void)
{
   if (m_dwFlags & VALUE_IS_NULL)
      return 0;

   return (m_dwFlags & VALUE_IS_NUMERIC) ? m_iValInt : 0;
}


//
// Get value as real number
//

double NXSL_Value::GetValueAsReal(void)
{
   if (m_dwFlags & VALUE_IS_NULL)
      return 0;

   return (m_dwFlags & VALUE_IS_NUMERIC) ? m_dValFloat : 0;
}


//
// Concatenate string value
//

void NXSL_Value::Concatenate(char *pszString, DWORD dwLen)
{
   if (!(m_dwFlags & VALUE_STRING_IS_VALID))
      UpdateString();
   m_pszValStr = (char *)realloc(m_pszValStr, m_dwStrLen + dwLen + 1);
   memcpy(&m_pszValStr[m_dwStrLen], pszString, dwLen);
   m_dwStrLen += dwLen;
   m_pszValStr[m_dwStrLen] = 0;
   UpdateNumbers();
}


//
// Increment value
//

void NXSL_Value::Increment(void)
{
   if (m_dwFlags & VALUE_IS_NUMERIC)
   {
      if (m_dwFlags & VALUE_IS_REAL)
      {
         m_dValFloat++;
         m_iValInt = (int)m_dValFloat;
      }
      else
      {
         m_iValInt++;
         m_dValFloat = m_iValInt;
      }
      safe_free(m_pszValStr);
      m_pszValStr = NULL;
      m_dwFlags &= ~VALUE_STRING_IS_VALID;
   }
}


//
// Decrement value
//

void NXSL_Value::Decrement(void)
{
   if (m_dwFlags & VALUE_IS_NUMERIC)
   {
      if (m_dwFlags & VALUE_IS_REAL)
      {
         m_dValFloat--;
         m_iValInt = (int)m_dValFloat;
      }
      else
      {
         m_iValInt--;
         m_dValFloat = m_iValInt;
      }
      safe_free(m_pszValStr);
      m_pszValStr = NULL;
      m_dwFlags &= ~VALUE_STRING_IS_VALID;
   }
}


//
// Negate value
//

void NXSL_Value::Negate(void)
{
   if (m_dwFlags & VALUE_IS_NUMERIC)
   {
      if (m_dwFlags & VALUE_IS_REAL)
      {
         m_dValFloat = -m_dValFloat;
         m_iValInt = (int)m_dValFloat;
      }
      else
      {
         m_iValInt = -m_iValInt;
         m_dValFloat = m_iValInt;
      }
      safe_free(m_pszValStr);
      m_pszValStr = NULL;
      m_dwFlags &= ~VALUE_STRING_IS_VALID;
   }
}


//
// Set to numeric value
//

void NXSL_Value::Set(int nValue)
{
   safe_free(m_pszValStr);
   m_dwFlags = VALUE_IS_NUMERIC;
   m_pszValStr = NULL;
   m_iValInt = nValue;
   m_dValFloat = nValue;
}
