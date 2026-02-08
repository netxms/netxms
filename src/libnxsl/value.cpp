/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
 * Create "null" value
 */
NXSL_Value::NXSL_Value()
{
   m_dataType = NXSL_DT_NULL;
   m_stringPtr = nullptr;
   m_length = 0;
#ifdef UNICODE
	m_mbString = nullptr;
#endif
	m_name = nullptr;
   m_stringIsValid = FALSE;
   m_refCount = 1;
}

/**
 * Create copy of given value
 */
NXSL_Value::NXSL_Value(const NXSL_Value *value)
{
   if (value != nullptr)
   {
      m_dataType = value->m_dataType;
      if (m_dataType == NXSL_DT_OBJECT)
      {
         NXSL_VM *vm = value->m_value.object->vm();
         m_value.object = vm->createObject(value->m_value.object);
      }
      else if (m_dataType == NXSL_DT_ARRAY)
      {
         m_value.arrayHandle = value->m_value.arrayHandle;
			m_value.arrayHandle->incRefCount();
      }
      else if (m_dataType == NXSL_DT_HASHMAP)
      {
         m_value.hashMapHandle = value->m_value.hashMapHandle;
			m_value.hashMapHandle->incRefCount();
      }
      else if (m_dataType == NXSL_DT_ITERATOR)
      {
         m_value.iterator = value->m_value.iterator;
			m_value.iterator->incRefCount();
      }
      else
      {
         memcpy(&m_value, &value->m_value, sizeof(m_value));
      }
      m_stringIsValid = value->m_stringIsValid;
      if (m_stringIsValid)
      {
         m_length = value->m_length;
         if (m_length < NXSL_SHORT_STRING_LENGTH)
         {
            memcpy(m_stringValue, value->m_stringValue, (m_length + 1) * sizeof(TCHAR));
            m_stringPtr = nullptr;
         }
         else
         {
            m_stringPtr = MemCopyBlock(value->m_stringPtr, (m_length + 1) * sizeof(TCHAR));
         }
      }
      else
      {
         m_stringPtr = nullptr;
      }
		m_name = MemCopyStringA(value->m_name);
   }
   else
   {
      m_dataType = NXSL_DT_NULL;
      m_stringPtr = nullptr;
		m_name = nullptr;
   }
#ifdef UNICODE
	m_mbString = nullptr;
#endif
   m_refCount = 1;
}

/**
 * Create "object" value
 */
NXSL_Value::NXSL_Value(NXSL_Object *object)
{
   m_dataType = NXSL_DT_OBJECT;
   m_value.object = object;
   m_stringPtr = nullptr;
   m_length = 0;
#ifdef UNICODE
	m_mbString = nullptr;
#endif
   m_stringIsValid = FALSE;
	m_name = nullptr;
   m_refCount = 1;
}

/**
 * Create "array" value
 */
NXSL_Value::NXSL_Value(NXSL_Array *array)
{
   m_dataType = NXSL_DT_ARRAY;
   m_value.arrayHandle = new NXSL_Handle<NXSL_Array>(array);
	m_value.arrayHandle->incRefCount();
   m_stringPtr = nullptr;
   m_length = 0;
#ifdef UNICODE
	m_mbString = nullptr;
#endif
   m_stringIsValid = FALSE;
	m_name = nullptr;
   m_refCount = 1;
}

/**
 * Create "hash map" value
 */
NXSL_Value::NXSL_Value(NXSL_HashMap *hashMap)
{
   m_dataType = NXSL_DT_HASHMAP;
   m_value.hashMapHandle = new NXSL_Handle<NXSL_HashMap>(hashMap);
	m_value.hashMapHandle->incRefCount();
   m_stringPtr = nullptr;
   m_length = 0;
#ifdef UNICODE
	m_mbString = nullptr;
#endif
   m_stringIsValid = FALSE;
	m_name = nullptr;
   m_refCount = 1;
}

/**
 * Create "iterator" value
 */
NXSL_Value::NXSL_Value(NXSL_Iterator *iterator)
{
   m_dataType = NXSL_DT_ITERATOR;
   m_value.iterator = iterator;
	iterator->incRefCount();
   m_stringPtr = nullptr;
   m_length = 0;
#ifdef UNICODE
	m_mbString = nullptr;
#endif
   m_stringIsValid = FALSE;
	m_name = nullptr;
   m_refCount = 1;
}

/**
 * Create "guid" value
 */
NXSL_Value::NXSL_Value(const uuid& guid)
{
   m_dataType = NXSL_DT_GUID;
   memcpy(m_value.guid, &guid.getValue(), UUID_LENGTH);
   m_stringPtr = nullptr;
   m_length = 0;
#ifdef UNICODE
   m_mbString = nullptr;
#endif
   m_stringIsValid = FALSE;
   m_name = nullptr;
   m_refCount = 1;
}

/**
 * Create "int32" value
 */
NXSL_Value::NXSL_Value(int32_t nValue)
{
   m_dataType = NXSL_DT_INT32;
   m_stringPtr = nullptr;
   m_length = 0;
#ifdef UNICODE
	m_mbString = nullptr;
#endif
   m_stringIsValid = FALSE;
   m_value.int32 = nValue;
	m_name = nullptr;
   m_refCount = 1;
}

/**
 * Create "uint32" value
 */
