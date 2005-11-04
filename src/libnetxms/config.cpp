/* 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: config.cpp
**
**/

#include "libnetxms.h"


//
// Universal configuration loader
//

DWORD LIBNETXMS_EXPORTABLE NxLoadConfig(TCHAR *pszFileName, TCHAR *pszSection,
                                        NX_CFG_TEMPLATE *pTemplateList, BOOL bPrint)
{
   FILE *cfg;
   TCHAR *ptr, *eptr, szBuffer[4096];
   int i, iSourceLine = 0, iErrors = 0, iLength;
   BOOL bActiveSection = (pszSection[0] == 0);

   cfg = _tfopen(pszFileName, _T("r"));
   if (cfg == NULL)
   {
      if (bPrint)
#ifndef UNDER_CE
#ifdef UNICODE
         wprintf(L"Unable to open configuration file\n");
#else
         printf("Unable to open configuration file: %s\n", strerror(errno));
#endif   /* UNICODE */
#endif   /* UNDER_CE */
      return NXCFG_ERR_NOFILE;
   }

   while(!feof(cfg))
   {
      // Read line from file
      szBuffer[0] = 0;
      _fgetts(szBuffer, 4095, cfg);
      iSourceLine++;
      ptr = _tcschr(szBuffer, _T('\n'));
      if (ptr != NULL)
         *ptr = 0;
      ptr = _tcschr(szBuffer, _T('#'));
      if (ptr != NULL)
         *ptr = 0;

      StrStrip(szBuffer);
      if (szBuffer[0] == 0)
         continue;

      // Check if it's a section name
      if (szBuffer[0] == _T('*'))
      {
         bActiveSection = !_tcsicmp(&szBuffer[1], pszSection);
      }
      else
      {
         if (!bActiveSection)
            continue;

         // Divide on two parts at = sign
         ptr = _tcschr(szBuffer, '=');
         if (ptr == NULL)
         {
            iErrors++;
            if (bPrint)
               _tprintf(_T("Syntax error in configuration file at line %d\n"), iSourceLine);
            continue;
         }
         *ptr = 0;
         ptr++;
         StrStrip(szBuffer);
         StrStrip(ptr);

         // Find corresponding token in template list
         for(i = 0; pTemplateList[i].iType != CT_END_OF_LIST; i++)
            if (!_tcsicmp(pTemplateList[i].szToken, szBuffer))
            {
               switch(pTemplateList[i].iType)
               {
                  case CT_LONG:
                     *((LONG *)pTemplateList[i].pBuffer) = _tcstol(ptr, &eptr, 0);
                     if (*eptr != 0)
                     {
                        iErrors++;
                        if (bPrint)
                           _tprintf(_T("Invalid number '%s' in configuration file at line %d\n"), ptr, iSourceLine);
                     }
                     break;
                  case CT_WORD:
                     *((WORD *)pTemplateList[i].pBuffer) = (WORD)_tcstoul(ptr, &eptr, 0);
                     if (*eptr != 0)
                     {
                        iErrors++;
                        if (bPrint)
                           _tprintf(_T("Invalid number '%s' in configuration file at line %d\n"),
						              ptr, iSourceLine);
                     }
                     break;
                  case CT_BOOLEAN:
                     if (!_tcsicmp(ptr, _T("yes")) || !_tcsicmp(ptr, _T("true")) ||
                         !_tcsicmp(ptr, _T("on")) || !_tcsicmp(ptr, _T("1")))
                     {
                        *((DWORD *)pTemplateList[i].pBuffer) |= pTemplateList[i].dwBufferSize;
                     }
                     else
                     {
                        *((DWORD *)pTemplateList[i].pBuffer) &= ~(pTemplateList[i].dwBufferSize);
                     }
                     break;
                  case CT_STRING:
                     nx_strncpy((TCHAR *)pTemplateList[i].pBuffer, ptr, pTemplateList[i].dwBufferSize);
                     break;
                  case CT_STRING_LIST:
                     iLength = _tcslen(ptr) + 2;
                     if (pTemplateList[i].dwBufferPos + iLength >= pTemplateList[i].dwBufferSize)
                     {
                        pTemplateList[i].dwBufferSize += max(1024, iLength);
                        *((TCHAR **)pTemplateList[i].pBuffer) = 
                           (TCHAR *)realloc(*((TCHAR **)pTemplateList[i].pBuffer), pTemplateList[i].dwBufferSize);
                     }

                     _tcscpy((*((TCHAR **)pTemplateList[i].pBuffer)) + pTemplateList[i].dwBufferPos, ptr);
                     pTemplateList[i].dwBufferPos += iLength - 1;
                     (*((TCHAR **)pTemplateList[i].pBuffer))[pTemplateList[i].dwBufferPos - 1] = pTemplateList[i].cSeparator;
                     (*((TCHAR **)pTemplateList[i].pBuffer))[pTemplateList[i].dwBufferPos] = 0;
                     break;
                  case CT_IGNORE:
                     break;
                  default:
                     break;
               }
               break;
            }

         // Invalid keyword
         if (pTemplateList[i].iType == CT_END_OF_LIST)
         {
            iErrors++;
            if (bPrint)
               _tprintf(_T("Invalid keyword %s in configuration file at line %d\n"), szBuffer, iSourceLine);
         }
      }
   }
   fclose(cfg);

   if ((!iErrors) && (bPrint))
      _tprintf(_T("Configuration file OK\n"));

   return (iErrors > 0) ? NXCFG_ERR_SYNTAX : NXCFG_ERR_OK;
}
