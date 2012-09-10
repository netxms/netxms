/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2011 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: value.cpp
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

NXSL_Value::NXSL_Value()
{
   m_nDataType = NXSL_DT_NULL;
   m_pszValStr = NULL;
	m_name = NULL;
   m_bStringIsValid = FALSE;
}

NXSL_Value::NXSL_Value(const NXSL_Value *pValue)
{
   if (pValue != NULL)
   {
      m_nDataType = pValue->m_nDataType;
      if (m_nDataType == NXSL_DT_OBJECT)
      {
         m_value.pObject = new NXSL_Object(pValue->m_value.pObject);
      }
      else if (m_nDataType == NXSL_DT_ARRAY)
      {
         m_value.pArray = pValue->m_value.pArray;
			m_value.pArray->incRefCount();
      }
      else if (m_nDataType == NXSL_DT_ITERATOR)
      {
         m_value.pIterator = pValue->m_value.pIterator;
			m_value.pIterator->incRefCount();
      }
      else
      {
         memcpy(&m_value, &pValue->m_value, sizeof(m_value));
      }
      m_bStringIsValid = pValue->m_bStringIsValid;
      if (m_bStringIsValid)
      {
         m_dwStrLen = pValue->m_dwStrLen;
         m_pszValStr = (TCHAR *)nx_memdup(pValue->m_pszValStr, (m_dwStrLen + 1) * sizeof(TCHAR));
      }
      else
      {
         m_pszValStr = NULL;
      }
		m_name = (pValue->m_name != NULL) ? _tcsdup(pValue->m_name) : NULL;
   }
   else
   {
      m_nDataType = NXSL_DT_NULL;
      m_pszValStr = NULL;
		m_name = NULL;
   }
}

NXSL_Value::NXSL_Value(NXSL_Object *pObject)
{
   m_nDataType = NXSL_DT_OBJECT;
   m_value.pObject = pObject;
   m_pszValStr = NULL;
   m_bStringIsValid = FALSE;
	m_name = NULL;
}

NXSL_Value::NXSL_Value(NXSL_Array *pArray)
{
   m_nDataType = NXSL_DT_ARRAY;
   m_value.pArray = pArray;
	pArray->incRefCount();
   m_pszValStr = NULL;
   m_bStringIsValid = FALSE;
	m_name = NULL;
}

NXSL_Value::NXSL_Value(NXSL_Iterator *pIterator)
{
   m_nDataType = NXSL_DT_ITERATOR;
   m_value.pIterator = pIterator;
	pIterator->incRefCount();
   m_pszValStr = NULL;
   m_bStringIsValid = FALSE;
	m_name = NULL;
}

NXSL_Value::NXSL_Value(LONG nValue)
{
   m_nDataType = NXSL_DT_INT32;
   m_pszValStr = NULL;
   m_bStringIsValid = FALSE;
   m_value.nInt32 = nValue;
	m_name = NULL;
}

NXSL_Value::NXSL_Value(DWORD uValue)
{
   m_nDataType = NXSL_DT_UINT32;
   m_pszValStr = NULL;
   m_bStringIsValid = FALSE;
   m_value.uInt32 = uValue;
	m_name = NULL;
}

NXSL_Value::NXSL_Value(INT64 nValue)
{
   m_nDataType = NXSL_DT_INT64;
   m_pszValStr = NULL;
   m_bStringIsValid = FALSE;
   m_value.nInt64 = nValue;
	m_name = NULL;
}

NXSL_Value::NXSL_Value(QWORD uValue)
{
   m_nDataType = NXSL_DT_UINT64;
   m_pszValStr = NULL;
   m_bStringIsValid = FALSE;
   m_value.uInt64 = uValue;
	m_name = NULL;
}

NXSL_Value::NXSL_Value(double dValue)
{
   m_nDataType = NXSL_DT_REAL;
   m_pszValStr = NULL;
   m_bStringIsValid = FALSE;
   m_value.dReal = dValue;
	m_name = NULL;
}

NXSL_Value::NXSL_Value(const TCHAR *pszValue)
{
   m_nDataType = NXSL_DT_STRING;
	if (pszValue != NULL)
	{
		m_dwStrLen = (DWORD)_tcslen(pszValue);
		m_pszValStr = _tcsdup(pszValue);
	}
	else
	{
		m_dwStrLen = 0;
		m_pszValStr = _tcsdup(_T(""));
	}
   m_bStringIsValid = TRUE;
   updateNumber();
	m_name = NULL;
}

