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
// Assign value to correct number field
//

#define ASSIGN_NUMERIC_VALUE(x) \
{ \
   switch(m_nDataType) \
   { \
      case NXSL_DT_INT32: \
         m_value.nInt32 = (x); \
         break; \
      case NXSL_DT_UINT32: \
         m_value.uInt32 = (x); \
         break; \
      case NXSL_DT_INT64: \
         m_value.nInt64 = (x); \
         break; \
      case NXSL_DT_UINT64: \
         m_value.uInt64 = (x); \
         break; \
      case NXSL_DT_REAL: \
         m_value.dReal = (x); \
         break; \
      default: \
         break; \
   } \
}


//
// Retrieve correct number field
//

#define RETRIEVE_NUMERIC_VALUE(x, dt) \
{ \
   switch(m_nDataType) \
   { \
      case NXSL_DT_INT32: \
         x = (dt)m_value.nInt32; \
         break; \
      case NXSL_DT_UINT32: \
         x = (dt)m_value.uInt32; \
         break; \
      case NXSL_DT_INT64: \
         x = (dt)m_value.nInt64; \
         break; \
      case NXSL_DT_UINT64: \
         x = (dt)m_value.uInt64; \
         break; \
      case NXSL_DT_REAL: \
         x = (dt)m_value.dReal; \
         break; \
      default: \
         x = 0; \
         break; \
   } \
}


//
// Constructors
//

NXSL_Value::NXSL_Value(void)
{
   m_nDataType = NXSL_DT_NULL;
   m_pszValStr = NULL;
   m_bStringIsValid = FALSE;
}

NXSL_Value::NXSL_Value(NXSL_Value *pValue)
{
   if (pValue != NULL)
   {
      m_nDataType = pValue->m_nDataType;
      if (m_nDataType == NXSL_DT_OBJECT)
      {
         m_value.pObject = new NXSL_Object(pValue->m_value.pObject);
      }
      else
      {
         memcpy(&m_value, &pValue->m_value, sizeof(m_value));
      }
      m_bStringIsValid = pValue->m_bStringIsValid;
      if (m_bStringIsValid)
      {
         m_dwStrLen = pValue->m_dwStrLen;
         m_pszValStr = (char *)nx_memdup(pValue->m_pszValStr, m_dwStrLen + 1);
      }
      else
      {
         m_pszValStr = NULL;
      }
   }
   else
   {
      m_nDataType = NXSL_DT_NULL;
      m_pszValStr = NULL;
   }
}

NXSL_Value::NXSL_Value(NXSL_Object *pObject)
{
   m_nDataType = NXSL_DT_OBJECT;
   m_value.pObject = pObject;
   m_pszValStr = NULL;
   m_bStringIsValid = FALSE;
}

NXSL_Value::NXSL_Value(LONG nValue)
{
   m_nDataType = NXSL_DT_INT32;
   m_pszValStr = NULL;
   m_bStringIsValid = FALSE;
   m_value.nInt32 = nValue;
}

NXSL_Value::NXSL_Value(DWORD uValue)
{
   m_nDataType = NXSL_DT_UINT32;
   m_pszValStr = NULL;
   m_bStringIsValid = FALSE;
   m_value.uInt32 = uValue;
}

NXSL_Value::NXSL_Value(INT64 nValue)
{
   m_nDataType = NXSL_DT_INT64;
   m_pszValStr = NULL;
   m_bStringIsValid = FALSE;
   m_value.nInt64 = nValue;
}

NXSL_Value::NXSL_Value(QWORD uValue)
{
   m_nDataType = NXSL_DT_UINT64;
   m_pszValStr = NULL;
   m_bStringIsValid = FALSE;
   m_value.uInt64 = uValue;
}

NXSL_Value::NXSL_Value(double dValue)
{
   m_nDataType = NXSL_DT_REAL;
   m_pszValStr = NULL;
   m_bStringIsValid = FALSE;
   m_value.dReal = dValue;
}

