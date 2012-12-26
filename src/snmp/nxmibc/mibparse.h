/* 
** NetXMS - Network Management System
** NetXMS MIB compiler
** Copyright (C) 2005 Victor Kirhenshtein
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

#include <nms_common.h>

#define YYERROR_VERBOSE

#define CREATE(x) (x *)__zmalloc(sizeof(x));


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


//
// Dynamic array structure
//

typedef struct
{
   int nSize;
   void **ppData;
} DynArray;


//
// Object syntax
//

typedef struct mp_syntax
{
   int nSyntax;
   char *pszStr;
	char *pszDescription;
} MP_SYNTAX;


//
// Numeric value
//

typedef struct mp_numeric_value
{
   int nType;
   union
   {
      LONG nInt32;
      INT64 nInt64;
   } value;
} MP_NUMERIC_VALUE;


//
// SUBID structure
//

typedef struct mp_subid
{
   DWORD dwValue;
   char *pszName;
   BOOL bResolved;
} MP_SUBID;


//
// Object structure
//

typedef struct mp_object
{
   int iType;
   char *pszName;
   char *pszDescription;
	char *pszTextualConvention;
   int iSyntax;
   char *pszDataType;   // For defined types
   int iStatus;
   int iAccess;
   DynArray *pOID;
} MP_OBJECT;


//
// Module structure
//

typedef struct mp_module
{
   char *pszName;
   DynArray *pImportList;
   DynArray *pObjectList;
} MP_MODULE;


//
// Import structure
//

typedef struct mp_import
{
   char *pszName;
   MP_MODULE *pModule;
   DynArray *pSymbols;
   DynArray *pObjects;
} MP_IMPORT_MODULE;


//
// Functions
//

#ifdef __cplusplus
extern "C" {
#endif

void InitStateStack();
MP_MODULE *ParseMIB(char *pszFilename);

DynArray *da_create(void);
void da_add(DynArray *pArray, void *pElement);
void da_join(DynArray *pArray, DynArray *pSource);
void *da_get(DynArray *pArray, int nIndex);
int da_size(DynArray *pArray);
void da_destroy(DynArray *pArray);

void *__zmalloc(unsigned int nSize);

#ifdef __cplusplus
}
#endif


#endif
