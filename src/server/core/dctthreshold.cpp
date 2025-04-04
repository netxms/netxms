/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: dctthreshold.cpp
**
**/

#include "nxcore.h"

/**
 * Create new threshold instance
 */
DCTableThresholdInstance::DCTableThresholdInstance(const TCHAR *name, int matchCount, bool active, int row)
{
   m_name = MemCopyString(name);
   m_matchCount = matchCount;
   m_active = active;
   m_row = row;
}

/**
 * Copy constructor threshold instance
 */
DCTableThresholdInstance::DCTableThresholdInstance(const DCTableThresholdInstance *src)
{
   m_name = MemCopyString(src->m_name);
   m_matchCount = src->m_matchCount;
   m_active = src->m_active;
   m_row = src->m_row;
}

/**
 * Threshold instance destructor
 */
DCTableThresholdInstance::~DCTableThresholdInstance()
{
   MemFree(m_name);
}

/**
 * Create new condition
 */
DCTableCondition::DCTableCondition(const TCHAR *column, int operation, const TCHAR *value)
{
   m_column = MemCopyString(CHECK_NULL_EX(column));
   m_operation = operation;
   m_value = value;
}

/**
 * Copy constructor
 */
DCTableCondition::DCTableCondition(DCTableCondition *src)
{
   m_column = MemCopyString(src->m_column);
   m_operation = src->m_operation;
   m_value = src->m_value;
}

/**
 * Condition destructor
 */
DCTableCondition::~DCTableCondition()
{
   MemFree(m_column);
}

/**
 * Check if condition is true
 */
