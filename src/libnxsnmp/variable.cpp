/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: variable.cpp
**
**/

#include "libnxsnmp.h"


//
// SNMP_Variable default constructor
//

SNMP_Variable::SNMP_Variable()
{
   m_pName = NULL;
   m_pValue = NULL;
   m_dwType = ASN_NULL;
   m_dwValueLength = 0;
}


//
// SNMP_Variable destructor
//

SNMP_Variable::~SNMP_Variable()
{
   delete m_pName;
   safe_free(m_pValue);
}
