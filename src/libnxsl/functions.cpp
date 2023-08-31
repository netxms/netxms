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
** File: functions.cpp
**
**/

#include "libnxsl.h"
#include <math.h>

/**
 * String methods
 */
int SM_left(NXSL_Value* thisString, int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM *vm);
int SM_right(NXSL_Value* thisString, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int SM_split(NXSL_Value* thisString, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

/**
 * NXSL type names
 */
const TCHAR *g_szTypeNames[] =
{
   _T("null"), _T("object"), _T("array"), _T("iterator"), _T("hashmap"), _T("string"),
   _T("boolean"), _T("real"), _T("int32"), _T("int64"), _T("uint32"), _T("uint64")
};

/**
 * NXSL function: Type of value
 */
int F_typeof(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   *ppResult = vm->createValue(g_szTypeNames[argv[0]->getDataType()]);
   return 0;
}

/**
 * NXSL function: Class of an object
 */
int F_classof(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;
		
   *ppResult = vm->createValue(argv[0]->getValueAsObject()->getClass()->getName());
   return 0;
}

/**
 * NXSL function: assert
 */
int F_assert(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if ((argc > 1) && !argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   if (argv[0]->isFalse())
   {
      if (argc > 1)
         vm->setAssertMessage(argv[1]->getValueAsCString());
      return NXSL_ERR_ASSERTION_FAILED;
   }

   *result = vm->createValue();
   return 0;
}

/**
 * NXSL function: Absolute value
 */
int F_abs(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   int nRet;

   if (argv[0]->isNumeric())
   {
      if (argv[0]->isReal())
      {
         *ppResult = vm->createValue(fabs(argv[0]->getValueAsReal()));
      }
      else
      {
         *ppResult = vm->createValue(argv[0]);
         if (!argv[0]->isUnsigned())
            if ((*ppResult)->getValueAsInt64() < 0)
               (*ppResult)->negate();
      }
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_NUMBER;
   }
   return nRet;
}

/**
 * Calculates x raised to the power of y
 */
int F_pow(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   int nRet;

   if ((argv[0]->isNumeric()) && (argv[1]->isNumeric()))
   {
      *ppResult = vm->createValue(pow(argv[0]->getValueAsReal(), argv[1]->getValueAsReal()));
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_NUMBER;
   }
   return nRet;
}

/**
 * Calculates square root
 */
int F_sqrt(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(sqrt(argv[0]->getValueAsReal()));
   return 0;
}

/**
 * Calculates natural logarithm
 */
int F_log(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   int nRet;

   if (argv[0]->isNumeric())
   {
      *ppResult = vm->createValue(log(argv[0]->getValueAsReal()));
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_NUMBER;
   }
   return nRet;
}

/**
 * Calculates common logarithm
 */
int F_log10(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   int nRet;

   if (argv[0]->isNumeric())
   {
      *ppResult = vm->createValue(log10(argv[0]->getValueAsReal()));
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_NUMBER;
   }
   return nRet;
}

/**
 * Calculates x raised to the power of e
 */
int F_exp(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(exp(argv[0]->getValueAsReal()));
   return 0;
}

/**
 * Calculates sine x
 */
int F_sin(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(sin(argv[0]->getValueAsReal()));
   return 0;
}

/**
 * Calculates cosine x
 */
int F_cos(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(cos(argv[0]->getValueAsReal()));
   return 0;
}

/**
 * Calculates tangent x
 */
int F_tan(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(tan(argv[0]->getValueAsReal()));
   return 0;
}

/**
 * Calculates hyperbolic sine x
 */
int F_sinh(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(sinh(argv[0]->getValueAsReal()));
   return 0;
}

/**
 * Calculates hyperbolic cosine x
 */
int F_cosh(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(cosh(argv[0]->getValueAsReal()));
   return 0;
}

/**
 * Calculates hyperbolic tangent x
 */
int F_tanh(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(tanh(argv[0]->getValueAsReal()));
   return 0;
}

/**
 * Calculates arc sine x
 */
int F_asin(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(asin(argv[0]->getValueAsReal()));
   return 0;
}

/**
 * Calculates arc cosine x
 */
int F_acos(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(acos(argv[0]->getValueAsReal()));
   return 0;
}

/**
 * Calculates arc tangent x
 */
int F_atan(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(atan(argv[0]->getValueAsReal()));
   return 0;
}

/**
 * Calculates 2-argument arc tangent
 */
int F_atan2(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric() || !argv[1]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(atan2(argv[0]->getValueAsReal(), argv[1]->getValueAsReal()));
   return 0;
}

/**
 * Common string transformation code for upper/lower
 */
template<typename T, T Transform(T)> int TransformString(NXSL_Value *v, NXSL_Value **result, NXSL_VM *vm)
{
   if (!v->isString())
      return NXSL_ERR_NOT_STRING;

   *result = vm->createValue(v);
   uint32_t len;
   TCHAR *s = (TCHAR *)(*result)->getValueAsString(&len);
   for(uint32_t i = 0; i < len; i++, s++)
      *s = Transform(*s);
   return NXSL_ERR_SUCCESS;
}

/**
 * Convert string to uppercase
 */
int F_upper(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
#ifdef UNICODE
   return TransformString<wint_t, towupper>(argv[0], result, vm);
#else
   return TransformString<int, toupper>(argv[0], result, vm);
#endif
}

/**
 * Convert string to lowercase
 */
int F_lower(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
#ifdef UNICODE
   return TransformString<wint_t, towlower>(argv[0], result, vm);
#else
   return TransformString<int, tolower>(argv[0], result, vm);
#endif
}

/**
 * String length
 */
int F_length(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   uint32_t length;
   argv[0]->getValueAsString(&length);
   *result = vm->createValue(length);
   return 0;
}

/**
 * Minimal value from the list of values
 */
int F_min(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   int i;
   NXSL_Value *pCurr;

   if (argc == 0)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   pCurr = argv[0];
   for(i = 1; i < argc; i++)
   {
      if (!argv[i]->isNumeric())
         return NXSL_ERR_NOT_NUMBER;

		if (argv[i]->getValueAsReal() < pCurr->getValueAsReal())
         pCurr = argv[i];
   }
   *ppResult = vm->createValue(pCurr);
   return 0;
}

/**
 * Maximal value from the list of values
 */
int F_max(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   int i;
   NXSL_Value *pCurr;

   if (argc == 0)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   pCurr = argv[0];
   for(i = 0; i < argc; i++)
   {
      if (!argv[i]->isNumeric())
         return NXSL_ERR_NOT_NUMBER;

		if (argv[i]->getValueAsReal() > pCurr->getValueAsReal())
         pCurr = argv[i];
   }
   *ppResult = vm->createValue(pCurr);
   return 0;
}

/**
 * Check if IP address is within given range
 */
int F_AddrInRange(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   int nRet;
   if (argv[0]->isString() && argv[1]->isString() && argv[2]->isString())
   {
      InetAddress addr = InetAddress::parse(argv[0]->getValueAsCString());
      InetAddress start = InetAddress::parse(argv[1]->getValueAsCString());
      InetAddress end = InetAddress::parse(argv[2]->getValueAsCString());
      if ((addr.getFamily() == start.getFamily()) && (addr.getFamily() == end.getFamily()))
      {
         *ppResult = vm->createValue((addr.compareTo(start) >= 0) && (addr.compareTo(end) <= 0));
      }
      else
      {
         *ppResult = vm->createValue(false);
      }
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_STRING;
   }
   return nRet;
}

/**
 * Check if IP address is within given subnet
 */
int F_AddrInSubnet(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   int nRet;
   if (argv[0]->isString() && argv[1]->isString() && argv[2]->isString())
   {
      InetAddress addr = InetAddress::parse(argv[0]->getValueAsCString());
      InetAddress subnet = InetAddress::parse(argv[1]->getValueAsCString());
      if (addr.getFamily() == subnet.getFamily())
      {
         if (argv[2]->isInteger())
         {
            subnet.setMaskBits(argv[2]->getValueAsUInt32());
         }
         else
         {
            InetAddress mask = InetAddress::parse(argv[2]->getValueAsCString());
            subnet.setMaskBits((mask.getFamily() == AF_INET) ? BitsInMask(mask.getAddressV4()) : BitsInMask(mask.getAddressV6(), 16));
         }
         *ppResult = vm->createValue(subnet.contain(addr));
      }
      else
      {
         *ppResult = vm->createValue(false);
      }
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_STRING;
   }
   return nRet;
}

/**
 * Convert seconds since uptime to string
 * PATCH: by Edgar Chupit
 */
int F_SecondsToUptime(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   uint64_t arg = argv[0]->getValueAsUInt64();
   *ppResult = vm->createValue(SecondsToUptime(arg, false));
   return 0;
}

/**
 * Get substring of a string
 * Possible usage:
 *    substr(string, start) - all characters from position 'start'
 *    substr(string, start, n) - n characters from position 'start'
 *    substr(string, NULL, n) - first n characters
 */
int F_substr(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 2) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   int start;
	if (argv[1]->isNull())
	{
		start = 0;
	}
	else if (argv[1]->isInteger())
	{
		start = argv[1]->getValueAsInt32();
		if (start > 0)
			start--;
		else
			start = 0;
	}
	else
	{
		return NXSL_ERR_NOT_INTEGER;
	}

	int count;
	if (argc == 3)
	{
		if (!argv[2]->isInteger())
			return NXSL_ERR_NOT_INTEGER;
		count = argv[2]->getValueAsInt32();
		if (count < 0)
			count = 0;
	}
	else
	{
		count = -1;
	}

   uint32_t len;
   const TCHAR *base = argv[0]->getValueAsString(&len);
	if ((uint32_t)start < len)
	{
		base += start;
		len -= start;
		if ((count == -1) || ((uint32_t)count > len))
		{
			count = len;
		}
		*result = vm->createValue(base, (uint32_t)count);
	}
	else
	{
		*result = vm->createValue(_T(""));
	}

	return NXSL_ERR_SUCCESS;
}

/**
 * Convert hexadecimal string to decimal value
 *   x2d(hex value)      -> value
 */
int F_x2d(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   UINT64 v = _tcstoull(argv[0]->getValueAsCString(), NULL, 16);
   *result = (v <= 0x7FFFFFFF) ? vm->createValue((UINT32)v) : vm->createValue(v);
   return 0;
}

/**
 * Convert decimal value to hexadecimal string
 *   d2x(value)          -> hex value
 *   d2x(value, padding) -> hex value padded with zeros
 */
int F_d2x(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	TCHAR buffer[128], format[32];

   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   if ((argc == 2) && (!argv[1]->isInteger()))
      return NXSL_ERR_NOT_INTEGER;

	if (argc == 1)
	{
		_tcscpy(format, _T("%X"));
	}
	else
	{
		_sntprintf(format, 32, _T("%%0%dX"), argv[1]->getValueAsInt32());
	}
	_sntprintf(buffer, 128, format, argv[0]->getValueAsUInt32());
	*ppResult = vm->createValue(buffer);
	return NXSL_ERR_SUCCESS;
}

/**
 * chr() - character from it's UNICODE value
 */
int F_chr(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   TCHAR buffer[2];
   buffer[0] = (TCHAR)argv[0]->getValueAsInt32();
   buffer[1] = 0;
   *result = vm->createValue(buffer);
   return NXSL_ERR_SUCCESS;
}

/**
 * ord() -  convert characters into their ASCII or Unicode values
 */
int F_ord(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   *result = vm->createValue((INT32)(argv[0]->getValueAsCString()[0]));
   return NXSL_ERR_SUCCESS;
}

/**
 * left() - take leftmost part of a string and pad or truncate it as necessary
 * Format: left(string, len, [pad])
 */
int F_left(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 2) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   return SM_left(argv[0], argc - 1, &argv[1], result, vm);
}

/**
 * right() - take rightmost part of a string and pad or truncate it as necessary
 * Format: right(string, len, [pad])
 */
int F_right(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 2) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   return SM_right(argv[0], argc - 1, &argv[1], result, vm);
}