bool DCTableCondition::check(Table *value, int row)
{
   int col = value->getColumnIndex(m_column);
   if (col == -1)
      return false;

   int dt = value->getColumnDataType(col);
   bool result = false;
   switch(m_operation)
   {
      case OP_LE:    // Less
         switch(dt)
         {
            case DCI_DT_INT:
               result = (value->getAsInt(row, col) < (INT32)m_value);
               break;
            case DCI_DT_UINT:
            case DCI_DT_COUNTER32:
               result = (value->getAsUInt(row, col) < (UINT32)m_value);
               break;
            case DCI_DT_INT64:
               result = (value->getAsInt64(row, col) < (INT64)m_value);
               break;
            case DCI_DT_UINT64:
            case DCI_DT_COUNTER64:
               result = (value->getAsUInt64(row, col) < (UINT64)m_value);
               break;
            case DCI_DT_FLOAT:
               result = (value->getAsDouble(row, col) < (double)m_value);
               break;
         }
         break;
      case OP_LE_EQ: // Less or equal
         switch(dt)
         {
            case DCI_DT_INT:
               result = (value->getAsInt(row, col) <= (INT32)m_value);
               break;
            case DCI_DT_UINT:
            case DCI_DT_COUNTER32:
               result = (value->getAsUInt(row, col) <= (UINT32)m_value);
               break;
            case DCI_DT_INT64:
               result = (value->getAsInt64(row, col) <= (INT64)m_value);
               break;
            case DCI_DT_UINT64:
            case DCI_DT_COUNTER64:
               result = (value->getAsUInt64(row, col) <= (UINT64)m_value);
               break;
            case DCI_DT_FLOAT:
               result = (value->getAsDouble(row, col) <= (double)m_value);
               break;
         }
         break;
      case OP_EQ:    // Equal
         switch(dt)
         {
            case DCI_DT_INT:
               result = (value->getAsInt(row, col) == (INT32)m_value);
               break;
            case DCI_DT_UINT:
            case DCI_DT_COUNTER32:
               result = (value->getAsUInt(row, col) == (UINT32)m_value);
               break;
            case DCI_DT_INT64:
               result = (value->getAsInt64(row, col) == (INT64)m_value);
               break;
            case DCI_DT_UINT64:
            case DCI_DT_COUNTER64:
               result = (value->getAsUInt64(row, col) == (UINT64)m_value);
               break;
            case DCI_DT_FLOAT:
               result = (value->getAsDouble(row, col) == (double)m_value);
               break;
            case DCI_DT_STRING:
               result = !_tcscmp(value->getAsString(row, col, _T("")), m_value.getString());
               break;
         }
         break;
      case OP_GT_EQ: // Greater or equal
         switch(dt)
         {
            case DCI_DT_INT:
               result = (value->getAsInt(row, col) >= (INT32)m_value);
               break;
            case DCI_DT_UINT:
            case DCI_DT_COUNTER32:
               result = (value->getAsUInt(row, col) >= (UINT32)m_value);
               break;
            case DCI_DT_INT64:
               result = (value->getAsInt64(row, col) >= (INT64)m_value);
               break;
            case DCI_DT_UINT64:
            case DCI_DT_COUNTER64:
               result = (value->getAsUInt64(row, col) >= (UINT64)m_value);
               break;
            case DCI_DT_FLOAT:
               result = (value->getAsDouble(row, col) >= (double)m_value);
               break;
         }
         break;
      case OP_GT:    // Greater
         switch(dt)
         {
            case DCI_DT_INT:
               result = (value->getAsInt(row, col) > (INT32)m_value);
               break;
            case DCI_DT_UINT:
            case DCI_DT_COUNTER32:
               result = (value->getAsUInt(row, col) > (UINT32)m_value);
               break;
            case DCI_DT_INT64:
               result = (value->getAsInt64(row, col) > (INT64)m_value);
               break;
            case DCI_DT_UINT64:
            case DCI_DT_COUNTER64:
               result = (value->getAsUInt64(row, col) > (UINT64)m_value);
               break;
            case DCI_DT_FLOAT:
               result = (value->getAsDouble(row, col) > (double)m_value);
               break;
         }
         break;
      case OP_NE:    // Not equal
         switch(dt)
         {
            case DCI_DT_INT:
               result = (value->getAsInt(row, col) != (INT32)m_value);
               break;
            case DCI_DT_UINT:
            case DCI_DT_COUNTER32:
               result = (value->getAsUInt(row, col) != (UINT32)m_value);
               break;
            case DCI_DT_INT64:
               result = (value->getAsInt64(row, col) != (INT64)m_value);
               break;
            case DCI_DT_UINT64:
            case DCI_DT_COUNTER64:
               result = (value->getAsUInt64(row, col) != (UINT64)m_value);
               break;
            case DCI_DT_FLOAT:
               result = (value->getAsDouble(row, col) != (double)m_value);
               break;
            case DCI_DT_STRING:
               result = _tcscmp(value->getAsString(row, col, _T("")), m_value.getString()) ? true : false;
               break;
         }
         break;
      case OP_LIKE:
         result = MatchString(m_value.getString(), value->getAsString(row, col, _T("")), true);
         break;
      case OP_NOTLIKE:
         result = !MatchString(m_value.getString(), value->getAsString(row, col, _T("")), true);
         break;
      case OP_ILIKE:
         result = MatchString(m_value.getString(), value->getAsString(row, col, _T("")), false);
         break;
      case OP_INOTLIKE:
         result = !MatchString(m_value.getString(), value->getAsString(row, col, _T("")), false);
         break;
      default:
         break;
   }
   return result;
}

/**
 * Serialize to JSON
 */
json_t *DCTableCondition::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "column", json_string_t(m_column));
   json_object_set_new(root, "operation", json_integer(m_operation));
   json_object_set_new(root, "value", json_string_t(m_value));
   return root;
}

/**
 * Check if this condition equals to given condition
 */
bool DCTableCondition::equals(DCTableCondition *c) const
{
   return !_tcsicmp(c->m_column, m_column) &&
          (c->m_operation == m_operation) &&
          !_tcscmp(c->m_value.getString(), m_value.getString());
}

/**
 * Condition group constructor
 */
DCTableConditionGroup::DCTableConditionGroup()
{
   m_conditions = new ObjectArray<DCTableCondition>(8, 8, Ownership::True);
}

/**
 * Condition group copy constructor
 */
