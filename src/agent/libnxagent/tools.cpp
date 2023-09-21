/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: tools.cpp
**
**/

#include "libnxagent.h"

/**
 * Get arguments for metric like name(arg1,...)
 * Returns FALSE on processing error
 */
static bool AgentGetMetricArgInternal(const TCHAR *param, int index, TCHAR *arg, size_t maxSize, bool inBrackets)
{
   arg[0] = 0;    // Default is empty string
   const TCHAR *ptr1 = inBrackets ? _tcschr(param, _T('(')) : param;
   if (ptr1 == nullptr)
      return true;  // No arguments at all

   bool success = true;
   int state, currIndex;
   size_t pos;
   const TCHAR *ptr2;
   for(ptr2 = ptr1 + 1, currIndex = 1, state = 0, pos = 0; state != -1; ptr2++)
   {
      switch(state)
      {
         case 0:  // Normal
            switch(*ptr2)
            {
               case _T(')'):
                  if (currIndex == index)
                     arg[pos] = 0;
                  state = -1;    // Finish processing
                  break;
               case _T('"'):
                  state = 1;     // String
                  break;
               case _T('\''):    // String, type 2
                  state = 2;
                  break;
               case _T(','):
                  if (currIndex == index)
                  {
                     arg[pos] = 0;
                     state = -1;
                  }
                  else
                  {
                     currIndex++;
                  }
                  break;
               case 0:
                  state = -1;       // Finish processing
                  if (inBrackets)   // No error flag if parameters were given without brackets
                     success = false;  // Set error flag
                  break;
               default:
                  if ((currIndex == index) && (pos < maxSize - 1))
                     arg[pos++] = *ptr2;
            }
            break;
         case 1:  // String in ""
            switch(*ptr2)
            {
               case _T('"'):
                  if (*(ptr2 + 1) != _T('"'))
                  {
                     state = 0;     // Normal
                  }
                  else
                  {
                     ptr2++;
                     if ((currIndex == index) && (pos < maxSize - 1))
                        arg[pos++] = *ptr2;
                  }
                  break;
               case 0:
                  state = -1;       // Finish processing
                  success = false;  // Set error flag
                  break;
               default:
                  if ((currIndex == index) && (pos < maxSize - 1))
                     arg[pos++] = *ptr2;
            }
            break;
         case 2:  // String in ''
            switch(*ptr2)
            {
               case _T('\''):
                  if (*(ptr2 + 1) != _T('\''))
                  {
                     state = 0;     // Normal
                  }
                  else
                  {
                     ptr2++;
                     if ((currIndex == index) && (pos < maxSize - 1))
                        arg[pos++] = *ptr2;
                  }
                  break;
               case 0:
                  state = -1;       // Finish processing
                  success = false;  // Set error flag
                  break;
               default:
                  if ((currIndex == index) && (pos < maxSize - 1))
                     arg[pos++] = *ptr2;
            }
            break;
      }
   }

   if (success)
      Trim(arg);
   return success;
}

/**
 * Get arguments for metric like name(arg1,...) as multibyte string
 * Returns FALSE on processing error
 */
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgA(const TCHAR *param, int index, char *arg, size_t maxSize, bool inBrackets)
{
#ifdef UNICODE
   WCHAR localBuffer[1024];
	WCHAR *temp = (maxSize > 1024) ? MemAllocStringW(maxSize) : localBuffer;
	bool success = AgentGetMetricArgInternal(param, index, temp, maxSize, inBrackets);
	if (success)
	{
		wchar_to_mb(temp, -1, arg, maxSize);
		arg[maxSize - 1] = 0;
	}
	if (temp != localBuffer)
	   MemFree(temp);
	return success;
#else
	return AgentGetMetricArgInternal(param, index, arg, maxSize, inBrackets);
#endif
}

/**
 * Get arguments for metric like name(arg1,...) as UNICODE string
 * Returns FALSE on processing error
 */
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgW(const TCHAR *param, int index, WCHAR *arg, size_t maxSize, bool inBrackets)
{
#ifdef UNICODE
	return AgentGetMetricArgInternal(param, index, arg, maxSize, inBrackets);
#else
   char localBuffer[1024];
   char *temp = (maxSize > 1024) ? MemAllocStringA(maxSize) : localBuffer;
	bool success = AgentGetMetricArgInternal(param, index, temp, maxSize, inBrackets);
	if (success)
	{
		mb_to_wchar(temp, -1, arg, maxSize);
		arg[maxSize - 1] = 0;
	}
   if (temp != localBuffer)
      MemFree(temp);
	return success;
#endif
}

/**
 * Get arguments for metric like name(arg1,...) as IP address object, with host name resolution if necessary
 * Returns invalid address on processing error
 */
