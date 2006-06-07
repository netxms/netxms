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

int ParseMIBFiles(int nNumFiles, char **ppszFileList, SNMP_MIBObject **ppRoot);


//
// Static data
//

static char m_szOutFile[MAX_PATH] = "netxms.mib";
static int m_iNumFiles = 0;
static char **m_ppFileList = NULL;


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
   { MIBC_ERROR, "Parser error - %s in line %d" },
   { MIBC_ERROR, "Cannot open input file (%s)" },
   { MIBC_ERROR, "Cannot resolve symbol %s" },
   { MIBC_WARNING, "Cannot resolve data type \"%s\" for object \"%s\"" }
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
// Display help and exit
//

static void Help(void)
{
   printf("Usage:\n\n"
          "nxmibc [options] source1 ... sourceN\n\n"
          "Valid options:\n"
          "   -d <dir>  : Include all MIB files from given directory to compilation\n"
          "   -o <file> : Set output file name (default is netxms.mib)\n"
          "   -s        : Strip descriptions from MIB objects\n"
          "   -z        : Compress output file\n"
          "\n");
   exit(255);
}


//
// Add file to compilation list
//

static void AddFileToList(char *pszFile)
{
   m_ppFileList = (char **)realloc(m_ppFileList, sizeof(char *) * (m_iNumFiles + 1));
   m_ppFileList[m_iNumFiles++] = strdup(pszFile);
}


//
// Scan directory for MIB files
//

static void ScanDirectory(char *pszPath)
{
   DIR *pDir;
   struct dirent *pFile;
   char szBuffer[MAX_PATH];

   pDir = opendir(pszPath);
   if (pDir != NULL)
   {
      while(1)
      {
         pFile = readdir(pDir);
         if (pFile == NULL)
            break;
         if (strcmp(pFile->d_name, ".") && strcmp(pFile->d_name, ".."))
         {
            if (MatchString("*.txt", pFile->d_name, FALSE))
            {
               sprintf(szBuffer, "%s" FS_PATH_SEPARATOR "%s",
                       pszPath, pFile->d_name);
               AddFileToList(szBuffer);
            }
         }
      }
      closedir(pDir);
   }
}


//
// Entry point
//

int main(int argc, char *argv[])
{
   SNMP_MIBObject *pRoot;
   DWORD dwFlags = 0, dwRet;
   int i, ch;

   printf("NetXMS MIB Compiler  Version " NETXMS_VERSION_STRING "\n"
          "Copyright (c) 2005, 2006 Victor Kirhenshtein\n\n");

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "d:ho:sz")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            Help();
            break;
         case 'd':
            ScanDirectory(optarg);
            break;
         case 'o':
            nx_strncpy(m_szOutFile, optarg, MAX_PATH);
            break;
         case 's':
            dwFlags |= SMT_SKIP_DESCRIPTIONS;
            break;
         case 'z':
            dwFlags |= SMT_COMPRESS_DATA;
            break;
         case '?':
            return 255;
         default:
            break;
      }
   }

   for(i = optind; i < argc; i++)
      AddFileToList(argv[i]);
   
   if (m_iNumFiles > 0)
   {
      ParseMIBFiles(m_iNumFiles, m_ppFileList, &pRoot);

      if (pRoot != NULL)
      {
         dwRet = SNMPSaveMIBTree(m_szOutFile, pRoot, dwFlags);
         if (dwRet != SNMP_ERR_SUCCESS)
            printf("ERROR: Cannot save output file %s (%s)\n", m_szOutFile,
                   SNMPGetErrorText(dwRet));
      }
   }
   else
   {
      printf("ERROR: No source files given\n");
      return 255;
   }

   return 0;
}
