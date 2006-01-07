/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2005 Victor Kirhenshtein
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
** $module: main.cpp
**
**/

#include "libnxsl.h"


//
// For unknown reasons, min() becames undefined on Linux, despite the fact
// that it is defined in nms_common.h
//

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif


//
// Interface to compiler
//

NXSL_SCRIPT LIBNXSL_EXPORTABLE NXSLCompile(TCHAR *pszSource,
                                           TCHAR *pszError, int nBufSize)
{
   NXSL_Compiler compiler;
   NXSL_Program *pResult;

   pResult = compiler.Compile(pszSource);
   if (pResult == NULL)
   {
      if (pszError != NULL)
         nx_strncpy(pszError, compiler.GetErrorText(), nBufSize);
   }
   return pResult;
}


//
// Run compiled script
//

int LIBNXSL_EXPORTABLE NXSLRun(NXSL_SCRIPT hScript)
{
   if (hScript != NULL)
      return ((NXSL_Program *)hScript)->Run(NULL);
   return -1;
}


//
// Destroy compiled script
//

void LIBNXSL_EXPORTABLE NXSLDestroy(NXSL_SCRIPT hScript)
{
   if (hScript != NULL)
      delete ((NXSL_Program *)hScript);
}


//
// Dump script disassembly to given file
//

void LIBNXSL_EXPORTABLE NXSLDump(NXSL_SCRIPT hScript, FILE *pFile)
{
   if (hScript != NULL)
      ((NXSL_Program *)hScript)->Dump(pFile);
}


//
// Get text of last runtime error
//

TCHAR LIBNXSL_EXPORTABLE *NXSLGetRuntimeError(NXSL_SCRIPT hScript)
{
   return (hScript == NULL) ? (TCHAR *)_T("Invalid script handle") : ((NXSL_Program *)hScript)->GetErrorText();
}


//
// Load file into memory
//

TCHAR LIBNXSL_EXPORTABLE *NXSLLoadFile(TCHAR *pszFileName, DWORD *pdwFileSize)
{
   int fd, iBufPos, iNumBytes, iBytesRead;
   TCHAR *pBuffer = NULL;
   struct stat fs;

   fd = open(pszFileName, O_RDONLY | O_BINARY);
   if (fd != -1)
   {
      if (fstat(fd, &fs) != -1)
      {
         pBuffer = (TCHAR *)malloc(fs.st_size + 1);
         if (pBuffer != NULL)
         {
            *pdwFileSize = fs.st_size;
            for(iBufPos = 0; iBufPos < fs.st_size; iBufPos += iBytesRead)
            {
               iNumBytes = min(16384, fs.st_size - iBufPos);
               if ((iBytesRead = read(fd, &pBuffer[iBufPos], iNumBytes)) < 0)
               {
                  free(pBuffer);
                  pBuffer = NULL;
                  break;
               }
            }
         }
      }
      close(fd);
   }
   return pBuffer;
}


//
// DLL entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif   /* _WIN32 */