DCTableConditionGroup::DCTableConditionGroup(DCTableConditionGroup *src)
{
   m_conditions = new ObjectArray<DCTableCondition>(src->m_conditions->size(), 8, Ownership::True);
   for(int i = 0; i < src->m_conditions->size(); i++)
      m_conditions->add(new DCTableCondition(src->m_conditions->get(i)));
}

/**
 * Create condition group from NXCP message
 */
DCTableConditionGroup::DCTableConditionGroup(const NXCPMessage& msg, uint32_t *baseId)
{
   uint32_t varId = *baseId;
   int count = msg.getFieldAsUInt32(varId++);
   m_conditions = new ObjectArray<DCTableCondition>(count, 8, Ownership::True);
   for(int i = 0; i < count; i++)
   {
      TCHAR column[MAX_COLUMN_NAME], value[MAX_RESULT_LENGTH];
      msg.getFieldAsString(varId++, column, MAX_COLUMN_NAME);
      int op = (int)msg.getFieldAsUInt16(varId++);
      msg.getFieldAsString(varId++, value, MAX_RESULT_LENGTH);
      m_conditions->add(new DCTableCondition(column, op, value));
   }
   *baseId = varId;
}

/**
 * Create condition group from NXMP record
 */
DCTableConditionGroup::DCTableConditionGroup(ConfigEntry *e)
{
	ConfigEntry *root = e->findEntry(_T("conditions"));
	if (root != NULL)
	{
      unique_ptr<ObjectArray<ConfigEntry>> conditions = root->getSubEntries(_T("condition#*"));
      m_conditions = new ObjectArray<DCTableCondition>(conditions->size(), 4, Ownership::True);
		for(int i = 0; i < conditions->size(); i++)
		{
         ConfigEntry *c = conditions->get(i);
         const TCHAR *column = c->getSubEntryValue(_T("column"), 0, _T(""));
         const TCHAR *value = c->getSubEntryValue(_T("value"), 0, _T(""));
         int op = c->getSubEntryValueAsInt(_T("operation"));
			m_conditions->add(new DCTableCondition(column, op, value));
      }
   }
   else
   {
   	m_conditions = new ObjectArray<DCTableCondition>(8, 8, Ownership::True);
	}
}

/**
 * Condition group destructor
 */
DCTableConditionGroup::~DCTableConditionGroup()
{
   delete m_conditions;
}

/**
 * Fill NXCP mesage
 */
uint32_t DCTableConditionGroup::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   uint32_t varId = baseId;
   msg->setField(varId++, static_cast<uint32_t>(m_conditions->size()));
   for(int i = 0; i < m_conditions->size(); i++)
   {
      const DCTableCondition *c = m_conditions->get(i);
      msg->setField(varId++, c->getColumn());
      msg->setField(varId++, static_cast<uint16_t>(c->getOperation()));
      msg->setField(varId++, c->getValue());
   }
   return varId;
}

/**
 * Serialize to JSON
 */
json_t *DCTableConditionGroup::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "conditions", json_object_array(m_conditions));
   return root;
}

/**
 * Check if this condition group is equal to given condition group
 */
bool DCTableConditionGroup::equals(DCTableConditionGroup *g) const
{
   if (m_conditions->size() != g->m_conditions->size())
      return false;
   for(int i = 0; i < m_conditions->size(); i++)
      if (!m_conditions->get(i)->equals(g->m_conditions->get(i)))
         return false;
   return true;
}

/**
 * Check if condition group is true
 */
bool DCTableConditionGroup::check(Table *value, int row)
{
   for(int i = 0; i < m_conditions->size(); i++)
      if (!m_conditions->get(i)->check(value, row))
         return false;
   return true;
}

/**
 * Table threshold constructor
 */
DCTableThreshold::DCTableThreshold() : m_groups(0, 8, Ownership::True), m_instances(Ownership::True), m_instancesBeforeMaint(Ownership::True)
{
   m_id = CreateUniqueId(IDG_THRESHOLD);
   m_activationEvent = EVENT_TABLE_THRESHOLD_ACTIVATED;
   m_deactivationEvent = EVENT_TABLE_THRESHOLD_DEACTIVATED;
   m_sampleCount = 1;
}

