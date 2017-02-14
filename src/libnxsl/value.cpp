/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2017 Victor Kirhenshtein
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

/**
 * Assign value to correct number field
 */
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

/**
 * Retrieve correct number field
 */
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

/**
 * Create "null" value
 */
NXSL_Value::NXSL_Value()
{
   m_nDataType = NXSL_DT_NULL;
   m_pszValStr = NULL;
   m_dwStrLen = 0;
#ifdef UNICODE
	m_valueMBStr = NULL;
#endif
	m_name = NULL;
   m_bStringIsValid = FALSE;
}

/**
 * Create copy of given value
 */
NXSL_Value::NXSL_Value(const NXSL_Value *value)
{
   if (value != NULL)
   {
      m_nDataType = value->m_nDataType;
      if (m_nDataType == NXSL_DT_OBJECT)
      {
         m_value.object = new NXSL_Object(value->m_value.object);
      }
      else if (m_nDataType == NXSL_DT_ARRAY)
      {
         m_value.arrayHandle = value->m_value.arrayHandle;
			m_value.arrayHandle->incRefCount();
      }
      else if (m_nDataType == NXSL_DT_HASHMAP)
      {
         m_value.hashMapHandle = value->m_value.hashMapHandle;
			m_value.hashMapHandle->incRefCount();
      }
      else if (m_nDataType == NXSL_DT_ITERATOR)
      {
         m_value.iterator = value->m_value.iterator;
			m_value.iterator->incRefCount();
      }
      else
      {
         memcpy(&m_value, &value->m_value, sizeof(m_value));
      }
      m_bStringIsValid = value->m_bStringIsValid;
      if (m_bStringIsValid)
      {
         m_dwStrLen = value->m_dwStrLen;
         m_pszValStr = (TCHAR *)nx_memdup(value->m_pszValStr, (m_dwStrLen + 1) * sizeof(TCHAR));
      }
      else
      {
         m_pszValStr = NULL;
      }
		m_name = (value->m_name != NULL) ? _tcsdup(value->m_name) : NULL;
   }
   else
   {
      m_nDataType = NXSL_DT_NULL;
      m_pszValStr = NULL;
		m_name = NULL;
   }
#ifdef UNICODE
	m_valueMBStr = NULL;
#endif
}

/**
 * Create "object" value
 */
NXSL_Value::NXSL_Value(NXSL_Object *object)
{
   m_nDataType = NXSL_DT_OBJECT;
   m_value.object = object;
   m_pszValStr = NULL;
   m_dwStrLen = 0;
#ifdef UNICODE
	m_valueMBStr = NULL;
#endif
   m_bStringIsValid = FALSE;
	m_name = NULL;
}

/**
 * Create "array" value
 */
NXSL_Value::NXSL_Value(NXSL_Array *array)
{
   m_nDataType = NXSL_DT_ARRAY;
   m_value.arrayHandle = new NXSL_Handle<NXSL_Array>(array);
	m_value.arrayHandle->incRefCount();
   m_pszValStr = NULL;
   m_dwStrLen = 0;
#ifdef UNICODE
	m_valueMBStr = NULL;
#endif
   m_bStringIsValid = FALSE;
	m_name = NULL;
}

/**
 * Create "hash map" value
 */
NXSL_Value::NXSL_Value(NXSL_HashMap *hashMap)
{
   m_nDataType = NXSL_DT_HASHMAP;
   m_value.hashMapHandle = new NXSL_Handle<NXSL_HashMap>(hashMap);
	m_value.hashMapHandle->incRefCount();
   m_pszValStr = NULL;
   m_dwStrLen = 0;
#ifdef UNICODE
	m_valueMBStr = NULL;
#endif
   m_bStringIsValid = FALSE;
	m_name = NULL;
}

/**
 * Create "iterator" value
 */
NXSL_Value::NXSL_Value(NXSL_Iterator *iterator)
{
   m_nDataType = NXSL_DT_ITERATOR;
   m_value.iterator = iterator;
	iterator->incRefCount();
   m_pszValStr = NULL;
   m_dwStrLen = 0;
#ifdef UNICODE
	m_valueMBStr = NULL;
#endif
   m_bStringIsValid = FALSE;
	m_name = NULL;
}

/**
 * Create "int32" value
 */
NXSL_Value::NXSL_Value(INT32 nValue)
{
   m_nDataType = NXSL_DT_INT32;
   m_pszValStr = NULL;
   m_dwStrLen = 0;
#ifdef UNICODE
	m_valueMBStr = NULL;
#endif
   m_bStringIsValid = FALSE;
   m_value.nInt32 = nValue;
	m_name = NULL;
}