/**
 * Exit from script
 */
int F_exit(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (argc > 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   *result = (argc == 0) ? vm->createValue((LONG)0) : vm->createValue(argv[0]);
   return NXSL_STOP_SCRIPT_EXECUTION;
}

/**
 * Trim whitespace characters from the string
 */
int F_trim(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   uint32_t len;
   const TCHAR *string = argv[0]->getValueAsString(&len);
   if (len > 0)
   {
      int i;
      for(i = 0; (i < (int)len) && (string[i] == _T(' ') || string[i] == _T('\t')); i++);

      int startPos = i;
      if (len > 0)
         for(i = (int)len - 1; (i >= startPos) && (string[i] == _T(' ') || string[i] == _T('\t')); i--);

      *result = vm->createValue(&string[startPos], i - startPos + 1);
   }
   else
   {
      *result = vm->createValue(argv[0]);
   }
   return NXSL_ERR_SUCCESS;
}

/**
 * Trim trailing whitespace characters from the string
 */
int F_rtrim(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

	uint32_t len;
	const TCHAR *string = argv[0]->getValueAsString(&len);
	
	int i;
	for(i = (int)len - 1; (i >= 0) && (string[i] == _T(' ') || string[i] == _T('\t')); i--);

	*result = vm->createValue(string, i + 1);
	return NXSL_ERR_SUCCESS;
}

/**
 * Trim leading whitespace characters from the string
 */
int F_ltrim(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

	uint32_t len;
	const TCHAR *string = argv[0]->getValueAsString(&len);
	
	int i;
	for(i = 0; (i < (int)len) && (string[i] == _T(' ') || string[i] == _T('\t')); i++);

	*result = vm->createValue(&string[i], (int)len - i);
	return NXSL_ERR_SUCCESS;
}

/**
 * Print
 */
int F_print(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   for(int i = 0; i < argc; i++)
   {
      if (i > 0)
         vm->print(_T(" "));
      vm->print(argv[i]->getValueAsCString());
   }
   *result = vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * Print with new line
 */
int F_println(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   F_print(argc, argv, result, vm);
   vm->print(_T("\n"));
   return NXSL_ERR_SUCCESS;
}

/**
 * Trace
 */
int F_trace(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	vm->trace(argv[0]->getValueAsInt32(), argv[1]->getValueAsCString());
	*result = vm->createValue();
	return NXSL_ERR_SUCCESS;
}

/**
 * Common implementation for index and rindex functions
 */
static int F_index_rindex(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm, bool reverse)
{
	if ((argc != 2) && (argc != 3))
		return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	if (!argv[0]->isString() || !argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	uint32_t strLength, substrLength;
	const TCHAR *str = argv[0]->getValueAsString(&strLength);
	const TCHAR *substr = argv[1]->getValueAsString(&substrLength);

	int start;
	if (argc == 3)
	{
		if (!argv[2]->isInteger())
			return NXSL_ERR_NOT_INTEGER;

		start = argv[2]->getValueAsInt32();
		if (start > 0)
		{
			start--;
			if (reverse && (start > (int)strLength - (int)substrLength))
				start = (int)strLength - (int)substrLength;
		}
		else
		{
			start = reverse ? (int)strLength - (int)substrLength : 0;
		}
	}
	else
	{
		start = reverse ? (int)strLength - (int)substrLength : 0;
	}

	int32_t index = 0;	// 0 = not found
	if ((substrLength <= strLength) && (substrLength > 0))
	{
		if (reverse)
		{
			for(int i = start; i >= 0; i--)
			{
				if (!memcmp(&str[i], substr, substrLength * sizeof(TCHAR)))
				{
					index = i + 1;
					break;
				}
			}
		}
		else
		{
			for(int i = start; i <= (int)(strLength - substrLength); i++)
			{
				if (!memcmp(&str[i], substr, substrLength * sizeof(TCHAR)))
				{
					index = i + 1;
					break;
				}
			}
		}
	}
	else if ((substrLength == strLength) && (substrLength > 0))
	{
		index = !memcmp(str, substr, substrLength * sizeof(TCHAR)) ? 1 : 0;
	}

	*result = vm->createValue(index);
	return 0;
}

/**
 * index(string, substring, [position])
 * Returns the position of the first occurrence of SUBSTRING in STRING at or after POSITION.
 * If you don't specify POSITION, the search starts at the beginning of STRING. If SUBSTRING
 * is not found, returns 0.
 */
int F_index(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	return F_index_rindex(argc, argv, result, vm, false);
}


/**
 * rindex(string, substring, [position])
 * Returns the position of the last occurrence of SUBSTRING in STRING at or before POSITION.
 * If you don't specify POSITION, the search starts at the end of STRING. If SUBSTRING
 * is not found, returns 0.
 */
int F_rindex(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	return F_index_rindex(argc, argv, result, vm, true);
}

/**
 * replace(string, target, replacement)
 * Replaces each substring of this string that matches the literal target sequence with the specified literal replacement
 * sequence. The replacement proceeds from the beginning of the string to the end, for example, replacing "aa" with "b"
 * in the string "aaa" will result in "ba" rather than "ab".
 */
int F_replace(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString() || !argv[1]->isString() || !argv[2]->isString())
      return NXSL_ERR_NOT_STRING;

   StringBuffer sb(argv[0]->getValueAsCString());
   sb.replace(argv[1]->getValueAsCString(), argv[2]->getValueAsCString());
   *result = vm->createValue(sb);
   return 0;
}

/**
 * NXSL function: Generate random number in given range
 */
int F_random(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isInteger() || !argv[1]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	int range = argv[1]->getValueAsInt32() - argv[0]->getValueAsInt32() + 1;
	*result = vm->createValue((rand() % range) + argv[0]->getValueAsInt32());
	return 0;
}

/**
 * Suspend execution for given number of milliseconds
 */
int F_sleep(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	ThreadSleepMs(argv[0]->getValueAsUInt32());
	*ppResult = vm->createValue();
	return 0;
}

/**
 * NXSL function: floor
 */
int F_floor(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isNumeric())
		return NXSL_ERR_NOT_NUMBER;

	*ppResult = vm->createValue(floor(argv[0]->getValueAsReal()));
	return 0;
}

/**
 * NXSL function: ceil
 */
int F_ceil(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isNumeric())
		return NXSL_ERR_NOT_NUMBER;

	*ppResult = vm->createValue(ceil(argv[0]->getValueAsReal()));
	return 0;
}

