/*
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2025 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: tools.cpp
**/

#include "libnxsl.h"

/**
 * Interface to compiler
 */
NXSL_Program LIBNXSL_EXPORTABLE *NXSLCompile(const TCHAR *source, NXSL_Environment *env, NXSL_CompilationDiagnostic *diag)
{
   NXSL_Compiler compiler;
   NXSL_Program *output = compiler.compile(source, env, &diag->warnings);
   if (output == nullptr)
   {
      diag->errorLineNumber = compiler.getErrorLineNumber();
      diag->errorText = compiler.getErrorText();
   }
   return output;
}

/**
 * Compile script and create VM
 */
NXSL_VM LIBNXSL_EXPORTABLE *NXSLCompileAndCreateVM(const TCHAR *source, NXSL_Environment *env, NXSL_CompilationDiagnostic *diag)
{
   NXSL_Program *p = NXSLCompile(source, env, diag);
   if (p == nullptr)
   {
      delete env;
      return nullptr;
   }

   NXSL_VM *vm = new NXSL_VM(env);
   if (!vm->load(p))
   {
      diag->errorText = vm->getErrorText();
      delete vm;
      vm = nullptr;
   }
   delete p;
   return vm;
}

/**
 * Convert given script to version 5
 */
StringBuffer LIBNXSL_EXPORTABLE NXSLConvertToV5(const TCHAR *source)
{
   NXSL_Compiler compiler;
   return compiler.convertToV5(source);
}

/**
 * Load NXSL source file into memory
 */
TCHAR LIBNXSL_EXPORTABLE *NXSLLoadFile(const TCHAR *fileName)
{
   char *content = LoadFileAsUTF8String(fileName);
   if (content == nullptr)
      return nullptr;
#ifdef UNICODE
	WCHAR *wContent = WideStringFromUTF8String(content);
	MemFree(content);
	return wContent;
#else
   return content;
#endif
}

/**
 * Get text for script VM handle failure reason
 */
const TCHAR *ScriptVMHandle::failureReasonText() const
{
   switch (m_failureReason)
   {
      case ScriptVMFailureReason::SUCCESS:
         return _T("Success");
      case ScriptVMFailureReason::SCRIPT_NOT_FOUND:
         return _T("Script not found");
      case ScriptVMFailureReason::SCRIPT_IS_EMPTY:
         return _T("Script is empty");
      case ScriptVMFailureReason::SCRIPT_LOAD_ERROR:
         return _T("Script load error");
      case ScriptVMFailureReason::SCRIPT_VALIDATION_ERROR:
         return _T("Script validation error");
   }
   return _T("Unknown");
}

/**
 * Convert compilation diagnostic object to JSON
 */
json_t *NXSL_CompilationDiagnostic::toJson() const
{
   json_t *json = json_object();

   json_t *je = json_object();
   json_object_set_new(je, "lineNumber", json_integer(errorLineNumber));
   json_object_set_new(je, "message", json_string_t(errorText));
   json_object_set_new(json, "error", je);

   json_t *jw = json_array();
   for(int i = 0; i < warnings.size(); i++)
   {
      json_array_append_new(jw, warnings.get(i)->toJson());
   }
   json_object_set_new(json, "warnings", jw);

   return json;
}