/**
 * Table threshold copy constructor
 */
DCTableThreshold::DCTableThreshold(const DCTableThreshold *src, bool shadowCopy) : m_groups(src->m_groups.size(), 8, Ownership::True), m_instances(Ownership::True), m_instancesBeforeMaint(Ownership::True)
{
   m_id = CreateUniqueId(IDG_THRESHOLD);
   for(int i = 0; i < src->m_groups.size(); i++)
      m_groups.add(new DCTableConditionGroup(src->m_groups.get(i)));
   m_activationEvent = src->m_activationEvent;
   m_deactivationEvent = src->m_deactivationEvent;
   m_sampleCount = src->m_sampleCount;
   if (shadowCopy)
   {
      StringList keys = src->m_instances.keys();
      for(int i = 0; i < keys.size(); i++)
      {
         const TCHAR *k = keys.get(i);
         m_instances.set(k, new DCTableThresholdInstance(src->m_instances.get(k)));
      }
   }
   if (shadowCopy)
   {
      StringList keys = src->m_instancesBeforeMaint.keys();
      for(int i = 0; i < keys.size(); i++)
      {
         const TCHAR *k = keys.get(i);
         m_instancesBeforeMaint.set(k, new DCTableThresholdInstance(src->m_instancesBeforeMaint.get(k)));
      }
   }
}

/**
 * Create table threshold from database
 * Expected column order: id,activation_event,deactivation_event,sample_count
 */
DCTableThreshold::DCTableThreshold(DB_HANDLE hdb, DB_RESULT hResult, int row) : m_groups(0, 8, Ownership::True), m_instances(Ownership::True), m_instancesBeforeMaint(Ownership::True)
{
   m_id = DBGetFieldLong(hResult, row, 0);
   m_activationEvent = DBGetFieldULong(hResult, row, 1);
   m_deactivationEvent = DBGetFieldULong(hResult, row, 2);
   m_sampleCount = DBGetFieldLong(hResult, row, 3);
   loadConditions(hdb);
   loadInstances(hdb);
}

/**
 * Create table threshold from NXCP message
 */
DCTableThreshold::DCTableThreshold(const NXCPMessage& msg, uint32_t *baseId) : m_groups(0, 8, Ownership::True), m_instances(Ownership::True), m_instancesBeforeMaint(Ownership::True)
{
   uint32_t fieldId = *baseId;
   m_id = msg.getFieldAsUInt32(fieldId++);
   if (m_id == 0)
      m_id = CreateUniqueId(IDG_THRESHOLD);
   m_activationEvent = msg.getFieldAsUInt32(fieldId++);
   m_deactivationEvent = msg.getFieldAsUInt32(fieldId++);
   m_sampleCount = msg.getFieldAsUInt32(fieldId++);
   int count = (int)msg.getFieldAsUInt32(fieldId++);
   *baseId = fieldId;
   for(int i = 0; i < count; i++)
      m_groups.add(new DCTableConditionGroup(msg, baseId));
}

/**
 * Create from NXMP record
 */
DCTableThreshold::DCTableThreshold(ConfigEntry *e) : m_groups(0, 8, Ownership::True), m_instances(Ownership::True), m_instancesBeforeMaint(Ownership::True)
{
   m_id = CreateUniqueId(IDG_THRESHOLD);
   m_activationEvent = EventCodeFromName(e->getSubEntryValue(_T("activationEvent"), 0, _T("SYS_TABLE_THRESHOLD_ACTIVATED")));
   m_deactivationEvent = EventCodeFromName(e->getSubEntryValue(_T("deactivationEvent"), 0, _T("SYS_TABLE_THRESHOLD_DEACTIVATED")));
   m_sampleCount = e->getSubEntryValueAsInt(_T("sampleCount"), 0, 1);

	ConfigEntry *groupsRoot = e->findEntry(_T("groups"));
	if (groupsRoot != nullptr)
	{
      unique_ptr<ObjectArray<ConfigEntry>> groups = groupsRoot->getSubEntries(_T("group#*"));
      for (int i = 0; i < groups->size(); i++)
      {
			m_groups.add(new DCTableConditionGroup(groups->get(i)));
      }
   }
}