/**
 * NXSL function: round
 */
int F_round(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if ((argc != 1) && (argc != 2))
		return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	if (!argv[0]->isNumeric())
		return NXSL_ERR_NOT_NUMBER;

	double d = argv[0]->getValueAsReal();
	if (argc == 1)
	{
		// round to whole number
		*ppResult = vm->createValue((d > 0.0) ? floor(d + 0.5) : ceil(d - 0.5));
	}
	else
	{
		// round with given number of decimal places
		if (!argv[1]->isInteger())
			return NXSL_ERR_NOT_INTEGER;

		int p = argv[1]->getValueAsInt32();
		if (p < 0)
			p = 0;

		d *= pow(10.0, p);
		d = (d > 0.0) ? floor(d + 0.5) : ceil(d - 0.5);
		d *= pow(10.0, -p);
		*ppResult = vm->createValue(d);
	}
	return 0;
}

/**
 * NXSL function: format
 */
int F_format(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if ((argc < 1) || (argc > 3))
		return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	if (!argv[0]->isNumeric())
		return NXSL_ERR_NOT_NUMBER;

	int width = 0;
	int precision = 0;
	if (argc >= 2)
	{
		if (!argv[1]->isInteger())
			return NXSL_ERR_NOT_INTEGER;
		width = argv[1]->getValueAsInt32();
		if (argc == 3)
		{
			if (!argv[2]->isInteger())
				return NXSL_ERR_NOT_INTEGER;
			precision = argv[2]->getValueAsInt32();
		}
	}

	if (width < -127)
	   width = -127;
	else if (width > 127)
	   width = 127;

	if (precision < 0)
	   precision = 0;
	else if (precision > 120)
	   precision = 120;

	TCHAR format[32], buffer[128];
	_sntprintf(format, 32, _T("%%%d.%df"), width, precision);
	_sntprintf(buffer, 128, format, argv[0]->getValueAsReal());
	*ppResult = vm->createValue(buffer);
	return 0;
}