NXSL_Value::NXSL_Value(char *pszValue)
{
   m_nDataType = NXSL_DT_STRING;
   m_dwStrLen = (DWORD)strlen(pszValue);
   m_pszValStr = strdup(pszValue);
   m_bStringIsValid = TRUE;
   UpdateNumber();
}


//
// Destructor
//

NXSL_Value::~NXSL_Value()
{
   safe_free(m_pszValStr);
   if (m_nDataType == NXSL_DT_OBJECT)
      delete m_value.pObject;
}


//
// Set value
//

void NXSL_Value::Set(LONG nValue)
{
   m_nDataType = NXSL_DT_INT32;
   safe_free(m_pszValStr);
   m_pszValStr = NULL;
   m_bStringIsValid = FALSE;
   m_value.nInt32 = nValue;
}


//
// Update numeric value after string change
//

void NXSL_Value::UpdateNumber(void)
{
   char *eptr;
   INT64 nVal;
   double dVal;

   nVal = strtoll(m_pszValStr, &eptr, 0);
   if ((*eptr == 0) && ((DWORD)(eptr - m_pszValStr) == m_dwStrLen))
   {
      if (nVal > 0x7FFFFFFF)
      {
         m_nDataType = NXSL_DT_INT64;
         m_value.nInt64 = nVal;
      }
      else
      {
         m_nDataType = NXSL_DT_INT32;
         m_value.nInt32 = (LONG)nVal;
      }
   }
   else
   {
      dVal = strtod(m_pszValStr, &eptr);
      if ((*eptr == 0) && ((DWORD)(eptr - m_pszValStr) == m_dwStrLen))
      {
         m_nDataType = NXSL_DT_REAL;
         m_value.dReal = dVal;
      }
   }
}


//
// Update string value
//

void NXSL_Value::UpdateString(void)
{
   char szBuffer[64];

   safe_free(m_pszValStr);
   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         sprintf(szBuffer, "%d", m_value.nInt32);
         break;
      case NXSL_DT_UINT32:
         sprintf(szBuffer, "%u", m_value.uInt32);
         break;
      case NXSL_DT_INT64:
         sprintf(szBuffer, INT64_FMT, m_value.nInt64);
         break;
      case NXSL_DT_UINT64:
         sprintf(szBuffer, UINT64_FMT, m_value.uInt64);
         break;
      case NXSL_DT_REAL:
         sprintf(szBuffer, "%f", m_value.dReal);
         break;
      default:
         szBuffer[0] = 0;
         break;
   }
   m_dwStrLen = (DWORD)strlen(szBuffer);
   m_pszValStr = strdup(szBuffer);
   m_bStringIsValid = TRUE;
}


//
// Convert to another data type
//

BOOL NXSL_Value::Convert(int nDataType)
{
   LONG nInt32;
   DWORD uInt32;
   INT64 nInt64;
   QWORD uInt64;
   double dReal;
   BOOL bRet = TRUE;

   if (m_nDataType == nDataType)
      return TRUE;

   switch(nDataType)
   {
      case NXSL_DT_INT32:
         RETRIEVE_NUMERIC_VALUE(nInt32, LONG);
         m_nDataType = nDataType;
         m_value.nInt32 = nInt32;
         break;
      case NXSL_DT_UINT32:
         RETRIEVE_NUMERIC_VALUE(uInt32, DWORD);
         m_nDataType = nDataType;
         m_value.uInt32 = uInt32;
         break;
      case NXSL_DT_INT64:
         RETRIEVE_NUMERIC_VALUE(nInt64, INT64);
         m_nDataType = nDataType;
         m_value.nInt64 = nInt64;
         break;
      case NXSL_DT_UINT64:
         RETRIEVE_NUMERIC_VALUE(uInt64, QWORD);
         m_nDataType = nDataType;
         m_value.uInt64 = uInt64;
         break;
      case NXSL_DT_REAL:
         if (m_nDataType == NXSL_DT_UINT64)
         {
            bRet = FALSE;
         }
         else
         {
            dReal = GetValueAsReal();
            m_nDataType = nDataType;
            m_value.dReal = dReal;
         }
         break;
      default:
         bRet = FALSE;
         break;
   }
   if (bRet)
      InvalidateString();
   return bRet;
}


