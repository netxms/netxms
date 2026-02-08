/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
 * Comparator for hardware components
 */
int HardwareComponentComparator(const HardwareComponent **c1, const HardwareComponent **c2)
{
   int rc = static_cast<int>((*c1)->getCategory()) - static_cast<int>((*c2)->getCategory());
   if (rc != 0)
      return rc;
   if ((*c1)->getIndex() != (*c2)->getIndex())
      return (*c1)->getIndex() < (*c2)->getIndex() ? -1 : 1;
   rc = _tcscmp((*c1)->getSerialNumber(), (*c2)->getSerialNumber());
   if (rc != 0)
      return rc;
   rc = _tcscmp((*c1)->getPartNumber(), (*c2)->getPartNumber());
   if (rc != 0)
      return rc;
   rc = _tcscmp((*c1)->getModel(), (*c2)->getModel());
   if (rc != 0)
      return rc;
   rc = _tcscmp((*c1)->getLocation(), (*c2)->getLocation());
   if (rc != 0)
      return rc;
   return _tcscmp((*c1)->getVendor(), (*c2)->getVendor());
}

/**
 * Calculate hardware changes
 */
ObjectArray<HardwareComponent> *CalculateHardwareChanges(ObjectArray<HardwareComponent> *oldSet, ObjectArray<HardwareComponent> *newSet)
{
   HardwareComponent *nc = nullptr, *oc = nullptr;
   int i;
   ObjectArray<HardwareComponent> *changes = new ObjectArray<HardwareComponent>(16, 16);

   for(i = 0; i < newSet->size(); i++)
   {
      nc = newSet->get(i);
      oc = oldSet->bsearch(nc, HardwareComponentComparator);
      if (oc == nullptr)
      {
         nc->setChangeCode(CHANGE_ADDED);
         changes->add(nc);
      }
   }

   for(i = 0; i < oldSet->size(); i++)
   {
      oc = oldSet->get(i);
      nc = newSet->bsearch(oc, HardwareComponentComparator);
      if (nc == nullptr)
      {
         oc->setChangeCode(CHANGE_REMOVED);
         changes->add(oc);
      }
   }

   return changes;
}

/**
 * Create hardware component
 */
HardwareComponent::HardwareComponent(HardwareComponentCategory category, uint32_t index, const wchar_t *type,
         const wchar_t *vendor, const wchar_t *model, const wchar_t *partNumber, const wchar_t *serialNumber)
{
   m_category = category;
   m_index = index;
   m_type = MemCopyString(type);
   m_vendor = MemCopyString(vendor);
   m_model = MemCopyString(model);
   m_location = nullptr;
   m_capacity = 0;
   m_partNumber = MemCopyString(partNumber);
   m_serialNumber = MemCopyString(serialNumber);
   m_description = nullptr;
   m_changeCode = CHANGE_NONE;
}

/**
 * Create hardware component from database
 * Expected field order: category,component_index,hw_type,vendor,model,location,capacity,part_number,serial_number,description
 */
HardwareComponent::HardwareComponent(DB_RESULT result, int row)
{
   m_category = static_cast<HardwareComponentCategory>(DBGetFieldLong(result, row, 0));
   m_index = DBGetFieldULong(result, row, 1);
   m_type = DBGetField(result, row, 2, nullptr, 0);
   m_vendor = DBGetField(result, row, 3, nullptr, 0);
   m_model = DBGetField(result, row, 4, nullptr, 0);
   m_location = DBGetField(result, row, 5, nullptr, 0);
   m_capacity = DBGetFieldUInt64(result, row, 6);
   m_partNumber = DBGetField(result, row, 7, nullptr, 0);
   m_serialNumber = DBGetField(result, row, 8, nullptr, 0);
   m_description = DBGetField(result, row, 9, nullptr, 0);
   m_changeCode = CHANGE_NONE;
}

/**
 * Create hardware component from table row
 */
HardwareComponent::HardwareComponent(HardwareComponentCategory category, const Table& table, int row)
{
   m_category = category;
   m_index = 0;
   m_type = nullptr;
   m_vendor = nullptr;
   m_model = nullptr;
   m_location = nullptr;
   m_capacity = 0;
   m_partNumber = nullptr;
   m_serialNumber = nullptr;
   m_description = nullptr;

   for(int i = 0; i < table.getNumColumns(); i++)
   {
      const wchar_t *cname = table.getColumnName(i);
      if (!_tcsicmp(cname, _T("HANDLE")) ||
          !_tcsicmp(cname, _T("NUMBER")) ||
          !_tcsicmp(cname, _T("INDEX")))
         m_index = table.getAsUInt(row, i);
      else if (!_tcsicmp(cname, _T("LOCATION")))
         m_location = MemCopyString(table.getAsString(row, i));
      else if (!_tcsicmp(cname, _T("MANUFACTURER")))
         m_vendor = MemCopyString(table.getAsString(row, i));
      else if (!_tcsicmp(table.getColumnName(i), _T("NAME")) ||
               !_tcsicmp(table.getColumnName(i), _T("VERSION")) ||
               !_tcsicmp(table.getColumnName(i), _T("FORM_FACTOR")) ||
               !_tcsicmp(table.getColumnName(i), _T("PRODUCT")))
         m_model = MemCopyString(table.getAsString(row, i));
      else if (!_tcsicmp(table.getColumnName(i), _T("PART_NUMBER")))
         m_partNumber = MemCopyString(table.getAsString(row, i));
      else if (!_tcsicmp(table.getColumnName(i), _T("CAPACITY")) ||
               !_tcsicmp(table.getColumnName(i), _T("SPEED")) ||
               !_tcsicmp(table.getColumnName(i), _T("CURR_SPEED")) ||
               !_tcsicmp(table.getColumnName(i), _T("SIZE")))
         m_capacity = table.getAsUInt64(row, i);
      else if (!_tcsicmp(table.getColumnName(i), _T("SERIAL_NUMBER")) ||
               !_tcsicmp(table.getColumnName(i), _T("MAC_ADDRESS")))
         m_serialNumber = MemCopyString(table.getAsString(row, i));
      else if ((!_tcsicmp(table.getColumnName(i), _T("TYPE")) && (category != HWC_STORAGE)) ||
               !_tcsicmp(table.getColumnName(i), _T("TYPE_DESCRIPTION")) ||
               !_tcsicmp(table.getColumnName(i), _T("CHEMISTRY")))
         m_type = MemCopyString(table.getAsString(row, i));
   }

   m_changeCode = CHANGE_NONE;
}