/**
 * NXSL function: inList
 * 
 * inList(string, separator, value)
 *
 * Check if given value is one of the values in given list using separator.
 *
 * Examples:
 *   inList("1,2,3", ",", "1") -> true
 *   inList("ab|cd|ef", "|", "test") -> false
 */
int F_inList(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isString() || !argv[1]->isString() || !argv[2]->isString())
		return NXSL_ERR_NOT_STRING;

   bool result = false;
   if ((argv[0]->getValueAsCString()[0] != 0) && (argv[1]->getValueAsCString()[0] != 0) && (argv[2]->getValueAsCString()[0] != 0))
   {
      const TCHAR *value = argv[2]->getValueAsCString();
      int count;
      TCHAR **strings = SplitString(argv[0]->getValueAsCString(), argv[1]->getValueAsCString()[0], &count);
      for(int i = 0; i < count; i++)
      {
         if (!_tcscmp(strings[i], value))
            result = true;
         free(strings[i]);
      }
      free(strings);
   }
   *ppResult = vm->createValue(result);
	return 0;
}

/**
 * md5() function implementation
 */
int F_md5(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

   BYTE hash[MD5_DIGEST_SIZE];
#ifdef UNICODE
   char *utf8Str = UTF8StringFromWideString(argv[0]->getValueAsCString());
   CalculateMD5Hash((BYTE *)utf8Str, strlen(utf8Str), hash);
   free(utf8Str);
#else
   const char *str = argv[0]->getValueAsCString();
   CalculateMD5Hash((BYTE *)str, strlen(str), hash);
#endif

   TCHAR text[MD5_DIGEST_SIZE * 2 + 1];
   BinToStr(hash, MD5_DIGEST_SIZE, text);
   *ppResult = vm->createValue(text);

	return 0;
}

