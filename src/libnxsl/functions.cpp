/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
 * NXSL type names
 */
const TCHAR *g_szTypeNames[] = { _T("null"), _T("object"), _T("array"), _T("iterator"), _T("hashmap"),
                                 _T("string"), _T("real"), _T("int32"), _T("int64"), _T("uint32"), _T("uint64") };

/**
 * NXSL function: Type of value
 */
int F_typeof(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   *ppResult = new NXSL_Value(g_szTypeNames[argv[0]->getDataType()]);
   return 0;
}

/**
 * NXSL function: Class of an object
 */
int F_classof(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;
		
   *ppResult = new NXSL_Value(argv[0]->getValueAsObject()->getClass()->getName());
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
         *ppResult = new NXSL_Value(fabs(argv[0]->getValueAsReal()));
      }
      else
      {
         *ppResult = new NXSL_Value(argv[0]);
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
      *ppResult = new NXSL_Value(pow(argv[0]->getValueAsReal(), argv[1]->getValueAsReal()));
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_NUMBER;
   }
   return nRet;
}

/**
 * Calculates natural logarithm
 */
int F_log(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   int nRet;

   if (argv[0]->isNumeric())
   {
      *ppResult = new NXSL_Value(log(argv[0]->getValueAsReal()));
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
      *ppResult = new NXSL_Value(log10(argv[0]->getValueAsReal()));
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
int F_exp(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   int nRet;

   if (argv[0]->isNumeric())
   {
      *ppResult = new NXSL_Value(exp(argv[0]->getValueAsReal()));
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_NUMBER;
   }
   return nRet;
}

/**
 * Convert string to uppercase
 */
int F_upper(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   int nRet;
   UINT32 i, dwLen;
   TCHAR *pStr;

   if (argv[0]->isString())
   {
      *ppResult = new NXSL_Value(argv[0]);
      pStr = (TCHAR *)(*ppResult)->getValueAsString(&dwLen);
      for(i = 0; i < dwLen; i++, pStr++)
         *pStr = toupper(*pStr);
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_STRING;
   }
   return nRet;
}

/**
 * Convert string to lowercase
 */
int F_lower(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   int nRet;
   UINT32 i, dwLen;
   TCHAR *pStr;

   if (argv[0]->isString())
   {
      *ppResult = new NXSL_Value(argv[0]);
      pStr = (TCHAR *)(*ppResult)->getValueAsString(&dwLen);
      for(i = 0; i < dwLen; i++, pStr++)
         *pStr = tolower(*pStr);
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_STRING;
   }
   return nRet;
}

/**
 * String length
 */
int F_length(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   int nRet;
   UINT32 dwLen;

   if (argv[0]->isString())
   {
      argv[0]->getValueAsString(&dwLen);
      *ppResult = new NXSL_Value(dwLen);
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_STRING;
   }
   return nRet;
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
   *ppResult = new NXSL_Value(pCurr);
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
   *ppResult = new NXSL_Value(pCurr);
   return 0;
}

/**
 * Check if IP address is within given range
 */
int F_AddrInRange(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   int nRet;
   UINT32 dwAddr, dwStart, dwEnd;

   if (argv[0]->isString() && argv[1]->isString() && argv[2]->isString())
   {
      dwAddr = ntohl(_t_inet_addr(argv[0]->getValueAsCString()));
      dwStart = ntohl(_t_inet_addr(argv[1]->getValueAsCString()));
      dwEnd = ntohl(_t_inet_addr(argv[2]->getValueAsCString()));
      *ppResult = new NXSL_Value((LONG)(((dwAddr >= dwStart) && (dwAddr <= dwEnd)) ? 1 : 0));
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
   UINT32 dwAddr, dwSubnet, dwMask;

   if (argv[0]->isString() && argv[1]->isString() && argv[2]->isString())
   {
      dwAddr = ntohl(_t_inet_addr(argv[0]->getValueAsCString()));
      dwSubnet = ntohl(_t_inet_addr(argv[1]->getValueAsCString()));
      dwMask = ntohl(_t_inet_addr(argv[2]->getValueAsCString()));
      *ppResult = new NXSL_Value((LONG)(((dwAddr & dwMask) == dwSubnet) ? 1 : 0));
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_STRING;
   }
   return nRet;
}

/**
 * Convert time_t into string
 * PATCH: by Edgar Chupit
 */
int F_strftime(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{   
   TCHAR buffer[512];
   time_t tTime;
	struct tm *ptm;
	BOOL bLocalTime;

   if ((argc == 0) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;
	if (argc > 1)
	{
		if (!argv[1]->isNumeric() && !argv[1]->isNull())
			return NXSL_ERR_NOT_NUMBER;
		tTime = (argv[1]->isNull()) ? time(NULL) : (time_t)argv[1]->getValueAsUInt64();

		if (argc > 2)
		{
			if (!argv[2]->isInteger())
				return NXSL_ERR_BAD_CONDITION;
			bLocalTime = argv[2]->getValueAsInt32() ? TRUE : FALSE;
		}
		else
		{
			// No third argument, assume localtime
			bLocalTime = TRUE;
		}
	}
	else
	{
		// No second argument
		tTime = time(NULL);
		bLocalTime = TRUE;
	}

   ptm = bLocalTime ? localtime(&tTime) : gmtime(&tTime);
   _tcsftime(buffer, 512, argv[0]->getValueAsCString(), ptm);
   *ppResult = new NXSL_Value(buffer);   
   
   return 0;
}


//
// Convert seconds since uptime to string
// PATCH: by Edgar Chupit
//

int F_SecondsToUptime(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   UINT32 d, h, n;

   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   QWORD arg = argv[0]->getValueAsUInt64();

   d = (UINT32)(arg / 86400);
   arg -= d * 86400;
   h = (UINT32)(arg / 3600);
   arg -= h * 3600;
   n = (UINT32)(arg / 60);
   arg -= n * 60;
   if (arg > 0)
       n++;

   TCHAR result[128];
   _sntprintf(result, 128, _T("%u days, %2u:%02u"), d, h, n);

   *ppResult = new NXSL_Value(result);
   return 0;
}


//
// Get current time
//

int F_time(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   *ppResult = new NXSL_Value((UINT32)time(NULL));
   return 0;
}


//
// NXSL "TIME" class
//

class NXSL_TimeClass : public NXSL_Class
{
public:
   NXSL_TimeClass();

   virtual NXSL_Value *getAttr(NXSL_Object *pObject, const TCHAR *pszAttr);
	virtual void onObjectDelete(NXSL_Object *object);
};


//
// Implementation of "TIME" class
//

NXSL_TimeClass::NXSL_TimeClass()
               :NXSL_Class()
{
   _tcscpy(m_name, _T("TIME"));
}

NXSL_Value *NXSL_TimeClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   struct tm *st;
   NXSL_Value *value;

   st = (struct tm *)pObject->getData();
   if (!_tcscmp(pszAttr, _T("sec")) || !_tcscmp(pszAttr, _T("tm_sec")))
   {
      value = new NXSL_Value((LONG)st->tm_sec);
   }
   else if (!_tcscmp(pszAttr, _T("min")) || !_tcscmp(pszAttr, _T("tm_min")))
   {
      value = new NXSL_Value((LONG)st->tm_min);
   }
   else if (!_tcscmp(pszAttr, _T("hour")) || !_tcscmp(pszAttr, _T("tm_hour")))
   {
      value = new NXSL_Value((LONG)st->tm_hour);
   }
   else if (!_tcscmp(pszAttr, _T("mday")) || !_tcscmp(pszAttr, _T("tm_mday")))
   {
      value = new NXSL_Value((LONG)st->tm_mday);
   }
   else if (!_tcscmp(pszAttr, _T("mon")) || !_tcscmp(pszAttr, _T("tm_mon")))
   {
      value = new NXSL_Value((LONG)st->tm_mon);
   }
   else if (!_tcscmp(pszAttr, _T("year")) || !_tcscmp(pszAttr, _T("tm_year")))
   {
      value = new NXSL_Value((LONG)(st->tm_year + 1900));
   }
   else if (!_tcscmp(pszAttr, _T("yday")) || !_tcscmp(pszAttr, _T("tm_yday")))
   {
      value = new NXSL_Value((LONG)st->tm_yday);
   }
   else if (!_tcscmp(pszAttr, _T("wday")) || !_tcscmp(pszAttr, _T("tm_wday")))
   {
      value = new NXSL_Value((LONG)st->tm_wday);
   }
   else if (!_tcscmp(pszAttr, _T("isdst")) || !_tcscmp(pszAttr, _T("tm_isdst")))
   {
      value = new NXSL_Value((LONG)st->tm_isdst);
   }
	else
	{
		value = NULL;	// Error
	}
   return value;
}

void NXSL_TimeClass::onObjectDelete(NXSL_Object *object)
{
	safe_free(object->getData());
}


//
// NXSL "TIME" class object
//

static NXSL_TimeClass m_nxslTimeClass;


//
// Return parsed local time
//

int F_localtime(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	struct tm *p;
	time_t t;

   if (argc == 0)
	{
		t = time(NULL);
	}
	else if (argc == 1)
	{
		if (!argv[0]->isInteger())
			return NXSL_ERR_NOT_INTEGER;

		t = argv[0]->getValueAsUInt32();
	}
	else
	{
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;
	}

	p = localtime(&t);
	*ppResult = new NXSL_Value(new NXSL_Object(&m_nxslTimeClass, nx_memdup(p, sizeof(struct tm))));
	return 0;
}


//
// Return parsed UTC time
//

int F_gmtime(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	struct tm *p;
	time_t t;

   if (argc == 0)
	{
		t = time(NULL);
	}
	else if (argc == 1)
	{
		if (!argv[0]->isInteger())
			return NXSL_ERR_NOT_INTEGER;

		t = argv[0]->getValueAsUInt32();
	}
	else
	{
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;
	}

	p = gmtime(&t);
	*ppResult = new NXSL_Value(new NXSL_Object(&m_nxslTimeClass, nx_memdup(p, sizeof(struct tm))));
	return 0;
}


//
// Get substring of a string
// Possible usage:
//    substr(string, start) - all characters from position 'start'
//    substr(string, start, n) - n characters from position 'start'
//    substr(string, NULL, n) - first n characters
//

int F_substr(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	int nStart, nCount;
	const TCHAR *pBase;
	UINT32 dwLen;

   if ((argc < 2) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

	if (argv[1]->isNull())
	{
		nStart = 0;
	}
	else if (argv[1]->isInteger())
	{
		nStart = argv[1]->getValueAsInt32();
		if (nStart > 0)
			nStart--;
		else
			nStart = 0;
	}
	else
	{
		return NXSL_ERR_NOT_INTEGER;
	}

	if (argc == 3)
	{
		if (!argv[2]->isInteger())
			return NXSL_ERR_NOT_INTEGER;
		nCount = argv[2]->getValueAsInt32();
		if (nCount < 0)
			nCount = 0;
	}
	else
	{
		nCount = -1;
	}

	pBase = argv[0]->getValueAsString(&dwLen);
	if ((UINT32)nStart < dwLen)
	{
		pBase += nStart;
		dwLen -= nStart;
		if ((nCount == -1) || ((UINT32)nCount > dwLen))
		{
			nCount = dwLen;
		}
		*ppResult = new NXSL_Value(pBase, (UINT32)nCount);
	}
	else
	{
		*ppResult = new NXSL_Value(_T(""));
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
   *result = (v <= 0x7FFFFFFF) ? new NXSL_Value((UINT32)v) : new NXSL_Value(v);
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
	*ppResult = new NXSL_Value(buffer);
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
   *result = new NXSL_Value(buffer);
   return NXSL_ERR_SUCCESS;
}

/**
 * ord() -  convert characters into their ASCII or Unicode values
 */
int F_ord(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   *result = new NXSL_Value((INT32)(argv[0]->getValueAsCString()[0]));
   return NXSL_ERR_SUCCESS;
}

/**
 * left() - take leftmost part of a string and pad or truncate it as necessary
 * Format: left(string, len, [pad])
 */
int F_left(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	const TCHAR *str;
	TCHAR *newStr, pad;
	LONG newLen;
	UINT32 i, len;

   if ((argc < 2) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[1]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   if ((!argv[0]->isString()) ||
		 ((argc == 3) && (!argv[2]->isString())))
		return NXSL_ERR_NOT_STRING;

	if (argc == 3)
	{
		pad = *(argv[2]->getValueAsCString());
		if (pad == 0)
			pad = _T(' ');
	}
	else
	{
		pad = _T(' ');
	}

	newLen = argv[1]->getValueAsInt32();
	if (newLen < 0)
		newLen = 0;

   str = argv[0]->getValueAsString(&len);
	if (len > (UINT32)newLen)
		len = (UINT32)newLen;
	newStr = (TCHAR *)malloc(newLen * sizeof(TCHAR));
	memcpy(newStr, str, len * sizeof(TCHAR));
   for(i = len; i < (UINT32)newLen; i++)
      newStr[i] = pad;
   *ppResult = new NXSL_Value(newStr, newLen);
	free(newStr);
	return NXSL_ERR_SUCCESS;
}


//
// right() - take rightmost part of a string and pad or truncate it as necessary
// Format: right(string, len, [pad])
//

int F_right(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	const TCHAR *str;
	TCHAR *newStr, pad;
	LONG newLen;
	UINT32 i, len, shift;

   if ((argc < 2) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[1]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   if ((!argv[0]->isString()) ||
		 ((argc == 3) && (!argv[2]->isString())))
		return NXSL_ERR_NOT_STRING;

	if (argc == 3)
	{
		pad = *(argv[2]->getValueAsCString());
		if (pad == 0)
			pad = _T(' ');
	}
	else
	{
		pad = _T(' ');
	}

	newLen = argv[1]->getValueAsInt32();
	if (newLen < 0)
		newLen = 0;

   str = argv[0]->getValueAsString(&len);
	if (len > (UINT32)newLen)
	{
		shift = len - (UINT32)newLen;
		len = (UINT32)newLen;
	}
	else
	{
		shift = 0;
	}
	newStr = (TCHAR *)malloc(newLen * sizeof(TCHAR));
	memcpy(&newStr[(UINT32)newLen - len], &str[shift], len * sizeof(TCHAR));
   for(i = 0; i < (UINT32)newLen - len; i++)
      newStr[i] = pad;
   *ppResult = new NXSL_Value(newStr, newLen);
	free(newStr);
	return 0;
}

/**
 * Exit from script
 */
int F_exit(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (argc > 1)
		return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	*ppResult = (argc == 0) ? new NXSL_Value((LONG)0) : new NXSL_Value(argv[0]);
   return NXSL_STOP_SCRIPT_EXECUTION;
}

/**
 * Trim whitespace characters from the string
 */
int F_trim(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

	UINT32 len;
	const TCHAR *string = argv[0]->getValueAsString(&len);
	
	int i;
	for(i = 0; (i < (int)len) && (string[i] == _T(' ') || string[i] == _T('\t')); i++);
	int startPos = i;
	if (len > 0)
		for(i = (int)len - 1; (i >= startPos) && (string[i] == _T(' ') || string[i] == _T('\t')); i--);

	*ppResult = new NXSL_Value(&string[startPos], i - startPos + 1);
	return 0;
}


//
// Trim trailing whitespace characters from the string
//

int F_rtrim(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

	UINT32 len;
	const TCHAR *string = argv[0]->getValueAsString(&len);
	
	int i;
	for(i = (int)len - 1; (i >= 0) && (string[i] == _T(' ') || string[i] == _T('\t')); i--);

	*ppResult = new NXSL_Value(string, i + 1);
	return 0;
}


//
// Trim leading whitespace characters from the string
//

int F_ltrim(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

	UINT32 len;
	const TCHAR *string = argv[0]->getValueAsString(&len);
	
	int i;
	for(i = 0; (i < (int)len) && (string[i] == _T(' ') || string[i] == _T('\t')); i++);

	*ppResult = new NXSL_Value(&string[i], (int)len - i);
	return 0;
}

/**
 * Trace
 */
int F_trace(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	vm->trace(argv[0]->getValueAsInt32(), argv[1]->getValueAsCString());
	*ppResult = new NXSL_Value();
	return 0;
}

/**
 * Common implementation for index and rindex functions
 */
static int F_index_rindex(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm, bool reverse)
{
	if ((argc != 2) && (argc != 3))
		return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	if (!argv[0]->isString() || !argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	UINT32 strLength, substrLength;
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

	int index = 0;	// 0 = not found
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

	*ppResult = new NXSL_Value((LONG)index);
	return 0;
}

/**
 * index(string, substring, [position])
 * Returns the position of the first occurrence of SUBSTRING in STRING at or after POSITION.
 * If you don't specify POSITION, the search starts at the beginning of STRING. If SUBSTRING
 * is not found, returns 0.
 */
int F_index(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	return F_index_rindex(argc, argv, ppResult, vm, false);
}


/**
 * rindex(string, substring, [position])
 * Returns the position of the last occurrence of SUBSTRING in STRING at or before POSITION.
 * If you don't specify POSITION, the search starts at the end of STRING. If SUBSTRING
 * is not found, returns 0.
 */
int F_rindex(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	return F_index_rindex(argc, argv, ppResult, vm, true);
}

/**
 * NXSL function: Generate random number in given range
 */
int F_random(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isInteger() || !argv[1]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	int range = argv[1]->getValueAsInt32() - argv[0]->getValueAsInt32() + 1;
	*ppResult = new NXSL_Value((rand() % range) + argv[0]->getValueAsInt32());
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
	*ppResult = new NXSL_Value;
	return 0;
}

/**
 * System functions, mostly for debugging
 */
int F_sys(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	switch(argv[0]->getValueAsInt32())
	{
		case 1:	// dump script code to stdout
			vm->dump(stdout);
			break;
		default:
			break;
	}

	*ppResult = new NXSL_Value;
	return 0;
}

/**
 * NXSL function: floor
 */
int F_floor(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isNumeric())
		return NXSL_ERR_NOT_NUMBER;

	*ppResult = new NXSL_Value(floor(argv[0]->getValueAsReal()));
	return 0;
}

/**
 * NXSL function: ceil
 */
int F_ceil(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isNumeric())
		return NXSL_ERR_NOT_NUMBER;

	*ppResult = new NXSL_Value(ceil(argv[0]->getValueAsReal()));
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
		*ppResult = new NXSL_Value((d > 0.0) ? floor(d + 0.5) : ceil(d - 0.5));
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
		*ppResult = new NXSL_Value(d);
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

	TCHAR format[32], buffer[64];
	_sntprintf(format, 32, _T("%%%d.%df"), width, precision);
	_sntprintf(buffer, 64, format, argv[0]->getValueAsReal());
	*ppResult = new NXSL_Value(buffer);
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
   *ppResult = new NXSL_Value(result ? 1 : 0);
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
#else
   const char *str = argv[0]->getValueAsCString();
   CalculateMD5Hash((BYTE *)str, strlen(str), hash);
#endif

   TCHAR text[MD5_DIGEST_SIZE * 2 + 1];
   BinToStr(hash, MD5_DIGEST_SIZE, text);
   *ppResult = new NXSL_Value(text);

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
#else
   const char *str = argv[0]->getValueAsCString();
   CalculateSHA1Hash((BYTE *)str, strlen(str), hash);
#endif

   TCHAR text[SHA1_DIGEST_SIZE * 2 + 1];
   BinToStr(hash, SHA1_DIGEST_SIZE, text);
   *ppResult = new NXSL_Value(text);

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
#else
   const char *str = argv[0]->getValueAsCString();
   CalculateSHA256Hash((BYTE *)str, strlen(str), hash);
#endif

   TCHAR text[SHA256_DIGEST_SIZE * 2 + 1];
   BinToStr(hash, SHA256_DIGEST_SIZE, text);
   *ppResult = new NXSL_Value(text);

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
         *ppResult = new NXSL_Value(buffer);
      }
      else
      {
         *ppResult = new NXSL_Value;
      }
   }
   else
   {
      *ppResult = new NXSL_Value;
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
      *ppResult = new NXSL_Value((const TCHAR *)addr.toString());
   }
   else
   {
      *ppResult = new NXSL_Value;
   }
   return 0;
}

/**
 * Convert array to string (recursively for array values)
 */
static void ArrayToString(NXSL_Array *a, String& s, const TCHAR *separator)
{
   for(int i = 0; i < a->size(); i++)
   {
      if (!s.isEmpty())
         s.append(separator);
      NXSL_Value *e = a->getByPosition(i);
      if (e->isArray())
      {
         ArrayToString(e->getValueAsArray(), s, separator);
      }
      else
      {
         s.append(e->getValueAsCString());
      }
   }
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

   String s;
   NXSL_Array *a = argv[0]->getValueAsArray();
   ArrayToString(a, s, argv[1]->getValueAsCString());
   *result = new NXSL_Value(s);
   return 0;
}
