/* 
** NetXMS - Network Management System
** NetXMS MIB compiler
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
** $module: nxmibc.cpp
**
**/

#include "nxmibc.h"


//
// Severity codes
//

#define MIBC_INFO      0
#define MIBC_WARNING   1
#define MIBC_ERROR     2


//
// Externals
//

int ParseMIBFiles(int nNumFiles, char **ppszFileList);


//
// Errors
//

static struct
{
   int nSeverity;
   char *pszText;
} m_errorList[] =
{
   { MIBC_INFO, "Operation completed successfully" },
   { MIBC_ERROR, "Import symbol \"%s\" unresolved" },
   { MIBC_ERROR, "Import module \"%s\" unresolved" },
   { MIBC_ERROR, "Parser error - %s in line %d" }
};


//
// Display error text and abort compilation
//

extern "C" void Error(int nError, char *pszModule, ...)
{
   va_list args;
   static char *m_szSeverityText[] = { "INFO", "WARNING", "ERROR" };

   printf("%s: %s %03d: ", pszModule, m_szSeverityText[m_errorList[nError].nSeverity], nError);
   va_start(args, pszModule);
   vprintf(m_errorList[nError].pszText, args);
   va_end(args);
   printf("\n");
   if (m_errorList[nError].nSeverity == MIBC_ERROR)
      exit(1);
}


//
// Entry point
//

int main(int argc, char *argv[])
{
   printf("NetXMS MIB Compiler  Version " NETXMS_VERSION_STRING "\n"
          "Copyright (c) 2005 Victor Kirhenshtein\n\n");
   if (argc == 1)
   {
      printf("Usage:\n\n"
             "nxmibc [options] source1 ... sourceN\n\n"
             "Valid options:\n"
             "   -d           : Source is a directory\n"
             "   -o <file>    : Set output file name (default is netxms.mib)\n"
             "\n");
      return 255;
   }
   ParseMIBFiles(argc - 1, &argv[1]);
   return 0;
}
