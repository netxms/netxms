/*
** NetXMS - Network Management System
** Copyright (C) 2003-2015 Victor Kirhenshtein
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

#include "appagent-internal.h"

/**
 * Get arguments for parameters like name(arg1,...)
 * Returns FALSE on processing error
 */
static bool AgentGetParameterArgInternal(const TCHAR *param, int index, TCHAR *arg, int maxSize)
{
   const TCHAR *ptr1, *ptr2;
   int state, currIndex, pos;
   bool success = true;

   arg[0] = 0;    // Default is empty string
   ptr1 = _tcschr(param, _T('('));
   if (ptr1 == NULL)
      return true;  // No arguments at all
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
               case _T('\''):        // String, type 2
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
                  state = 0;     // Normal
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
                  state = 0;     // Normal
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
      StrStrip(arg);
   return success;
}

/**
 * Get arguments for parameters like name(arg1,...) as multibyte string
 * Returns FALSE on processing error
 */
bool APPAGENT_EXPORTABLE AppAgentGetParameterArgA(const TCHAR *param, int index, char *arg, int maxSize)
{
#ifdef UNICODE
	WCHAR *temp = (WCHAR *)malloc(maxSize * sizeof(WCHAR));
	bool success = AgentGetParameterArgInternal(param, index, temp, maxSize);
	if (success)
	{
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, temp, -1, arg, maxSize, NULL, NULL);
		arg[maxSize - 1] = 0;
	}
	free(temp);
	return success;
#else
	return AgentGetParameterArgInternal(param, index, arg, maxSize);
#endif
}

/**
 * Get arguments for parameters like name(arg1,...) as UNICODE string
 * Returns FALSE on processing error
 */
bool APPAGENT_EXPORTABLE AppAgentGetParameterArgW(const TCHAR *param, int index, WCHAR *arg, int maxSize)
{
#ifdef UNICODE
	return AgentGetParameterArgInternal(param, index, arg, maxSize);
#else
	char *temp = (char *)malloc(maxSize);
	bool success = AgentGetParameterArgInternal(param, index, temp, maxSize);
	if (success)
	{
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, temp, -1, arg, maxSize);
		arg[maxSize - 1] = 0;
	}
	free(temp);
	return success;
#endif
}
