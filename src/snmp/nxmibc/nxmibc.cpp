/* 
** NetXMS - Network Management System
** NetXMS MIB compiler
** Copyright (C) 2005-2012 Victor Kirhenshtein
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
** File: nxmibc.cpp
**
**/

#include "nxmibc.h"

#ifdef _WIN32
#include <conio.h>
#endif


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
static bool s_pauseBeforeExit = false;


//
// Errors
//

static struct
{
   int nSeverity;
   const char *pszText;
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
// Pause if needed
//

static void Pause()
{
	if (s_pauseBeforeExit)
	{
#ifdef _WIN32
		printf("Press any key to continue...\n");
		_getch();
#else
		printf("Press ENTER to continue...\n");
		char temp[256];
		fgets(temp, 255, stdin);
#endif
	}
}


//
// Display error text and abort compilation
//

extern "C" void Error(int nError, char *pszModule, ...)
{
   va_list args;
   static const char *m_szSeverityText[] = { "INFO", "WARNING", "ERROR" };

   printf("%s: %s %03d: ", pszModule, m_szSeverityText[m_errorList[nError].nSeverity], nError);
   va_start(args, pszModule);
   vprintf(m_errorList[nError].pszText, args);
   va_end(args);
   printf("\n");
   if (m_errorList[nError].nSeverity == MIBC_ERROR)
	{
		Pause();
      exit(1);
	}
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
			 "   -P        : Pause before exit\n"
          "   -s        : Strip descriptions from MIB objects\n"
          "   -z        : Compress output file\n"
          "\n");
   exit(0);
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
				int len = (int)strlen(pFile->d_name);
            if ((len > 4) && (!stricmp(&pFile->d_name[len - 4], ".txt")))
            {
               sprintf(szBuffer, "%s" FS_PATH_SEPARATOR_A "%s", pszPath, pFile->d_name);
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
   int i, ch, rc = 0;

   printf("NetXMS MIB Compiler  Version " NETXMS_VERSION_STRING_A "\n"
          "Copyright (c) 2005-2012 Victor Kirhenshtein\n\n");

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "d:ho:Psz")) != -1)
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
            strncpy(m_szOutFile, optarg, MAX_PATH);
				m_szOutFile[MAX_PATH - 1] = 0;
            break;
			case 'P':
            s_pauseBeforeExit = true;
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
#ifdef UNICODE
			WCHAR *wname = WideStringFromMBString(m_szOutFile);
         dwRet = SNMPSaveMIBTree(wname, pRoot, dwFlags);
			free(wname);
#else
         dwRet = SNMPSaveMIBTree(m_szOutFile, pRoot, dwFlags);
#endif
         if (dwRet != SNMP_ERR_SUCCESS)
            printf("ERROR: Cannot save output file %s (%s)\n", m_szOutFile,
                   SNMPGetErrorText(dwRet));
      }
   }
   else
   {
      printf("ERROR: No source files given\n");
      rc = 1;
   }

	Pause();
   return rc;
}