//
// Get value as ASCIIZ string
//

char *NXSL_Value::GetValueAsCString(void)
{
   if (IsNull() || IsObject())
      return NULL;

   if (!m_bStringIsValid)
      UpdateString();
   return m_pszValStr;
}


//
// Get value as string
//

char *NXSL_Value::GetValueAsString(DWORD *pdwLen)
{
   if (IsNull() || IsObject())
      return NULL;

   if (!m_bStringIsValid)
      UpdateString();
   *pdwLen = m_dwStrLen;
   return m_pszValStr;
}


//
// Get value as 32 bit integer
//

LONG NXSL_Value::GetValueAsInt32(void)
{
   LONG nVal;

   RETRIEVE_NUMERIC_VALUE(nVal, LONG);
   return nVal;
}


//
// Get value as unsigned 32 bit integer
//

DWORD NXSL_Value::GetValueAsUInt32(void)
{
   DWORD uVal;

   RETRIEVE_NUMERIC_VALUE(uVal, DWORD);
   return uVal;
}


//
// Get value as 64 bit integer
//

INT64 NXSL_Value::GetValueAsInt64(void)
{
   INT64 nVal;

   RETRIEVE_NUMERIC_VALUE(nVal, INT64);
   return nVal;
}


//
// Get value as unsigned 64 bit integer
//

QWORD NXSL_Value::GetValueAsUInt64(void)
{
   QWORD uVal;

   RETRIEVE_NUMERIC_VALUE(uVal, QWORD);
   return uVal;
}


//
// Get value as real number
//

double NXSL_Value::GetValueAsReal(void)
{
   double dVal;

   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         dVal = (double)m_value.nInt32;
         break;
      case NXSL_DT_UINT32:
         dVal = (double)m_value.uInt32;
         break;
      case NXSL_DT_INT64:
         dVal = (double)m_value.nInt64;
         break;
      case NXSL_DT_UINT64:
         dVal = (double)((INT64)m_value.uInt64);
         break;
      case NXSL_DT_REAL:
         dVal = m_value.dReal;
         break;
      default:
         dVal = 0;
         break;
   }
   return dVal;
}


//
// Concatenate string value
//

void NXSL_Value::Concatenate(char *pszString, DWORD dwLen)
{
   if (!m_bStringIsValid)
      UpdateString();
   m_pszValStr = (char *)realloc(m_pszValStr, m_dwStrLen + dwLen + 1);
   memcpy(&m_pszValStr[m_dwStrLen], pszString, dwLen);
   m_dwStrLen += dwLen;
   m_pszValStr[m_dwStrLen] = 0;
   UpdateNumber();
}


//
// Increment value
//

void NXSL_Value::Increment(void)
{
   if (IsNumeric())
   {
      switch(m_nDataType)
      {
         case NXSL_DT_INT32:
            m_value.nInt32++;
            break;
         case NXSL_DT_UINT32:
            m_value.uInt32++;
            break;
         case NXSL_DT_INT64:
            m_value.nInt64++;
            break;
         case NXSL_DT_UINT64:
            m_value.uInt64++;
            break;
         case NXSL_DT_REAL:
            m_value.dReal++;
            break;
         default:
            break;
      }
      InvalidateString();
   }
}


//
// Decrement value
//

void NXSL_Value::Decrement(void)
{
   if (IsNumeric())
   {
      switch(m_nDataType)
      {
         case NXSL_DT_INT32:
            m_value.nInt32++;
            break;
         case NXSL_DT_UINT32:
            m_value.uInt32++;
            break;
         case NXSL_DT_INT64:
            m_value.nInt64++;
            break;
         case NXSL_DT_UINT64:
            m_value.uInt64++;
            break;
         case NXSL_DT_REAL:
            m_value.dReal++;
            break;
         default:
            break;
      }
      InvalidateString();
   }
}


//
// Negate value
//