/**
 * Create "uint32" value
 */
NXSL_Value::NXSL_Value(UINT32 uValue)
{
   m_nDataType = NXSL_DT_UINT32;
   m_pszValStr = NULL;
   m_dwStrLen = 0;
#ifdef UNICODE
	m_valueMBStr = NULL;
#endif
   m_bStringIsValid = FALSE;
   m_value.uInt32 = uValue;
	m_name = NULL;
}

/**
 * Create "int64" value
 */
NXSL_Value::NXSL_Value(INT64 nValue)
{
   m_nDataType = NXSL_DT_INT64;
   m_pszValStr = NULL;
   m_dwStrLen = 0;
#ifdef UNICODE
	m_valueMBStr = NULL;
#endif
   m_bStringIsValid = FALSE;
   m_value.nInt64 = nValue;
	m_name = NULL;
}

/**
 * Create "uint64" value
 */
NXSL_Value::NXSL_Value(UINT64 uValue)
{
   m_nDataType = NXSL_DT_UINT64;
   m_pszValStr = NULL;
   m_dwStrLen = 0;
#ifdef UNICODE
	m_valueMBStr = NULL;
#endif
   m_bStringIsValid = FALSE;
   m_value.uInt64 = uValue;
	m_name = NULL;
}

/**
 * Create "real" value
 */
NXSL_Value::NXSL_Value(double dValue)
{
   m_nDataType = NXSL_DT_REAL;
   m_pszValStr = NULL;
   m_dwStrLen = 0;
#ifdef UNICODE
	m_valueMBStr = NULL;
#endif
   m_bStringIsValid = FALSE;
   m_value.dReal = dValue;
	m_name = NULL;
}

/**
 * Create "string" value
 */
NXSL_Value::NXSL_Value(const TCHAR *value)
{
   m_nDataType = NXSL_DT_STRING;
	if (value != NULL)
	{
		m_dwStrLen = (UINT32)_tcslen(value);
		m_pszValStr = _tcsdup(value);
	}
	else
	{
		m_dwStrLen = 0;
		m_pszValStr = _tcsdup(_T(""));
	}
#ifdef UNICODE
	m_valueMBStr = NULL;
#endif
   m_bStringIsValid = TRUE;
   updateNumber();
	m_name = NULL;
}

#ifdef UNICODE

/**
 * Create "string" value from UTF-8 string
 */
NXSL_Value::NXSL_Value(const char *value)
{
   m_nDataType = NXSL_DT_STRING;
	if (value != NULL)
	{
		m_pszValStr = WideStringFromUTF8String(value);
		m_dwStrLen = (UINT32)_tcslen(m_pszValStr);
	}
	else
	{
		m_dwStrLen = 0;
		m_pszValStr = _tcsdup(_T(""));
	}
	m_valueMBStr = NULL;
   m_bStringIsValid = TRUE;
   updateNumber();
	m_name = NULL;
}

#endif

/**
 * Create "string" value from non null-terminated string
 */
NXSL_Value::NXSL_Value(const TCHAR *value, UINT32 dwLen)
{
   m_nDataType = NXSL_DT_STRING;
   m_dwStrLen = dwLen;
   m_pszValStr = (TCHAR *)malloc((dwLen + 1) * sizeof(TCHAR));
	if (value != NULL)
	{
		memcpy(m_pszValStr, value, dwLen * sizeof(TCHAR));
	   m_pszValStr[dwLen] = 0;
	}
	else
	{
		memset(m_pszValStr, 0, (dwLen + 1) * sizeof(TCHAR));
	}
#ifdef UNICODE
	m_valueMBStr = NULL;
#endif
   m_bStringIsValid = TRUE;
   updateNumber();
	m_name = NULL;
}

/**
 * Destructor
 */
NXSL_Value::~NXSL_Value()
{
	free(m_name);
   free(m_pszValStr);
#ifdef UNICODE
	free(m_valueMBStr);
#endif
   switch(m_nDataType)
	{
		case NXSL_DT_OBJECT:
			delete m_value.object;
			break;
		case NXSL_DT_ARRAY:
			m_value.arrayHandle->decRefCount();
			if (m_value.arrayHandle->isUnused())
				delete m_value.arrayHandle;
			break;
		case NXSL_DT_HASHMAP:
			m_value.hashMapHandle->decRefCount();
			if (m_value.hashMapHandle->isUnused())
				delete m_value.hashMapHandle;
			break;
		case NXSL_DT_ITERATOR:
			m_value.iterator->decRefCount();
			if (m_value.iterator->isUnused())
				delete m_value.iterator;
			break;
	}
}