NXSL_Value::NXSL_Value(uint32_t uValue)
{
   m_dataType = NXSL_DT_UINT32;
   m_stringPtr = nullptr;
   m_length = 0;
#ifdef UNICODE
	m_mbString = nullptr;
#endif
   m_stringIsValid = FALSE;
   m_value.uint32 = uValue;
	m_name = nullptr;
   m_refCount = 1;
}

/**
 * Create "int64" value
 */
NXSL_Value::NXSL_Value(int64_t nValue)
{
   m_dataType = NXSL_DT_INT64;
   m_stringPtr = nullptr;
   m_length = 0;
#ifdef UNICODE
	m_mbString = nullptr;
#endif
   m_stringIsValid = FALSE;
   m_value.int64 = nValue;
	m_name = nullptr;
   m_refCount = 1;
}

/**
 * Create "uint64" value
 */
NXSL_Value::NXSL_Value(uint64_t uValue)
{
   m_dataType = NXSL_DT_UINT64;
   m_stringPtr = nullptr;
   m_length = 0;
#ifdef UNICODE
	m_mbString = nullptr;
#endif
   m_stringIsValid = FALSE;
   m_value.uint64 = uValue;
	m_name = nullptr;
   m_refCount = 1;
}

/**
 * Create "real" value
 */
NXSL_Value::NXSL_Value(double dValue)
{
   m_dataType = NXSL_DT_REAL;
   m_stringPtr = nullptr;
   m_length = 0;
#ifdef UNICODE
	m_mbString = nullptr;
#endif
   m_stringIsValid = FALSE;
   m_value.real = dValue;
	m_name = nullptr;
   m_refCount = 1;
}

/**
 * Create "boolean" value
 */
NXSL_Value::NXSL_Value(bool bValue)
{
   m_dataType = NXSL_DT_BOOLEAN;
   m_stringPtr = nullptr;
   m_length = 0;
#ifdef UNICODE
   m_mbString = nullptr;
#endif
   m_stringIsValid = FALSE;
   m_value.int32 = bValue ? 1 : 0;
   m_name = nullptr;
   m_refCount = 1;
}

/**
 * Create "int64" value from timestamp
 */
NXSL_Value::NXSL_Value(Timestamp value)
{
   m_dataType = NXSL_DT_INT64;
   m_stringPtr = nullptr;
   m_length = 0;
#ifdef UNICODE
   m_mbString = nullptr;
#endif
   m_stringIsValid = FALSE;
   m_value.int64 = value.asMilliseconds();
   m_name = nullptr;
   m_refCount = 1;
}

/**
 * Create "string" value
 */
NXSL_Value::NXSL_Value(const TCHAR *value)
{
   m_dataType = NXSL_DT_STRING;
	if (value != nullptr)
	{
		m_length = static_cast<uint32_t>(_tcslen(value));
		if (m_length < NXSL_SHORT_STRING_LENGTH)
		{
		   _tcscpy(m_stringValue, value);
		   m_stringPtr = nullptr;
		}
		else
		{
		   m_stringPtr = MemCopyString(value);
		}
	}
	else
	{
		m_length = 0;
		m_stringValue[0] = 0;
		m_stringPtr = nullptr;
	}
#ifdef UNICODE
	m_mbString = nullptr;
#endif
   m_stringIsValid = TRUE;
   updateNumber();
   m_name = nullptr;
   m_refCount = 1;
}

#ifdef UNICODE

/**
 * Create "string" value from UTF-8 string
 */
NXSL_Value::NXSL_Value(const char *value)
{
   m_dataType = NXSL_DT_STRING;
	if (value != nullptr)
	{
		m_stringPtr = WideStringFromUTF8String(value);
		m_length = static_cast<uint32_t>(_tcslen(m_stringPtr));
		if (m_length < NXSL_SHORT_STRING_LENGTH)
		{
		   _tcscpy(m_stringValue, m_stringPtr);
		   MemFree(m_stringPtr);
		   m_stringPtr = nullptr;
		}
	}
	else
	{
		m_length = 0;
      m_stringValue[0] = 0;
      m_stringPtr = nullptr;
	}
	m_mbString = nullptr;
   m_stringIsValid = TRUE;
   updateNumber();
   m_name = nullptr;
   m_refCount = 1;
}

#endif

/**
 * Create "string" value from non null-terminated string
 */
NXSL_Value::NXSL_Value(const TCHAR *value, size_t len)
{
   m_dataType = NXSL_DT_STRING;
   m_length = static_cast<uint32_t>(len);
   if (m_length < NXSL_SHORT_STRING_LENGTH)
   {
      if (value != nullptr)
      {
         memcpy(m_stringValue, value, len * sizeof(TCHAR));
         m_stringValue[len] = 0;
      }
      else
      {
         memset(m_stringValue, 0, (len + 1) * sizeof(TCHAR));
      }
      m_stringPtr = nullptr;
   }
   else
   {
      m_stringPtr = MemAllocString(len + 1);
      if (value != nullptr)
      {
         memcpy(m_stringPtr, value, len * sizeof(TCHAR));
         m_stringPtr[len] = 0;
      }
      else
      {
         memset(m_stringPtr, 0, (len + 1) * sizeof(TCHAR));
      }
   }
#ifdef UNICODE
	m_mbString = nullptr;
#endif
   m_stringIsValid = TRUE;
   updateNumber();
   m_name = nullptr;
   m_refCount = 1;
}