#ifdef UNICODE

NXSL_Value::NXSL_Value(const char *pszValue)
{
   m_nDataType = NXSL_DT_STRING;
	if (pszValue != NULL)
	{
		m_pszValStr = WideStringFromUTF8String(pszValue);
		m_dwStrLen = (DWORD)_tcslen(m_pszValStr);
	}
	else
	{
		m_dwStrLen = 0;
		m_pszValStr = _tcsdup(_T(""));
	}
   m_bStringIsValid = TRUE;
   updateNumber();
	m_name = NULL;
}

#endif

NXSL_Value::NXSL_Value(const TCHAR *pszValue, DWORD dwLen)
{
   m_nDataType = NXSL_DT_STRING;
   m_dwStrLen = dwLen;
   m_pszValStr = (TCHAR *)malloc((dwLen + 1) * sizeof(TCHAR));
	if (pszValue != NULL)
	{
		memcpy(m_pszValStr, pszValue, dwLen * sizeof(TCHAR));
	   m_pszValStr[dwLen] = 0;
	}
	else
	{
		memset(m_pszValStr, 0, (dwLen + 1) * sizeof(TCHAR));
	}
   m_bStringIsValid = TRUE;
   updateNumber();
	m_name = NULL;
}


//
// Destructor
//

NXSL_Value::~NXSL_Value()
{
	safe_free(m_name);
   safe_free(m_pszValStr);
   switch(m_nDataType)
	{
		case NXSL_DT_OBJECT:
			delete m_value.pObject;
			break;
		case NXSL_DT_ARRAY:
			m_value.pArray->decRefCount();
			if (m_value.pArray->isUnused())
				delete m_value.pArray;
			break;
		case NXSL_DT_ITERATOR:
			m_value.pIterator->decRefCount();
			if (m_value.pIterator->isUnused())
				delete m_value.pIterator;
			break;
	}
}


//
// Set value
//

void NXSL_Value::set(LONG nValue)
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

void NXSL_Value::updateNumber()
{
   TCHAR *eptr;
   INT64 nVal;
   double dVal;

   nVal = _tcstoll(m_pszValStr, &eptr, 0);
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
      dVal = _tcstod(m_pszValStr, &eptr);
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

void NXSL_Value::updateString()
{
   TCHAR szBuffer[64];

   safe_free(m_pszValStr);
   switch(m_nDataType)
   {
      case NXSL_DT_INT32:
         _sntprintf(szBuffer, 64, _T("%d"), m_value.nInt32);
         break;
      case NXSL_DT_UINT32:
         _sntprintf(szBuffer, 64, _T("%u"), m_value.uInt32);
         break;
      case NXSL_DT_INT64:
         _sntprintf(szBuffer, 64, INT64_FMT, m_value.nInt64);
         break;
      case NXSL_DT_UINT64:
         _sntprintf(szBuffer, 64, UINT64_FMT, m_value.uInt64);
         break;
      case NXSL_DT_REAL:
         _sntprintf(szBuffer, 64, _T("%f"), m_value.dReal);
         break;
      default:
         szBuffer[0] = 0;
         break;
   }
   m_dwStrLen = (DWORD)_tcslen(szBuffer);
   m_pszValStr = _tcsdup(szBuffer);
   m_bStringIsValid = TRUE;
}


//
// Convert to another data type
//

bool NXSL_Value::convert(int nDataType)
{
   LONG nInt32;
   DWORD uInt32;
   INT64 nInt64;
   QWORD uInt64;
   double dReal;
   bool bRet = true;

   if (m_nDataType == nDataType)
      return true;

	if ((nDataType == NXSL_DT_STRING) && isString())
		return true;

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
            bRet = false;
         }
         else
         {
            dReal = getValueAsReal();
            m_nDataType = nDataType;
            m_value.dReal = dReal;
         }
         break;
		case NXSL_DT_STRING:
			if (m_nDataType == NXSL_DT_NULL)
			{
				m_nDataType = NXSL_DT_STRING;
				bRet = true;
				// String value will be invalidated on exit, and next
				// call to updateString() will create empty string value
			}
			break;
      default:
         bRet = false;
         break;
   }
   if (bRet)
      invalidateString();
   return bRet;
}


//
// Get value as ASCIIZ string
//