/**
 * Set value to "int32"
 */
void NXSL_Value::set(INT32 nValue)
{
   m_nDataType = NXSL_DT_INT32;
	safe_free_and_null(m_pszValStr);
#ifdef UNICODE
	safe_free_and_null(m_valueMBStr);
#endif
   m_bStringIsValid = FALSE;
   m_value.nInt32 = nValue;
}

/**
 * Update numeric value after string change
 */
void NXSL_Value::updateNumber()
{
   if (m_pszValStr[0] == 0)
      return;

   TCHAR *eptr;
   INT64 nVal = _tcstoll(m_pszValStr, &eptr, 0);
   if ((*eptr == 0) && ((UINT32)(eptr - m_pszValStr) == m_dwStrLen))
   {
      if (nVal > 0x7FFFFFFF)
      {
         m_nDataType = NXSL_DT_INT64;
         m_value.nInt64 = nVal;
      }
      else
      {
         m_nDataType = NXSL_DT_INT32;
         m_value.nInt32 = (INT32)nVal;
      }
   }
   else
   {
      double dVal = _tcstod(m_pszValStr, &eptr);
      if ((*eptr == 0) && ((UINT32)(eptr - m_pszValStr) == m_dwStrLen))
      {
         m_nDataType = NXSL_DT_REAL;
         m_value.dReal = dVal;
      }
   }
}

/**
 * Update string value
 */
void NXSL_Value::updateString()
{
   TCHAR szBuffer[64];

   safe_free(m_pszValStr);
#ifdef UNICODE
	safe_free_and_null(m_valueMBStr);
#endif
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
      case NXSL_DT_NULL:
         _tcscpy(szBuffer, _T(""));
         break;
      case NXSL_DT_OBJECT:
         _sntprintf(szBuffer, 64, _T("%s@%p"), m_value.object->getClass()->getName(), m_value.object);
         break;
      case NXSL_DT_ARRAY:
         _sntprintf(szBuffer, 64, _T("[A@%p]"), m_value.arrayHandle->getObject());
         break;
      case NXSL_DT_HASHMAP:
         _sntprintf(szBuffer, 64, _T("[M@%p]"), m_value.hashMapHandle->getObject());
         break;
      case NXSL_DT_ITERATOR:
         _sntprintf(szBuffer, 64, _T("[I@%p]"), m_value.iterator);
         break;
      default:
         szBuffer[0] = 0;
         break;
   }
   m_dwStrLen = (UINT32)_tcslen(szBuffer);
   m_pszValStr = _tcsdup(szBuffer);
   m_bStringIsValid = TRUE;
}

/**
 * Convert to another data type
 */