/**
 * Copy constructor
 */
HardwareComponent::HardwareComponent(const HardwareComponent& src)
{
   m_category = src.m_category;
   m_changeCode = src.m_changeCode;
   m_index = src.m_index;
   m_capacity = src.m_capacity;
   m_type = MemCopyString(src.m_type);
   m_vendor = MemCopyString(src.m_vendor);
   m_model = MemCopyString(src.m_model);
   m_partNumber = MemCopyString(src.m_partNumber);
   m_serialNumber = MemCopyString(src.m_serialNumber);
   m_location = MemCopyString(src.m_location);
   m_description = MemCopyString(src.m_description);
}

/**
 * Destructor
 */
HardwareComponent::~HardwareComponent()
{
   MemFree(m_type);
   MemFree(m_vendor);
   MemFree(m_model);
   MemFree(m_partNumber);
   MemFree(m_serialNumber);
   MemFree(m_location);
   MemFree(m_description);
}

/**
 * Fill NXCPMessage
 */
void HardwareComponent::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   uint32_t fieldId = baseId;
   msg->setField(fieldId++, static_cast<int16_t>(m_category));
   msg->setField(fieldId++, m_index);
   msg->setField(fieldId++, CHECK_NULL_EX(m_type));
   msg->setField(fieldId++, CHECK_NULL_EX(m_vendor));
   msg->setField(fieldId++, CHECK_NULL_EX(m_model));
   msg->setField(fieldId++, CHECK_NULL_EX(m_location));
   msg->setField(fieldId++, m_capacity);
   msg->setField(fieldId++, CHECK_NULL_EX(m_partNumber));
   msg->setField(fieldId++, CHECK_NULL_EX(m_serialNumber));
   msg->setField(fieldId++, CHECK_NULL_EX(m_description));
}

/**
 * Serialize to JSON
 */
json_t *HardwareComponent::toJson() const
{
   json_t *root = json_object();

   json_object_set_new(root, "category", json_integer(static_cast<int>(m_category)));
   json_object_set_new(root, "index", json_integer(m_index));
   json_object_set_new(root, "type", json_string_t(CHECK_NULL_EX(m_type)));
   json_object_set_new(root, "vendor", json_string_t(CHECK_NULL_EX(m_vendor)));
   json_object_set_new(root, "model", json_string_t(CHECK_NULL_EX(m_model)));
   json_object_set_new(root, "location", json_string_t(CHECK_NULL_EX(m_location)));
   json_object_set_new(root, "capacity", json_integer(m_capacity));
   json_object_set_new(root, "partNumber", json_string_t(CHECK_NULL_EX(m_partNumber)));
   json_object_set_new(root, "serialNumber", json_string_t(CHECK_NULL_EX(m_serialNumber)));
   json_object_set_new(root, "description", json_string_t(CHECK_NULL_EX(m_description)));

   return root;
}

/**
 * Save to database
 * Field order: node_id,category,component_index,hw_type,vendor,model,location,capacity,part_number,serial_number,description
 */
bool HardwareComponent::saveToDatabase(DB_STATEMENT hStmt) const
{
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_category);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_index);
   DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_type, DB_BIND_STATIC, 47);
   DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_vendor, DB_BIND_STATIC, 127);
   DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_model, DB_BIND_STATIC, 127);
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, m_location, DB_BIND_STATIC, 63);
   DBBind(hStmt, 8, DB_SQLTYPE_BIGINT, m_capacity);
   DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, m_partNumber, DB_BIND_STATIC, 63);
   DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, m_serialNumber, DB_BIND_STATIC, 63);
   DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC, 255);
   return DBExecute(hStmt);
}

/**
 * Get category name
 */
const wchar_t *HardwareComponent::getCategoryName() const
{
   static const wchar_t *names[] = { _T("Other"), _T("Baseboard"), _T("Processor"), _T("Memory"), _T("Storage"), _T("Battery"), _T("NetworkAdapter") };
   uint32_t index = (uint32_t)m_category;
   return names[(index < sizeof(names) / sizeof(wchar_t*)) ? index : 0];
}