/**
 * Load conditions from database
 */
void DCTableThreshold::loadConditions(DB_HANDLE hdb)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT group_id,column_name,check_operation,check_value FROM dct_threshold_conditions WHERE threshold_id=? ORDER BY group_id,sequence_number"));
   if (hStmt == nullptr)
      return;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      if (count > 0)
      {
         DCTableConditionGroup *group = nullptr;
         int groupId = -1;
         for(int i = 0; i < count; i++)
         {
            if ((DBGetFieldLong(hResult, i, 0) != groupId) || (group == nullptr))
            {
               groupId = DBGetFieldLong(hResult, i, 0);
               group = new DCTableConditionGroup();
               m_groups.add(group);
            }
            TCHAR column[MAX_COLUMN_NAME], value[MAX_RESULT_LENGTH];
            group->addCondition(
               new DCTableCondition(
                  DBGetField(hResult, i, 1, column, MAX_COLUMN_NAME),
                  DBGetFieldLong(hResult, i, 2),
                  DBGetField(hResult, i, 3, value, MAX_RESULT_LENGTH)));
         }
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
}

/**
 * Load instances from database
 */
void DCTableThreshold::loadInstances(DB_HANDLE hdb)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT instance,match_count,is_active,tt_row_number FROM dct_threshold_instances WHERE threshold_id=? AND maint_copy='0'"));
   if (hStmt == nullptr)
      return;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         TCHAR name[1024];
         DBGetField(hResult, i, 0, name, 1024);
         m_instances.set(name, new DCTableThresholdInstance(name, DBGetFieldLong(hResult, i, 1), DBGetFieldLong(hResult, i, 2) ? true : false, DBGetFieldLong(hResult, i, 3)));
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);

   hStmt = DBPrepare(hdb, _T("SELECT instance,match_count,is_active,tt_row_number FROM dct_threshold_instances WHERE threshold_id=? AND maint_copy='1'"));
   if (hStmt == nullptr)
      return;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         TCHAR name[1024];
         DBGetField(hResult, i, 0, name, 1024);
         m_instancesBeforeMaint.set(name, new DCTableThresholdInstance(name, DBGetFieldLong(hResult, i, 1), DBGetFieldLong(hResult, i, 2) ? true : false, DBGetFieldLong(hResult, i, 3)));
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
}

/**
 * Data for save threshold instances callback
 */
struct SaveThresholdInstancesCallbackData
{
   DB_STATEMENT hStmt;
   uint32_t instanceId;
};

/**
 * Enumeration callback for saving threshold instances
 */
static EnumerationCallbackResult SaveThresholdInstancesCallback(const TCHAR *key, const DCTableThresholdInstance *instance, SaveThresholdInstancesCallbackData *context)
{
   DB_STATEMENT hStmt = context->hStmt;
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, context->instanceId++);
   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, instance->getName(), DB_BIND_STATIC);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, instance->getMatchCount());
   DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, instance->isActive() ? _T("1") : _T("0"), DB_BIND_STATIC);
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, instance->getRow());
   DBExecute(hStmt);
   return _CONTINUE;
}

/**
 * Save threshold to database
 */
