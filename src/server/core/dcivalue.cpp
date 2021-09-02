/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Victor Kirhenshtein
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
   m_int64 = 0;
   m_uint64 = 0;
   m_double = 0;
   m_timestamp = time(nullptr);
}

/**
 * Construct value object from string value
 */
ItemValue::ItemValue(const TCHAR *value, time_t timestamp)
{
   _tcslcpy(m_string, value, MAX_DB_STRING);
   m_int64 = _tcstoll(m_string, nullptr, 0);
   m_uint64 = _tcstoull(m_string, nullptr, 0);
   m_double = _tcstod(m_string, nullptr);
   m_timestamp = (timestamp == 0) ? time(nullptr) : timestamp;
}

/**
 * Construct value object from another ItemValue object
 */
ItemValue::ItemValue(const ItemValue *value)
{
   _tcscpy(m_string, value->m_string);
   m_int64 = value->m_int64;
   m_uint64 = value->m_uint64;
   m_double = value->m_double;
   m_timestamp = value->m_timestamp;
}

/**
 * Assignment operators
 */
const ItemValue& ItemValue::operator=(const ItemValue &src)
{
   memcpy(m_string, src.m_string, sizeof(TCHAR) * MAX_DB_STRING);
   m_int64 = src.m_int64;
   m_uint64 = src.m_uint64;
   m_double = src.m_double;
   return *this;
}

const ItemValue& ItemValue::operator=(const TCHAR *value)
{
   _tcslcpy(m_string, CHECK_NULL_EX(value), MAX_DB_STRING);
   m_int64 = _tcstoll(m_string, nullptr, 0);
   m_uint64 = _tcstoull(m_string, nullptr, 0);
   m_double = _tcstod(m_string, nullptr);
   return *this;
}

const ItemValue& ItemValue::operator=(double value)
{
   m_double = value;
   _sntprintf(m_string, MAX_DB_STRING, _T("%f"), m_double);
   m_int64 = static_cast<int64_t>(m_double);
   m_uint64 = static_cast<uint64_t>(m_double);
   return *this;
}

const ItemValue& ItemValue::operator=(int32_t value)
{
   m_int64 = value;
   _sntprintf(m_string, MAX_DB_STRING, _T("%d"), value);
   m_double = value;
   m_uint64 = value;
   return *this;
}

const ItemValue& ItemValue::operator=(int64_t value)
{
   m_int64 = value;
   _sntprintf(m_string, MAX_DB_STRING, INT64_FMT, m_int64);
   m_double = m_int64;
   m_uint64 = static_cast<uint64_t>(m_int64);
   return *this;
}

const ItemValue& ItemValue::operator=(uint32_t value)
{
   m_uint64 = value;
   _sntprintf(m_string, MAX_DB_STRING, _T("%u"), value);
   m_double = value;
   m_int64 = value;
   return *this;
}

const ItemValue& ItemValue::operator=(uint64_t value)
{
   m_uint64 = value;
   _sntprintf(m_string, MAX_DB_STRING, UINT64_FMT, m_uint64);
   m_double = static_cast<double>(static_cast<int64_t>(m_uint64));
   m_int64 = static_cast<int64_t>(m_uint64);
   return *this;
}

/**
 * Signed diff for unsigned int32 values
 */
inline int64_t diff_uint32(uint32_t curr, uint32_t prev)
{
   return (curr >= prev) ? (int64_t)(curr - prev) : -((int64_t)(prev - curr));
}

/**
 * Signed diff for unsigned int64 values
 */
