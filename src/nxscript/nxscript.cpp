/* 
** NetXMS - Network Management System
** NetXMS Scripting Host
** Copyright (C) 2005-2025 Victor Kirhenshtein
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
#include <netxms_getopt.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(nxscript)

int F_SetCurrentDirectory(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_GetCurrentDirectory(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_Execute(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

/**
 * Additional NXSL functions for OS-level operations
 */
static NXSL_ExtFunction s_nxslSystemFunctions[] =
{
   { "Execute", F_Execute, 1 },
   { "GetCurrentDirectory", F_GetCurrentDirectory, 0 },
   { "SetCurrentDirectory", F_SetCurrentDirectory , 1 }
};
static NXSL_ExtModule s_nxslSystemModule = { "OS", sizeof(s_nxslSystemFunctions) / sizeof(NXSL_ExtFunction), s_nxslSystemFunctions };

/**
 * Environment for external scripts
 */
class NXSL_ExternalScriptEnv : public NXSL_Environment
{
public:
   NXSL_ExternalScriptEnv() : NXSL_Environment()
   {
      registerIOFunctions();
      registerModuleSet(1, &s_nxslSystemModule);
   }

   virtual void configureVM(NXSL_VM *vm) override;
   virtual void trace(int level, const TCHAR *text) override;
};

/**
 * Configure VM
 */
void NXSL_ExternalScriptEnv::configureVM(NXSL_VM *vm)
{
   NXSL_Environment::configureVM(vm);
   vm->setGlobalVariable("$nxscript", vm->createValue(NETXMS_VERSION_STRING));
}

/**
 * Trace output
 */
