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
** File: dctthreshold.cpp
**
**/

#include "nxcore.h"

/**
 * Create new threshold instance
 */
DCTableThresholdInstance::DCTableThresholdInstance(const TCHAR *name, int matchCount, bool active)
{
   m_name = _tcsdup(name);
   m_matchCount = matchCount;
   m_active = active;
}

/**
 * Copy constructor threshold instance
 */
DCTableThresholdInstance::DCTableThresholdInstance(const DCTableThresholdInstance *src)
{
   m_name = _tcsdup(src->m_name);
   m_matchCount = src->m_matchCount;
   m_active = src->m_active;
}

/**
 * Threshold instance destructor
 */
DCTableThresholdInstance::~DCTableThresholdInstance()
{
   free(m_name);
}

/**
 * Create new condition
 */
DCTableCondition::DCTableCondition(const TCHAR *column, int operation, const TCHAR *value)
{
   m_column = _tcsdup(CHECK_NULL_EX(column));
   m_operation = operation;
   m_value = value;
}

/**
 * Copy constructor
 */
DCTableCondition::DCTableCondition(DCTableCondition *src)
{
   m_column = _tcsdup(src->m_column);
   m_operation = src->m_operation;
   m_value = src->m_value;
}

/**
 * Condition destructor
 */
DCTableCondition::~DCTableCondition()
{
   free(m_column);
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
               result = (value->getAsUInt(row, col) < (UINT32)m_value);
               break;
            case DCI_DT_INT64:
               result = (value->getAsInt64(row, col) < (INT64)m_value);
               break;
            case DCI_DT_UINT64:
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
               result = (value->getAsUInt(row, col) <= (UINT32)m_value);
               break;
            case DCI_DT_INT64:
               result = (value->getAsInt64(row, col) <= (INT64)m_value);
               break;
            case DCI_DT_UINT64:
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
               result = (value->getAsUInt(row, col) == (UINT32)m_value);
               break;
            case DCI_DT_INT64:
               result = (value->getAsInt64(row, col) == (INT64)m_value);
               break;
            case DCI_DT_UINT64:
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
               result = (value->getAsUInt(row, col) >= (UINT32)m_value);
               break;
            case DCI_DT_INT64:
               result = (value->getAsInt64(row, col) >= (INT64)m_value);
               break;
            case DCI_DT_UINT64:
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
               result = (value->getAsUInt(row, col) > (UINT32)m_value);
               break;
            case DCI_DT_INT64:
               result = (value->getAsInt64(row, col) > (INT64)m_value);
               break;
            case DCI_DT_UINT64:
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
               result = (value->getAsUInt(row, col) != (UINT32)m_value);
               break;
            case DCI_DT_INT64:
               result = (value->getAsInt64(row, col) != (INT64)m_value);
               break;
            case DCI_DT_UINT64:
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
 * Condition group constructor
 */
DCTableConditionGroup::DCTableConditionGroup()
{
   m_conditions = new ObjectArray<DCTableCondition>(8, 8, true);
}

/**
 * Condition group copy constructor
 */
DCTableConditionGroup::DCTableConditionGroup(DCTableConditionGroup *src)
{
   m_conditions = new ObjectArray<DCTableCondition>(src->m_conditions->size(), 8, true);
   for(int i = 0; i < src->m_conditions->size(); i++)
      m_conditions->add(new DCTableCondition(src->m_conditions->get(i)));
}

/**
 * Create condition group from NXCP message
 */
DCTableConditionGroup::DCTableConditionGroup(NXCPMessage *msg, UINT32 *baseId)
{
   UINT32 varId = *baseId;
   int count = msg->getFieldAsUInt32(varId++);
   m_conditions = new ObjectArray<DCTableCondition>(count, 8, true);
   for(int i = 0; i < count; i++)
   {
      TCHAR column[MAX_COLUMN_NAME], value[MAX_RESULT_LENGTH];
      msg->getFieldAsString(varId++, column, MAX_COLUMN_NAME);
      int op = (int)msg->getFieldAsUInt16(varId++);
      msg->getFieldAsString(varId++, value, MAX_RESULT_LENGTH);
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
		ObjectArray<ConfigEntry> *conditions = root->getSubEntries(_T("condition#*"));
      m_conditions = new ObjectArray<DCTableCondition>(conditions->size(), 4, true);
		for(int i = 0; i < conditions->size(); i++)
		{
         ConfigEntry *c = conditions->get(i);
         const TCHAR *column = c->getSubEntryValue(_T("column"), 0, _T(""));
         const TCHAR *value = c->getSubEntryValue(_T("value"), 0, _T(""));
         int op = c->getSubEntryValueAsInt(_T("operation"));
			m_conditions->add(new DCTableCondition(column, op, value));
		}
		delete conditions;
	}
	else
	{
   	m_conditions = new ObjectArray<DCTableCondition>(8, 8, true);
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
UINT32 DCTableConditionGroup::fillMessage(NXCPMessage *msg, UINT32 baseId)
{
   UINT32 varId = baseId;
   msg->setField(varId++, (UINT32)m_conditions->size());
   for(int i = 0; i < m_conditions->size(); i++)
   {
      DCTableCondition *c = m_conditions->get(i);
      msg->setField(varId++, c->getColumn());
      msg->setField(varId++, (UINT16)c->getOperation());
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
DCTableThreshold::DCTableThreshold()
{
   m_id = CreateUniqueId(IDG_THRESHOLD);
   m_groups = new ObjectArray<DCTableConditionGroup>(4, 4, true);
   m_activationEvent = EVENT_TABLE_THRESHOLD_ACTIVATED;
   m_deactivationEvent = EVENT_TABLE_THRESHOLD_DEACTIVATED;
   m_sampleCount = 1;
   m_instances = new StringObjectMap<DCTableThresholdInstance>(true);
}

/**
 * Table threshold copy constructor
 */
DCTableThreshold::DCTableThreshold(DCTableThreshold *src)
{
   m_id = CreateUniqueId(IDG_THRESHOLD);
   m_groups = new ObjectArray<DCTableConditionGroup>(src->m_groups->size(), 4, true);
   for(int i = 0; i < src->m_groups->size(); i++)
      m_groups->add(new DCTableConditionGroup(src->m_groups->get(i)));
   m_activationEvent = src->m_activationEvent;
   m_deactivationEvent = src->m_deactivationEvent;
   m_sampleCount = src->m_sampleCount;
   m_instances = new StringObjectMap<DCTableThresholdInstance>(true);
}

/**
 * Create table threshold from database
 * Expected column order: id,activation_event,deactivation_event,sample_count
 */
DCTableThreshold::DCTableThreshold(DB_HANDLE hdb, DB_RESULT hResult, int row)
{
   m_id = DBGetFieldLong(hResult, row, 0);
   m_activationEvent = DBGetFieldULong(hResult, row, 1);
   m_deactivationEvent = DBGetFieldULong(hResult, row, 2);
   m_groups = new ObjectArray<DCTableConditionGroup>(4, 4, true);
   m_sampleCount = DBGetFieldLong(hResult, row, 3);
   m_instances = new StringObjectMap<DCTableThresholdInstance>(true);
   loadConditions(hdb);
   loadInstances(hdb);
}

/**
 * Create table threshold from NXCP message
 */
DCTableThreshold::DCTableThreshold(NXCPMessage *msg, UINT32 *baseId)
{
   UINT32 fieldId = *baseId;
   m_id = msg->getFieldAsUInt32(fieldId++);
   if (m_id == 0)
      m_id = CreateUniqueId(IDG_THRESHOLD);
   m_activationEvent = msg->getFieldAsUInt32(fieldId++);
   m_deactivationEvent = msg->getFieldAsUInt32(fieldId++);
   m_sampleCount = msg->getFieldAsUInt32(fieldId++);
   int count = (int)msg->getFieldAsUInt32(fieldId++);
   m_groups = new ObjectArray<DCTableConditionGroup>(count, 4, true);
   *baseId = fieldId;
   for(int i = 0; i < count; i++)
      m_groups->add(new DCTableConditionGroup(msg, baseId));
   m_instances = new StringObjectMap<DCTableThresholdInstance>(true);
}

/**
 * Create from NXMP record
 */
DCTableThreshold::DCTableThreshold(ConfigEntry *e)
{
   m_id = CreateUniqueId(IDG_THRESHOLD);
   m_activationEvent = EventCodeFromName(e->getSubEntryValue(_T("activationEvent"), 0, _T("SYS_TABLE_THRESHOLD_ACTIVATED")));
   m_deactivationEvent = EventCodeFromName(e->getSubEntryValue(_T("deactivationEvent"), 0, _T("SYS_TABLE_THRESHOLD_DEACTIVATED")));
   m_sampleCount = e->getSubEntryValueAsInt(_T("sampleCount"), 0, 1);

	ConfigEntry *groupsRoot = e->findEntry(_T("groups"));
	if (groupsRoot != NULL)
	{
		ObjectArray<ConfigEntry> *groups = groupsRoot->getSubEntries(_T("group#*"));
      m_groups = new ObjectArray<DCTableConditionGroup>(groups->size(), 4, true);
		for(int i = 0; i < groups->size(); i++)
		{
			m_groups->add(new DCTableConditionGroup(groups->get(i)));
		}
		delete groups;
	}
	else
	{
   	m_groups = new ObjectArray<DCTableConditionGroup>(4, 4, true);
	}
   m_instances = new StringObjectMap<DCTableThresholdInstance>(true);
}

/**
 * Load conditions from database
 */
void DCTableThreshold::loadConditions(DB_HANDLE hdb)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT group_id,column_name,check_operation,check_value FROM dct_threshold_conditions WHERE threshold_id=? ORDER BY group_id,sequence_number"));
   if (hStmt == NULL)
      return;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      if (count > 0)
      {
         DCTableConditionGroup *group = NULL;
         int groupId = -1;
         for(int i = 0; i < count; i++)
         {
            if ((DBGetFieldLong(hResult, i, 0) != groupId) || (group == NULL))
            {
               groupId = DBGetFieldLong(hResult, i, 0);
               group = new DCTableConditionGroup();
               m_groups->add(group);
            }
            TCHAR column[MAX_COLUMN_NAME], value[MAX_RESULT_LENGTH];
            group->getConditions()->add(
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
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT instance,match_count,is_active FROM dct_threshold_instances WHERE threshold_id=?"));
   if (hStmt == NULL)
      return;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         TCHAR name[1024];
         DBGetField(hResult, i, 0, name, 1024);
         m_instances->set(name, new DCTableThresholdInstance(name, DBGetFieldLong(hResult, i, 1), DBGetFieldLong(hResult, i, 2) ? true : false));
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
}

/**
 * Table threshold destructor
 */
DCTableThreshold::~DCTableThreshold()
{
   delete m_groups;
   delete m_instances;
}

/**
 * Enumeration callback for saving threshold instances
 */
static EnumerationCallbackResult SaveThresholdInstancesCallback(const TCHAR *key, const void *value, void *arg)
{
   DB_STATEMENT hStmt = (DB_STATEMENT)arg;
   const DCTableThresholdInstance *i = (const DCTableThresholdInstance *)value;
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, i->getName(), DB_BIND_STATIC);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, i->getMatchCount());
   DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, i->isActive() ? _T("1") : _T("0"), DB_BIND_STATIC);
   DBExecute(hStmt);
   return _CONTINUE;
}

/**
 * Save threshold to database
 */
bool DCTableThreshold::saveToDatabase(DB_HANDLE hdb, UINT32 tableId, int seq)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO dct_thresholds (id,table_id,sequence_number,activation_event,deactivation_event,sample_count) VALUES (?,?,?,?,?,?)"));
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, tableId);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (INT32)seq);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_activationEvent);
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_deactivationEvent);
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_sampleCount);
   DBExecute(hStmt);
   DBFreeStatement(hStmt);

   if (m_groups->size() > 0)
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO dct_threshold_conditions (threshold_id,group_id,sequence_number,column_name,check_operation,check_value) VALUES (?,?,?,?,?,?)"));
      if (hStmt == NULL)
         return false;
      for(int i = 0; i < m_groups->size(); i++)
      {
         DCTableConditionGroup *group = m_groups->get(i);
         ObjectArray<DCTableCondition> *conditions = group->getConditions();
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

   if (m_instances->size() > 0)
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO dct_threshold_instances (threshold_id,instance,match_count,is_active) VALUES (?,?,?,?)"));
      if (hStmt == NULL)
         return false;

      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      m_instances->forEach(SaveThresholdInstancesCallback, hStmt);
      DBFreeStatement(hStmt);
   }
   return true;
}