inline int64_t diff_uint64(uint64_t curr, uint64_t prev)
{
   return (curr >= prev) ? (int64_t)(curr - prev) : -((int64_t)(prev - curr));
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
      case DCI_DT_COUNTER32:
         result = diff_uint32(curr.getUInt32(), prev.getUInt32());
         break;
      case DCI_DT_INT64:
         result = curr.getInt64() - prev.getInt64();
         break;
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER64:
         result = diff_uint64(curr.getUInt64(), prev.getUInt64());
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
void CalculateItemValueAverage(ItemValue &result, int nDataType, const ItemValue * const *valueList, size_t numValues)
{
#define CALC_AVG_VALUE(vtype) \
{ \
   vtype var; \
   var = 0; \
   for(i = 0, valueCount = 0; i < numValues; i++) \
   { \
      if (valueList[i]->getTimeStamp() != 1) \
      { \
         var += (vtype)(*valueList[i]); \
         valueCount++; \
      } \
   } \
   if (valueCount == 0) { valueCount = 1; } \
   result = var / (vtype)valueCount; \
}

   size_t i;
   int valueCount;

   switch(nDataType)
   {
      case DCI_DT_INT:
         CALC_AVG_VALUE(INT32);
         break;
      case DCI_DT_UINT:
      case DCI_DT_COUNTER32:
         CALC_AVG_VALUE(UINT32);
         break;
      case DCI_DT_INT64:
         CALC_AVG_VALUE(INT64);
         break;
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER64:
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
void CalculateItemValueTotal(ItemValue &result, int nDataType, const ItemValue * const *valueList, size_t numValues)
{
#define CALC_TOTAL_VALUE(vtype) \
{ \
   vtype var; \
   var = 0; \
   for(i = 0; i < numValues; i++) \
   { \
      if (valueList[i]->getTimeStamp() != 1) \
      { \
         var += (vtype)(*valueList[i]); \
      } \
   } \
   result = var; \
}

   size_t i;

   switch(nDataType)
   {
      case DCI_DT_INT:
         CALC_TOTAL_VALUE(INT32);
         break;
      case DCI_DT_UINT:
      case DCI_DT_COUNTER32:
         CALC_TOTAL_VALUE(UINT32);
         break;
      case DCI_DT_INT64:
         CALC_TOTAL_VALUE(INT64);
         break;
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER64:
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
void CalculateItemValueMD(ItemValue &result, int nDataType, const ItemValue * const *valueList, size_t numValues)
{
#define CALC_MD_VALUE(vtype) \
{ \
   vtype mean, dev; \
   mean = 0; \
   for(i = 0, valueCount = 0; i < numValues; i++) \
   { \
      if (valueList[i]->getTimeStamp() != 1) \
      { \
         mean += (vtype)(*valueList[i]); \
         valueCount++; \
      } \
   } \
   mean /= (vtype)valueCount; \
   dev = 0; \
   for(i = 0, valueCount = 0; i < numValues; i++) \
   { \
      if (valueList[i]->getTimeStamp() != 1) \
      { \
         dev += ABS((vtype)(*valueList[i]) - mean); \
         valueCount++; \
      } \
   } \
   result = dev / (vtype)valueCount; \
}

   size_t i;
   int valueCount;

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
      case DCI_DT_COUNTER32:
#undef ABS
#define ABS(x) (x)
         CALC_MD_VALUE(UINT32);
         break;
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER64:
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
void CalculateItemValueMin(ItemValue &result, int nDataType, const ItemValue * const *valueList, size_t numValues)
{
#define CALC_MIN_VALUE(vtype) \
{ \
   bool first = true; \
   vtype var = 0; \
   for(i = 0; i < numValues; i++) \
   { \
      if (valueList[i]->getTimeStamp() != 1) \
      { \
         vtype curr = (vtype)(*valueList[i]); \
         if (first || (curr < var)) { var = curr; first = false; } \
      } \
   } \
   result = var; \
}

   size_t i;

   switch(nDataType)
   {
      case DCI_DT_INT:
         CALC_MIN_VALUE(INT32);
         break;
      case DCI_DT_UINT:
      case DCI_DT_COUNTER32:
         CALC_MIN_VALUE(UINT32);
         break;
      case DCI_DT_INT64:
         CALC_MIN_VALUE(INT64);
         break;
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER64:
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
void CalculateItemValueMax(ItemValue &result, int nDataType, const ItemValue * const *valueList, size_t numValues)
{
#define CALC_MAX_VALUE(vtype) \
{ \
   bool first = true; \
   vtype var = 0; \
   for(i = 0; i < numValues; i++) \
   { \
      if (valueList[i]->getTimeStamp() != 1) \
      { \
         vtype curr = (vtype)(*valueList[i]); \
         if (first || (curr > var)) { var = curr; first = false; } \
      } \
   } \
   result = var; \
}

   size_t i;

   switch(nDataType)
   {
      case DCI_DT_INT:
         CALC_MAX_VALUE(INT32);
         break;
      case DCI_DT_UINT:
      case DCI_DT_COUNTER32:
         CALC_MAX_VALUE(UINT32);
         break;
      case DCI_DT_INT64:
         CALC_MAX_VALUE(INT64);
         break;
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER64:
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
