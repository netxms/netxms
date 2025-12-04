/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
   m_timestamp = Timestamp::now();
}

/**
 * Construct value object from string value
 */
ItemValue::ItemValue(const wchar_t *value, Timestamp timestamp, bool parseSuffix)
{
   wcslcpy(m_string, value, MAX_DB_STRING);
   parseStringValue(parseSuffix);
   m_timestamp = timestamp.isNull() ? Timestamp::now() : timestamp;
}

/**
 * Construct value object from database field
 */
ItemValue::ItemValue(DB_RESULT hResult, int row, int column, Timestamp timestamp, bool parseSuffix)
{
   DBGetField(hResult, row, column, m_string, MAX_DB_STRING);
   parseStringValue(parseSuffix);
   m_timestamp = timestamp.isNull() ? Timestamp::now() : timestamp;
}

/**
 * Parse string value
 */
void ItemValue::parseStringValue(bool parseSuffix)
{
   wchar_t *eptr;
   m_double = wcstod(m_string, &eptr);
   if (parseSuffix)
   {
      while(*eptr == ' ')
         eptr++;

      int p;
      if ((*eptr == 'K') || (*eptr == 'k'))
         p = 1;
      else if ((*eptr == 'M') || (*eptr == 'm'))
         p = 2;
      else if ((*eptr == 'G') || (*eptr == 'g'))
         p = 3;
      else if ((*eptr == 'T') || (*eptr == 't'))
         p = 4;
      else
         p = 0;

      if (p > 0)
      {
         int64_t multiplier;
         eptr++;
         if ((*eptr == 'I') || (*eptr == 'i'))
            multiplier = static_cast<int64_t>(pow(1024.0, p));
         else
            multiplier = static_cast<int64_t>(pow(1000.0, p));

         m_double *= multiplier;
         m_int64 = static_cast<int64_t>(m_double);
         m_uint64 = static_cast<uint64_t>(m_double);
      }
      else
      {
         m_int64 = wcstoll(m_string, nullptr, 0);
         m_uint64 = wcstoull(m_string, nullptr, 0);
      }
   }
   else
   {
      m_int64 = wcstoll(m_string, nullptr, 0);
      m_uint64 = wcstoull(m_string, nullptr, 0);
   }
}

/**
 * Set from string value
 */
void ItemValue::set(const TCHAR *value, bool parseSuffix)
{
   _tcslcpy(m_string, CHECK_NULL_EX(value), MAX_DB_STRING);
   parseStringValue(parseSuffix);
}

/**
 * Set from double value
 */
void ItemValue::set(double value, const TCHAR *stringValue)
{
   m_double = value;
   if (stringValue != nullptr)
      _tcslcpy(m_string, stringValue, MAX_DB_STRING);
   else
      _sntprintf(m_string, MAX_DB_STRING, _T("%f"), m_double);
   m_int64 = static_cast<int64_t>(m_double);
   m_uint64 = static_cast<uint64_t>(m_double);
}

/**
 * Set from 32 bit integer value
 */
void ItemValue::set(int32_t value, const TCHAR *stringValue)
{
   m_int64 = value;
   if (stringValue != nullptr)
      _tcslcpy(m_string, stringValue, MAX_DB_STRING);
   else
      IntegerToString(value, m_string);
   m_double = value;
   m_uint64 = value;
}

/**
 * Set from 64 bit integer value
 */
void ItemValue::set(int64_t value, const TCHAR *stringValue)
{
   m_int64 = value;
   if (stringValue != nullptr)
      _tcslcpy(m_string, stringValue, MAX_DB_STRING);
   else
      IntegerToString(value, m_string);
   m_double = static_cast<double>(m_int64);
   m_uint64 = static_cast<uint64_t>(m_int64);
}

/**
 * Set from unsigned 32 bit integer value
 */
void ItemValue::set(uint32_t value, const TCHAR *stringValue)
{
   m_uint64 = value;
   if (stringValue != nullptr)
      _tcslcpy(m_string, stringValue, MAX_DB_STRING);
   else
      IntegerToString(value, m_string);
   m_double = value;
   m_int64 = value;
}

/**
 * Set from unsigned 64 bit integer value
 */