/**
 * Fill NXCP message with threshold data
 */
UINT32 DCTableThreshold::fillMessage(NXCPMessage *msg, UINT32 baseId)
{
   UINT32 fieldId = baseId;
   msg->setField(fieldId++, m_id);
   msg->setField(fieldId++, m_activationEvent);
   msg->setField(fieldId++, m_deactivationEvent);
   msg->setField(fieldId++, m_sampleCount);
   msg->setField(fieldId++, (UINT32)m_groups->size());
   for(int i = 0; i < m_groups->size(); i++)
   {
      fieldId = m_groups->get(i)->fillMessage(msg, fieldId);
   }
   return fieldId;
}

/**
 * Check threshold
 * Method will return the following codes:
 *    ACTIVATED - when value match the threshold condition while previous check doesn't
 *    DEACTIVATED - when value doesn't match the threshold condition while previous check do
 *    ALREADY_ACTIVE - when value match the threshold condition and threshold is already active
 *    ALREADY_INACTIVE - when value doesn't match the threshold condition and threshold is already inactive
 */
ThresholdCheckResult DCTableThreshold::check(Table *value, int row, const TCHAR *instance)
{
   for(int i = 0; i < m_groups->size(); i++)
   {
      if (m_groups->get(i)->check(value, row))
      {
         DCTableThresholdInstance *i = m_instances->get(instance);
         if (i != NULL)
         {
            i->incMatchCount();
            if (i->isActive())
               return ALREADY_ACTIVE;
         }
         else
         {
            i = new DCTableThresholdInstance(instance, 1, false);
            m_instances->set(instance, i);
         }
         if (i->getMatchCount() >= m_sampleCount)
         {
            i->setActive();
            return ACTIVATED;
         }
         return ALREADY_INACTIVE;
      }
   }

   // no match
   DCTableThresholdInstance *i = m_instances->get(instance);
   if (i != NULL)
   {
      bool deactivated = i->isActive();
      m_instances->remove(instance);
      return deactivated ? DEACTIVATED : ALREADY_INACTIVE;
   }
   return ALREADY_INACTIVE;
}