InetAddress LIBNXAGENT_EXPORTABLE AgentGetMetricArgAsInetAddress(const TCHAR *metric, int index, bool inBrackets)
{
   TCHAR hostName[256];
   if (!AgentGetMetricArg(metric, index, hostName, 256, inBrackets))
      return InetAddress();
   if (hostName[0] == 0)
      return InetAddress();
   return InetAddress::resolveHostName(hostName);
}

/**
 * Get arguments for metric like name(arg1,...) as given integer type
 * Returns false on processing error
 */
template<typename _Tv, typename _Ti> static inline bool AgentGetMetricArgAsInteger(const TCHAR *metric, int index, _Tv *value, _Tv defaultValue, bool inBrackets, _Ti (*f)(const TCHAR*, TCHAR**, int))
{
   TCHAR buffer[256];
   if (!AgentGetMetricArg(metric, index, buffer, 256, inBrackets))
      return false;
   if (buffer[0] == 0)
   {
      *value = defaultValue;
      return true;
   }
   TCHAR *eptr;
   *value = static_cast<_Tv>(f(buffer, &eptr, 0));
   return *eptr == 0;
}

/**
 * Get arguments for metric like name(arg1,...) as 16 bit signed integer
 * Returns false on processing error
 */
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgAsInt16(const TCHAR *metric, int index, int16_t *value, int16_t defaultValue, bool inBrackets)
{
   return AgentGetMetricArgAsInteger(metric, index, value, defaultValue, inBrackets, _tcstol);
}

/**
 * Get arguments for metric like name(arg1,...) as 16 bit unsigned integer
 * Returns false on processing error
 */
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgAsUInt16(const TCHAR *metric, int index, uint16_t *value, uint16_t defaultValue, bool inBrackets)
{
   return AgentGetMetricArgAsInteger(metric, index, value, defaultValue, inBrackets, _tcstoul);
}

/**
 * Get arguments for metric like name(arg1,...) as 32 bit signed integer
 * Returns false on processing error
 */
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgAsInt32(const TCHAR *metric, int index, int32_t *value, int32_t defaultValue, bool inBrackets)
{
   return AgentGetMetricArgAsInteger(metric, index, value, defaultValue, inBrackets, _tcstol);
}

/**
 * Get arguments for metric like name(arg1,...) as 32 bit unsigned integer
 * Returns false on processing error
 */
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgAsUInt32(const TCHAR *metric, int index, uint32_t *value, uint32_t defaultValue, bool inBrackets)
{
   return AgentGetMetricArgAsInteger(metric, index, value, defaultValue, inBrackets, _tcstoul);
}

/**
 * Get arguments for metric like name(arg1,...) as 64 bit signed integer
 * Returns false on processing error
 */
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgAsInt64(const TCHAR *metric, int index, int64_t *value, int64_t defaultValue, bool inBrackets)
{
   return AgentGetMetricArgAsInteger(metric, index, value, defaultValue, inBrackets, _tcstoll);
}

/**
 * Get arguments for metric like name(arg1,...) as 64 bit unsigned integer
 * Returns false on processing error
 */
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgAsUInt64(const TCHAR *metric, int index, uint64_t *value, uint64_t defaultValue, bool inBrackets)
{
   return AgentGetMetricArgAsInteger(metric, index, value, defaultValue, inBrackets, _tcstoull);
}

/**
 * Get arguments for metric like name(arg1,...) as boolean value
 * Returns false on processing error
 */
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgAsBoolean(const TCHAR *metric, int index, bool *value, bool defaultValue, bool inBrackets)
{
   TCHAR buffer[256];
   if (!AgentGetMetricArg(metric, index, buffer, 256, inBrackets))
      return false;
   if (buffer[0] == 0)
   {
      *value = defaultValue;
   }
   else if (!_tcsicmp(buffer, _T("true")) || !_tcsicmp(buffer, _T("yes")) || (_tcstol(buffer, nullptr, 10) != 0))
   {
      *value = true;
   }
   else
   {
      *value = false;
   }
   return true;
}

/**
 * Convert test to integer type representation
 */
int LIBNXAGENT_EXPORTABLE TextToDataType(const TCHAR *name)
{
   static const TCHAR *dtNames[] = { _T("int32"), _T("uint32"), _T("int64"), _T("uint64"), _T("string"), _T("float"), _T("null"), _T("counter32"), _T("counter64"), nullptr };
   for(int i = 0; dtNames[i] != nullptr; i++)
      if (!_tcsicmp(dtNames[i], name))
      {
         return i;
      }

   // Check for "int" and "uint" for compatibility with removed function NxDCIDataTypeFromText
   if (!_tcsicmp(name, _T("int")))
      return 0;
   if (!_tcsicmp(name, _T("uint")))
      return 1;

   return -1;
}
