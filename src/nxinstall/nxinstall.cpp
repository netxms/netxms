/* 
** NetXMS - Network Management System
** NXSL-based installer tool collection
** Copyright (C) 2005-2024 Victor Kirhenshtein
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
** File: nxinstall.cpp
**
**/

#include "nxinstall.h"
#include <netxms_getopt.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(nxinstall)

/**
 * Trace level
 */
int g_traceLevel = 0;

/**
 * main()
 */
int main(int argc, char *argv[])
{
   TCHAR *pszSource, szError[1024];
   int i, ch;
   bool dump = false, printResult = false, quiet = false;

   InitNetXMSProcess(true);

   // Parse command line
   opterr = 1;
	while((ch = getopt(argc, argv, "drt:q")) != -1)
   {
      switch(ch)
      {
         case 'd':
            dump = true;
            break;
			case 'r':
				printResult = true;
				break;
			case 't':	// Trace level
				g_traceLevel = strtol(optarg, NULL, 0);
				if (g_traceLevel < 0)
				{
					_tprintf(_T("WARNING: Invalid trace level %d, setting to 0\n"), g_traceLevel);
					g_traceLevel = 0;
				}
				else if (g_traceLevel > 9)
				{
					_tprintf(_T("WARNING: Invalid trace level %d, setting to 9\n"), g_traceLevel);
					g_traceLevel = 9;
				}
				break;
			case 'q':
				quiet = true;
				break;
         case '?':
            return 127;
         default:
            break;
      }
   }

	if (!quiet)
	{
		_tprintf(_T("NetXMS Installer Tools  Version ") NETXMS_VERSION_STRING _T("\n")
					_T("Copyright (c) 2005-2024 Victor Kirhenshtein\n\n"));
	}

   if (argc - optind < 1)
   {
      printf("Usage: nxinstall [options] script [arg1 [... argN]]\n\n"
             "Valid options are:\n"
             "   -d         Dump compiled script code\n"
             "   -r         Print script return value\n"
				 "   -t <level> Set trace level (default is 0)\n"
				 "   -q         Quiet mode - suppress version and banner\n"
             "\n");
      return 127;
   }

#ifdef UNICODE
	WCHAR *ucName = WideStringFromMBString(argv[optind]);
   pszSource = NXSLLoadFile(ucName);
	MemFree(ucName);
#else
   pszSource = NXSLLoadFile(argv[optind]);
#endif
	if (pszSource != nullptr)
	{
      NXSL_VM *vm = NXSLCompileAndCreateVM(pszSource, szError, 1024, new NXSL_InstallerEnvironment());
		MemFree(pszSource);
		if (vm != nullptr)
		{
			if (dump)
				vm->dump(stdout);

			// Prepare arguments
         NXSL_Value **ppArgs;
         if (argc - optind > 1)
			{
				ppArgs = MemAllocArray<NXSL_Value*>(argc - optind - 1);
				for(i = optind + 1; i < argc; i++)
					ppArgs[i - optind - 1] = vm->createValue(argv[i]);
			}
			else
			{
				ppArgs = nullptr;
			}

			if (vm->run(argc - optind - 1, ppArgs))
			{
				NXSL_Value *result = vm->getResult();
				if (printResult)
					_tprintf(_T("Result = %s\n"), (result != nullptr) ? result->getValueAsCString() : _T("(null)"));
			}
			else
			{
				_tprintf(_T("%s\n"), vm->getErrorText());
			}
         delete vm;
			MemFree(ppArgs);
		}
		else
		{
			_tprintf(_T("%s\n"), szError);
		}
	}
	else
	{
		_tprintf(_T("Error: cannot load input file \"%hs\"\n"), argv[optind]);
		return 1;
	}
   return 0;
}
