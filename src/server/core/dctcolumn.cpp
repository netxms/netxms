/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: dctcolumn.cpp
**
**/

#include "nxcore.h"


//
// Copy constructor
//

DCTableColumn::DCTableColumn(const DCTableColumn *src)
{
	nx_strncpy(m_name, src->m_name, MAX_COLUMN_NAME);
	m_dataType = src->m_dataType;
	m_snmpOid = (src->m_snmpOid != NULL) ? new SNMP_ObjectId(src->m_snmpOid->getLength(), src->m_snmpOid->getValue()) : NULL;
	setTransformationScript(src->m_scriptSource);
}


//
// Destructor
//

DCTableColumn::~DCTableColumn()
{
	delete m_snmpOid;
	safe_free(m_scriptSource);
	delete m_script;
}


//
// Set new transformation script
//

void DCTableColumn::setTransformationScript(const TCHAR *script)
{
   safe_free(m_scriptSource);
   delete m_script;
   if (script != NULL)
   {
      m_scriptSource = _tcsdup(script);
      StrStrip(m_scriptSource);
      if (m_scriptSource[0] != 0)
      {
			/* TODO: add compilation error handling */
         m_script = (NXSL_Program *)NXSLCompile(m_scriptSource, NULL, 0);
      }
      else
      {
         m_script = NULL;
      }
   }
   else
   {
      m_scriptSource = NULL;
      m_script = NULL;
   }
}