/**
 * Create NXMP record for threshold
 */
void DCTableThreshold::createNXMPRecord(String &str, int id)
{
   TCHAR activationEvent[MAX_EVENT_NAME], deactivationEvent[MAX_EVENT_NAME];

   EventNameFromCode(m_activationEvent, activationEvent);
   EventNameFromCode(m_deactivationEvent, deactivationEvent);
   str.appendFormattedString(_T("\t\t\t\t\t\t<threshold id=\"%d\">\n")
                          _T("\t\t\t\t\t\t\t<activationEvent>%s</activationEvent>\n")
                          _T("\t\t\t\t\t\t\t<deactivationEvent>%s</deactivationEvent>\n")
                          _T("\t\t\t\t\t\t\t<sampleCount>%d</sampleCount>\n")
                          _T("\t\t\t\t\t\t\t<groups>\n"),
								  id, (const TCHAR *)EscapeStringForXML2(activationEvent),
								  (const TCHAR *)EscapeStringForXML2(deactivationEvent),
								  m_sampleCount);
   for(int i = 0; i < m_groups->size(); i++)
   {
      str.appendFormattedString(_T("\t\t\t\t\t\t\t\t<group id=\"%d\">\n\t\t\t\t\t\t\t\t\t<conditions>\n"), i + 1);
      ObjectArray<DCTableCondition> *conditions = m_groups->get(i)->getConditions();
      for(int j = 0; j < conditions->size(); j++)
      {
         DCTableCondition *c = conditions->get(j);
         str.appendFormattedString(_T("\t\t\t\t\t\t\t\t\t\t<condition id=\"%d\">\n")
                                _T("\t\t\t\t\t\t\t\t\t\t\t<column>%s</column>\n")
                                _T("\t\t\t\t\t\t\t\t\t\t\t<operation>%d</operation>\n")
                                _T("\t\t\t\t\t\t\t\t\t\t\t<value>%s</value>\n")
                                _T("\t\t\t\t\t\t\t\t\t\t</condition>\n"),
                                j + 1, (const TCHAR *)EscapeStringForXML2(c->getColumn()),
                                c->getOperation(), (const TCHAR *)EscapeStringForXML2(c->getValue()));
      }
      str += _T("\t\t\t\t\t\t\t\t\t</conditions>\n\t\t\t\t\t\t\t\t</group>\n");
   }
   str += _T("\t\t\t\t\t\t\t</groups>\n\t\t\t\t\t\t</threshold>\n");
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
 * Callback for cloning threshld instances
 */
static EnumerationCallbackResult CloneThresholdInstances(const TCHAR *key, const void *value, void *data)
{
   ((StringObjectMap<DCTableThresholdInstance> *)data)->set(key, new DCTableThresholdInstance((const DCTableThresholdInstance *)value));
   return _CONTINUE;
}

/**
 * Copy threshold state
 */
void DCTableThreshold::copyState(DCTableThreshold *src)
{
   m_instances->clear();
   src->m_instances->forEach(CloneThresholdInstances, m_instances);
}