bool DCTableThreshold::saveToDatabase(DB_HANDLE hdb, uint32_t tableId, int seq) const
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO dct_thresholds (id,table_id,sequence_number,activation_event,deactivation_event,sample_count) VALUES (?,?,?,?,?,?)"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, tableId);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (INT32)seq);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_activationEvent);
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_deactivationEvent);
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_sampleCount);
   DBExecute(hStmt);
   DBFreeStatement(hStmt);

   if (m_groups.size() > 0)
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO dct_threshold_conditions (threshold_id,group_id,sequence_number,column_name,check_operation,check_value) VALUES (?,?,?,?,?,?)"));
      if (hStmt == nullptr)
         return false;
      for(int i = 0; i < m_groups.size(); i++)
      {
         DCTableConditionGroup *group = m_groups.get(i);
         const ObjectArray<DCTableCondition> *conditions = group->getConditions();
         for(int j = 0; j < conditions->size(); j++)
         {
            DCTableCondition *c = conditions->get(j);
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (INT32)i);
            DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (INT32)j);
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, c->getColumn(), DB_BIND_STATIC);
            DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (INT32)c->getOperation());
            DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, c->getValue(), DB_BIND_STATIC);
            DBExecute(hStmt);
         }
      }
      DBFreeStatement(hStmt);
   }

   if ((m_instances.size() > 0) || (m_instancesBeforeMaint.size() > 0))
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO dct_threshold_instances (threshold_id,instance_id,instance,match_count,is_active,tt_row_number,maint_copy) VALUES (?,?,?,?,?,?,?)"));
      if (hStmt == nullptr)
         return false;

      SaveThresholdInstancesCallbackData data;
      data.hStmt = hStmt;
      data.instanceId = 1;

      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);

      DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, _T("0"), DB_BIND_STATIC);
      m_instances.forEach(SaveThresholdInstancesCallback, &data);

      DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, _T("1"), DB_BIND_STATIC);
      m_instancesBeforeMaint.forEach(SaveThresholdInstancesCallback, &data);

      DBFreeStatement(hStmt);
   }

   return true;
}

/**
 * Fill NXCP message with threshold data
 */
uint32_t DCTableThreshold::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   uint32_t fieldId = baseId;
   msg->setField(fieldId++, m_id);
   msg->setField(fieldId++, m_activationEvent);
   msg->setField(fieldId++, m_deactivationEvent);
   msg->setField(fieldId++, m_sampleCount);
   msg->setField(fieldId++, static_cast<uint32_t>(m_groups.size()));
   for(int i = 0; i < m_groups.size(); i++)
   {
      fieldId = m_groups.get(i)->fillMessage(msg, fieldId);
   }
   return fieldId;
}

/**
 * Get threshold condition as text
 */
StringBuffer DCTableThreshold::getConditionAsText() const
{
   StringBuffer sb;
   for(int i = 0; i < m_groups.size(); i++)
   {
      if (sb.length() > 0)
         sb.append(_T(" OR "));
      if (m_groups.size() > 1)
         sb.append(_T('('));
      const ObjectArray<DCTableCondition> *conditions = m_groups.get(i)->getConditions();
      for(int j = 0; j < conditions->size(); j++)
      {
         DCTableCondition *c = conditions->get(j);
         if (j > 0)
            sb.append(_T(" AND "));
         sb.append(c->getColumn());
         sb.append(_T(' '));
         if ((c->getOperation() >= 0) && (c->getOperation() <= 7))
         {
            static const TCHAR operations[][9] = { _T("<"), _T("<="), _T("=="), _T(">="), _T(">"), _T("!="), _T("LIKE"), _T("NOT LIKE") };
            sb.append(operations[c->getOperation()]);
         }
         else
         {
            sb.append(_T('?'));
         }
         sb.append(_T(' '));
         sb.append(c->getValue());
      }
      if (m_groups.size() > 1)
         sb.append(_T(')'));
   }
   return sb;
}

/**
 * Check threshold
 * Method will return the following codes:
 *    ACTIVATED - when value match the threshold condition while previous check doesn't
 *    DEACTIVATED - when value doesn't match the threshold condition while previous check do
 *    ALREADY_ACTIVE - when value match the threshold condition and threshold is already active
 *    ALREADY_INACTIVE - when value doesn't match the threshold condition and threshold is already inactive
 */
