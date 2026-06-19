/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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

/**
 * Copy constructor
 */
DCTableColumn::DCTableColumn(const DCTableColumn *src) : m_snmpOid(src->m_snmpOid)
{
	_tcslcpy(m_name, src->m_name, MAX_COLUMN_NAME);
	m_flags = src->m_flags;
   m_displayName = MemCopyString(src->m_displayName);
}

/**
 * Create column object from NXCP message
 */
DCTableColumn::DCTableColumn(const NXCPMessage& msg, uint32_t baseId)
{
	msg.getFieldAsString(baseId, m_name, MAX_COLUMN_NAME);
	m_flags = msg.getFieldAsUInt16(baseId + 1);
   m_displayName = msg.getFieldAsString(baseId + 3);

   if (msg.isFieldExist(baseId + 2))
	{
      uint32_t oid[256];
		size_t len = msg.getFieldAsInt32Array(baseId + 2, 256, oid);
		if (len > 0)
		{
			m_snmpOid = SNMP_ObjectId(oid, len);
		}
	}
}

/**
 * Create column object from database result set
 * Expected field order is following:
 *    column_name,flags,snmp_oid,display_name
 */
DCTableColumn::DCTableColumn(DB_RESULT hResult, int row)
{
	DBGetField(hResult, row, 0, m_name, MAX_COLUMN_NAME);
	m_flags = static_cast<uint16_t>(DBGetFieldULong(hResult, row, 1));
   m_displayName = DBGetField(hResult, row, 3, nullptr, 0);

	TCHAR oid[1024];
	oid[0] = 0;
	DBGetField(hResult, row, 2, oid, 1024);
	Trim(oid);
   m_snmpOid = SNMP_ObjectId::parse(oid);
}

/**
 * Create from NXMP record
 */
DCTableColumn::DCTableColumn(ConfigEntry *e)
{
   _tcslcpy(m_name, e->getSubEntryValue(_T("name"), 0, _T("")), MAX_COLUMN_NAME);
   m_flags = (UINT16)e->getSubEntryValueAsUInt(_T("flags"));
   m_displayName = MemCopyString(e->getSubEntryValue(_T("displayName"), 0, _T("")));

   const TCHAR *oid = e->getSubEntryValue(_T("snmpOid"));
   if ((oid != nullptr) && (*oid != 0))
   {
      m_snmpOid = SNMP_ObjectId::parse(oid);
   }
}

/**
 * Destructor
 */

/**
 * Create from JSON record
 */
DCTableColumn::DCTableColumn(json_t *json)
{
   String name = json_object_get_string(json, "name", _T(""));
   _tcslcpy(m_name, name, MAX_COLUMN_NAME);

   m_flags = static_cast<uint16_t>(json_object_get_int32(json, "flags"));

   // Allow symbolic overrides of the packed flag fields (REST API)
   int dataType = TCF_GET_DATA_TYPE(m_flags);
   if (json_object_update_enum(json, "dataType", g_dciDataTypeNames, &dataType))
      m_flags = static_cast<uint16_t>(TCF_SET_DATA_TYPE(m_flags, dataType));
   int aggregation = TCF_GET_AGGREGATION_FUNCTION(m_flags);
   if (json_object_update_enum(json, "aggregationFunction", g_dctColumnAggregationNames, &aggregation))
      m_flags = static_cast<uint16_t>((m_flags & ~TCF_AGGREGATE_FUNCTION_MASK) | ((aggregation & 0x07) << 4));
   json_t *flagValue = json_object_get(json, "instanceColumn");
   if (json_is_boolean(flagValue))
      m_flags = json_is_true(flagValue) ? (m_flags | TCF_INSTANCE_COLUMN) : (m_flags & ~TCF_INSTANCE_COLUMN);
   flagValue = json_object_get(json, "convertSnmpStringToHex");
   if (json_is_boolean(flagValue))
      m_flags = json_is_true(flagValue) ? (m_flags | TCF_SNMP_HEX_STRING) : (m_flags & ~TCF_SNMP_HEX_STRING);

   String displayName = json_object_get_string(json, "displayName", _T(""));
   m_displayName = MemCopyString(displayName);

   String oidStr = json_object_get_string(json, "snmpOid", _T(""));
   if (!oidStr.isEmpty())
   {
      m_snmpOid = SNMP_ObjectId::parse(oidStr);
   }
}

DCTableColumn::~DCTableColumn()
{
   MemFree(m_displayName);
}

/**
 * Fill NXCP message
 */
void DCTableColumn::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   msg->setField(baseId, m_name);
   msg->setField(baseId + 1, m_flags);
   if (m_snmpOid.isValid())
      msg->setFieldFromInt32Array(baseId + 2, m_snmpOid.length(), m_snmpOid.value());
   msg->setField(baseId + 3, m_displayName);
}

/**
 * Create NXMP record
 */
/**
 * Serialize to JSON
 */
json_t *DCTableColumn::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "displayName", json_string_t(m_displayName));
   json_object_set_new(root, "snmpOid", m_snmpOid.isValid() ? json_string_t(m_snmpOid.toString()) : json_null());
   json_object_set_new(root, "dataType", json_string_w(CodeToText(TCF_GET_DATA_TYPE(m_flags), g_dciDataTypeNames, L"int32")));
   json_object_set_new(root, "aggregationFunction", json_string_w(CodeToText(TCF_GET_AGGREGATION_FUNCTION(m_flags), g_dctColumnAggregationNames, L"last")));
   json_object_set_new(root, "instanceColumn", json_boolean((m_flags & TCF_INSTANCE_COLUMN) != 0));
   json_object_set_new(root, "convertSnmpStringToHex", json_boolean((m_flags & TCF_SNMP_HEX_STRING) != 0));
   json_object_set_new(root, "flags", json_integer(m_flags));
   return root;
}

/**
 * Create export record
 */
json_t *DCTableColumn::createExportRecord() const
{
   return toJson();
}
