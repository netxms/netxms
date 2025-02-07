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
** File: system.cpp
**
**/

#include "nxscript.h"
#include <nxproc.h>

/**
 * Change current directory
 * Parameters:
 *   1) path
 * Returns true on success, false otherwise
 */
int F_SetCurrentDirectory(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

#ifdef _WIN32
   *result = vm->createValue(SetCurrentDirectory(argv[0]->getValueAsCString()) ? true : false);
#else
   *result = vm->createValue(chdir(argv[0]->getValueAsMBString()) == 0);
#endif
   return 0;
}

/**
 * Get current directory
 */
int F_GetCurrentDirectory(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
#ifdef _WIN32
   TCHAR path[MAX_PATH];
   if (GetCurrentDirectory(MAX_PATH, path) > 0)
      *result = vm->createValue(path);
   else
      *result = vm->createValue();
#else
   char path[MAX_PATH];
   if (getcwd(path, MAX_PATH) != nullptr)
      *result = vm->createValue(path);
   else
      *result = vm->createValue();
#endif

   return 0;
}

/**
 * Execute command in a shell.
 * Parameters:
 *   1) command
 */
int F_Execute(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

   ProcessExecutor executor(argv[0]->getValueAsCString());
   int32_t exitCode;
   if (executor.execute())
   {
      executor.waitForCompletion(INFINITE);
      exitCode = executor.getExitCode();
   }
   else
   {
      exitCode = 127;
   }
   *result = vm->createValue(exitCode);
   return 0;
}