/**
 * sha1() function implementation
 */
int F_sha1(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

   BYTE hash[SHA1_DIGEST_SIZE];
#ifdef UNICODE
   char *utf8Str = UTF8StringFromWideString(argv[0]->getValueAsCString());
   CalculateSHA1Hash((BYTE *)utf8Str, strlen(utf8Str), hash);
   free(utf8Str);
#else
   const char *str = argv[0]->getValueAsCString();
   CalculateSHA1Hash((BYTE *)str, strlen(str), hash);
#endif

   TCHAR text[SHA1_DIGEST_SIZE * 2 + 1];
   BinToStr(hash, SHA1_DIGEST_SIZE, text);
   *ppResult = vm->createValue(text);

	return 0;
}

/**
 * sha256() function implementation
 */
int F_sha256(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

   BYTE hash[SHA256_DIGEST_SIZE];
#ifdef UNICODE
   char *utf8Str = UTF8StringFromWideString(argv[0]->getValueAsCString());
   CalculateSHA256Hash((BYTE *)utf8Str, strlen(utf8Str), hash);
   free(utf8Str);
#else
   const char *str = argv[0]->getValueAsCString();
   CalculateSHA256Hash((BYTE *)str, strlen(str), hash);
#endif

   TCHAR text[SHA256_DIGEST_SIZE * 2 + 1];
   BinToStr(hash, SHA256_DIGEST_SIZE, text);
   *ppResult = vm->createValue(text);

	return 0;
}

