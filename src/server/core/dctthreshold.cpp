/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
DCTableConditionGroup::DCTableConditionGroup(CSCPMessage *msg, UINT32 *baseId)
{
   UINT32 varId = *baseId;
   int count = msg->GetVariableLong(varId++);
   m_conditions = new ObjectArray<DCTableCondition>(count, 8, true);
   for(int i = 0; i < count; i++)
   {
      TCHAR column[MAX_COLUMN_NAME], value[MAX_RESULT_LENGTH];
      msg->GetVariableStr(varId++, column, MAX_COLUMN_NAME);
      int op = (int)msg->GetVariableShort(varId++);
      msg->GetVariableStr(varId++, value, MAX_RESULT_LENGTH);
      m_conditions->add(new DCTableCondition(column, op, value));
   }
   *baseId = varId;
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
UINT32 DCTableConditionGroup::fillMessage(CSCPMessage *msg, UINT32 baseId)
{
   UINT32 varId = baseId;
   msg->SetVariable(varId++, (UINT32)m_conditions->size());
   for(int i = 0; i < m_conditions->size(); i++)
   {
      DCTableCondition *c = m_conditions->get(i);
      msg->SetVariable(varId++, c->getColumn());
      msg->SetVariable(varId++, (UINT16)c->getOperation());
      msg->SetVariable(varId++, c->getValue());
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
   m_activationEvent = EVENT_THRESHOLD_REACHED;
   m_deactivationEvent = EVENT_THRESHOLD_REARMED;
   m_currentState = false;
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
   m_currentState = false;
}

/**
 * Create table threshold from database
 * Expected column order: id,current_state,activation_event,deactivation_event
 */
DCTableThreshold::DCTableThreshold(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldLong(hResult, row, 0);
   m_currentState = DBGetFieldLong(hResult, row, 1) ? true : false;
   m_activationEvent = DBGetFieldULong(hResult, row, 2);
   m_deactivationEvent = DBGetFieldULong(hResult, row, 3);
   m_groups = new ObjectArray<DCTableConditionGroup>(4, 4, true);
   loadConditions();
}

/**
 * Create table threshold from NXCP message
 */
DCTableThreshold::DCTableThreshold(CSCPMessage *msg, UINT32 *baseId)
{
   UINT32 varId = *baseId;
   m_id = msg->GetVariableLong(varId++);
   m_currentState = false;
   m_activationEvent = msg->GetVariableLong(varId++);
   m_deactivationEvent = msg->GetVariableLong(varId++);
   int count = (int)msg->GetVariableLong(varId++);
   m_groups = new ObjectArray<DCTableConditionGroup>(count, 4, true);
   *baseId = varId;
   for(int i = 0; i < count; i++)
      m_groups->add(new DCTableConditionGroup(msg, baseId));
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
            if (DBGetFieldLong(hResult, i, 0) != groupId)
            {
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
}

/**
 * Save threshold to database
 */
bool DCTableThreshold::saveToDatabase(DB_HANDLE hdb, UINT32 tableId, int seq)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO dct_thresholds (id,table_id,sequence_number,current_state,activation_event,deactivation_event) VALUES (?,?,?,?,?,?)"));
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, tableId);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (INT32)seq);
   DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_currentState ? _T("1") : _T("0"), DB_BIND_STATIC);
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_activationEvent);
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_deactivationEvent);
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
UINT32 DCTableThreshold::fillMessage(CSCPMessage *msg, UINT32 baseId)
{
   UINT32 varId = baseId;
   msg->SetVariable(varId++, m_id);
   msg->SetVariable(varId++, (UINT16)(m_currentState ? 1 : 0));
   msg->SetVariable(varId++, m_activationEvent);
   msg->SetVariable(varId++, m_deactivationEvent);
   msg->SetVariable(varId++, (UINT32)m_groups->size());
   for(int i = 0; i < m_groups->size(); i++)
   {
      varId = m_groups->get(i)->fillMessage(msg, varId);
   }
   return varId;
}

/**
 * Check threshold
 */
bool DCTableThreshold::check(Table *value, int row)
{
   for(int i = 0; i < m_groups->size(); i++)
      if (m_groups->get(i)->check(value, row))
         return true;
   return false;
}
