/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: dcivalue.cpp
**
**/

#include "nxcore.h"

/**
 * Default constructor
 */
ItemValue::ItemValue()
{
   m_string[0] = 0;
   m_int32 = 0;
   m_int64 = 0;
   m_uint32 = 0;
   m_uint64 = 0;
   m_double = 0;
   m_timestamp = time(NULL);
}

/**
 * Construct value object from string value
 */
ItemValue::ItemValue(const TCHAR *value, time_t timestamp)
{
   nx_strncpy(m_string, value, MAX_DB_STRING);
   m_int32 = _tcstol(m_string, NULL, 0);
   m_int64 = _tcstoll(m_string, NULL, 0);
   m_uint32 = _tcstoul(m_string, NULL, 0);
   m_uint64 = _tcstoull(m_string, NULL, 0);
   m_double = _tcstod(m_string, NULL);
   m_timestamp = (timestamp == 0) ? time(NULL) : timestamp;
}

/**
 * Construct value object from another ItemValue object
 */
ItemValue::ItemValue(const ItemValue *value)
{
   _tcscpy(m_string, value->m_string);
   m_int32 = value->m_int32;
   m_int64 = value->m_int64;
   m_uint32 = value->m_uint32;
   m_uint64 = value->m_uint64;
   m_double = value->m_double;
   m_timestamp = value->m_timestamp;
}

/**
 * Destructor
 */
ItemValue::~ItemValue()
{
}

/**
 * Assignment operators
 */
const ItemValue& ItemValue::operator=(const ItemValue &src)
{
   _tcscpy(m_string, src.m_string);
   m_int32 = src.m_int32;
   m_int64 = src.m_int64;
   m_uint32 = src.m_uint32;
   m_uint64 = src.m_uint64;
   m_double = src.m_double;
   return *this;
}

const ItemValue& ItemValue::operator=(const TCHAR *value)
{
   nx_strncpy(m_string, CHECK_NULL_EX(value), MAX_DB_STRING);
   m_int32 = _tcstol(m_string, NULL, 0);
   m_int64 = _tcstoll(m_string, NULL, 0);
   m_uint32 = _tcstoul(m_string, NULL, 0);
   m_uint64 = _tcstoull(m_string, NULL, 0);
   m_double = _tcstod(m_string, NULL);
   return *this;
}

const ItemValue& ItemValue::operator=(double value)
{
   m_double = value;
   _sntprintf(m_string, MAX_DB_STRING, _T("%f"), m_double);
   m_int32 = (INT32)m_double;
   m_int64 = (INT64)m_double;
   m_uint32 = (UINT32)m_double;
   m_uint64 = (UINT64)m_double;
   return *this;
}

const ItemValue& ItemValue::operator=(INT32 value)
{
   m_int32 = value;
   _sntprintf(m_string, MAX_DB_STRING, _T("%d"), m_int32);
   m_double = (double)m_int32;
   m_int64 = (INT64)m_int32;
   m_uint32 = (UINT32)m_int32;
   m_uint64 = (UINT64)m_int32;
   return *this;
}

const ItemValue& ItemValue::operator=(INT64 value)
{
   m_int64 = value;
   _sntprintf(m_string, MAX_DB_STRING, INT64_FMT, m_int64);
   m_double = (double)m_int64;
   m_int32 = (INT32)m_int64;
   m_uint32 = (UINT32)m_int64;
   m_uint64 = (UINT64)m_int64;
   return *this;
}

const ItemValue& ItemValue::operator=(UINT32 value)
{
   m_uint32 = value;
   _sntprintf(m_string, MAX_DB_STRING, _T("%u"), m_uint32);
   m_double = (double)m_uint32;
   m_int32 = (INT32)m_uint32;
   m_int64 = (INT64)m_uint32;
   m_uint64 = (UINT64)m_uint32;
   return *this;
}

const ItemValue& ItemValue::operator=(UINT64 value)
{
   m_uint64 = value;
   _sntprintf(m_string, MAX_DB_STRING, UINT64_FMT, m_uint64);
   m_double = (double)((INT64)m_uint64);
   m_int32 = (INT32)m_uint64;
   m_int64 = (INT64)m_uint64;
   return *this;
}

