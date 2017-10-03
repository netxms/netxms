/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: swpkg.cpp
**
**/

#include "nxcore.h"

/**
 * Extract value from source table cell
 */
#define EXTRACT_VALUE(name, field) \
	{ \
		if (!_tcsicmp(table->getColumnName(i), _T(name))) \
		{ \
			const TCHAR *value = table->getAsString(row, i); \
			pkg->field = _tcsdup_ex(value); \
			continue; \
		} \
	}

/**
 * Create from agent table
 *
 * @param table table received from agent
 * @param row row number in a table
 * @return new object on success, NULL on parse error
 */
SoftwarePackage *SoftwarePackage::createFromTableRow(const Table *table, int row)
{
   SoftwarePackage *pkg = new SoftwarePackage();
   for(int i = 0; i < table->getNumColumns(); i++)
   {
      EXTRACT_VALUE("NAME", m_name);
      EXTRACT_VALUE("VERSION", m_version);
      EXTRACT_VALUE("VENDOR", m_vendor);
      EXTRACT_VALUE("URL", m_url);
      EXTRACT_VALUE("DESCRIPTION", m_description);

      if (!_tcsicmp(table->getColumnName(i), _T("DATE")))
         pkg->m_date = (time_t)table->getAsInt(row, i);
   }

   if (pkg->m_name == NULL)
   {
      delete pkg;
      return NULL;
   }
   return pkg;
}

/**
 * Constructor
 *
 * @param table table received from agent
 * @param row row number in a table
 */
SoftwarePackage::SoftwarePackage()
{
	m_name = NULL;
	m_version = NULL;
	m_vendor = NULL;
	m_date = 0;
	m_url = NULL;
	m_description = NULL;
}

/**
 * Destructor
 */
SoftwarePackage::~SoftwarePackage()
{
	free(m_name);
	free(m_version);
	free(m_vendor);
	free(m_url);
	free(m_description);
}

/**
 * Fill NXCP message with package data
 */
void SoftwarePackage::fillMessage(NXCPMessage *msg, UINT32 baseId) const
{
	UINT32 varId = baseId;
	msg->setField(varId++, CHECK_NULL_EX(m_name));
	msg->setField(varId++, CHECK_NULL_EX(m_version));
	msg->setField(varId++, CHECK_NULL_EX(m_vendor));
	msg->setField(varId++, (UINT32)m_date);
	msg->setField(varId++, CHECK_NULL_EX(m_url));
	msg->setField(varId++, CHECK_NULL_EX(m_description));
}