ThresholdCheckResult DCTableThreshold::check(Table *value, int row, const TCHAR *instanceString)
{
   for(int i = 0; i < m_groups.size(); i++)
   {
      if (m_groups.get(i)->check(value, row))
      {
         DCTableThresholdInstance *instance = m_instances.get(instanceString);
         if (instance != nullptr)
         {
            instance->updateRow(row);
            instance->incMatchCount();
            if (instance->isActive())
               return ThresholdCheckResult::ALREADY_ACTIVE;
         }
         else
         {
            instance = new DCTableThresholdInstance(instanceString, 1, false, row);
            m_instances.set(instanceString, instance);
         }
         if (instance->getMatchCount() >= m_sampleCount)
         {
            instance->setActive();
            return ThresholdCheckResult::ACTIVATED;
         }
         return ThresholdCheckResult::ALREADY_INACTIVE;
      }
   }

   // no match
   DCTableThresholdInstance *instance = m_instances.get(instanceString);
   if (instance != nullptr)
   {
      bool deactivated = instance->isActive();
      m_instances.remove(instanceString);
      return deactivated ? ThresholdCheckResult::DEACTIVATED : ThresholdCheckResult::ALREADY_INACTIVE;
   }
   return ThresholdCheckResult::ALREADY_INACTIVE;
}

/**
 * Get missing instances and remove them form thresholds
 */
StringList DCTableThreshold::removeMissingInstances(const StringList &instanceList)
{
   StringList result;
   StringList keys = m_instances.keys();
   for (int i = 0; i < keys.size(); i++)
   {
      if (!instanceList.contains(keys.get(i)))
      {
         result.add(keys.get(i));
         m_instances.remove(keys.get(i));
      }
   }

   return result;
}

/**
 * Callback for cloning threshold instances
 */
static EnumerationCallbackResult CloneThresholdInstances(const TCHAR *key, const DCTableThresholdInstance *value, StringObjectMap<DCTableThresholdInstance> *instances)
{
   instances->set(key, new DCTableThresholdInstance(value));
   return _CONTINUE;
}

/**
 * Save information about threshold state before maintenance
 */
void DCTableThreshold::saveStateBeforeMaintenance()
{
   m_instancesBeforeMaint.clear();
   m_instances.forEach(CloneThresholdInstances, &m_instancesBeforeMaint);
}

/**
 * Callback for event generation that persist after maintenance
 */
void DCTableThreshold::eventGenerationCallback(const TCHAR *key, const DCTableThresholdInstance *value, DCTable *table, bool originalList)
{
   const DCTableThresholdInstance *is;
   const DCTableThresholdInstance *was;
   if (originalList)
   {
      is = value;
      was = findInstance(key, false);
   }
   else
   {
      was = value;
      is = findInstance(key, true);
   }

   if ((was != nullptr) && (is != nullptr) && is->isActive() && was->isActive()) // state remains the same
      return;

   if ((was != nullptr) && was->isActive() && ((is == nullptr) || !is->isActive()))
   {
      EventBuilder(m_deactivationEvent, table->getOwnerId())
         .dci(table->getId())
         .param(_T("dciName"), table->getName())
         .param(_T("dciDescription"), table->getDescription())
         .param(_T("dciId"), table->getId(), EventBuilder::OBJECT_ID_FORMAT)
         .param(_T("row"), was->getRow())
         .param(_T("instance"), key)
         .post();
   }
   else if (((was == nullptr) || !was->isActive()) && (is != nullptr) && is->isActive())
   {
      EventBuilder(m_activationEvent, table->getOwnerId())
         .dci(table->getId())
         .param(_T("dciName"), table->getName())
         .param(_T("dciDescription"), table->getDescription())
         .param(_T("dciId"), table->getId(), EventBuilder::OBJECT_ID_FORMAT)
         .param(_T("row"), is->getRow())
         .param(_T("instance"), key)
         .post();
   }
}

/**
 * Generate events that persist after maintenance
 */