/**
 * Destructor
 */
NXSL_Value::~NXSL_Value()
{
   dispose();
   MemFree(m_name);
}

/**
 * Dispose value - prepare for destructor call or value replacement
 */
void NXSL_Value::dispose(bool disposeString)
{
   if (disposeString && m_stringIsValid)
   {
      MemFreeAndNull(m_stringPtr);
#ifdef UNICODE
      MemFreeAndNull(m_mbString);
#endif
      m_stringIsValid = FALSE;
   }

   switch(m_dataType)
   {
      case NXSL_DT_OBJECT:
         m_value.object->vm()->destroyObject(m_value.object);
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
void NXSL_Value::set(int32_t value)
{
   dispose();
   m_dataType = NXSL_DT_INT32;
   m_value.int32 = value;
}

/**
 * Set value to "boolean"
 */
void NXSL_Value::set(bool value)
{
   dispose();
   m_dataType = NXSL_DT_BOOLEAN;
   m_value.int32 = value ? 1 : 0;
}

/**
 * Update numeric value after string change
 */
void NXSL_Value::updateNumber()
{
   const TCHAR *s = (m_length < NXSL_SHORT_STRING_LENGTH) ? m_stringValue : m_stringPtr;
   assert(s != nullptr);   // For clang static analyzer
   if (s[0] == 0)
      return;

   int radix = (((s[0] == '-') && (s[1] == '0') && (s[2] == 'x')) || ((s[0] == '0') && (s[1] == 'x'))) ? 16 : 10;
   TCHAR *eptr;
   int64_t nVal = _tcstoll(s, &eptr, radix);
   if ((*eptr == 0) && ((uint32_t)(eptr - s) == m_length))
   {
      if ((nVal > _LL(2147483647)) || (nVal < _LL(-2147483648)))
      {
         m_dataType = NXSL_DT_INT64;
         m_value.int64 = nVal;
      }
      else
      {
         m_dataType = NXSL_DT_INT32;
         m_value.int32 = (INT32)nVal;
      }
   }
   else if (*eptr == '.')
   {
      double dVal = _tcstod(s, &eptr);
      if ((*eptr == 0) && ((uint32_t)(eptr - s) == m_length))
      {
         m_dataType = NXSL_DT_REAL;
         m_value.real = dVal;
      }
   }
}

/**
 * Update string value
 */
void NXSL_Value::updateString()
{
   MemFree(m_stringPtr);
#ifdef UNICODE
   MemFreeAndNull(m_mbString);
#endif

   if ((m_dataType == NXSL_DT_ARRAY) || (m_dataType == NXSL_DT_HASHMAP) || (m_dataType == NXSL_DT_OBJECT))
   {
      StringBuffer sb;
      if (m_dataType == NXSL_DT_OBJECT)
         m_value.object->getClass()->toString(&sb, m_value.object);
      else if (m_dataType == NXSL_DT_ARRAY)
         m_value.arrayHandle->getObject()->toString(&sb, _T(", "), true);
      else
         m_value.hashMapHandle->getObject()->toString(&sb, _T(", "), true);

      m_length = static_cast<uint32_t>(sb.length());
      if (m_length < NXSL_SHORT_STRING_LENGTH)
      {
         _tcscpy(m_stringValue, sb.cstr());
         m_stringPtr = nullptr;
      }
      else
      {
         m_stringPtr = MemCopyString(sb.cstr());
      }
   }
   else if (m_dataType == NXSL_DT_GUID)
   {
      TCHAR text[40];
      _uuid_to_string(m_value.guid, text);
      m_length = 36;
      m_stringPtr = MemCopyString(text);
   }
   else
   {
      TCHAR buffer[64];
      switch(m_dataType)
      {
         case NXSL_DT_BOOLEAN:
            _tcscpy(buffer, m_value.int32 ? _T("true") : _T("false"));
            break;
         case NXSL_DT_INT32:
            IntegerToString(m_value.int32, buffer);
            break;
         case NXSL_DT_UINT32:
            IntegerToString(m_value.uint32, buffer);
            break;
         case NXSL_DT_INT64:
            IntegerToString(m_value.int64, buffer);
            break;
         case NXSL_DT_UINT64:
            IntegerToString(m_value.uint64, buffer);
            break;
         case NXSL_DT_REAL:
            _sntprintf(buffer, 64, _T("%f"), m_value.real);
            break;
         case NXSL_DT_NULL:
            buffer[0] = 0;
            break;
         case NXSL_DT_HASHMAP:
            _sntprintf(buffer, 64, _T("[M@%p]"), m_value.hashMapHandle->getObject());
            break;
         case NXSL_DT_ITERATOR:
            _sntprintf(buffer, 64, _T("[I@%p]"), m_value.iterator);
            break;
         default:
            buffer[0] = 0;
            break;
      }
      m_length = static_cast<uint32_t>(_tcslen(buffer));
      if (m_length < NXSL_SHORT_STRING_LENGTH)
      {
         _tcscpy(m_stringValue, buffer);
         m_stringPtr = nullptr;
      }
      else
      {
         m_stringPtr = MemCopyString(buffer);
      }
   }
   m_stringIsValid = TRUE;
}

/**
 * Convert to another data type
 */
bool NXSL_Value::convert(int targetDataType)
{
   if (m_dataType == targetDataType)
      return true;

	if (targetDataType == NXSL_DT_STRING)
   {
	   if (isString())
	      return true;

      if (m_dataType == NXSL_DT_NULL)
      {
         // Next call to updateString() will create empty string value
         m_dataType = NXSL_DT_STRING;
         invalidateString();
         return true;
      }

	   if ((m_dataType == NXSL_DT_ARRAY) || (m_dataType == NXSL_DT_OBJECT) || (m_dataType == NXSL_DT_ITERATOR) || (m_dataType == NXSL_DT_HASHMAP) || (m_dataType == NXSL_DT_GUID))
	   {
	      // Call to updateString() will create string representation of object/array/etc.
	      // and dispose(false) will destroy object value leaving string representation intact
	      updateString();
	      dispose(false);
         m_dataType = NXSL_DT_STRING;
         return true;
	   }
   }

   if (targetDataType == NXSL_DT_BOOLEAN)
   {
      bool value = isTrue();
      dispose(true);
      m_dataType = NXSL_DT_BOOLEAN;
      m_value.int32 = value ? 1 : 0;
      return true;
   }

   int32_t nInt32;
   uint32_t uInt32;
   int64_t nInt64;
   uint64_t uInt64;
   double dReal;
   bool bRet = true;

   switch(targetDataType)
   {
      case NXSL_DT_INT32:
         nInt32 = getValueAsIntegerType<int32_t>();
         m_dataType = targetDataType;
         m_value.int32 = nInt32;
         break;
      case NXSL_DT_UINT32:
         uInt32 = getValueAsIntegerType<uint32_t>();
         m_dataType = targetDataType;
         m_value.uint32 = uInt32;
         break;
      case NXSL_DT_INT64:
         nInt64 = getValueAsIntegerType<int64_t>();
         m_dataType = targetDataType;
         m_value.int64 = nInt64;
         break;
      case NXSL_DT_UINT64:
         uInt64 = getValueAsIntegerType<uint64_t>();
         m_dataType = targetDataType;
         m_value.uint64 = uInt64;
         break;
      case NXSL_DT_REAL:
         if ((m_dataType == NXSL_DT_UINT64) && (m_value.uint64 > _ULL(9007199254740992)))
         {
            // forbid automatic conversion if 64 bit number may not be represented exactly as floating point number
            bRet = false;
         }
         else if ((m_dataType == NXSL_DT_INT64) && ((m_value.int64 > _LL(9007199254740992)) || (m_value.int64 < _LL(-9007199254740992))))
         {
            // forbid automatic conversion if 64 bit number may not be represented exactly as floating point number
            bRet = false;
         }
         else
         {
            dReal = getValueAsReal();
            m_dataType = targetDataType;
            m_value.real = dReal;
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
 * Get value as null-terminated string
 */
const TCHAR *NXSL_Value::getValueAsCString()
{
   if (!m_stringIsValid || (m_dataType == NXSL_DT_ARRAY) || (m_dataType == NXSL_DT_HASHMAP) || (m_dataType == NXSL_DT_OBJECT))
      updateString();
   return (m_length < NXSL_SHORT_STRING_LENGTH) ? m_stringValue : m_stringPtr;
}

/**
 * Get value as multibyte string
 */
#ifdef UNICODE

const char *NXSL_Value::getValueAsMBString()
{
   if ((m_dataType == NXSL_DT_ARRAY) || (m_dataType == NXSL_DT_HASHMAP) || (m_dataType == NXSL_DT_OBJECT))
   {
      updateString();   // Will also free existing m_mbString
      m_mbString = MBStringFromWideString((m_length < NXSL_SHORT_STRING_LENGTH) ? m_stringValue : CHECK_NULL_EX(m_stringPtr));
   }
   else if (m_mbString == nullptr)
   {
      if (!m_stringIsValid)
         updateString();
      m_mbString = MBStringFromWideString((m_length < NXSL_SHORT_STRING_LENGTH) ? m_stringValue : CHECK_NULL_EX(m_stringPtr));
   }
   return m_mbString;
}

#endif

/**
 * Get value as string
 */
const TCHAR *NXSL_Value::getValueAsString(UINT32 *pdwLen)
{
   if (!m_stringIsValid || (m_dataType == NXSL_DT_ARRAY) || (m_dataType == NXSL_DT_HASHMAP) || (m_dataType == NXSL_DT_OBJECT))
      updateString();
   *pdwLen = m_length;
   return (m_length < NXSL_SHORT_STRING_LENGTH) ? m_stringValue : m_stringPtr;
}

/**
 * Get value as real number
 */
double NXSL_Value::getValueAsReal()
{
   double dVal;
   switch(m_dataType)
   {
      case NXSL_DT_BOOLEAN:
         dVal = (double)(m_value.int32 ? 1 : 0);
         break;
      case NXSL_DT_INT32:
         dVal = (double)m_value.int32;
         break;
      case NXSL_DT_UINT32:
         dVal = (double)m_value.uint32;
         break;
      case NXSL_DT_INT64:
         dVal = (double)m_value.int64;
         break;
      case NXSL_DT_UINT64:
         dVal = (double)((int64_t)m_value.uint64);
         break;
      case NXSL_DT_REAL:
         dVal = m_value.real;
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
   if (!m_stringIsValid)
	{
      updateString();
	}
#ifdef UNICODE
	else
	{
		MemFreeAndNull(m_mbString);
	}
#endif
   if (m_length < NXSL_SHORT_STRING_LENGTH)
   {
      if (m_length + dwLen < NXSL_SHORT_STRING_LENGTH)
      {
         memcpy(&m_stringValue[m_length], pszString, dwLen * sizeof(TCHAR));
         m_length += dwLen;
         m_stringValue[m_length] = 0;
      }
      else
      {
         m_stringPtr = MemAllocString(m_length + dwLen + 1);
         memcpy(m_stringPtr, m_stringValue, m_length * sizeof(TCHAR));
         memcpy(&m_stringPtr[m_length], pszString, dwLen * sizeof(TCHAR));
         m_length += dwLen;
         m_stringPtr[m_length] = 0;
      }
   }
   else
   {
      m_stringPtr = MemRealloc(m_stringPtr, (m_length + dwLen + 1) * sizeof(TCHAR));
      memcpy(&m_stringPtr[m_length], pszString, dwLen * sizeof(TCHAR));
      m_length += dwLen;
      m_stringPtr[m_length] = 0;
   }
   m_dataType = NXSL_DT_STRING;
   updateNumber();
}

/**
 * Increment value
 */
void NXSL_Value::increment()
{
   if (isNumeric())
   {
      switch(m_dataType)
      {
         case NXSL_DT_INT32:
            m_value.int32++;
            break;
         case NXSL_DT_UINT32:
            m_value.uint32++;
            break;
         case NXSL_DT_INT64:
            m_value.int64++;
            break;
         case NXSL_DT_UINT64:
            m_value.uint64++;
            break;
         case NXSL_DT_REAL:
            m_value.real++;
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
      switch(m_dataType)
      {
         case NXSL_DT_INT32:
            m_value.int32--;
            break;
         case NXSL_DT_UINT32:
            m_value.uint32--;
            break;
         case NXSL_DT_INT64:
            m_value.int64--;
            break;
         case NXSL_DT_UINT64:
            m_value.uint64--;
            break;
         case NXSL_DT_REAL:
            m_value.real--;
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
      switch(m_dataType)
      {
         case NXSL_DT_BOOLEAN:
            m_value.int32 = m_value.int32 ? 0 : 1;
            break;
         case NXSL_DT_INT32:
            m_value.int32 = -m_value.int32;
            break;
         case NXSL_DT_UINT32:
            m_value.int32 = -((INT32)m_value.uint32);
            m_dataType = NXSL_DT_INT32;
            break;
         case NXSL_DT_INT64:
            m_value.int64 = -m_value.int64;
            break;
         case NXSL_DT_UINT64:
            m_value.int64 = -((INT64)m_value.uint64);
            m_dataType = NXSL_DT_INT64;
            break;
         case NXSL_DT_REAL:
            m_value.real = -m_value.real;
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
      switch(m_dataType)
      {
         case NXSL_DT_INT32:
            m_value.int32 = ~m_value.int32;
            break;
         case NXSL_DT_UINT32:
            m_value.int32 = ~m_value.uint32;
            break;
         case NXSL_DT_INT64:
            m_value.int64 = ~m_value.int64;
            break;
         case NXSL_DT_UINT64:
            m_value.int64 = ~m_value.uint64;
            break;
         default:
            break;
      }
      invalidateString();
   }
}

/**
 * Check if value is false
 */
bool NXSL_Value::isFalse() const
{
   bool result;
   switch(m_dataType)
   {
      case NXSL_DT_BOOLEAN:
      case NXSL_DT_INT32:
         result = (m_value.int32 == 0);
         break;
      case NXSL_DT_UINT32:
         result = (m_value.uint32 == 0);
         break;
      case NXSL_DT_INT64:
         result = (m_value.int64 == 0);
         break;
      case NXSL_DT_UINT64:
         result = (m_value.uint64 == 0);
         break;
      case NXSL_DT_REAL:
         result = (m_value.real == 0);
         break;
      case NXSL_DT_NULL:
         result = true;
         break;
      default:
         result = false;
         break;
   }
   return result;
}

/**
 * Check if value is true
 */
bool NXSL_Value::isTrue() const
{
   bool result;
   switch(m_dataType)
   {
      case NXSL_DT_BOOLEAN:
      case NXSL_DT_INT32:
         result = (m_value.int32 != 0);
         break;
      case NXSL_DT_UINT32:
         result = (m_value.uint32 != 0);
         break;
      case NXSL_DT_INT64:
         result = (m_value.int64 != 0);
         break;
      case NXSL_DT_UINT64:
         result = (m_value.uint64 != 0);
         break;
      case NXSL_DT_REAL:
         result = (m_value.real != 0);
         break;
      case NXSL_DT_NULL:
         result = false;
         break;
      default:
         result = true;
         break;
   }
   return result;
}

/**
 * Binary operation - EQ
 */
bool NXSL_Value::EQ(const NXSL_Value *value) const
{
   bool result = FALSE;

   switch(m_dataType)
   {
      case NXSL_DT_INT32:
         result = (m_value.int32 == value->m_value.int32);
         break;
      case NXSL_DT_UINT32:
         result = (m_value.uint32 == value->m_value.uint32);
         break;
      case NXSL_DT_INT64:
         result = (m_value.int64 == value->m_value.int64);
         break;
      case NXSL_DT_UINT64:
         result = (m_value.uint64 == value->m_value.uint64);
         break;
      case NXSL_DT_REAL:
         result = (m_value.real == value->m_value.real);
         break;
      default:
         break;
   }
   return result;
}

/**
 * Binary operation - LT
 */
bool NXSL_Value::LT(const NXSL_Value *value) const
{
   bool result = FALSE;

   switch(m_dataType)
   {
      case NXSL_DT_INT32:
         result = (m_value.int32 < value->m_value.int32);
         break;
      case NXSL_DT_UINT32:
         result = (m_value.uint32 < value->m_value.uint32);
         break;
      case NXSL_DT_INT64:
         result = (m_value.int64 < value->m_value.int64);
         break;
      case NXSL_DT_UINT64:
         result = (m_value.uint64 < value->m_value.uint64);
         break;
      case NXSL_DT_REAL:
         result = (m_value.real < value->m_value.real);
         break;
      default:
         break;
   }
   return result;
}

/**
 * Binary operation - LE
 */
bool NXSL_Value::LE(const NXSL_Value *value) const
{
   bool result = FALSE;

   switch(m_dataType)
   {
      case NXSL_DT_INT32:
         result = (m_value.int32 <= value->m_value.int32);
         break;
      case NXSL_DT_UINT32:
         result = (m_value.uint32 <= value->m_value.uint32);
         break;
      case NXSL_DT_INT64:
         result = (m_value.int64 <= value->m_value.int64);
         break;
      case NXSL_DT_UINT64:
         result = (m_value.uint64 <= value->m_value.uint64);
         break;
      case NXSL_DT_REAL:
         result = (m_value.real <= value->m_value.real);
         break;
      default:
         break;
   }
   return result;
}

/**
 * Binary operation - GT
 */
bool NXSL_Value::GT(const NXSL_Value *value) const
{
   bool result = FALSE;

   switch(m_dataType)
   {
      case NXSL_DT_INT32:
         result = (m_value.int32 > value->m_value.int32);
         break;
      case NXSL_DT_UINT32:
         result = (m_value.uint32 > value->m_value.uint32);
         break;
      case NXSL_DT_INT64:
         result = (m_value.int64 > value->m_value.int64);
         break;
      case NXSL_DT_UINT64:
         result = (m_value.uint64 > value->m_value.uint64);
         break;
      case NXSL_DT_REAL:
         result = (m_value.real > value->m_value.real);
         break;
      default:
         break;
   }
   return result;
}

/**
 * Binary operation - GE
 */
bool NXSL_Value::GE(const NXSL_Value *value) const
{
   bool result = FALSE;

   switch(m_dataType)
   {
      case NXSL_DT_INT32:
         result = (m_value.int32 >= value->m_value.int32);
         break;
      case NXSL_DT_UINT32:
         result = (m_value.uint32 >= value->m_value.uint32);
         break;
      case NXSL_DT_INT64:
         result = (m_value.int64 >= value->m_value.int64);
         break;
      case NXSL_DT_UINT64:
         result = (m_value.uint64 >= value->m_value.uint64);
         break;
      case NXSL_DT_REAL:
         result = (m_value.real >= value->m_value.real);
         break;
      default:
         break;
   }
   return result;
}

/**
 * Add
 */
void NXSL_Value::add(NXSL_Value *v)
{
   switch(m_dataType)
   {
      case NXSL_DT_INT32:
         m_value.int32 += v->m_value.int32;
         break;
      case NXSL_DT_UINT32:
         m_value.uint32 += v->m_value.uint32;
         break;
      case NXSL_DT_INT64:
         m_value.int64 += v->m_value.int64;
         break;
      case NXSL_DT_UINT64:
         m_value.uint64 += v->m_value.uint64;
         break;
      case NXSL_DT_REAL:
         m_value.real += v->m_value.real;
         break;
      default:
         break;
   }
   invalidateString();
}

/**
 * Subtract
 */
void NXSL_Value::sub(NXSL_Value *v)
{
   switch(m_dataType)
   {
      case NXSL_DT_INT32:
         m_value.int32 -= v->m_value.int32;
         break;
      case NXSL_DT_UINT32:
         m_value.uint32 -= v->m_value.uint32;
         break;
      case NXSL_DT_INT64:
         m_value.int64 -= v->m_value.int64;
         break;
      case NXSL_DT_UINT64:
         m_value.uint64 -= v->m_value.uint64;
         break;
      case NXSL_DT_REAL:
         m_value.real -= v->m_value.real;
         break;
      default:
         break;
   }
   invalidateString();
}

/**
 * Multiply
 */
void NXSL_Value::mul(NXSL_Value *v)
{
   switch(m_dataType)
   {
      case NXSL_DT_INT32:
         m_value.int32 *= v->m_value.int32;
         break;
      case NXSL_DT_UINT32:
         m_value.uint32 *= v->m_value.uint32;
         break;
      case NXSL_DT_INT64:
         m_value.int64 *= v->m_value.int64;
         break;
      case NXSL_DT_UINT64:
         m_value.uint64 *= v->m_value.uint64;
         break;
      case NXSL_DT_REAL:
         m_value.real *= v->m_value.real;
         break;
      default:
         break;
   }
   invalidateString();
}

/**
 * Divide
 */
void NXSL_Value::div(NXSL_Value *v)
{
   switch(m_dataType)
   {
      case NXSL_DT_INT32:
         m_value.int32 /= v->m_value.int32;
         break;
      case NXSL_DT_UINT32:
         m_value.uint32 /= v->m_value.uint32;
         break;
      case NXSL_DT_INT64:
         m_value.int64 /= v->m_value.int64;
         break;
      case NXSL_DT_UINT64:
         m_value.uint64 /= v->m_value.uint64;
         break;
      case NXSL_DT_REAL:
         m_value.real /= v->m_value.real;
         break;
      default:
         break;
   }
   invalidateString();
}

/**
 * Calculate division remainder
 */
void NXSL_Value::rem(NXSL_Value *v)
{
   switch(m_dataType)
   {
      case NXSL_DT_INT32:
         m_value.int32 %= v->m_value.int32;
         break;
      case NXSL_DT_UINT32:
         m_value.uint32 %= v->m_value.uint32;
         break;
      case NXSL_DT_INT64:
         m_value.int64 %= v->m_value.int64;
         break;
      case NXSL_DT_UINT64:
         m_value.uint64 %= v->m_value.uint64;
         break;
      default:
         break;
   }
   invalidateString();
}

/**
 * Bitwise AND
 */
void NXSL_Value::bitAnd(NXSL_Value *v)
{
   switch(m_dataType)
   {
      case NXSL_DT_INT32:
         m_value.int32 &= v->m_value.int32;
         break;
      case NXSL_DT_UINT32:
         m_value.uint32 &= v->m_value.uint32;
         break;
      case NXSL_DT_INT64:
         m_value.int64 &= v->m_value.int64;
         break;
      case NXSL_DT_UINT64:
         m_value.uint64 &= v->m_value.uint64;
         break;
      default:
         break;
   }
   invalidateString();
}

/**
 * Bitwise OR
 */
void NXSL_Value::bitOr(NXSL_Value *v)
{
   switch(m_dataType)
   {
      case NXSL_DT_INT32:
         m_value.int32 |= v->m_value.int32;
         break;
      case NXSL_DT_UINT32:
         m_value.uint32 |= v->m_value.uint32;
         break;
      case NXSL_DT_INT64:
         m_value.int64 |= v->m_value.int64;
         break;
      case NXSL_DT_UINT64:
         m_value.uint64 |= v->m_value.uint64;
         break;
      default:
         break;
   }
   invalidateString();
}

/**
 * Bitwise ZOR
 */
void NXSL_Value::bitXor(NXSL_Value *v)
{
   switch(m_dataType)
   {
      case NXSL_DT_INT32:
         m_value.int32 ^= v->m_value.int32;
         break;
      case NXSL_DT_UINT32:
         m_value.uint32 ^= v->m_value.uint32;
         break;
      case NXSL_DT_INT64:
         m_value.int64 ^= v->m_value.int64;
         break;
      case NXSL_DT_UINT64:
         m_value.uint64 ^= v->m_value.uint64;
         break;
      default:
         break;
   }
   invalidateString();
}

/**
 * Bitwise shift to the left
 */
void NXSL_Value::lshift(int nBits)
{
   switch(m_dataType)
   {
      case NXSL_DT_INT32:
         m_value.int32 <<= nBits;
         break;
      case NXSL_DT_UINT32:
         m_value.uint32 <<= nBits;
         break;
      case NXSL_DT_INT64:
         m_value.int64 <<= nBits;
         break;
      case NXSL_DT_UINT64:
         m_value.uint64 <<= nBits;
         break;
      default:
         break;
   }
   invalidateString();
}

/**
 * Bitwise shift to the right
 */
void NXSL_Value::rshift(int nBits)
{
   switch(m_dataType)
   {
      case NXSL_DT_INT32:
         m_value.int32 >>= nBits;
         break;
      case NXSL_DT_UINT32:
         m_value.uint32 >>= nBits;
         break;
      case NXSL_DT_INT64:
         m_value.int64 >>= nBits;
         break;
      case NXSL_DT_UINT64:
         m_value.uint64 >>= nBits;
         break;
      default:
         break;
   }
   invalidateString();
}

/**
 * Check if value is an object of given class
 */
bool NXSL_Value::isObject(const TCHAR *className) const
{
	if (m_dataType != NXSL_DT_OBJECT)
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
   if (v->m_dataType != m_dataType)
      return false;
   switch(m_dataType)
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
      case NXSL_DT_BOOLEAN:
         return (v->m_value.int32 && m_value.int32) || (!v->m_value.int32 && !m_value.int32);
      case NXSL_DT_INT32:
         return v->m_value.int32 == m_value.int32;
      case NXSL_DT_INT64:
         return v->m_value.int64 == m_value.int64;
      case NXSL_DT_ITERATOR:
         return false;
      case NXSL_DT_GUID:
         return memcmp(v->m_value.guid, m_value.guid, UUID_LENGTH) == 0;
      case NXSL_DT_NULL:
         return true;
      case NXSL_DT_OBJECT:
         return (v->m_value.object->getData() == m_value.object->getData()) && 
            !_tcscmp(v->m_value.object->getClass()->getName(), m_value.object->getClass()->getName());
         break;
      case NXSL_DT_REAL:
         return v->m_value.real == m_value.real;
      case NXSL_DT_STRING:
         if (v->m_length != m_length)
            return false;
         if (m_length < NXSL_SHORT_STRING_LENGTH)
            return !_tcscmp(v->m_stringValue, m_stringValue) ? true : false;
         return !_tcscmp(v->m_stringPtr, m_stringPtr) ? true : false;
      case NXSL_DT_UINT32:
         return v->m_value.uint32 == m_value.uint32;
      case NXSL_DT_UINT64:
         return v->m_value.uint64 == m_value.uint64;
   }
   return false;
}

/**
 * Serialize
 */
void NXSL_Value::serialize(ByteStream &s) const
{
   s.write(m_dataType);
   switch(m_dataType)
   {
      case NXSL_DT_ARRAY:
         {
            s.writeB((UINT16)m_value.arrayHandle->getObject()->size());
            for(int i = 0; i < m_value.arrayHandle->getObject()->size(); i++)
            {
               m_value.arrayHandle->getObject()->get(i)->serialize(s);
            }
         }
         break;
      case NXSL_DT_HASHMAP:
         s.writeB((UINT16)m_value.hashMapHandle->getObject()->size());
         if (m_value.hashMapHandle->getObject()->size() > 0)
         {
            // TODO: hashmap serialize
         }
         break;
      case NXSL_DT_BOOLEAN:
      case NXSL_DT_INT32:
         s.writeB(m_value.int32);
         break;
      case NXSL_DT_INT64:
         s.writeB(m_value.int64);
         break;
      case NXSL_DT_ITERATOR:
         break;
      case NXSL_DT_NULL:
         break;
      case NXSL_DT_OBJECT:
         break;
      case NXSL_DT_REAL:
         s.writeB(m_value.real);
         break;
      case NXSL_DT_STRING:
         s.writeString((m_length < NXSL_SHORT_STRING_LENGTH) ? m_stringValue : m_stringPtr, "UTF-8", -1, true, false);
         break;
      case NXSL_DT_UINT32:
         s.writeB(m_value.uint32);
         break;
      case NXSL_DT_UINT64:
         s.writeB(m_value.uint64);
         break;
      case NXSL_DT_GUID:
         s.write(m_value.guid, UUID_LENGTH);
         break;
   }
}

/**
 * Load value from byte stream
 */
NXSL_Value *NXSL_Value::load(NXSL_ValueManager *vm, ByteStream &s)
{
   NXSL_Value *v = vm->createValue();
   v->m_dataType = s.readByte();
   switch(v->m_dataType)
   {
      case NXSL_DT_ARRAY:
         {
            v->m_value.arrayHandle = new NXSL_Handle<NXSL_Array>(new NXSL_Array(vm));
            int size = (int)s.readUInt16B();
            for(int i = 0; i < size; i++)
            {
               v->m_value.arrayHandle->getObject()->set(i, load(vm, s));
            }
         }
         break;
      case NXSL_DT_HASHMAP:
         {
            v->m_value.hashMapHandle = new NXSL_Handle<NXSL_HashMap>(new NXSL_HashMap(vm));
            int size = (int)s.readUInt16B();
            for(int i = 0; i < size; i++)
            {
               // TODO: implement hash map load
            }
         }
         break;
      case NXSL_DT_BOOLEAN:
      case NXSL_DT_INT32:
         v->m_value.int32 = s.readInt32B();
         break;
      case NXSL_DT_INT64:
         v->m_value.int64 = s.readInt64B();
         break;
      case NXSL_DT_ITERATOR:
         break;
      case NXSL_DT_NULL:
         break;
      case NXSL_DT_OBJECT:
         break;
      case NXSL_DT_REAL:
         v->m_value.real = s.readDoubleB();
         break;
      case NXSL_DT_STRING:
         v->m_stringPtr = s.readPStringW("UTF-8");
         v->m_length = (UINT32)_tcslen(v->m_stringPtr);
         if (v->m_length < NXSL_SHORT_STRING_LENGTH)
         {
            _tcscpy(v->m_stringValue, v->m_stringPtr);
            MemFree(v->m_stringPtr);
            v->m_stringPtr = nullptr;
         }
         v->m_stringIsValid = TRUE;
         v->updateNumber();
         break;
      case NXSL_DT_UINT32:
         v->m_value.uint32 = s.readUInt32B();
         break;
      case NXSL_DT_UINT64:
         v->m_value.uint64 = s.readUInt64B();
         break;
      case NXSL_DT_GUID:
         s.read(v->m_value.guid, UUID_LENGTH);
         break;
   }
   return v;
}

/**
 * Do copy on write if needed
 */
void NXSL_Value::copyOnWrite()
{
   if ((m_dataType == NXSL_DT_ARRAY) && m_value.arrayHandle->isSharedObject())
   {
      m_value.arrayHandle->cloneObject();
   }
   else if ((m_dataType == NXSL_DT_HASHMAP) && m_value.hashMapHandle->isSharedObject())
   {
      m_value.hashMapHandle->cloneObject();
   }
}

/**
 * Called when value set as variable's value
 */
void NXSL_Value::onVariableSet()
{
   switch(m_dataType)
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