/**
 * Resolve IP address to host name
 */
int F_gethostbyaddr(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

   InetAddress addr = InetAddress::parse(argv[0]->getValueAsCString());
   if (addr.isValid())
   {
      TCHAR buffer[256];
      if (addr.getHostByAddr(buffer, 256) != NULL)
      {
         *ppResult = vm->createValue(buffer);
      }
      else
      {
         *ppResult = vm->createValue();
      }
   }
   else
   {
      *ppResult = vm->createValue();
   }

   return 0;
}

/**
 * Resolve hostname to IP address
 */
int F_gethostbyname(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

   int af = AF_INET;
   if (argc > 1)
   {
      if (!argv[1]->isInteger())
   		return NXSL_ERR_NOT_INTEGER;
      
      af = (argv[1]->getValueAsInt32() == 6) ? AF_INET6 : AF_INET;
   }

   InetAddress addr = InetAddress::resolveHostName(argv[0]->getValueAsCString(), af);
   if (addr.isValid())
   {
      *ppResult = vm->createValue((const TCHAR *)addr.toString());
   }
   else
   {
      *ppResult = vm->createValue();
   }
   return 0;
}

/**
 * Convert array to string
 */
int F_ArrayToString(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isArray())
      return NXSL_ERR_NOT_ARRAY;

   if (!argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   StringBuffer s;
   argv[0]->getValueAsArray()->toString(&s, argv[1]->getValueAsCString(), false);
   *result = vm->createValue(s);
   return NXSL_ERR_SUCCESS;
}