void DCTableThreshold::generateEventsAfterMaintenance(DCTable *table)
{
   m_instances.forEach(
      [this, table] (const TCHAR *key, const DCTableThresholdInstance *value) -> EnumerationCallbackResult
      {
         eventGenerationCallback(key, value, table, true);
         return _CONTINUE;
      });
   m_instances.forEach(
      [this, table] (const TCHAR *key, const DCTableThresholdInstance *value) -> EnumerationCallbackResult
      {
         eventGenerationCallback(key, value, table, false);
         return _CONTINUE;
      });
   m_instancesBeforeMaint.clear();
}

/**
 * Create NXMP record for threshold
 */
void DCTableThreshold::createExportRecord(TextFileWriter& xml, int id) const
{
   TCHAR activationEvent[MAX_EVENT_NAME], deactivationEvent[MAX_EVENT_NAME];

   EventNameFromCode(m_activationEvent, activationEvent);
   EventNameFromCode(m_deactivationEvent, deactivationEvent);
   xml.appendFormattedString(_T("\t\t\t\t\t\t<threshold id=\"%d\">\n")
                          _T("\t\t\t\t\t\t\t<activationEvent>%s</activationEvent>\n")
                          _T("\t\t\t\t\t\t\t<deactivationEvent>%s</deactivationEvent>\n")
                          _T("\t\t\t\t\t\t\t<sampleCount>%d</sampleCount>\n")
                          _T("\t\t\t\t\t\t\t<groups>\n"),
								  id, (const TCHAR *)EscapeStringForXML2(activationEvent),
								  (const TCHAR *)EscapeStringForXML2(deactivationEvent),
								  m_sampleCount);
   for(int i = 0; i < m_groups.size(); i++)
   {
      xml.appendFormattedString(_T("\t\t\t\t\t\t\t\t<group id=\"%d\">\n\t\t\t\t\t\t\t\t\t<conditions>\n"), i + 1);
      const ObjectArray<DCTableCondition> *conditions = m_groups.get(i)->getConditions();
      for(int j = 0; j < conditions->size(); j++)
      {
         DCTableCondition *c = conditions->get(j);
         xml.appendFormattedString(_T("\t\t\t\t\t\t\t\t\t\t<condition id=\"%d\">\n")
                                _T("\t\t\t\t\t\t\t\t\t\t\t<column>%s</column>\n")
                                _T("\t\t\t\t\t\t\t\t\t\t\t<operation>%d</operation>\n")
                                _T("\t\t\t\t\t\t\t\t\t\t\t<value>%s</value>\n")
                                _T("\t\t\t\t\t\t\t\t\t\t</condition>\n"),
                                j + 1, (const TCHAR *)EscapeStringForXML2(c->getColumn()),
                                c->getOperation(), (const TCHAR *)EscapeStringForXML2(c->getValue()));
      }
      xml.append(_T("\t\t\t\t\t\t\t\t\t</conditions>\n\t\t\t\t\t\t\t\t</group>\n"));
   }
   xml.append(_T("\t\t\t\t\t\t\t</groups>\n\t\t\t\t\t\t</threshold>\n"));
}

/**
 * Serialize object to JSON
 */
json_t *DCTableThreshold::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "groups", json_object_array(m_groups));
   json_object_set_new(root, "activationEvent", json_integer(m_activationEvent));
   json_object_set_new(root, "deactivationEvent", json_integer(m_deactivationEvent));
   json_object_set_new(root, "sampleCount", json_integer(m_sampleCount));
   return root;
}

/**
 * Copy threshold state
 */
void DCTableThreshold::copyState(DCTableThreshold *src)
{
   m_instances.clear();
   src->m_instances.forEach(CloneThresholdInstances, &m_instances);
}

/**
 * Check if this threshold is equal to given threshold
 */
bool DCTableThreshold::equals(const DCTableThreshold *t) const
{
   if ((m_activationEvent != t->m_activationEvent) ||
       (m_deactivationEvent != t->m_deactivationEvent) ||
       (m_sampleCount != t->m_sampleCount) ||
       (m_groups.size() != t->m_groups.size()))
      return false;

   for(int i = 0; i < m_groups.size(); i++)
      if (!m_groups.get(i)->equals(t->m_groups.get(i)))
         return false;

   return true;
}
