/* 
** NetXMS - Network Management System
** NetXMS Scripting Host
** Copyright (C) 2005-2013 Victor Kirhenshtein
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
** File: nxscript.cpp
**
**/

#define _CRT_NONSTDC_NO_WARNINGS

#include "nxscript.h"


static NXSL_TestClass m_testClass;


int F_new(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	TCHAR *buffer = (TCHAR *)malloc(1024);
	_tcscpy(buffer, _T("test value"));
   *ppResult = new NXSL_Value(new NXSL_Object(&m_testClass, buffer));
   return 0;
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   TCHAR *pszSource, szError[1024];
   TCHAR entryPoint[256] = _T("");
   char outFile[MAX_PATH] = "";
   UINT32 dwSize;
   NXSL_Program *pScript;
   NXSL_Environment *pEnv;
   NXSL_Value **ppArgs;
   NXSL_ExtFunction func;
   int i, ch;
   bool dump = false, printResult = false, compileOnly = false, binary = false;
   int runCount = 1;

   func.m_iNumArgs = 0;
   func.m_pfHandler = F_new;
   _tcscpy(func.m_name, _T("new"));

   WriteToTerminal(_T("NetXMS Scripting Host  Version \x1b[1m") NETXMS_VERSION_STRING _T("\x1b[0m\n")
                   _T("Copyright (c) 2005-2015 Victor Kirhenshtein\n\n"));

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "bcC:de:o:r")) != -1)
   {
      switch(ch)
      {
         case 'b':
            binary = true;
            break;
         case 'c':
            compileOnly = true;
            break;
         case 'C':
            runCount = strtol(optarg, NULL, 0);
            break;
         case 'd':
            dump = true;
            break;
			case 'e':
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, entryPoint, 255);
				entryPoint[255] = 0;
#else
				nx_strncpy(entryPoint, optarg, 256);
#endif
				break;
         case 'o':
				strncpy(outFile, optarg, MAX_PATH - 1);
            outFile[MAX_PATH - 1] = 0;
            break;
			case 'r':
				printResult = true;
				break;
         case '?':
            return 127;
         default:
            break;
      }
   }

   if (argc - optind < 1)
   {
      _tprintf(_T("Usage: nxscript [options] script [arg1 [... argN]]\n\n")
               _T("Valid options are:\n")
               _T("   -b         Input is a binary file\n")
               _T("   -c         Compile only\n")
               _T("   -C <count> Run script multiple times\n")
               _T("   -d         Dump compiled script code\n")
				   _T("   -e <name>  Entry point\n")
               _T("   -o <file>  Write compiled script\n")
               _T("   -r         Print script return value\n")
               _T("\n"));
      return 127;
   }

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(2, &wsaData);
#endif

   if (binary)
   {
#ifdef UNICODE
	   WCHAR *ucName = WideStringFromMBString(argv[optind]);
      ByteStream *s = ByteStream::load(ucName);
	   free(ucName);
#else
      ByteStream *s = ByteStream::load(argv[optind]);
#endif
      if (s == NULL)
      {
		   _tprintf(_T("Error: cannot load input file \"%hs\"\n"), argv[optind]);
         return 1;
      }

      pScript = NXSL_Program::load(*s, szError, 1024);
      delete s;
   }
   else
   {
#ifdef UNICODE
	   WCHAR *ucName = WideStringFromMBString(argv[optind]);
      pszSource = NXSLLoadFile(ucName, &dwSize);
	   free(ucName);
#else
      pszSource = NXSLLoadFile(argv[optind], &dwSize);
#endif
	   if (pszSource == NULL)
	   {
		   _tprintf(_T("Error: cannot load input file \"%hs\"\n"), argv[optind]);
         return 1;
      }

		pScript = NXSLCompile(pszSource, szError, 1024, NULL);
		free(pszSource);
   }

	if (pScript != NULL)
	{
		if (dump)
			pScript->dump(stdout);

      if (outFile[0] != 0)
      {
         int f = open(outFile, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
         if (f != -1)
         {
            ByteStream s(8192);
            pScript->serialize(s);
            s.save(f);
            close(f);
         }
         else
         {
            _tprintf(_T("ERROR: cannot open output file \"%hs\": %s\n"), outFile, _tcserror(errno));
         }
      }

      if (!compileOnly)
      {
		   pEnv = new NXSL_Environment;
		   pEnv->registerFunctionSet(1, &func);

         // Create VM
         NXSL_VM *vm = new NXSL_VM(pEnv);
         if (vm->load(pScript))
         {
            while(runCount-- > 0)
            {
		         // Prepare arguments
		         if (argc - optind > 1)
		         {
			         ppArgs = (NXSL_Value **)malloc(sizeof(NXSL_Value *) * (argc - optind - 1));
			         for(i = optind + 1; i < argc; i++)
				         ppArgs[i - optind - 1] = new NXSL_Value(argv[i]);
		         }
		         else
		         {
			         ppArgs = NULL;
		         }

               if (vm->run(argc - optind - 1, ppArgs, NULL, NULL, NULL, (entryPoint[0] != 0) ? entryPoint : NULL))
		         {
			         NXSL_Value *result = vm->getResult();
			         if (printResult)
                     WriteToTerminalEx(_T("Result = %s\n"), (result != NULL) ? result->getValueAsCString() : _T("(null)"));
		         }
		         else
		         {
			         WriteToTerminalEx(_T("%s\n"), vm->getErrorText());
		         }
		         safe_free(ppArgs);
            }
         }
         delete vm;
      }
      delete pScript;
	}
	else
	{
		WriteToTerminalEx(_T("%s\n"), szError);
	}
   return 0;
}
