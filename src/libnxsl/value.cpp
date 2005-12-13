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
   m_dwFlags = VALUE_IS_NULL | VALUE_VALID_INT | VALUE_VALID_FLOAT;
   m_pszValStr = NULL;
   m_iValInt = 0;
   m_dValFloat = 0;
}

NXSL_Value::NXSL_Value(int nValue)
{
   m_dwFlags = VALUE_VALID_INT | VALUE_VALID_FLOAT;
   m_pszValStr = NULL;
   m_iValInt = nValue;
   m_dValFloat = nValue;
}

NXSL_Value::NXSL_Value(char *pszValue)
{
   m_dwFlags = VALUE_VALID_STRING | VALUE_VALID_INT | VALUE_VALID_FLOAT;
   m_pszValStr = strdup(pszValue);
   m_iValInt = strtol(pszValue, NULL, 0);
   m_dValFloat = strtod(pszValue, NULL);
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

   if (!(m_dwFlags & VALUE_VALID_STRING))
   {
      safe_free(m_pszValStr);
      if (m_dwFlags & VALUE_VALID_FLOAT)
         sprintf(szBuffer, "%f", m_dValFloat);
      else if (m_dwFlags & VALUE_VALID_INT)
         sprintf(szBuffer, "%d", m_iValInt);
      else
         szBuffer[0] = 0;
      m_pszValStr = strdup(szBuffer);
      m_dwFlags |= VALUE_VALID_STRING;
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

   if (!(m_dwFlags & VALUE_VALID_INT))
   {
      if (m_dwFlags & VALUE_VALID_FLOAT)
         m_iValInt = (int)m_dValFloat;
      else if (m_dwFlags & VALUE_VALID_STRING)
         m_iValInt = strtol(m_pszValStr, NULL, 0);
      else
         m_iValInt = 0;
      m_dwFlags |= VALUE_VALID_INT;
   }
   return m_iValInt;
}