const TCHAR *NXSL_Value::getValueAsCString()
{
   if (isNull() || isObject() || isArray())
      return NULL;

   if (!m_bStringIsValid)
      updateString();
   return m_pszValStr;
}


//
// Get value as string
//

const TCHAR *NXSL_Value::getValueAsString(DWORD *pdwLen)
{
   if (isNull() || isObject() || isArray())
	{
		*pdwLen = 0;
      return NULL;
	}

   if (!m_bStringIsValid)
      updateString();
   *pdwLen = m_dwStrLen;
   return m_pszValStr;
}


//
// Get value as 32 bit integer
//

LONG NXSL_Value::getValueAsInt32()
{
   LONG nVal;

   RETRIEVE_NUMERIC_VALUE(nVal, LONG);
   return nVal;
}


//
// Get value as unsigned 32 bit integer
//

DWORD NXSL_Value::getValueAsUInt32()
{
   DWORD uVal;

   RETRIEVE_NUMERIC_VALUE(uVal, DWORD);
   return uVal;
}


//
// Get value as 64 bit integer
//

INT64 NXSL_Value::getValueAsInt64()
{
   INT64 nVal;

   RETRIEVE_NUMERIC_VALUE(nVal, INT64);
   return nVal;
}


//
// Get value as unsigned 64 bit integer
//

QWORD NXSL_Value::getValueAsUInt64()
{
   QWORD uVal;

   RETRIEVE_NUMERIC_VALUE(uVal, QWORD);
   return uVal;
}


//
// Get value as real number
//

double NXSL_Value::getValueAsReal()
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

void NXSL_Value::concatenate(const TCHAR *pszString, DWORD dwLen)
{
   if (!m_bStringIsValid)
      updateString();
   m_pszValStr = (TCHAR *)realloc(m_pszValStr, (m_dwStrLen + dwLen + 1) * sizeof(TCHAR));
   memcpy(&m_pszValStr[m_dwStrLen], pszString, dwLen * sizeof(TCHAR));
   m_dwStrLen += dwLen;
   m_pszValStr[m_dwStrLen] = 0;
   updateNumber();
}


//
// Increment value
//

void NXSL_Value::increment()
{
   if (isNumeric())
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
      invalidateString();
   }
}


//
// Decrement value
//

void NXSL_Value::decrement()
{
   if (isNumeric())
   {
      switch(m_nDataType)
      {
         case NXSL_DT_INT32:
            m_value.nInt32--;
            break;
         case NXSL_DT_UINT32:
            m_value.uInt32--;
            break;
         case NXSL_DT_INT64:
            m_value.nInt64--;
            break;
         case NXSL_DT_UINT64:
            m_value.uInt64--;
            break;
         case NXSL_DT_REAL:
            m_value.dReal--;
            break;
         default:
            break;
      }
      invalidateString();
   }
}


//
// Negate value
//

void NXSL_Value::negate()
{
   if (isNumeric())
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
      invalidateString();
   }
}


//
// Bitwise NOT
//

void NXSL_Value::bitNot()
{
   if (isNumeric())
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
      invalidateString();
   }
}


//
// Check if value is zero
//

bool NXSL_Value::isZero()
{
   bool bVal = false;

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

bool NXSL_Value::isNonZero()
{
   bool bVal = false;

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

void NXSL_Value::add(NXSL_Value *pVal)
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
   invalidateString();
}

void NXSL_Value::sub(NXSL_Value *pVal)
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
   invalidateString();
}

void NXSL_Value::mul(NXSL_Value *pVal)
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
   invalidateString();
}

void NXSL_Value::div(NXSL_Value *pVal)
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
   invalidateString();
}

void NXSL_Value::rem(NXSL_Value *pVal)
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
   invalidateString();
}

void NXSL_Value::bitAnd(NXSL_Value *pVal)
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
   invalidateString();
}

void NXSL_Value::bitOr(NXSL_Value *pVal)
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
   invalidateString();
}

void NXSL_Value::bitXor(NXSL_Value *pVal)
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
   invalidateString();
}


//
// Bit shift operations
//

void NXSL_Value::lshift(int nBits)
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
   invalidateString();
}

void NXSL_Value::rshift(int nBits)
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
   invalidateString();
}


//
// Check if vaue is an object of given class
//

bool NXSL_Value::isObject(const TCHAR *className)
{
	if (m_nDataType != NXSL_DT_OBJECT)
		return false;

	return !_tcscmp(m_value.pObject->getClass()->getName(), className) ? true : false;
}