void NXSL_Value::Negate(void)
{
   if (IsNumeric())
   {
      switch(m_nDataType)
      {
         case NXSL_DT_INT32:
            m_value.nInt32 = -m_value.nInt32;
            break;
         case NXSL_DT_UINT32:
            m_value.nInt32 = -((LONG)m_value.uInt32);
            m_nDataType = NXSL_DT_INT32;
            break;
         case NXSL_DT_INT64:
            m_value.nInt64 = -m_value.nInt64;
            break;
         case NXSL_DT_UINT64:
            m_value.nInt64 = -((INT64)m_value.uInt64);
            m_nDataType = NXSL_DT_INT64;
            break;
         case NXSL_DT_REAL:
            m_value.dReal = -m_value.dReal;
            break;
         default:
            break;
      }
      InvalidateString();
   }
}


//
// Bitwise NOT
//

void NXSL_Value::BitNot(void)
{
   if (IsNumeric())
   {
      switch(m_nDataType)
      {
         case NXSL_DT_INT32:
            m_value.nInt32 = ~m_value.nInt32;
            break;
         case NXSL_DT_UINT32:
            m_value.nInt32 = ~m_value.uInt32;
            break;
         case NXSL_DT_INT64:
            m_value.nInt64 = ~m_value.nInt64;
            break;
         case NXSL_DT_UINT64:
            m_value.nInt64 = ~m_value.uInt64;
            break;
         default:
            break;
      }
      InvalidateString();
   }
}


//
// Check if value is zero
//

BOOL NXSL_Value::IsZero(void)
{
   BOOL bVal = FALSE;

   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         bVal = (m_value.nInt32 == 0);
         break;
      case NXSL_DT_UINT32:
         bVal = (m_value.uInt32 == 0);
         break;
      case NXSL_DT_INT64:
         bVal = (m_value.nInt64 == 0);
         break;
      case NXSL_DT_UINT64:
         bVal = (m_value.uInt64 == 0);
         break;
      case NXSL_DT_REAL:
         bVal = (m_value.dReal == 0);
         break;
      default:
         break;
   }
   return bVal;
}


//
// Check if value is not a zero
//

BOOL NXSL_Value::IsNonZero(void)
{
   BOOL bVal = FALSE;

   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         bVal = (m_value.nInt32 != 0);
         break;
      case NXSL_DT_UINT32:
         bVal = (m_value.uInt32 != 0);
         break;
      case NXSL_DT_INT64:
         bVal = (m_value.nInt64 != 0);
         break;
      case NXSL_DT_UINT64:
         bVal = (m_value.uInt64 != 0);
         break;
      case NXSL_DT_REAL:
         bVal = (m_value.dReal != 0);
         break;
      default:
         break;
   }
   return bVal;
}


//
// Compare value
// All these functions assumes that both values have same type
//

BOOL NXSL_Value::EQ(NXSL_Value *pVal)
{
   BOOL bVal = FALSE;

   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         bVal = (m_value.nInt32 == pVal->m_value.nInt32);
         break;
      case NXSL_DT_UINT32:
         bVal = (m_value.uInt32 == pVal->m_value.uInt32);
         break;
      case NXSL_DT_INT64:
         bVal = (m_value.nInt64 == pVal->m_value.nInt64);
         break;
      case NXSL_DT_UINT64:
         bVal = (m_value.uInt64 == pVal->m_value.uInt64);
         break;
      case NXSL_DT_REAL:
         bVal = (m_value.dReal == pVal->m_value.dReal);
         break;
      default:
         break;
   }
   return bVal;
}

BOOL NXSL_Value::LT(NXSL_Value *pVal)
{
   BOOL bVal = FALSE;

   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         bVal = (m_value.nInt32 < pVal->m_value.nInt32);
         break;
      case NXSL_DT_UINT32:
         bVal = (m_value.uInt32 < pVal->m_value.uInt32);
         break;
      case NXSL_DT_INT64:
         bVal = (m_value.nInt64 < pVal->m_value.nInt64);
         break;
      case NXSL_DT_UINT64:
         bVal = (m_value.uInt64 < pVal->m_value.uInt64);
         break;
      case NXSL_DT_REAL:
         bVal = (m_value.dReal < pVal->m_value.dReal);
         break;
      default:
         break;
   }
   return bVal;
}

