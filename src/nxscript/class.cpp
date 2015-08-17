/* 
** NetXMS - Network Management System
** NetXMS Scripting Host
** Copyright (C) 2005-2013 Victor Kirhenshtein
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
** File: class.cpp
**
**/

#include "nxscript.h"

NXSL_METHOD_DEFINITION(debug)
{
   *result = new NXSL_Value(_T("Sample debug output"));
   return 0;
}

NXSL_METHOD_DEFINITION(quote)
{
   TCHAR buffer[4096];
   _sntprintf(buffer, 4096, _T("\"%s\""), argv[0]->getValueAsCString());
   *result = new NXSL_Value(buffer);
   return 0;
}

NXSL_TestClass::NXSL_TestClass() : NXSL_Class()
{
   _tcscpy(m_name, _T("TEST"));
   NXSL_REGISTER_METHOD(debug, 0);
   NXSL_REGISTER_METHOD(quote, 1);
}

NXSL_Value *NXSL_TestClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   NXSL_Value *pValue = NULL;

   if (!_tcscmp(pszAttr, _T("name")))
   {
      pValue = new NXSL_Value(_T("Demo Object"));
   }
   else if (!_tcscmp(pszAttr, _T("value")))
   {
		pValue = new NXSL_Value((TCHAR *)pObject->getData());
   }
   return pValue;
}

BOOL NXSL_TestClass::setAttr(NXSL_Object *pObject, const TCHAR *pszAttr, NXSL_Value *pValue)
{
   if (!_tcscmp(pszAttr, _T("value")))
   {
		_tcscpy((TCHAR *)pObject->getData(), CHECK_NULL(pValue->getValueAsCString()));
		return TRUE;
   }
   return FALSE;
}
