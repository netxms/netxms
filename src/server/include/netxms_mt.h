/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: netxms_mt.h
**
**/

#ifndef _netxms_mt_h_
#define _netxms_mt_h_

/**
 * Mapping table flags
 */
#define MTF_NUMERIC_KEYS	0x00000001

/**
 * Mapping table element
 */
class MappingTableElement
{
private:
	TCHAR *m_value;
	TCHAR *m_description;

public:
	MappingTableElement(TCHAR *value, TCHAR *description) { m_value = value; m_description = description; }
	~MappingTableElement() { safe_free(m_value); safe_free(m_description); }

	const TCHAR *getValue() { return CHECK_NULL_EX(m_value); }
	const TCHAR *getDescription() { return CHECK_NULL_EX(m_description); }
};

/**
 * Mapping table
 */
class NXCORE_EXPORTABLE MappingTable
{
private:
	LONG m_id;
	TCHAR *m_name;
	UINT32 m_flags;
	TCHAR *m_description;
	StringObjectMap<MappingTableElement> *m_data;

	MappingTable(LONG id, TCHAR *name, UINT32 flags, TCHAR *description);

public:
	~MappingTable();

	static MappingTable *createFromMessage(CSCPMessage *msg);
	static MappingTable *createFromDatabase(LONG id);

	bool saveToDatabase();
	bool deleteFromDatabase();
	void fillMessage(CSCPMessage *msg);

	void createUniqueId() { m_id = CreateUniqueId(IDG_MAPPING_TABLE); }

	const TCHAR *get(const TCHAR *key);
	LONG getId() { return m_id; }
	const TCHAR *getName() { return CHECK_NULL(m_name); }
	const TCHAR *getDescription() { return CHECK_NULL_EX(m_description); }
	UINT32 getFlags() { return m_flags; }
};

/**
 * Mapping tables API
 */
void InitMappingTables();
UINT32 UpdateMappingTable(CSCPMessage *msg, LONG *newId);
UINT32 DeleteMappingTable(LONG id);
UINT32 GetMappingTable(LONG id, CSCPMessage *msg);
UINT32 ListMappingTables(CSCPMessage *msg);

#endif
