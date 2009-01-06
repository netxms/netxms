/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: functions.cpp
**
**/

#include "libnxsl.h"
#include <math.h>


//
// Global data
//

const char *g_szTypeNames[] = { "null", "object", "string", "real", "int32",
                                "int64", "uint32", "uint64" };



//
// Type of value
//

int F_typeof(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   *ppResult = new NXSL_Value(g_szTypeNames[argv[0]->DataType()]);
   return 0;
}


//
// Class of an object
//

int F_classof(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
	if (!argv[0]->IsObject())
		return NXSL_ERR_NOT_OBJECT;
		
   *ppResult = new NXSL_Value(argv[0]->GetValueAsObject()->Class()->Name());
   return 0;
}


//
// Absolute value
//

int F_abs(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   int nRet;

   if (argv[0]->IsNumeric())
   {
      if (argv[0]->IsReal())
      {
         *ppResult = new NXSL_Value(fabs(argv[0]->GetValueAsReal()));
      }
      else
      {
         *ppResult = new NXSL_Value(argv[0]);
         if (!argv[0]->IsUnsigned())
            if ((*ppResult)->GetValueAsInt64() < 0)
               (*ppResult)->Negate();
      }
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_NUMBER;
   }
   return nRet;
}


//
// Calculates x raised to the power of y
//

int F_pow(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   int nRet;

   if ((argv[0]->IsNumeric()) && (argv[1]->IsNumeric()))
   {
      *ppResult = new NXSL_Value(pow(argv[0]->GetValueAsReal(), argv[1]->GetValueAsReal()));
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_NUMBER;
   }
   return nRet;
}


//
// Convert string to uppercase
//