BOOL NXSL_Value::LE(NXSL_Value *pVal)
{
   BOOL bVal = FALSE;

   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         bVal = (m_value.nInt32 <= pVal->m_value.nInt32);
         break;
      case NXSL_DT_UINT32:
         bVal = (m_value.uInt32 <= pVal->m_value.uInt32);
         break;
      case NXSL_DT_INT64:
         bVal = (m_value.nInt64 <= pVal->m_value.nInt64);
         break;
      case NXSL_DT_UINT64:
         bVal = (m_value.uInt64 <= pVal->m_value.uInt64);
         break;
      case NXSL_DT_REAL:
         bVal = (m_value.dReal <= pVal->m_value.dReal);
         break;
      default:
         break;
   }
   return bVal;
}

BOOL NXSL_Value::GT(NXSL_Value *pVal)
{
   BOOL bVal = FALSE;

   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         bVal = (m_value.nInt32 > pVal->m_value.nInt32);
         break;
      case NXSL_DT_UINT32:
         bVal = (m_value.uInt32 > pVal->m_value.uInt32);
         break;
      case NXSL_DT_INT64:
         bVal = (m_value.nInt64 > pVal->m_value.nInt64);
         break;
      case NXSL_DT_UINT64:
         bVal = (m_value.uInt64 > pVal->m_value.uInt64);
         break;
      case NXSL_DT_REAL:
         bVal = (m_value.dReal > pVal->m_value.dReal);
         break;
      default:
         break;
   }
   return bVal;
}

BOOL NXSL_Value::GE(NXSL_Value *pVal)
{
   BOOL bVal = FALSE;

   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         bVal = (m_value.nInt32 >= pVal->m_value.nInt32);
         break;
      case NXSL_DT_UINT32:
         bVal = (m_value.uInt32 >= pVal->m_value.uInt32);
         break;
      case NXSL_DT_INT64:
         bVal = (m_value.nInt64 >= pVal->m_value.nInt64);
         break;
      case NXSL_DT_UINT64:
         bVal = (m_value.uInt64 >= pVal->m_value.uInt64);
         break;
      case NXSL_DT_REAL:
         bVal = (m_value.dReal >= pVal->m_value.dReal);
         break;
      default:
         break;
   }
   return bVal;
}


//
// Arithmetic and bit operations
// All these functions assumes that both values have same type
//

void NXSL_Value::Add(NXSL_Value *pVal)
{
   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         m_value.nInt32 += pVal->m_value.nInt32;
         break;
      case NXSL_DT_UINT32:
         m_value.uInt32 += pVal->m_value.uInt32;
         break;
      case NXSL_DT_INT64:
         m_value.nInt64 += pVal->m_value.nInt64;
         break;
      case NXSL_DT_UINT64:
         m_value.uInt64 += pVal->m_value.uInt64;
         break;
      case NXSL_DT_REAL:
         m_value.dReal += pVal->m_value.dReal;
         break;
      default:
         break;
   }
   InvalidateString();
}

void NXSL_Value::Sub(NXSL_Value *pVal)
{
   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         m_value.nInt32 -= pVal->m_value.nInt32;
         break;
      case NXSL_DT_UINT32:
         m_value.uInt32 -= pVal->m_value.uInt32;
         break;
      case NXSL_DT_INT64:
         m_value.nInt64 -= pVal->m_value.nInt64;
         break;
      case NXSL_DT_UINT64:
         m_value.uInt64 -= pVal->m_value.uInt64;
         break;
      case NXSL_DT_REAL:
         m_value.dReal -= pVal->m_value.dReal;
         break;
      default:
         break;
   }
   InvalidateString();
}

void NXSL_Value::Mul(NXSL_Value *pVal)
{
   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         m_value.nInt32 *= pVal->m_value.nInt32;
         break;
      case NXSL_DT_UINT32:
         m_value.uInt32 *= pVal->m_value.uInt32;
         break;
      case NXSL_DT_INT64:
         m_value.nInt64 *= pVal->m_value.nInt64;
         break;
      case NXSL_DT_UINT64:
         m_value.uInt64 *= pVal->m_value.uInt64;
         break;
      case NXSL_DT_REAL:
         m_value.dReal *= pVal->m_value.dReal;
         break;
      default:
         break;
   }
   InvalidateString();
}

