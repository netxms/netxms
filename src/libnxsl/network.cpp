/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2013 Alex Kirhenshtein
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
** File: network.cpp
**
**/

#include "libnxsl.h"

/**
 * Instance of coinnector class definition
 */
NXSL_ConnectorClass LIBNXSL_EXPORTABLE g_nxslConnectorClass;

/**
 * read() method
 */
NXSL_METHOD_DEFINITION(read)
{
   *result = new NXSL_Value;
   return 0;
}

/**
 * Implementation of "Connector" class: constructor
 */
NXSL_ConnectorClass::NXSL_ConnectorClass() : NXSL_Class()
{
   _tcscpy(m_name, _T("Connector"));

   NXSL_REGISTER_METHOD(read, 0);
}

/**
 * Implementation of "Connector" class: destructor
 */
NXSL_ConnectorClass::~NXSL_ConnectorClass()
{
}

/**
 * Object delete
 */
void NXSL_ConnectorClass::onObjectDelete(NXSL_Object *object)
{
}

/**
 * Implementation of "Connector" class: get attribute
 */
NXSL_Value *NXSL_ConnectorClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
   NXSL_Value *value = NULL;
   return value;
}

/**
 * NXSL "Connector" class object
 */
static NXSL_ConnectorClass m_nxslConnectorClass;

/**
 * Create TCP Connector
 */
int F_tcpConnector(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	*ppResult = new NXSL_Value(new NXSL_Object(&m_nxslConnectorClass, NULL));
   return 0;
}

/**
 * Create UDP Connector
 */
int F_udpConnector(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	*ppResult = new NXSL_Value(new NXSL_Object(&m_nxslConnectorClass, NULL));
   return 0;
}