int F_upper(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   int nRet;
   DWORD i, dwLen;
   char *pStr;

   if (argv[0]->IsString())
   {
      *ppResult = new NXSL_Value(argv[0]);
      pStr = (*ppResult)->GetValueAsString(&dwLen);
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


//
// Convert string to lowercase
//

int F_lower(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   int nRet;
   DWORD i, dwLen;
   char *pStr;

   if (argv[0]->IsString())
   {
      *ppResult = new NXSL_Value(argv[0]);
      pStr = (*ppResult)->GetValueAsString(&dwLen);
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


//
// String length
//

int F_length(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   int nRet;
   DWORD dwLen;

   if (argv[0]->IsString())
   {
      argv[0]->GetValueAsString(&dwLen);
      *ppResult = new NXSL_Value(dwLen);
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_STRING;
   }
   return nRet;
}


//
// Minimal value from the list of values
//

int F_min(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   int i;
   NXSL_Value *pCurr;

   if (argc == 0)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   pCurr = argv[0];
   for(i = 1; i < argc; i++)
   {
      if (!argv[i]->IsNumeric())
         return NXSL_ERR_NOT_NUMBER;

      if (argv[i]->LT(pCurr))
         pCurr = argv[i];
   }
   *ppResult = new NXSL_Value(pCurr);
   return 0;
}


//
// Maximal value from the list of values
//

int F_max(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   int i;
   NXSL_Value *pCurr;

   if (argc == 0)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   pCurr = argv[0];
   for(i = 0; i < argc; i++)
   {
      if (!argv[i]->IsNumeric())
         return NXSL_ERR_NOT_NUMBER;

      if (argv[i]->GT(pCurr))
         pCurr = argv[i];
   }
   *ppResult = new NXSL_Value(pCurr);
   return 0;
}


//
// Check if IP address is within given range
//

int F_AddrInRange(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   int nRet;
   DWORD dwAddr, dwStart, dwEnd;

   if (argv[0]->IsString() && argv[1]->IsString() && argv[2]->IsString())
   {
      dwAddr = ntohl(inet_addr(argv[0]->GetValueAsCString()));
      dwStart = ntohl(inet_addr(argv[1]->GetValueAsCString()));
      dwEnd = ntohl(inet_addr(argv[2]->GetValueAsCString()));
      *ppResult = new NXSL_Value((LONG)(((dwAddr >= dwStart) && (dwAddr <= dwEnd)) ? 1 : 0));
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_STRING;
   }
   return nRet;
}


//
// Check if IP address is within given subnet
//

int F_AddrInSubnet(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   int nRet;
   DWORD dwAddr, dwSubnet, dwMask;

   if (argv[0]->IsString() && argv[1]->IsString() && argv[2]->IsString())
   {
      dwAddr = ntohl(inet_addr(argv[0]->GetValueAsCString()));
      dwSubnet = ntohl(inet_addr(argv[1]->GetValueAsCString()));
      dwMask = ntohl(inet_addr(argv[2]->GetValueAsCString()));
      *ppResult = new NXSL_Value((LONG)(((dwAddr & dwMask) == dwSubnet) ? 1 : 0));
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_STRING;
   }
   return nRet;
}


//
// Convert time_t into string
// PATCH: by Edgar Chupit
//

int F_strftime(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{   
   TCHAR buffer[512];
   time_t tTime;
	struct tm *ptm;
	BOOL bLocalTime;

   if ((argc == 0) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->IsString())
      return NXSL_ERR_NOT_STRING;
	if (argc > 1)
	{
		if (!argv[1]->IsNumeric() && !argv[1]->IsNull())
			return NXSL_ERR_NOT_NUMBER;
		tTime = (argv[1]->IsNull()) ? time(NULL) : (time_t)argv[1]->GetValueAsUInt64();

		if (argc > 2)
		{
			if (!argv[2]->IsInteger())
				return NXSL_ERR_BAD_CONDITION;
			bLocalTime = argv[2]->GetValueAsInt32() ? TRUE : FALSE;
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
	}

   ptm = bLocalTime ? localtime(&tTime) : gmtime(&tTime);
   _tcsftime(buffer, 512, argv[0]->GetValueAsCString(), ptm);
   *ppResult = new NXSL_Value(buffer);   
   
   return 0;
}


//
// Convert seconds since uptime to string
// PATCH: by Edgar Chupit
//

int F_SecondsToUptime(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   DWORD d, h, n;

   if (!argv[0]->IsNumeric())
      return NXSL_ERR_NOT_NUMBER;

   QWORD arg = argv[0]->GetValueAsUInt64();

   d = (DWORD)(arg / 86400);
   arg -= d * 86400;
   h = (DWORD)(arg / 3600);
   arg -= h * 3600;
   n = (DWORD)(arg / 60);
   arg -= n * 60;
   if (arg > 0)
       n++;

   char pResult[MAX_PATH];
   pResult[0] = 0;

   snprintf(pResult, MAX_PATH, "%u days, %2u:%02u", d, h, n);

   *ppResult = new NXSL_Value(pResult);
   return 0;
}


//
// Get current time
//

int F_time(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   *ppResult = new NXSL_Value((DWORD)time(NULL));
   return 0;
}


//
// NXSL "TIME" class
//

class NXSL_TimeClass : public NXSL_Class
{
public:
   NXSL_TimeClass();

   virtual NXSL_Value *GetAttr(NXSL_Object *pObject, char *pszAttr);
	virtual void OnObjectDelete(NXSL_Object *object);
};


//
// Implementation of "TIME" class
//

NXSL_TimeClass::NXSL_TimeClass()
               :NXSL_Class()
{
   strcpy(m_szName, "TIME");
}

NXSL_Value *NXSL_TimeClass::GetAttr(NXSL_Object *pObject, char *pszAttr)
{
   struct tm *st;
   NXSL_Value *value;

   st = (struct tm *)pObject->Data();
   if (!strcmp(pszAttr, "sec") || !strcmp(pszAttr, "tm_sec"))
   {
      value = new NXSL_Value((LONG)st->tm_sec);
   }
   else if (!strcmp(pszAttr, "min") || !strcmp(pszAttr, "tm_min"))
   {
      value = new NXSL_Value((LONG)st->tm_min);
   }
   else if (!strcmp(pszAttr, "hour") || !strcmp(pszAttr, "tm_hour"))
   {
      value = new NXSL_Value((LONG)st->tm_hour);
   }
   else if (!strcmp(pszAttr, "mday") || !strcmp(pszAttr, "tm_mday"))
   {
      value = new NXSL_Value((LONG)st->tm_mday);
   }
   else if (!strcmp(pszAttr, "mon") || !strcmp(pszAttr, "tm_mon"))
   {
      value = new NXSL_Value((LONG)st->tm_mon);
   }
   else if (!strcmp(pszAttr, "year") || !strcmp(pszAttr, "tm_year"))
   {
      value = new NXSL_Value((LONG)(st->tm_year + 1900));
   }
   else if (!strcmp(pszAttr, "yday") || !strcmp(pszAttr, "tm_yday"))
   {
      value = new NXSL_Value((LONG)st->tm_yday);
   }
   else if (!strcmp(pszAttr, "wday") || !strcmp(pszAttr, "tm_wday"))
   {
      value = new NXSL_Value((LONG)st->tm_wday);
   }
   else if (!strcmp(pszAttr, "isdst") || !strcmp(pszAttr, "tm_isdst"))
   {
      value = new NXSL_Value((LONG)st->tm_isdst);
   }
	else
	{
		value = NULL;	// Error
	}
   return value;
}

void NXSL_TimeClass::OnObjectDelete(NXSL_Object *object)
{
	safe_free(object->Data());
}


//
// NXSL "TIME" class object
//

static NXSL_TimeClass m_nxslTimeClass;


//
// Return parsed local time
//

int F_localtime(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
	struct tm *p;
	time_t t;

   if (argc == 0)
	{
		t = time(NULL);
	}
	else if (argc == 1)
	{
		if (!argv[0]->IsInteger())
			return NXSL_ERR_NOT_INTEGER;

		t = argv[0]->GetValueAsUInt32();
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

int F_gmtime(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
	struct tm *p;
	time_t t;

   if (argc == 0)
	{
		t = time(NULL);
	}
	else if (argc == 1)
	{
		if (!argv[0]->IsInteger())
			return NXSL_ERR_NOT_INTEGER;

		t = argv[0]->GetValueAsUInt32();
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

int F_substr(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
	int nStart, nCount;
	TCHAR *pBase;
	DWORD dwLen;

   if ((argc < 2) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->IsString())
      return NXSL_ERR_NOT_STRING;

	if (argv[1]->IsNull())
	{
		nStart = 0;
	}
	else if (argv[1]->IsInteger())
	{
		nStart = argv[1]->GetValueAsInt32();
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
		if (!argv[2]->IsInteger())
			return NXSL_ERR_NOT_INTEGER;
		nCount = argv[2]->GetValueAsInt32();
		if (nCount < 0)
			nCount = 0;
	}
	else
	{
		nCount = -1;
	}

	pBase = argv[0]->GetValueAsString(&dwLen);
	if ((DWORD)nStart < dwLen)
	{
		pBase += nStart;
		dwLen -= nStart;
		if ((nCount == -1) || ((DWORD)nCount > dwLen))
		{
			nCount = dwLen;
		}
		*ppResult = new NXSL_Value(pBase, (DWORD)nCount);
	}
	else
	{
		*ppResult = new NXSL_Value("");
	}

	return 0;
}


//
// Convert decimal value to hexadecimal string
//   d2x(value)          -> hex value
//   d2x(value, padding) -> hex value padded vith zeros
//

int F_d2x(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
	TCHAR buffer[128], format[32];

   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->IsInteger())
      return NXSL_ERR_NOT_INTEGER;

   if ((argc == 2) && (!argv[1]->IsInteger()))
      return NXSL_ERR_NOT_INTEGER;

	if (argc == 1)
	{
		_tcscpy(format, _T("%X"));
	}
	else
	{
		_sntprintf(format, 32, _T("%%0%dX"), argv[1]->GetValueAsInt32());
	}
	_sntprintf(buffer, 128, format, argv[0]->GetValueAsUInt32());
	*ppResult = new NXSL_Value(buffer);
	return 0;
}


//
// left() - take leftmost part of a string and pad or truncate it as necessary
// Format: left(string, len, [pad])
//

int F_left(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
	TCHAR *str, *newStr, pad;
	LONG newLen;
	DWORD i, len;

   if ((argc < 2) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[1]->IsInteger())
      return NXSL_ERR_NOT_INTEGER;

   if ((!argv[0]->IsString()) ||
		 ((argc == 3) && (!argv[2]->IsString())))
		return NXSL_ERR_NOT_STRING;

	if (argc == 3)
	{
		pad = *(argv[2]->GetValueAsCString());
		if (pad == 0)
			pad = _T(' ');
	}
	else
	{
		pad = _T(' ');
	}

	newLen = argv[1]->GetValueAsInt32();
	if (newLen < 0)
		newLen = 0;

   str = argv[0]->GetValueAsString(&len);
	if (len > (DWORD)newLen)
		len = (DWORD)newLen;
	newStr = (TCHAR *)malloc(newLen * sizeof(TCHAR));
	memcpy(newStr, str, len * sizeof(TCHAR));
   for(i = len; i < (DWORD)newLen; i++)
      newStr[i] = pad;
   *ppResult = new NXSL_Value(newStr, newLen);
	free(newStr);
	return 0;
}


//
// right() - take rightmost part of a string and pad or truncate it as necessary
// Format: right(string, len, [pad])
//

int F_right(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
	TCHAR *str, *newStr, pad;
	LONG newLen;
	DWORD i, len, shift;

   if ((argc < 2) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[1]->IsInteger())
      return NXSL_ERR_NOT_INTEGER;

   if ((!argv[0]->IsString()) ||
		 ((argc == 3) && (!argv[2]->IsString())))
		return NXSL_ERR_NOT_STRING;

	if (argc == 3)
	{
		pad = *(argv[2]->GetValueAsCString());
		if (pad == 0)
			pad = _T(' ');
	}
	else
	{
		pad = _T(' ');
	}

	newLen = argv[1]->GetValueAsInt32();
	if (newLen < 0)
		newLen = 0;

   str = argv[0]->GetValueAsString(&len);
	if (len > (DWORD)newLen)
	{
		shift = len - (DWORD)newLen;
		len = (DWORD)newLen;
	}
	else
	{
		shift = 0;
	}
	newStr = (TCHAR *)malloc(newLen * sizeof(TCHAR));
	memcpy(&newStr[(DWORD)newLen - len], &str[shift], len * sizeof(TCHAR));
   for(i = 0; i < (DWORD)newLen - len; i++)
      newStr[i] = pad;
   *ppResult = new NXSL_Value(newStr, newLen);
	free(newStr);
	return 0;
}
