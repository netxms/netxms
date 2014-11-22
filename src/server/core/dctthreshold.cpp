/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
   safe_free(m_column);
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
               result = !_tcscmp(value->getAsString(row, col), m_value.getString());
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
               result = _tcscmp(value->getAsString(row, col), m_value.getString()) ? true : false;
               break;
         }
         break;
      case OP_LIKE:
         result = MatchString(m_value.getString(), value->getAsString(row, col), true);
         break;
      case OP_NOTLIKE:
         result = !MatchString(m_value.getString(), value->getAsString(row, col), true);
         break;
      default:
         break;
   }
   return result;
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
   m_activeKeys = new StringSet;
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
   m_activeKeys = new StringSet;
}

/**
 * Create table threshold from database
 * Expected column order: id,activation_event,deactivation_event
 */
DCTableThreshold::DCTableThreshold(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldLong(hResult, row, 0);
   m_activationEvent = DBGetFieldULong(hResult, row, 1);
   m_deactivationEvent = DBGetFieldULong(hResult, row, 2);
   m_groups = new ObjectArray<DCTableConditionGroup>(4, 4, true);
   loadConditions();
   m_activeKeys = new StringSet;
}

/**
 * Create table threshold from NXCP message
 */
DCTableThreshold::DCTableThreshold(NXCPMessage *msg, UINT32 *baseId)
{
   UINT32 varId = *baseId;
   m_id = msg->getFieldAsUInt32(varId++);
   if (m_id == 0)
      m_id = CreateUniqueId(IDG_THRESHOLD);
   m_activationEvent = msg->getFieldAsUInt32(varId++);
   m_deactivationEvent = msg->getFieldAsUInt32(varId++);
   int count = (int)msg->getFieldAsUInt32(varId++);
   m_groups = new ObjectArray<DCTableConditionGroup>(count, 4, true);
   *baseId = varId;
   for(int i = 0; i < count; i++)
      m_groups->add(new DCTableConditionGroup(msg, baseId));
   m_activeKeys = new StringSet;
}

/**
 * Create from NXMP record
 */
DCTableThreshold::DCTableThreshold(ConfigEntry *e)
{
   m_activationEvent = EventCodeFromName(e->getSubEntryValue(_T("activationEvent"), 0, _T("SYS_TABLE_THRESHOLD_ACTIVATED")));
   m_deactivationEvent = EventCodeFromName(e->getSubEntryValue(_T("deactivationEvent"), 0, _T("SYS_TABLE_THRESHOLD_DEACTIVATED")));

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
   m_activeKeys = new StringSet;
}

/**
 * Load conditions from database
 */
void DCTableThreshold::loadConditions()
{
   DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT group_id,column_name,check_operation,check_value FROM dct_threshold_conditions WHERE threshold_id=? ORDER BY group_id,sequence_number"));
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
 * Table threshold destructor
 */
DCTableThreshold::~DCTableThreshold()
{
   delete m_groups;
   delete m_activeKeys;
}

/**
 * Save threshold to database
 */
bool DCTableThreshold::saveToDatabase(DB_HANDLE hdb, UINT32 tableId, int seq)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO dct_thresholds (id,table_id,sequence_number,activation_event,deactivation_event) VALUES (?,?,?,?,?)"));
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, tableId);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (INT32)seq);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_activationEvent);
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_deactivationEvent);
   DBExecute(hStmt);
   DBFreeStatement(hStmt);

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
   return true;
}

/**
 * Fill NXCP message with threshold data
 */
UINT32 DCTableThreshold::fillMessage(NXCPMessage *msg, UINT32 baseId)
{
   UINT32 varId = baseId;
   msg->setField(varId++, m_id);
   msg->setField(varId++, m_activationEvent);
   msg->setField(varId++, m_deactivationEvent);
   msg->setField(varId++, (UINT32)m_groups->size());
   for(int i = 0; i < m_groups->size(); i++)
   {
      varId = m_groups->get(i)->fillMessage(msg, varId);
   }
   return varId;
}

/**
 * Check threshold
 * Method will return the following codes:
 *    THRESHOLD_REACHED - when value match the threshold condition while previous check doesn't
 *    THRESHOLD_REARMED - when value doesn't match the threshold condition while previous check do
 *    NO_ACTION - when there are no changes in value match to threshold's condition
 */
ThresholdCheckResult DCTableThreshold::check(Table *value, int row, const TCHAR *instance)
{
   for(int i = 0; i < m_groups->size(); i++)
   {
      if (m_groups->get(i)->check(value, row))
      {
         if (m_activeKeys->contains(instance))
         {
            return ALREADY_ACTIVE;
         }
         m_activeKeys->add(instance);
         return ACTIVATED;
      }
   }

   // no match
   if (m_activeKeys->contains(instance))
   {
      m_activeKeys->remove(instance);
      return DEACTIVATED;
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
   str.addFormattedString(_T("\t\t\t\t\t\t<threshold id=\"%d\">\n")
                          _T("\t\t\t\t\t\t\t<activationEvent>%s</activationEvent>\n")
                          _T("\t\t\t\t\t\t\t<deactivationEvent>%s</deactivationEvent>\n")
                          _T("\t\t\t\t\t\t\t<groups>\n"),
								  id, (const TCHAR *)EscapeStringForXML2(activationEvent),
								  (const TCHAR *)EscapeStringForXML2(deactivationEvent));
   for(int i = 0; i < m_groups->size(); i++)
   {
      str.addFormattedString(_T("\t\t\t\t\t\t\t\t<group id=\"%d\">\n\t\t\t\t\t\t\t\t\t<conditions>\n"), i + 1);
      ObjectArray<DCTableCondition> *conditions = m_groups->get(i)->getConditions();
      for(int j = 0; j < conditions->size(); j++)
      {
         DCTableCondition *c = conditions->get(j);
         str.addFormattedString(_T("\t\t\t\t\t\t\t\t\t\t<condition id=\"%d\">\n")
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
 * Copy threshold state
 */
void DCTableThreshold::copyState(DCTableThreshold *src)
{
   m_activeKeys->clear();
   m_activeKeys->addAll(src->m_activeKeys);
}
