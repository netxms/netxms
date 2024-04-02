/* 
** NetXMS - Network Management System
** NetXMS MIB compiler
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
** File: mibparse.h
**
**/

#ifndef _mibparse_h_
#define _mibparse_h_

#include <nms_util.h>

#define YYERROR_VERBOSE

//
// Internal object types
//

#define MIBC_UNKNOWN                0
#define MIBC_OBJECT                 1
#define MIBC_MACRO                  2
#define MIBC_TEXTUAL_CONVENTION     3
#define MIBC_TYPEDEF                4
#define MIBC_CHOICE                 5
#define MIBC_SEQUENCE               6
#define MIBC_VALUE                  7

/**
 * Object syntax
 */
class MP_SYNTAX
{
public:
   int nSyntax;
   char *pszStr;
	char *pszDescription;

   MP_SYNTAX()
   {
      nSyntax = 0;
      pszStr = nullptr;
	   pszDescription = nullptr;
   }

   ~MP_SYNTAX()
   {
      MemFree(pszStr);
      MemFree(pszDescription);
   }
};

/**
 * Numeric value
 */
typedef struct mp_numeric_value
{
   int nType;
   union
   {
      int32_t nInt32;
      int64_t nInt64;
   } value;
} MP_NUMERIC_VALUE;

/**
 * SUBID structure
 */
class MP_SUBID
{
public:
   uint32_t dwValue;
   char *pszName;
   BOOL bResolved;

   MP_SUBID()
   {
      dwValue = 0;
      pszName = NULL;
      bResolved = FALSE;
   }

   MP_SUBID(MP_SUBID *src)
   {
      dwValue = src->dwValue;
      pszName = MemCopyStringA(src->pszName);
      bResolved = src->bResolved;
   }

   ~MP_SUBID()
   {
      MemFree(pszName);
   }
};

/**
 * Object structure
 */
class MP_OBJECT
{
public:
   int iType;
   char *pszName;
   char *pszDescription;
   char *pszTextualConvention;
   int iSyntax;
   char *pszDataType;   // For defined types
   int iStatus;
   int iAccess;
   ObjectArray<MP_SUBID> *oid;
   Array *index;  // List of index fields
   bool valid;

   MP_OBJECT()
   {
      iType = 0;
      iSyntax = 0;
      iStatus = 0;
      iAccess = 0;
      pszName = nullptr;
      pszDescription = nullptr;
      pszTextualConvention = nullptr;
      pszDataType = nullptr;
      oid = nullptr;
      index = nullptr;
      valid = true;
   }

   ~MP_OBJECT()
   {
      MemFree(pszName);
      MemFree(pszDescription);
      MemFree(pszTextualConvention);
      MemFree(pszDataType);
      delete oid;
      delete index;
   }
};

class MP_MODULE;

/**
 * Import structure
 */
class MP_IMPORT_MODULE
{
public:
   char *name;
   MP_MODULE *module;
   Array *symbols;
   ObjectArray<MP_OBJECT> objects;

   MP_IMPORT_MODULE(char *_name, Array *_symbols) : objects(0, 16, Ownership::False)
   {
      name = _name;
      module = nullptr;
      symbols = _symbols;
   }

   ~MP_IMPORT_MODULE()
   {
      MemFree(name);
      delete symbols;
   }
};

/**
 * Module structure
 */
class MP_MODULE
{
public:
   char *pszName;
   ObjectArray<MP_IMPORT_MODULE> *pImportList;
   ObjectArray<MP_OBJECT> *pObjectList;

   MP_MODULE()
   {
      pszName = NULL;
      pImportList = new ObjectArray<MP_IMPORT_MODULE>(0, 8, Ownership::True);
      pObjectList = new ObjectArray<MP_OBJECT>(0, 16, Ownership::True);
   }

   ~MP_MODULE()
   {
      MemFree(pszName);
      delete pImportList;
      delete pObjectList;
   }
};

//
// Functions
//
void InitStateStack();
MP_MODULE *ParseMIB(const TCHAR *fileName);

#endif
