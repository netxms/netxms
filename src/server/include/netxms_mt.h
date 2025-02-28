/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
	~MappingTableElement() { MemFree(m_value); MemFree(m_description); }

	const TCHAR *getValue() const { return CHECK_NULL_EX(m_value); }
	const TCHAR *getDescription() const { return CHECK_NULL_EX(m_description); }
};

#ifdef _WIN32
template class NXCORE_TEMPLATE_EXPORTABLE StringObjectMap<MappingTableElement>;
#endif

/**
 * Mapping table
 */
class NXCORE_EXPORTABLE MappingTable
{
private:
	uint32_t m_id;
	TCHAR *m_name;
	uint32_t m_flags;
	TCHAR *m_description;
	StringObjectMap<MappingTableElement> m_data;

	MappingTable(int32_t id, TCHAR *name, uint32_t flags, TCHAR *description);

public:
	~MappingTable();

	static MappingTable *createFromMessage(const NXCPMessage& msg);
	static MappingTable *createFromDatabase(DB_HANDLE hdb, uint32_t id);

   void createUniqueId() { m_id = CreateUniqueId(IDG_MAPPING_TABLE); }

   void fillMessage(NXCPMessage *msg) const;

	bool saveToDatabase() const;
	bool deleteFromDatabase();

	const TCHAR *get(const TCHAR *key) const
	{
	   MappingTableElement *e = m_data.get(key);
	   return (e != nullptr) ? e->getValue() : nullptr;
	}

	uint32_t getId() const { return m_id; }
	const TCHAR *getName() const { return CHECK_NULL(m_name); }
	const TCHAR *getDescription() const { return CHECK_NULL_EX(m_description); }
	uint32_t getFlags() const { return m_flags; }

	json_t *toJson() const;

	NXSL_Value *getKeysForNXSL(NXSL_VM *vm) const;
};

/**
 * Mapping tables API
 */
void InitMappingTables();
uint32_t UpdateMappingTable(const NXCPMessage& msg, uint32_t *newId, ClientSession *session);
uint32_t DeleteMappingTable(uint32_t id, ClientSession *session);
uint32_t GetMappingTable(uint32_t id, NXCPMessage *msg);
uint32_t ListMappingTables(NXCPMessage *msg);

#endif
