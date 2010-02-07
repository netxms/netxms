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
** $module: class.cpp
**
**/

#include "nxscript.h"


NXSL_TestClass::NXSL_TestClass()
               :NXSL_Class()
{
   strcpy(m_szName, "TEST");
}

NXSL_Value *NXSL_TestClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   NXSL_Value *pValue = NULL;

   if (!strcmp(pszAttr, "name"))
   {
      pValue = new NXSL_Value("Demo Object");
   }
   else if (!strcmp(pszAttr, "value"))
   {
		pValue = new NXSL_Value((char *)pObject->getData());
   }
   return pValue;
}

BOOL NXSL_TestClass::setAttr(NXSL_Object *pObject, const TCHAR *pszAttr, NXSL_Value *pValue)
{
   if (!strcmp(pszAttr, "value"))
   {
		_tcscpy((char *)pObject->getData(), CHECK_NULL(pValue->getValueAsCString()));
		return TRUE;
   }
   return FALSE;
}