bool NXSL_Value::convert(int nDataType)
{
   INT32 nInt32;
   UINT32 uInt32;
   INT64 nInt64;
   UINT64 uInt64;
   double dReal;
   bool bRet = true;

   if (m_nDataType == nDataType)
      return true;

	if ((nDataType == NXSL_DT_STRING) && isString())
		return true;

   switch(nDataType)
   {
      case NXSL_DT_INT32:
         RETRIEVE_NUMERIC_VALUE(nInt32, INT32);
         m_nDataType = nDataType;
         m_value.nInt32 = nInt32;
         break;
      case NXSL_DT_UINT32:
         RETRIEVE_NUMERIC_VALUE(uInt32, UINT32);
         m_nDataType = nDataType;
         m_value.uInt32 = uInt32;
         break;
      case NXSL_DT_INT64:
         RETRIEVE_NUMERIC_VALUE(nInt64, INT64);
         m_nDataType = nDataType;
         m_value.nInt64 = nInt64;
         break;
      case NXSL_DT_UINT64:
         RETRIEVE_NUMERIC_VALUE(uInt64, UINT64);
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

/**
 * Get value as ASCIIZ string
 */
const TCHAR *NXSL_Value::getValueAsCString()
{
   if (isNull() || isObject() || isArray())
      return NULL;

   if (!m_bStringIsValid)
      updateString();
   return m_pszValStr;
}

/**
 * Get value as multibyte string
 */
#ifdef UNICODE

const char *NXSL_Value::getValueAsMBString()
{
   if (isNull() || isObject() || isArray())
      return NULL;

	if (m_valueMBStr != NULL)
		return m_valueMBStr;

   if (!m_bStringIsValid)
      updateString();
	if (m_pszValStr != NULL)
		m_valueMBStr = MBStringFromWideString(m_pszValStr);
   return m_valueMBStr;
}

#endif

/**
 * Get value as string
 */
const TCHAR *NXSL_Value::getValueAsString(UINT32 *pdwLen)
{
   if (!m_bStringIsValid)
      updateString();
   *pdwLen = m_dwStrLen;
   return m_pszValStr;
}

/**
 * Get value as 32 bit integer
 */
INT32 NXSL_Value::getValueAsInt32()
{
   INT32 nVal;

   RETRIEVE_NUMERIC_VALUE(nVal, INT32);
   return nVal;
}

/**
 * Get value as unsigned 32 bit integer
 */
UINT32 NXSL_Value::getValueAsUInt32()
{
   UINT32 uVal;

   RETRIEVE_NUMERIC_VALUE(uVal, UINT32);
   return uVal;
}

/**
 * Get value as 64 bit integer
 */
INT64 NXSL_Value::getValueAsInt64()
{
   INT64 nVal;

   RETRIEVE_NUMERIC_VALUE(nVal, INT64);
   return nVal;
}

/**
 * Get value as unsigned 64 bit integer
 */
UINT64 NXSL_Value::getValueAsUInt64()
{
   UINT64 uVal;

   RETRIEVE_NUMERIC_VALUE(uVal, UINT64);
   return uVal;
}

/**
 * Get value as real number
 */
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

/**
 * Concatenate string value
 */
void NXSL_Value::concatenate(const TCHAR *pszString, UINT32 dwLen)
{
   if (!m_bStringIsValid)
	{
      updateString();
	}
#ifdef UNICODE
	else
	{
		safe_free_and_null(m_valueMBStr);
	}
#endif
   m_pszValStr = (TCHAR *)realloc(m_pszValStr, (m_dwStrLen + dwLen + 1) * sizeof(TCHAR));
   memcpy(&m_pszValStr[m_dwStrLen], pszString, dwLen * sizeof(TCHAR));
   m_dwStrLen += dwLen;
   m_pszValStr[m_dwStrLen] = 0;
   m_nDataType = NXSL_DT_STRING;
   updateNumber();
}

/**
 * Increment value
 */
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

/**
 * Decrement value
 */
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

/**
 * Negate value
 */
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
            m_value.nInt32 = -((INT32)m_value.uInt32);
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

/**
 * Bitwise NOT
 */
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

/**
 * Check if value is zero
 */
bool NXSL_Value::isZero() const
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

/**
 * Check if value is not a zero
 */
bool NXSL_Value::isNonZero() const
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

/**
 * Check if vaue is an object of given class
 */
bool NXSL_Value::isObject(const TCHAR *className) const
{
	if (m_nDataType != NXSL_DT_OBJECT)
		return false;

	return m_value.object->getClass()->instanceOf(className);
}

/**
 * Check if two values are strictly equals
 */
bool NXSL_Value::equals(const NXSL_Value *v) const
{
   if (v == this)
      return true;
   if (v->m_nDataType != m_nDataType)
      return false;
   switch(m_nDataType)
   {
      case NXSL_DT_ARRAY:
         {
            if (v->m_value.arrayHandle->getObject() == m_value.arrayHandle->getObject())
               return true;
            if (v->m_value.arrayHandle->getObject()->size() != m_value.arrayHandle->getObject()->size())
               return false;
            for(int i = 0; i < m_value.arrayHandle->getObject()->size(); i++)
            {
               if (!m_value.arrayHandle->getObject()->get(i)->equals(v->m_value.arrayHandle->getObject()->get(i)))
                  return false;
            }
         }
         return true;
      case NXSL_DT_HASHMAP:
         if (v->m_value.hashMapHandle->getObject() == m_value.hashMapHandle->getObject())
            return true;
         if (v->m_value.hashMapHandle->getObject()->size() != m_value.hashMapHandle->getObject()->size())
            return false;
         if (m_value.hashMapHandle->getObject()->size() == 0)
            return true;
         return false;
      case NXSL_DT_INT32:
         return v->m_value.nInt32 == m_value.nInt32;
      case NXSL_DT_INT64:
         return v->m_value.nInt64 == m_value.nInt64;
      case NXSL_DT_ITERATOR:
         return false;
      case NXSL_DT_NULL:
         return true;
      case NXSL_DT_OBJECT:
         return (v->m_value.object->getData() == m_value.object->getData()) && 
            !_tcscmp(v->m_value.object->getClass()->getName(), m_value.object->getClass()->getName());
         break;
      case NXSL_DT_REAL:
         return v->m_value.dReal == m_value.dReal;
      case NXSL_DT_STRING:
         return !_tcscmp(v->m_pszValStr, m_pszValStr) ? true : false;
      case NXSL_DT_UINT32:
         return v->m_value.uInt32 == m_value.uInt32;
      case NXSL_DT_UINT64:
         return v->m_value.uInt64 == m_value.uInt64;
   }
   return false;
}

/**
 * Serialize
 */
void NXSL_Value::serialize(ByteStream &s) const
{
   s.write(m_nDataType);
   switch(m_nDataType)
   {
      case NXSL_DT_ARRAY:
         {
            s.write((UINT16)m_value.arrayHandle->getObject()->size());
            for(int i = 0; i < m_value.arrayHandle->getObject()->size(); i++)
            {
               m_value.arrayHandle->getObject()->get(i)->serialize(s);
            }
         }
         break;
      case NXSL_DT_HASHMAP:
         s.write((UINT16)m_value.hashMapHandle->getObject()->size());
         if (m_value.hashMapHandle->getObject()->size() > 0)
         {
            // TODO: hashmap serialize
         }
         break;
      case NXSL_DT_INT32:
         s.write(m_value.nInt32);
         break;
      case NXSL_DT_INT64:
         s.write(m_value.nInt64);
         break;
      case NXSL_DT_ITERATOR:
         break;
      case NXSL_DT_NULL:
         break;
      case NXSL_DT_OBJECT:
         break;
      case NXSL_DT_REAL:
         s.write(m_value.dReal);
         break;
      case NXSL_DT_STRING:
         s.writeString(m_pszValStr);
         break;
      case NXSL_DT_UINT32:
         s.write(m_value.uInt32);
         break;
      case NXSL_DT_UINT64:
         s.write(m_value.uInt64);
         break;
   }
}

/**
 * Load value from byte stream
 */
NXSL_Value *NXSL_Value::load(ByteStream &s)
{
   NXSL_Value *v = new NXSL_Value();
   v->m_nDataType = s.readByte();
   switch(v->m_nDataType)
   {
      case NXSL_DT_ARRAY:
         {
            v->m_value.arrayHandle = new NXSL_Handle<NXSL_Array>(new NXSL_Array());
            int size = (int)s.readUInt16();
            for(int i = 0; i < size; i++)
            {
               v->m_value.arrayHandle->getObject()->set(i, load(s));
            }
         }
         break;
      case NXSL_DT_HASHMAP:
         {
            v->m_value.hashMapHandle = new NXSL_Handle<NXSL_HashMap>(new NXSL_HashMap());
            int size = (int)s.readUInt16();
            for(int i = 0; i < size; i++)
            {
               // TODO: implement hash map load
            }
         }
         break;
      case NXSL_DT_INT32:
         v->m_value.nInt32 = s.readInt32();
         break;
      case NXSL_DT_INT64:
         v->m_value.nInt64 = s.readInt64();
         break;
      case NXSL_DT_ITERATOR:
         break;
      case NXSL_DT_NULL:
         break;
      case NXSL_DT_OBJECT:
         break;
      case NXSL_DT_REAL:
         v->m_value.dReal = s.readDouble();
         break;
      case NXSL_DT_STRING:
         v->m_pszValStr = s.readString();
         v->m_dwStrLen = (UINT32)_tcslen(v->m_pszValStr);
         v->m_bStringIsValid = TRUE;
         v->updateNumber();
         break;
      case NXSL_DT_UINT32:
         v->m_value.uInt32 = s.readUInt32();
         break;
      case NXSL_DT_UINT64:
         v->m_value.uInt64 = s.readUInt64();
         break;
   }
   return v;
}

/**
 * Do copy on write if needed
 */
void NXSL_Value::copyOnWrite()
{
   if ((m_nDataType == NXSL_DT_ARRAY) && m_value.arrayHandle->isSharedObject())
   {
      m_value.arrayHandle->cloneObject();
   }
   else if ((m_nDataType == NXSL_DT_HASHMAP) && m_value.hashMapHandle->isSharedObject())
   {
      m_value.hashMapHandle->cloneObject();
   }
}

/**
 * Called when value set as variable's value
 */
void NXSL_Value::onVariableSet()
{
   switch(m_nDataType)
   {
      case NXSL_DT_ARRAY:
         if (m_value.arrayHandle->isShared())
         {
            m_value.arrayHandle->decRefCount();
            m_value.arrayHandle = new NXSL_Handle<NXSL_Array>(m_value.arrayHandle);
            m_value.arrayHandle->incRefCount();
         }
         break;
      case NXSL_DT_HASHMAP:
         if (m_value.hashMapHandle->isShared())
         {
            m_value.hashMapHandle->decRefCount();
            m_value.hashMapHandle = new NXSL_Handle<NXSL_HashMap>(m_value.hashMapHandle);
            m_value.hashMapHandle->incRefCount();
         }
         break;
      default:
         break;
   }
}