void NXSL_ExternalScriptEnv::trace(int level, const TCHAR *text)
{
   WriteToTerminalEx(_T("\x1b[30;1m[%d] %s\x1b[0m\n"), level, CHECK_NULL(text));
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   TCHAR errorMessage[1024];
   const char *entryPoint = nullptr;
   char outFile[MAX_PATH] = "";
   NXSL_Program *pScript;
   NXSL_Environment *pEnv;
   NXSL_Value **ppArgs;
   int i, ch;
   bool dump = false, printResult = false, compileOnly = false, binary = false, showExprVars = false, showMemoryUsage = false, showMetadata = false, instructionTrace = false, showRequiredModules = false, convertToV5 = false;
   int runCount = 1, rc = 0;

   InitNetXMSProcess(true);

   WriteToTerminal(_T("NetXMS Scripting Host  Version \x1b[1m") NETXMS_VERSION_STRING _T("\x1b[0m\n")
                   _T("Copyright (c) 2005-2025 Victor Kirhenshtein\n\n"));

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "5bcC:de:EmMo:rRt")) != -1)
   {
      switch(ch)
      {
         case '5':
            convertToV5 = true;
            break;
         case 'b':
            binary = true;
            break;
         case 'c':
            compileOnly = true;
            break;
         case 'C':
            runCount = strtol(optarg, nullptr, 0);
            break;
         case 'd':
            dump = true;
            break;
         case 'e':
            entryPoint = optarg;
            break;
			case 'E':
				showExprVars = true;
				break;
         case 'm':
            showMemoryUsage = true;
            break;
         case 'M':
            showMetadata = true;
            break;
         case 'o':
				strlcpy(outFile, optarg, MAX_PATH);
            break;
			case 'r':
				printResult = true;
				break;
         case 'R':
            showRequiredModules = true;
            break;
         case 't':
            instructionTrace = true;
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
               _T("   -5         Convert given script to NXSL version 5\n")
               _T("   -b         Input is a binary file\n")
               _T("   -c         Compile only\n")
               _T("   -C <count> Run script multiple times\n")
               _T("   -d         Dump compiled script code\n")
				   _T("   -e <name>  Entry point\n")
               _T("   -E         Show expression variables on exit\n")
               _T("   -m         Show memory usage information\n")
               _T("   -M         Show program metadata\n")
               _T("   -o <file>  Write compiled script\n")
               _T("   -r         Print script return value\n")
               _T("   -R         Show list of required modules\n")
               _T("   -t         Enable instruction trace\n")
               _T("\n"));
      return 127;
   }

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(2, &wsaData);
#endif

   if (convertToV5)
   {
#ifdef UNICODE
      WCHAR ucName[MAX_PATH];
      MultiByteToWideCharSysLocale(argv[optind], ucName, MAX_PATH);
      WCHAR *source = NXSLLoadFile(ucName);
#else
      char *source = NXSLLoadFile(argv[optind]);
#endif
      if (source == nullptr)
      {
         _tprintf(_T("Error: cannot load input file \"%hs\"\n"), argv[optind]);
         return 1;
      }

      StringBuffer output = NXSLConvertToV5(source);
      MemFree(source);
      _putts(output);
      return 0;
   }

   if (binary)
   {
#ifdef UNICODE
	   WCHAR *ucName = WideStringFromMBString(argv[optind]);
      ByteStream *s = ByteStream::load(ucName);
	   MemFree(ucName);
#else
      ByteStream *s = ByteStream::load(argv[optind]);
#endif
      if (s == nullptr)
      {
		   _tprintf(_T("Error: cannot load input file \"%hs\"\n"), argv[optind]);
         return 1;
      }

      pScript = NXSL_Program::load(*s, errorMessage, 1024);
      delete s;
   }
   else
   {
#ifdef UNICODE
      WCHAR ucName[MAX_PATH];
      MultiByteToWideCharSysLocale(argv[optind], ucName, MAX_PATH);
      WCHAR *source = NXSLLoadFile(ucName);
#else
      char *source = NXSLLoadFile(argv[optind]);
#endif
      if (source == nullptr)
	   {
		   _tprintf(_T("Error: cannot load input file \"%hs\"\n"), argv[optind]);
         return 1;
      }

	   NXSL_CompilationDiagnostic diag;
	   NXSL_ExternalScriptEnv env;
		pScript = NXSLCompile(source, &env, &diag);
		MemFree(source);

		for(NXSL_CompilationWarning *w : diag.warnings)
         _tprintf(_T("Compilation warning in line %d: %s\n"), w->lineNumber, w->message.cstr());
		_tcslcpy(errorMessage, diag.errorText, 1024);
   }

	if (pScript != nullptr)
	{
	   if (showMemoryUsage)
	      WriteToTerminalEx(_T("Compiled object memory usage: %u KBytes\n\n"), static_cast<uint32_t>(pScript->getMemoryUsage() / 1024));

      if (showMetadata)
      {
         WriteToTerminal(_T("Program metadata:\n"));
         pScript->getMetadata().forEach(
            [] (const TCHAR *key, const void *value) -> EnumerationCallbackResult
            {
               WriteToTerminalEx(_T("   %s = %s\n"), key, static_cast<const TCHAR*>(value));
               return _CONTINUE;
            });
         WriteToTerminal(_T("\n"));
      }

      if (showRequiredModules)
      {
         WriteToTerminal(_T("Required modules:\n"));
         StringList *modules = pScript->getRequiredModules(true);
         for(int i = 0; i < modules->size(); i++)
            WriteToTerminalEx(_T("   %s\n"), modules->get(i));
         delete modules;
         WriteToTerminal(_T("\n"));
      }

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
            WriteToTerminalEx(_T("ERROR: cannot open output file \"%hs\": %s\n"), outFile, _tcserror(errno));
            rc = 1;
         }
      }

      if (!compileOnly)
      {
		   pEnv = new NXSL_ExternalScriptEnv;
		   pEnv->registerIOFunctions();

         // Create VM
         NXSL_VM *vm = new NXSL_VM(pEnv);
         if (vm->load(pScript))
         {
            if (dump)
               vm->dump(stdout);

            if (instructionTrace)
               vm->setInstructionTraceFile(stdout);

            while(runCount-- > 0)
            {
		         // Prepare arguments
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

		         NXSL_VariableSystem *globalVariables = nullptr;
               NXSL_VariableSystem *expressionVariables = nullptr;
               if (vm->run(argc - optind - 1, ppArgs, &globalVariables, showExprVars ? &expressionVariables : nullptr, nullptr, entryPoint))
		         {
			         NXSL_Value *result = vm->getResult();
			         if (printResult)
                     WriteToTerminalEx(_T("Result = %s\n"), (result != nullptr) ? result->getValueAsCString() : _T("(null)"));
			         if (showExprVars)
			         {
			            WriteToTerminal(_T("Expression variables:\n"));
			            expressionVariables->dump(stdout);
			         }
		         }
		         else
		         {
			         WriteToTerminalEx(_T("%s\n"), vm->getErrorText());
			         rc = 1;
		         }
		         MemFree(ppArgs);
		         delete globalVariables;
		         delete expressionVariables;
            }
         }
         else
         {
            if (dump)
               pScript->dump(stdout);

            WriteToTerminalEx(_T("%s\n"), vm->getErrorText());
            rc = 1;
         }
         delete vm;
      }
      else if (dump)
      {
         pScript->dump(stdout);
      }
      delete pScript;
   }
   else
   {
      WriteToTerminalEx(_T("%s\n"), errorMessage);
      rc = 1;
   }
   return rc;
}
