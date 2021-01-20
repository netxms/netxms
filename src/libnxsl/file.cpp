/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2005-2021 Raden Solutions
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
** File: file.cpp
**
**/

#include "libnxsl.h"

/**
 * Check file access.
 * Parameters:
 *   1) file name
 *   2) desired access
 */
int F_FileAccess(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

	if (!argv[1]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	*result = vm->createValue(_taccess(argv[0]->getValueAsCString(), argv[1]->getValueAsInt32()) == 0);
	return 0;
}

/**
 * Copy file
 * Parameters:
 *   1) source file name
 *   2) destination file name
 */
int F_CopyFile(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	*result = vm->createValue(CopyFileOrDirectory(argv[0]->getValueAsCString(), argv[1]->getValueAsCString()));
	return 0;
}

/**
 * Rename file or directory
 * Parameters:
 *   1) old file name
 *   2) new file name
 */
int F_RenameFile(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	*result = vm->createValue(_trename(argv[0]->getValueAsCString(), argv[1]->getValueAsCString()) == 0);
	return 0;
}

/**
 * Delete file.
 * Parameters:
 *   1) file name
 */
int F_DeleteFile(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

	*result = vm->createValue(_tremove(argv[0]->getValueAsCString()) == 0);
	return 0;
}

/**
 * Create directory
 * Parameters:
 *   1) directory name
 */
int F_CreateDirectory(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

#ifdef _WIN32
	*result = vm->createValue(CreateDirectory(argv[0]->getValueAsCString(), nullptr));
#else
	*result = vm->createValue(_tmkdir(argv[0]->getValueAsCString(), 0755) == 0);
#endif
	return 0;
}

/**
 * Remove directory
 * Parameters:
 *   1) directory name
 */
int F_RemoveDirectory(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

#ifdef _WIN32
   *result = vm->createValue(RemoveDirectory(argv[0]->getValueAsCString()));
#else
	*result = vm->createValue(_trmdir(argv[0]->getValueAsCString()) == 0);
#endif
	return 0;
}