/**
 * Signed diff for unsigned int32 values
 */
inline INT64 diff_uint32(UINT32 curr, UINT32 prev)
{
   return (curr >= prev) ? (INT64)(curr - prev) : -((INT64)(prev - curr));
}

/**
 * Signed diff for unsigned int64 values
 */
inline INT64 diff_uint64(UINT64 curr, UINT64 prev)
{
   return (curr >= prev) ? (INT64)(curr - prev) : -((INT64)(prev - curr));
}

/**
 * Calculate difference between two values
 */
void CalculateItemValueDiff(ItemValue &result, int nDataType, const ItemValue &curr, const ItemValue &prev)
{
   switch(nDataType)
   {
      case DCI_DT_INT:
         result = curr.getInt32() - prev.getInt32();
         break;
      case DCI_DT_UINT:
         result = diff_uint32(curr, prev);
         break;
      case DCI_DT_INT64:
         result = curr.getInt64() - prev.getInt64();
         break;
      case DCI_DT_UINT64:
         result = diff_uint64(curr, prev);
         break;
      case DCI_DT_FLOAT:
         result = curr.getDouble() - prev.getDouble();
         break;
      case DCI_DT_STRING:
         result = (INT32)((_tcscmp(curr.getString(), prev.getString()) == 0) ? 0 : 1);
         break;
      default:
         // Delta calculation is not supported for other types
         result = curr;
         break;
   }
}

/**
 * Calculate average value for set of values
 */
void CalculateItemValueAverage(ItemValue &result, int nDataType, int nNumValues, ItemValue **ppValueList)
{
#define CALC_AVG_VALUE(vtype) \
{ \
   vtype var; \
   var = 0; \
   for(i = 0, nValueCount = 0; i < nNumValues; i++) \
   { \
      if (ppValueList[i]->getTimeStamp() != 1) \
      { \
         var += (vtype)(*ppValueList[i]); \
         nValueCount++; \
      } \
   } \
   if (nValueCount == 0) { nValueCount = 1; } \
   result = var / (vtype)nValueCount; \
}

   int i, nValueCount;

   switch(nDataType)
   {
      case DCI_DT_INT:
         CALC_AVG_VALUE(INT32);
         break;
      case DCI_DT_UINT:
         CALC_AVG_VALUE(UINT32);
         break;
      case DCI_DT_INT64:
         CALC_AVG_VALUE(INT64);
         break;
      case DCI_DT_UINT64:
         CALC_AVG_VALUE(UINT64);
         break;
      case DCI_DT_FLOAT:
         CALC_AVG_VALUE(double);
         break;
      case DCI_DT_STRING:
         result = _T("");   // Average value for string is meaningless
         break;
      default:
         break;
   }
}

/**
 * Calculate total value for set of values
 */
void CalculateItemValueTotal(ItemValue &result, int nDataType, int nNumValues, ItemValue **ppValueList)
{
#define CALC_TOTAL_VALUE(vtype) \
{ \
   vtype var; \
   var = 0; \
   for(i = 0; i < nNumValues; i++) \
   { \
      if (ppValueList[i]->getTimeStamp() != 1) \
      { \
         var += (vtype)(*ppValueList[i]); \
      } \
   } \
   result = var; \
}

   int i;

   switch(nDataType)
   {
      case DCI_DT_INT:
         CALC_TOTAL_VALUE(INT32);
         break;
      case DCI_DT_UINT:
         CALC_TOTAL_VALUE(UINT32);
         break;
      case DCI_DT_INT64:
         CALC_TOTAL_VALUE(INT64);
         break;
      case DCI_DT_UINT64:
         CALC_TOTAL_VALUE(UINT64);
         break;
      case DCI_DT_FLOAT:
         CALC_TOTAL_VALUE(double);
         break;
      case DCI_DT_STRING:
         result = _T("");
         break;
      default:
         break;
   }
}

/**
 * Calculate mean absolute deviation for set of values
 */
