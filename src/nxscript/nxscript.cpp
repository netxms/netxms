/* $Id$ */
/* 
** NetXMS - Network Management System
** NetXMS Scripting Host
** Copyright (C) 2005, 2006 Victor Kirhenshtein
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


int F_new(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   *ppResult = new NXSL_Value(new NXSL_Object(m_pTestClass, NULL));
   return 0;
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
   NXSL_Value **ppArgs;
   NXSL_ExtFunction func;
   int i, ch;
   bool dump = false, printResult = false;

   func.m_iNumArgs = 0;
   func.m_pfHandler = F_new;
   strcpy(func.m_szName, "new");

   m_pTestClass = new NXSL_TestClass;

   printf("NetXMS Scripting Host  Version " NETXMS_VERSION_STRING "\n"
          "Copyright (c) 2005-2009 Victor Kirhenshtein\n\n");

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "dr")) != -1)
   {
      switch(ch)
      {
         case 'd':
            dump = true;
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
             "   -d Dump compiled script code\n"
             "   -r Print script return value\n"
             "\n");
      return 127;
   }

   pszSource = NXSLLoadFile(argv[optind], &dwSize);
	if (pszSource != NULL)
	{
		pScript = NXSLCompile(pszSource, szError, 1024);
		free(pszSource);
		if (pScript != NULL)
		{
			if (dump)
				pScript->Dump(stdout);
			pEnv = new NXSL_Environment;
			pEnv->SetIO(stdin, stdout);
			pEnv->RegisterFunctionSet(1, &func);

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

			if (pScript->Run(pEnv, argc - optind - 1, ppArgs) == 0)
			{
				NXSL_Value *result = pScript->GetResult();
				if (printResult)
					printf("Result = %s\n", (result != NULL) ? result->GetValueAsCString() : "(null)");
			}
			else
			{
				printf("%s\n", pScript->GetErrorText());
			}
			delete pScript;
			safe_free(ppArgs);
		}
		else
		{
			printf("%s\n", szError);
		}
	}
	else
	{
		printf("Error: cannot load input file \"%s\"\n", argv[optind]);
	}
   return 0;
}