void ItemValue::set(uint64_t value, const TCHAR *stringValue)
{
   m_uint64 = value;
   if (stringValue != nullptr)
      _tcslcpy(m_string, stringValue, MAX_DB_STRING);
   else
      IntegerToString(value, m_string);
   m_double = static_cast<double>(static_cast<int64_t>(m_uint64));
   m_int64 = static_cast<int64_t>(m_uint64);
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
void CalculateItemValueDiff(ItemValue *result, int dataType, const ItemValue &curr, const ItemValue &prev)
{
   switch(dataType)
   {
      case DCI_DT_INT:
         *result = curr.getInt32() - prev.getInt32();
         break;
      case DCI_DT_UINT:
      case DCI_DT_COUNTER32:
         *result = diff_uint32(curr.getUInt32(), prev.getUInt32());
         break;
      case DCI_DT_INT64:
         *result = curr.getInt64() - prev.getInt64();
         break;
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER64:
         *result = diff_uint64(curr.getUInt64(), prev.getUInt64());
         break;
      case DCI_DT_FLOAT:
         *result = curr.getDouble() - prev.getDouble();
         break;
      case DCI_DT_STRING:
         *result = static_cast<int32_t>((_tcscmp(curr.getString(), prev.getString()) == 0) ? 0 : 1);
         break;
      default:
         // Delta calculation is not supported for other types
         *result = curr;
         break;
   }
}

/**
 * Calculate average value for values of given type
 */
template<typename T> static T CalculateAverage(const ItemValue * const *valueList, size_t sampleCount)
{
   T sum = 0;
   int count = 0;
   for(size_t i = 0; i < sampleCount; i++)
   {
      if (valueList[i]->getTimeStamp().asMilliseconds() != 1)
      {
         sum += static_cast<T>(*valueList[i]);
         count++;
      }
   }
   return (count > 0) ? sum / static_cast<T>(count) : 0;
}

/**
 * Calculate average value for set of values
 */
void CalculateItemValueAverage(ItemValue *result, int dataType, const ItemValue * const *valueList, size_t sampleCount)
{
   switch(dataType)
   {
      case DCI_DT_INT:
         *result = CalculateAverage<int32_t>(valueList, sampleCount);
         break;
      case DCI_DT_UINT:
      case DCI_DT_COUNTER32:
         *result = CalculateAverage<uint32_t>(valueList, sampleCount);
         break;
      case DCI_DT_INT64:
         *result = CalculateAverage<int64_t>(valueList, sampleCount);
         break;
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER64:
         *result = CalculateAverage<uint64_t>(valueList, sampleCount);
         break;
      case DCI_DT_FLOAT:
         *result = CalculateAverage<double>(valueList, sampleCount);
         break;
      case DCI_DT_STRING:
         *result = _T("");   // Average value for string is meaningless
         break;
      default:
         break;
   }
}

/**
 * Calculate total value for values of given type
 */
template<typename T> static T CalculateSum(const ItemValue * const *valueList, size_t sampleCount)
{
   T sum = 0;
   for(size_t i = 0; i < sampleCount; i++)
   {
      if (valueList[i]->getTimeStamp().asMilliseconds() != 1)
         sum += static_cast<T>(*valueList[i]);
   }
   return sum;
}

/**
 * Calculate total value for set of values
 */
void CalculateItemValueTotal(ItemValue *result, int dataType, const ItemValue * const *valueList, size_t sampleCount)
{
   switch(dataType)
   {
      case DCI_DT_INT:
         *result = CalculateSum<int32_t>(valueList, sampleCount);
         break;
      case DCI_DT_UINT:
      case DCI_DT_COUNTER32:
         *result = CalculateSum<uint32_t>(valueList, sampleCount);
         break;
      case DCI_DT_INT64:
         *result = CalculateSum<int64_t>(valueList, sampleCount);
         break;
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER64:
         *result = CalculateSum<uint64_t>(valueList, sampleCount);
         break;
      case DCI_DT_FLOAT:
         *result = CalculateSum<double>(valueList, sampleCount);
         break;
      case DCI_DT_STRING:
         *result = _T("");
         break;
      default:
         break;
   }
}

/**
 * Calculate mean absolute deviation for values of given type
 */
template<typename T, T (*ABS)(T)> static T CalculateMeanDeviation(const ItemValue * const *valueList, size_t sampleCount)
{
   T mean = 0;
   int count = 0;
   for(size_t i = 0; i < sampleCount; i++)
   {
      if (valueList[i]->getTimeStamp().asMilliseconds() != 1)
      {
         mean += static_cast<T>(*valueList[i]);
         count++;
      }
   }
   mean /= static_cast<T>(count);
   T dev = 0;
   for(size_t i = 0; i < sampleCount; i++)
   {
      if (valueList[i]->getTimeStamp().asMilliseconds() != 1)
         dev += ABS(static_cast<T>(*valueList[i]) - mean);
   }
   return dev / static_cast<T>(count);
}

/**
 * Get absolute value for 32 bit integer
 */
static int32_t abs32(int32_t v)
{
   return v < 0 ? -v : v;
}

/**
 * Get absolute value for 64 bit integer
 */
static int64_t abs64(int64_t v)
{
   return v < 0 ? -v : v;
}

/**
 * Do nothing with unsigned 32 bit value
 */
static uint32_t noop32(uint32_t v)
{
   return v;
}

/**
 * Do nothing with unsigned 64 bit value
 */
static uint64_t noop64(uint64_t v)
{
   return v;
}

/**
 * Calculate mean absolute deviation for set of values
 */
void CalculateItemValueMeanDeviation(ItemValue *result, int dataType, const ItemValue * const *valueList, size_t sampleCount)
{
   switch(dataType)
   {
      case DCI_DT_INT:
         *result = CalculateMeanDeviation<int32_t, abs32>(valueList, sampleCount);
         break;
      case DCI_DT_UINT:
      case DCI_DT_COUNTER32:
         *result = CalculateMeanDeviation<uint32_t, noop32>(valueList, sampleCount);
         break;
      case DCI_DT_INT64:
         *result = CalculateMeanDeviation<int64_t, abs64>(valueList, sampleCount);
         break;
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER64:
         *result = CalculateMeanDeviation<uint64_t, noop64>(valueList, sampleCount);
         break;
      case DCI_DT_FLOAT:
         *result = CalculateMeanDeviation<double, fabs>(valueList, sampleCount);
         break;
      case DCI_DT_STRING:
         *result = _T("");   // Mean deviation for string is meaningless
         break;
      default:
         break;
   }
}

/**
 * Calculate min value for values of given type
 */
template<typename T> static T CalculateMin(const ItemValue * const *valueList, size_t sampleCount)
{
   bool first = true;
   T value = 0;
   for(size_t i = 0; i < sampleCount; i++)
   {
      if (valueList[i]->getTimeStamp().asMilliseconds() != 1)
      {
         T curr = static_cast<T>(*valueList[i]);
         if (first || (curr < value))
         {
            value = curr;
            first = false;
         }
      }
   }
   return value;
}

/**
 * Calculate min value for set of values
 */
void CalculateItemValueMin(ItemValue *result, int dataType, const ItemValue * const *valueList, size_t sampleCount)
{
   switch(dataType)
   {
      case DCI_DT_INT:
         *result = CalculateMin<int32_t>(valueList, sampleCount);
         break;
      case DCI_DT_UINT:
      case DCI_DT_COUNTER32:
         *result = CalculateMin<uint32_t>(valueList, sampleCount);
         break;
      case DCI_DT_INT64:
         *result = CalculateMin<int64_t>(valueList, sampleCount);
         break;
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER64:
         *result = CalculateMin<uint64_t>(valueList, sampleCount);
         break;
      case DCI_DT_FLOAT:
         *result = CalculateMin<double>(valueList, sampleCount);
         break;
      case DCI_DT_STRING:
         *result = _T("");   // Min value for string is meaningless
         break;
      default:
         break;
   }
}

/**
 * Calculate max value for values of given type
 */
template<typename T> static T CalculateMax(const ItemValue * const *valueList, size_t sampleCount)
{
   bool first = true;
   T value = 0;
   for(size_t i = 0; i < sampleCount; i++)
   {
      if (valueList[i]->getTimeStamp().asMilliseconds() != 1)
      {
         T curr = static_cast<T>(*valueList[i]);
         if (first || (curr > value))
         {
            value = curr;
            first = false;
         }
      }
   }
   return value;
}

/**
 * Calculate max value for set of values
 */
void CalculateItemValueMax(ItemValue *result, int nDataType, const ItemValue * const *valueList, size_t sampleCount)
{
   switch(nDataType)
   {
      case DCI_DT_INT:
         *result = CalculateMax<int32_t>(valueList, sampleCount);
         break;
      case DCI_DT_UINT:
      case DCI_DT_COUNTER32:
         *result = CalculateMax<uint32_t>(valueList, sampleCount);
         break;
      case DCI_DT_INT64:
         *result = CalculateMax<int64_t>(valueList, sampleCount);
         break;
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER64:
         *result = CalculateMax<uint64_t>(valueList, sampleCount);
         break;
      case DCI_DT_FLOAT:
         *result = CalculateMax<double>(valueList, sampleCount);
         break;
      case DCI_DT_STRING:
         *result = _T("");   // Max value for string is meaningless
         break;
      default:
         break;
   }
}