void NXSL_Value::Div(NXSL_Value *pVal)
{
   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         m_value.nInt32 /= pVal->m_value.nInt32;
         break;
      case NXSL_DT_UINT32:
         m_value.uInt32 /= pVal->m_value.uInt32;
         break;
      case NXSL_DT_INT64:
         m_value.nInt64 /= pVal->m_value.nInt64;
         break;
      case NXSL_DT_UINT64:
         m_value.uInt64 /= pVal->m_value.uInt64;
         break;
      case NXSL_DT_REAL:
         m_value.dReal /= pVal->m_value.dReal;
         break;
      default:
         break;
   }
   InvalidateString();
}

void NXSL_Value::Rem(NXSL_Value *pVal)
{
   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         m_value.nInt32 %= pVal->m_value.nInt32;
         break;
      case NXSL_DT_UINT32:
         m_value.uInt32 %= pVal->m_value.uInt32;
         break;
      case NXSL_DT_INT64:
         m_value.nInt64 %= pVal->m_value.nInt64;
         break;
      case NXSL_DT_UINT64:
         m_value.uInt64 %= pVal->m_value.uInt64;
         break;
      default:
         break;
   }
   InvalidateString();
}

void NXSL_Value::BitAnd(NXSL_Value *pVal)
{
   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         m_value.nInt32 &= pVal->m_value.nInt32;
         break;
      case NXSL_DT_UINT32:
         m_value.uInt32 &= pVal->m_value.uInt32;
         break;
      case NXSL_DT_INT64:
         m_value.nInt64 &= pVal->m_value.nInt64;
         break;
      case NXSL_DT_UINT64:
         m_value.uInt64 &= pVal->m_value.uInt64;
         break;
      default:
         break;
   }
   InvalidateString();
}

void NXSL_Value::BitOr(NXSL_Value *pVal)
{
   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         m_value.nInt32 |= pVal->m_value.nInt32;
         break;
      case NXSL_DT_UINT32:
         m_value.uInt32 |= pVal->m_value.uInt32;
         break;
      case NXSL_DT_INT64:
         m_value.nInt64 |= pVal->m_value.nInt64;
         break;
      case NXSL_DT_UINT64:
         m_value.uInt64 |= pVal->m_value.uInt64;
         break;
      default:
         break;
   }
   InvalidateString();
}

void NXSL_Value::BitXor(NXSL_Value *pVal)
{
   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         m_value.nInt32 ^= pVal->m_value.nInt32;
         break;
      case NXSL_DT_UINT32:
         m_value.uInt32 ^= pVal->m_value.uInt32;
         break;
      case NXSL_DT_INT64:
         m_value.nInt64 ^= pVal->m_value.nInt64;
         break;
      case NXSL_DT_UINT64:
         m_value.uInt64 ^= pVal->m_value.uInt64;
         break;
      default:
         break;
   }
   InvalidateString();
}


//
// Bit shift operations
//

void NXSL_Value::LShift(int nBits)
{
   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         m_value.nInt32 <<= nBits;
         break;
      case NXSL_DT_UINT32:
         m_value.uInt32 <<= nBits;
         break;
      case NXSL_DT_INT64:
         m_value.nInt64 <<= nBits;
         break;
      case NXSL_DT_UINT64:
         m_value.uInt64 <<= nBits;
         break;
      default:
         break;
   }
   InvalidateString();
}

void NXSL_Value::RShift(int nBits)
{
   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         m_value.nInt32 >>= nBits;
         break;
      case NXSL_DT_UINT32:
         m_value.uInt32 >>= nBits;
         break;
      case NXSL_DT_INT64:
         m_value.nInt64 >>= nBits;
         break;
      case NXSL_DT_UINT64:
         m_value.uInt64 >>= nBits;
         break;
      default:
         break;
   }
   InvalidateString();
}
