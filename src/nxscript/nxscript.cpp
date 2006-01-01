/* 
** NetXMS - Network Management System
** NetXMS Scripting Host
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
** $module: nxscript.cpp
**
**/

#include "nxscript.h"


static char *LoadFile(char *pszFileName, DWORD *pdwFileSize)
{
   int fd, iBufPos, iNumBytes, iBytesRead;
   char *pBuffer = NULL;
   struct stat fs;

   fd = open(pszFileName, O_RDONLY | O_BINARY);
   if (fd != -1)
   {
      if (fstat(fd, &fs) != -1)
      {
         pBuffer = (char *)malloc(fs.st_size + 1);
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
// main()
//

int main(int argc, char *argv[])
{
   char *pszSource, szError[1024];
   DWORD dwSize;
   NXSL_Program *pScript;
   NXSL_Environment *pEnv;

   printf("NetXMS Scripting Host  Version " NETXMS_VERSION_STRING "\n"
          "Copyright (c) 2005 Victor Kirhenshtein\n\n");

   if (argc == 1)
   {
      printf("Usage: nxscript script [arg1 [... argN]]\n\n");
      return 127;
   }

   pszSource = LoadFile(argv[1], &dwSize);
   pScript = (NXSL_Program *)NXSLCompile(pszSource, szError, 1024);
   if (pScript != NULL)
   {
      pScript->Dump(stdout);
      pEnv = new NXSL_Environment;
      pEnv->SetIO(stdin, stdout);
      if (pScript->Run(pEnv) == -1)
      {
         printf("%s\n", pScript->GetErrorText());
      }
      delete pScript;
   }
   else
   {
      printf("%s\n", szError);
   }
   return 0;
}
