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

DWORD LIBNETXMS_EXPORTABLE NxLoadConfig(char *pszFileName, NX_CFG_TEMPLATE *pTemplateList, BOOL bPrint)
{
   FILE *cfg;
   char *ptr, *eptr, szBuffer[4096];
   int i, iSourceLine = 0, iErrors = 0;

   cfg = fopen(pszFileName, "r");
   if (cfg == NULL)
   {
      if (bPrint)
         printf("Unable to open configuration file: %s\n", strerror(errno));
      return NXCFG_ERR_NOFILE;
   }

   while(!feof(cfg))
   {
      // Read line from file
      szBuffer[0] = 0;
      fgets(szBuffer, 4095, cfg);
      iSourceLine++;
      ptr = strchr(szBuffer, '\n');
      if (ptr != NULL)
         *ptr = 0;
      ptr = strchr(szBuffer, '#');
      if (ptr != NULL)
         *ptr = 0;

      StrStrip(szBuffer);
      if (szBuffer[0] == 0)
         continue;

      // Divide on two parts at = sign
      ptr = strchr(szBuffer, '=');
      if (ptr == NULL)
      {
         iErrors++;
         if (bPrint)
            printf("Syntax error in configuration file at line %d\n", iSourceLine);
         continue;
      }
      *ptr = 0;
      ptr++;
      StrStrip(szBuffer);
      StrStrip(ptr);

      // Find corresponding token in template list
      for(i = 0; pTemplateList[i].iType != CT_END_OF_LIST; i++)
         if (!stricmp(pTemplateList[i].szToken, szBuffer))
         {
            switch(pTemplateList[i].iType)
            {
               case CT_LONG:
                  *((long *)pTemplateList[i].pBuffer) = strtol(ptr, &eptr, 0);
                  if (*eptr != 0)
                  {
                     iErrors++;
                     if (bPrint)
                        printf("Invalid number '%s' in configuration file at line %d\n", ptr, iSourceLine);
                  }
                  break;
               case CT_WORD:
                  *((WORD *)pTemplateList[i].pBuffer) = (WORD)strtoul(ptr, &eptr, 0);
                  if (*eptr != 0)
                  {
                     iErrors++;
                     if (bPrint)
                        printf("Invalid number '%s' in configuration file at line %d\n", ptr, iSourceLine);
                  }
                  break;
               case CT_BOOLEAN:
                  if (!stricmp(ptr, "yes") || !stricmp(ptr, "true") ||
                      !stricmp(ptr, "on") || !stricmp(ptr, "1"))
                  {
                     *((DWORD *)pTemplateList[i].pBuffer) |= pTemplateList[i].dwBufferSize;
                  }
                  else
                  {
                     *((DWORD *)pTemplateList[i].pBuffer) &= ~(pTemplateList[i].dwBufferSize);
                  }
                  break;
               case CT_STRING:
                  strncpy((char *)pTemplateList[i].pBuffer, ptr, pTemplateList[i].dwBufferSize);
                  break;
               case CT_STRING_LIST:
                  if (pTemplateList[i].dwBufferPos < pTemplateList[i].dwBufferSize)
                  {
                     strncpy((char *)pTemplateList[i].pBuffer + pTemplateList[i].dwBufferPos, 
                             ptr, pTemplateList[i].dwBufferSize - pTemplateList[i].dwBufferPos - 1);
                     pTemplateList[i].dwBufferPos += strlen(ptr);
                     if (pTemplateList[i].dwBufferPos < pTemplateList[i].dwBufferSize)
                        ((char *)pTemplateList[i].pBuffer)[pTemplateList[i].dwBufferPos++] = pTemplateList[i].cSeparator;
                  }
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
            printf("Invalid keyword %s in configuration file at line %d\n", szBuffer, iSourceLine);
      }
   }
   fclose(cfg);

   if ((!iErrors) && (bPrint))
      printf("Configuration file OK\n");

   return (iErrors > 0) ? NXCFG_ERR_SYNTAX : NXCFG_ERR_OK;
}