/**
 * Split string into array of strings at given delimiter
 */
int F_SplitString(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString() || !argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   return SM_split(argv[0], 1, &argv[1], result, vm);
}

/**
 * Read persistent storage
 */
int F_ReadPersistentStorage(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   *result = vm->storageRead(argv[0]->getValueAsCString());
   return NXSL_ERR_SUCCESS;
}

/**
 * Write to persistent storage
 */
int F_WritePersistentStorage(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   NXSL_Value *value = vm->createValue(argv[1]);
   vm->storageWrite(argv[0]->getValueAsCString(), value);
   vm->destroyValue(value);

   *result = vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * Decode base64 encoded string. Accepts string to encode and optional character encoding.
 * Valid character encodings are "UTF-8", "UCS-2", "UCS-4", "SYSTEM". Default is UTF-8.
 */
int F_Base64Decode(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString() || ((argc > 1) && !argv[1]->isString()))
      return NXSL_ERR_NOT_STRING;

#ifdef UNICODE
   char *in = MBStringFromWideString(argv[0]->getValueAsCString());
#else
   const char *in = argv[0]->getValueAsCString();
#endif
   size_t ilen = strlen(in);

   size_t olen = 3 * (ilen / 4) + 8;
   char *out = MemAllocArray<char>(olen);
   bool success = base64_decode(in, ilen, out, &olen);
#ifdef UNICODE
   MemFree(in);
#endif

   if (success)
   {
      const TCHAR *encoding = (argc > 1) ? argv[1]->getValueAsCString() : _T("UTF-8");
      if (!_tcsicmp(encoding, _T("UCS-4")))
      {
#ifdef UNICODE
#ifdef UNICODE_UCS2
         WCHAR *s = UCS2StringFromUCS4String((UCS4CHAR *)out);
         *result = vm->createValue(s);
         MemFree(s);
#else
         *result = vm->createValue((WCHAR *)out);
#endif
#else
         char *s = MBStringFromUCS4String((UCS4CHAR *)out);
         *result = vm->createValue(s);
         MemFree(s);
#endif
      }
      else if (!_tcsicmp(encoding, _T("UCS-2")))
      {
#ifdef UNICODE
#ifdef UNICODE_UCS2
         *result = vm->createValue((WCHAR *)out);
#else
         WCHAR *s = UCS4StringFromUCS2String((UCS2CHAR *)out);
         *result = vm->createValue(s);
         MemFree(s);
#endif
#else
         char *s = MBStringFromUCS2String((UCS2CHAR *)out);
         *result = vm->createValue(s);
         MemFree(s);
#endif
      }
      else if (!_tcsicmp(encoding, _T("SYSTEM")))
      {
         *result = vm->createValue(out);
      }
      else
      {
#ifdef UNICODE
         WCHAR *s = WideStringFromUTF8String(out);
#else
         char *s = MBStringFromUTF8String(out);
#endif
         *result = vm->createValue(s);
         MemFree(s);
      }
   }
   else
   {
      *result = vm->createValue();
   }
   MemFree(out);
   return 0;
}