void CalculateItemValueMD(ItemValue &result, int nDataType, int nNumValues, ItemValue **ppValueList)
{
#define CALC_MD_VALUE(vtype) \
{ \
   vtype mean, dev; \
   mean = 0; \
   for(i = 0, nValueCount = 0; i < nNumValues; i++) \
   { \
      if (ppValueList[i]->getTimeStamp() != 1) \
      { \
         mean += (vtype)(*ppValueList[i]); \
         nValueCount++; \
      } \
   } \
   mean /= (vtype)nValueCount; \
   dev = 0; \
   for(i = 0, nValueCount = 0; i < nNumValues; i++) \
   { \
      if (ppValueList[i]->getTimeStamp() != 1) \
      { \
         dev += ABS((vtype)(*ppValueList[i]) - mean); \
         nValueCount++; \
      } \
   } \
   result = dev / (vtype)nValueCount; \
}

   int i, nValueCount;

   switch(nDataType)
   {
      case DCI_DT_INT:
#define ABS(x) ((x) < 0 ? -(x) : (x))
         CALC_MD_VALUE(INT32);
         break;
      case DCI_DT_INT64:
         CALC_MD_VALUE(INT64);
         break;
      case DCI_DT_FLOAT:
         CALC_MD_VALUE(double);
         break;
      case DCI_DT_UINT:
#undef ABS
#define ABS(x) (x)
         CALC_MD_VALUE(UINT32);
         break;
      case DCI_DT_UINT64:
         CALC_MD_VALUE(UINT64);
         break;
      case DCI_DT_STRING:
         result = _T("");   // Mean deviation for string is meaningless
         break;
      default:
         break;
   }
}

/**
 * Calculate min value for set of values
 */
void CalculateItemValueMin(ItemValue &result, int nDataType, int nNumValues, ItemValue **ppValueList)
{
#define CALC_MIN_VALUE(vtype) \
{ \
   bool first = true; \
   vtype var = 0; \
   for(i = 0; i < nNumValues; i++) \
   { \
      if (ppValueList[i]->getTimeStamp() != 1) \
      { \
         vtype curr = (vtype)(*ppValueList[i]); \
         if (first || (curr < var)) { var = curr; first = false; } \
      } \
   } \
   result = var; \
}

   int i;

   switch(nDataType)
   {
      case DCI_DT_INT:
         CALC_MIN_VALUE(INT32);
         break;
      case DCI_DT_UINT:
         CALC_MIN_VALUE(UINT32);
         break;
      case DCI_DT_INT64:
         CALC_MIN_VALUE(INT64);
         break;
      case DCI_DT_UINT64:
         CALC_MIN_VALUE(UINT64);
         break;
      case DCI_DT_FLOAT:
         CALC_MIN_VALUE(double);
         break;
      case DCI_DT_STRING:
         result = _T("");   // Min value for string is meaningless
         break;
      default:
         break;
   }
}


/**
 * Calculate max value for set of values
 */
void CalculateItemValueMax(ItemValue &result, int nDataType, int nNumValues, ItemValue **ppValueList)
{
#define CALC_MAX_VALUE(vtype) \
{ \
   bool first = true; \
   vtype var = 0; \
   for(i = 0; i < nNumValues; i++) \
   { \
      if (ppValueList[i]->getTimeStamp() != 1) \
      { \
         vtype curr = (vtype)(*ppValueList[i]); \
         if (first || (curr > var)) { var = curr; first = false; } \
      } \
   } \
   result = var; \
}

   int i;

   switch(nDataType)
   {
      case DCI_DT_INT:
         CALC_MAX_VALUE(INT32);
         break;
      case DCI_DT_UINT:
         CALC_MAX_VALUE(UINT32);
         break;
      case DCI_DT_INT64:
         CALC_MAX_VALUE(INT64);
         break;
      case DCI_DT_UINT64:
         CALC_MAX_VALUE(UINT64);
         break;
      case DCI_DT_FLOAT:
         CALC_MAX_VALUE(double);
         break;
      case DCI_DT_STRING:
         result = _T("");   // Max value for string is meaningless
         break;
      default:
         break;
   }
}
