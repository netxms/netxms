/* 
** NetXMS - Network Management System
** NXSL-based installer tool collection
** Copyright (C) 2005-2011 Victor Kirhenshtein
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
** File: io.cpp
**
**/

#include "nxinstall.h"

/**
 * Implementation of "FILE" class
 */
NXSL_FileClass::NXSL_FileClass() : NXSL_Class()
{
   _tcscpy(m_szName, _T("FILE"));
}

NXSL_Value *NXSL_FileClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
	NXSL_Value *value = NULL;
	if (!_tcscmp(pszAttr, _T("eof")))
	{
		value = new NXSL_Value((LONG)feof((FILE *)pObject->getData()));
	}
	return value;
}

/**
 * Check file access.
 * Parameters:
 *   1) file name
 *   2) desired access
 */
int F_access(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

	if (!argv[1]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	*ppResult = new NXSL_Value((LONG)((_taccess(argv[0]->getValueAsCString(), argv[1]->getValueAsInt32()) == 0) ? 1 : 0));
	return 0;
}

/**
 * Open file. Returns FILE object or null.
 * Parameters:
 *   1) file name
 *   2) mode (optional, default "r")
 */
int F_fopen(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

	if ((argc < 1) || (argc > 2))
		return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	const TCHAR *mode;
	if (argc == 2)
	{
		if (!argv[1]->isString())
			return NXSL_ERR_NOT_STRING;
		mode = argv[1]->getValueAsCString();
	}
	else
	{
		mode = _T("r");
	}

	FILE *file = _tfopen(argv[0]->getValueAsCString(), mode);
	if (file != NULL)
	{
		*ppResult = new NXSL_Value(new NXSL_Object(&g_nxslFileClass, file));
	}
	else
	{
		*ppResult = new NXSL_Value;
	}
	return 0;
}

/**
 * Close file.
 * Parameters:
 *   1) file object
 */
int F_fclose(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;
	
	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslFileClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	FILE *file = (FILE *)object->getData();
	fclose(file);
	*ppResult = new NXSL_Value;
	return 0;
}

/**
 * Check if at EOF.
 * Parameters:
 *   1) file object
 */
int F_feof(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;
	
	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslFileClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	FILE *file = (FILE *)object->getData();
	*ppResult = new NXSL_Value((LONG)feof(file));
	return 0;
}

/**
 * Read line from file. New line character will be stripped off the line.
 * Returns null if EOF reached or read error occured.
 * Parameters:
 *   1) file object
 */
int F_fgets(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;
	
	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslFileClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	FILE *file = (FILE *)object->getData();

	TCHAR buffer[8192] = _T("");
	if (_fgetts(buffer, 8192, file) != NULL)
	{
		TCHAR *ptr = _tcschr(buffer, _T('\n'));
		if (ptr != NULL)
			*ptr = 0;
		*ppResult = new NXSL_Value(buffer);
	}
	else
	{
		*ppResult = new NXSL_Value;
	}
	return 0;
}

/**
 * Write line to file.
 * Parameters:
 *   1) file object
 *   2) text to write
 */
int F_fputs(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;
	
	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslFileClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;
	
	FILE *file = (FILE *)object->getData();
	_fputts(argv[1]->getValueAsCString(), file);
	_fputts(_T("\n"), file);
	*ppResult = new NXSL_Value;
	return 0;
}
