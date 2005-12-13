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
         m_pszValStr = _tcsdup(pValue->m_pszValStr);
      else
         m_pszValStr = NULL;
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
   m_dwFlags = VALUE_IS_NUMERIC | VALUE_IS_REAL;
   m_pszValStr = NULL;
   m_iValInt = (int)dValue;
   m_dValFloat = dValue;
}

NXSL_Value::NXSL_Value(char *pszValue)
{
   char *eptr;

   m_dwFlags = VALUE_STRING_IS_VALID;
   m_pszValStr = strdup(pszValue);
   m_iValInt = strtol(pszValue, &eptr, 0);
   if (*eptr == 0)
   {
      m_dwFlags |= VALUE_IS_NUMERIC;
      m_dValFloat = m_iValInt;
   }
   else
   {
      m_dValFloat = strtod(pszValue, &eptr);
      if (*eptr == 0)
      {
         m_dwFlags |= VALUE_IS_NUMERIC | VALUE_IS_REAL;
         m_iValInt = (int)m_dValFloat;
      }
   }
}


//
// Destructor
//

NXSL_Value::~NXSL_Value()
{
   safe_free(m_pszValStr);
}


//
// Get value as string
//

char *NXSL_Value::GetValueAsString(void)
{
   char szBuffer[64];

   if (m_dwFlags & VALUE_IS_NULL)
      return "";

   if (!(m_dwFlags & VALUE_STRING_IS_VALID))
   {
      safe_free(m_pszValStr);
      if (m_dwFlags & VALUE_IS_REAL)
         sprintf(szBuffer, "%f", m_dValFloat);
      else if (m_dwFlags & VALUE_IS_NUMERIC)
         sprintf(szBuffer, "%d", m_iValInt);
      else
         szBuffer[0] = 0;
      m_pszValStr = strdup(szBuffer);
      m_dwFlags |= VALUE_STRING_IS_VALID;
   }
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