/**
 * Encode string as base64. Accepts string to encode and optional character encoding. 
 * Valid character encodings are "UTF-8", "UCS-2", "UCS-4", "SYSTEM". Default is UTF-8.
 */
int F_Base64Encode(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString() || ((argc > 1) && !argv[1]->isString()))
      return NXSL_ERR_NOT_STRING;

   char *in;
   size_t ilen;
   const TCHAR *encoding = (argc > 1) ? argv[1]->getValueAsCString() : _T("UTF-8");
   if (!_tcsicmp(encoding, _T("UCS-4")))
   {
#ifdef UNICODE
#ifdef UNICODE_UCS2
      in = (char *)UCS4StringFromUCS2String(argv[0]->getValueAsCString());
#else
      in = (char *)argv[0]->getValueAsCString();
#endif
#else
      in = (char *)UCS4StringFromMBString(argv[0]->getValueAsCString());
#endif
      ilen = ucs4_strlen((UCS4CHAR *)in) * sizeof(UCS4CHAR);
   }
   else if (!_tcsicmp(encoding, _T("UCS-2")))
   {
#ifdef UNICODE
#ifdef UNICODE_UCS2
      in = (char *)argv[0]->getValueAsCString();
#else
      in = (char *)UCS2StringFromUCS4String(argv[0]->getValueAsCString());
#endif
#else
      in = (char *)UCS2StringFromMBString(argv[0]->getValueAsCString());
#endif
      ilen = ucs2_strlen((UCS2CHAR *)in) * sizeof(UCS2CHAR);
   }
   else if (!_tcsicmp(encoding, _T("SYSTEM")))
   {
#ifdef UNICODE
      in = MBStringFromWideString(argv[0]->getValueAsCString());
#else
      in = (char *)argv[0]->getValueAsCString();
#endif
      ilen = strlen(in);
   }
   else  // UTF-8 by default
   {
      in = UTF8StringFromTString(argv[0]->getValueAsCString());
      ilen = strlen(in);
   }

   char *out = nullptr;
   base64_encode_alloc(in, ilen, &out);
   *result = vm->createValue(CHECK_NULL_EX_A(out));

   if (in != (char *)argv[0]->getValueAsCString())
      MemFree(in);
   MemFree(out);

   return 0;
}

/**
 * Calculate Weierstrass function for given x, a, and b
 */
int F_weierstrass(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric() || !argv[1]->isNumeric() || !argv[2]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   double a = argv[0]->getValueAsReal();
   double b = argv[1]->getValueAsReal();
   double x = argv[2]->getValueAsReal();

   if (b < 7)
      b = 7;

   // Because 0 < a < 1, that value becomes smaller as n grows larger. For example,
   // if a = 0.9, pow(a, 100) is around 0.000027, so that term adds little to the total.
   // Because terms with larger values of n don't contribute much to the total, only
   // first 100 terms are used.
   double y = 0;
   for(int n = 0; n < 100; n++)
   {
      double c = cos(pow(b, n) * 3.1415926535 * x);
      if ((c > 1) || (c < -1))
         c = 0;
      y += pow(a, n) * c;
   }

   *result = vm->createValue(y);
   return 0;
}

/**
 * Get names of all thread pools
 */
int F_GetThreadPoolNames(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   StringList *pools = ThreadPoolGetAllPools();
   *result = vm->createValue(new NXSL_Array(vm, pools));
   delete pools;
   return 0;
}

/**
 * Convert numeric value to human-readable form with metric prefix
 */
int F_FormatMetricPrefix(int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM* vm)
{
   if ((argc < 1) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   if ((argc > 2) && !argv[2]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   double inVal = argv[0]->getValueAsReal();
   bool useBinaryPrefixes = (argc > 1) ? argv[1]->isTrue() : false;
   int precision = (argc > 2) ? argv[2]->getValueAsInt32() : 2;

   if (precision > 20)
      precision = 20;
   else if (precision < -20)
      precision = -20;

   *result = vm->createValue(FormatNumber(inVal, useBinaryPrefixes, 0, precision));
   return NXSL_ERR_SUCCESS;
}
