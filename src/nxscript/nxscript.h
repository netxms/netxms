/* 
** NetXMS - Network Management System
** NetXMS Scripting Host
** Copyright (C) 2005-2009 Victor Kirhenshtein
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
** File: nxscript.h
**
**/

#ifndef _nxscript_h_
#define _nxscript_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxsl.h>

#if HAVE_GETOPT_H
#include <getopt.h>
#endif


//
// Test class
//

class NXSL_TestClass : public NXSL_Class
{
public:
   NXSL_TestClass();

   virtual NXSL_Value *getAttr(NXSL_Object *pObject, const TCHAR *pszAttr);
   virtual BOOL setAttr(NXSL_Object *pObject, const TCHAR *pszAttr, NXSL_Value *pValue);
   virtual int callMethod(const TCHAR *name, NXSL_Object *object, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_Program *program);
};

#endif
