/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2005-2022 Raden Solutions
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

#include "libnxsl.h"

/**
 * "FILE" class
 */
class NXSL_FileClass : public NXSL_Class
{
public:
   NXSL_FileClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * File metadata
 */
struct NXSL_FileHandle
{
   FILE *handle;
   TCHAR *name;
   bool closed;

   NXSL_FileHandle(const TCHAR *n, FILE *h)
   {
      name = MemCopyString(n);
      handle = h;
      closed = false;
   }

   ~NXSL_FileHandle()
   {
      MemFree(name);
      if (!closed)
         fclose(handle);
   }
};

/**
 * "FILE" class instance
 */
static NXSL_FileClass s_nxslFileClass;

/**
 * Method FILE::close
 */
NXSL_METHOD_DEFINITION(FILE, close)
{
   NXSL_FileHandle *f = static_cast<NXSL_FileHandle*>(object->getData());
   if (!f->closed)
   {
      fclose(f->handle);
      f->closed = true;
   }
   *result = vm->createValue();
   return 0;
}

/**
 * Method FILE::read
 */
NXSL_METHOD_DEFINITION(FILE, read)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   NXSL_FileHandle *f = static_cast<NXSL_FileHandle*>(object->getData());
   if (!f->closed && !feof(f->handle))
   {
      int count = argv[0]->getValueAsInt32();
      if (count > 0)
      {
         char sbuffer[1024];
         char *buffer = (count > 1023) ? (char *)MemAlloc(count + 1) : sbuffer;

         int bytes = (int)fread(buffer, 1, count, f->handle);
         if (bytes > 0)
         {
            buffer[count] = 0;
            *result = vm->createValue(buffer);
         }
         else
         {
            *result = vm->createValue();
         }

         if (buffer != sbuffer)
            MemFree(buffer);
      }
      else
      {
         *result = vm->createValue(_T(""));
      }
   }
   else
   {
      *result = vm->createValue();
   }
   return 0;
}

/**
 * Method FILE::readLine
 */
NXSL_METHOD_DEFINITION(FILE, readLine)
{
   NXSL_FileHandle *f = static_cast<NXSL_FileHandle*>(object->getData());
   if (!f->closed && !feof(f->handle))
   {
      TCHAR buffer[8192] = _T("");
      if (_fgetts(buffer, 8192, f->handle) != NULL)
      {
         TCHAR *ptr = _tcschr(buffer, _T('\n'));
         if (ptr != NULL)
            *ptr = 0;
         *result = vm->createValue(buffer);
      }
      else
      {
         *result = vm->createValue();
      }
   }
   else
   {
      *result = vm->createValue();
   }
   return 0;
}

/**
 * Method FILE::write
 */
NXSL_METHOD_DEFINITION(FILE, write)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   const TCHAR *s = argv[0]->getValueAsCString();
   size_t slen = _tcslen(s);
   if (slen > 0)
   {
      NXSL_FileHandle *f = static_cast<NXSL_FileHandle*>(object->getData());
      if (!f->closed)
      {
#ifdef UNICODE
         char *data = MBStringFromWideStringSysLocale(s);
         fwrite(data, 1, strlen(data), f->handle);
         free(data);
#else
         fwrite(s, 1, slen, f->handle);
#endif
      }
   }
   *result = vm->createValue();
   return 0;
}

/**
 * Method FILE::writeLine
 */
NXSL_METHOD_DEFINITION(FILE, writeLine)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   const TCHAR *s = argv[0]->getValueAsCString();
   size_t slen = _tcslen(s);
   if (slen > 0)
   {
      NXSL_FileHandle *f = static_cast<NXSL_FileHandle*>(object->getData());
      if (!f->closed)
      {
#ifdef UNICODE
         char *data = MBStringFromWideStringSysLocale(s);
         fwrite(data, 1, strlen(data), f->handle);
         free(data);
#else
         fwrite(s, 1, slen, f->handle);
#endif
         fputc('\n', f->handle);
      }
   }
   *result = vm->createValue();
   return 0;
}

/**
 * Implementation of "FILE" class
 */
NXSL_FileClass::NXSL_FileClass() : NXSL_Class()
{
   setName(_T("FILE"));
   NXSL_REGISTER_METHOD(FILE, close, 0);
   NXSL_REGISTER_METHOD(FILE, read, 1);
   NXSL_REGISTER_METHOD(FILE, readLine, 0);
   NXSL_REGISTER_METHOD(FILE, write, 1);
   NXSL_REGISTER_METHOD(FILE, writeLine, 1);
}

/**
 * Get attributes
 */
NXSL_Value *NXSL_FileClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_FileHandle *f = static_cast<NXSL_FileHandle*>(object->getData());
	if (compareAttributeName(attr, "eof"))
	{
		value = object->vm()->createValue((INT32)feof(f->handle));
	}
	else if (compareAttributeName(attr, "name"))
   {
      value = object->vm()->createValue(f->name);
   }
	return value;
}

/**
 * File object destructor
 */
void NXSL_FileClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<NXSL_FileHandle*>(object->getData());
}

/**
 * Open file. Returns FILE object or null.
 * Parameters:
 *   1) file name
 *   2) mode (optional, default "r")
 */
int F_OpenFile(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
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

	FILE *handle = _tfopen(argv[0]->getValueAsCString(), mode);
	if (handle != NULL)
	{
		*ppResult = vm->createValue(vm->createObject(&s_nxslFileClass, new NXSL_FileHandle(argv[0]->getValueAsCString(), handle)));
	}
	else
	{
		*ppResult = vm->createValue();
	}
	return 0;
}
