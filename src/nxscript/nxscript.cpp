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

#include "nxscript.h"


static NXSL_TestClass *m_pTestClass;


int F_new(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	TCHAR *buffer = (TCHAR *)malloc(1024);
	_tcscpy(buffer, _T("test value"));
   *ppResult = new NXSL_Value(new NXSL_Object(m_pTestClass, buffer));
   return 0;
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   TCHAR *pszSource, szError[1024], entryPoint[256] = _T("");
   UINT32 dwSize;
   NXSL_Program *pScript;
   NXSL_Environment *pEnv;
   NXSL_Value **ppArgs;
   NXSL_ExtFunction func;
   int i, ch;
   bool dump = false, printResult = false;

   func.m_iNumArgs = 0;
   func.m_pfHandler = F_new;
   _tcscpy(func.m_szName, _T("new"));

   m_pTestClass = new NXSL_TestClass;

   WriteToTerminal(_T("NetXMS Scripting Host  Version \x1b[1m") NETXMS_VERSION_STRING _T("\x1b[0m\n")
                   _T("Copyright (c) 2005-2013 Victor Kirhenshtein\n\n"));

   // Parse command line
   opterr = 1;
	while((ch = getopt(argc, argv, "de:ru")) != -1)
   {
      switch(ch)
      {
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
      printf("Usage: nxscript [options] script [arg1 [... argN]]\n\n"
             "Valid options are:\n"
             "   -d         Dump compiled script code\n"
				 "   -e <name>  Entry point\n"
             "   -r         Print script return value\n"
             "\n");
      return 127;
   }

#ifdef UNICODE
	WCHAR *ucName = WideStringFromMBString(argv[optind]);
   pszSource = NXSLLoadFile(ucName, &dwSize);
	free(ucName);
#else
   pszSource = NXSLLoadFile(argv[optind], &dwSize);
#endif
	if (pszSource != NULL)
	{
		pScript = NXSLCompile(pszSource, szError, 1024);
		free(pszSource);
		if (pScript != NULL)
		{
			if (dump)
				pScript->dump(stdout);
			pEnv = new NXSL_Environment;
			pEnv->registerFunctionSet(1, &func);

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

			if (pScript->run(pEnv, argc - optind - 1, ppArgs, NULL, NULL, NULL, (entryPoint[0] != 0) ? entryPoint : NULL) == 0)
			{
				NXSL_Value *result = pScript->getResult();
				if (printResult)
               WriteToTerminalEx(_T("Result = %s\n"), (result != NULL) ? result->getValueAsCString() : _T("(null)"));
			}
			else
			{
				WriteToTerminalEx(_T("%s\n"), pScript->getErrorText());
			}
			delete pScript;
			safe_free(ppArgs);
		}
		else
		{
			WriteToTerminalEx(_T("%s\n"), szError);
		}
	}
	else
	{
		printf("Error: cannot load input file \"%s\"\n", argv[optind]);
	}
   return 0;
}
