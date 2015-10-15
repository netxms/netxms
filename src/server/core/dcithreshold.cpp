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
** File: dcithreshold.cpp
**
**/

#include "nxcore.h"

/**
 * Create new threshold for given DCI
 */
Threshold::Threshold(DCItem *pRelatedItem)
{
   m_id = 0;
   m_itemId = pRelatedItem->getId();
   m_targetId = pRelatedItem->getTarget()->getId();
   m_eventCode = EVENT_THRESHOLD_REACHED;
   m_rearmEventCode = EVENT_THRESHOLD_REARMED;
   m_function = F_LAST;
   m_operation = OP_EQ;
   m_dataType = pRelatedItem->getDataType();
   m_sampleCount = 1;
   m_scriptSource = NULL;
   m_script = NULL;
   m_isReached = FALSE;
	m_currentSeverity = SEVERITY_NORMAL;
	m_repeatInterval = -1;
	m_lastEventTimestamp = 0;
	m_numMatches = 0;
}

/**
 * Constructor for NXMP parser
 */
Threshold::Threshold()
{
   m_id = 0;
   m_itemId = 0;
   m_targetId = 0;
   m_eventCode = EVENT_THRESHOLD_REACHED;
   m_rearmEventCode = EVENT_THRESHOLD_REARMED;
   m_function = F_LAST;
   m_operation = OP_EQ;
   m_dataType = 0;
   m_sampleCount = 1;
   m_scriptSource = NULL;
   m_script = NULL;
   m_isReached = FALSE;
	m_currentSeverity = SEVERITY_NORMAL;
	m_repeatInterval = -1;
	m_lastEventTimestamp = 0;
	m_numMatches = 0;
}

/**
 * Create from another threshold object
 */
Threshold::Threshold(Threshold *src)
{
   m_id = src->m_id;
   m_itemId = src->m_itemId;
   m_targetId = src->m_targetId;
   m_eventCode = src->m_eventCode;
   m_rearmEventCode = src->m_rearmEventCode;
   m_value = src->m_value;
   m_function = src->m_function;
   m_operation = src->m_operation;
   m_dataType = src->m_dataType;
   m_sampleCount = src->m_sampleCount;
   m_scriptSource = NULL;
   m_script = NULL;
   setScript((src->m_scriptSource != NULL) ? _tcsdup(src->m_scriptSource) : NULL);
   m_isReached = FALSE;
	m_currentSeverity = SEVERITY_NORMAL;
	m_repeatInterval = src->m_repeatInterval;
	m_lastEventTimestamp = 0;
	m_numMatches = 0;
}

/**
 * Constructor for creating object from database
 * This constructor assumes that SELECT query look as following:
 * SELECT threshold_id,fire_value,rearm_value,check_function,check_operation,
 *        sample_count,script,event_code,current_state,rearm_event_code,
 *        repeat_interval,current_severity,last_event_timestamp,match_count FROM thresholds
 */
Threshold::Threshold(DB_RESULT hResult, int iRow, DCItem *pRelatedItem)
{
   TCHAR szBuffer[MAX_DB_STRING];

   m_id = DBGetFieldULong(hResult, iRow, 0);
   m_itemId = pRelatedItem->getId();
   m_targetId = pRelatedItem->getTarget()->getId();
   m_eventCode = DBGetFieldULong(hResult, iRow, 7);
   m_rearmEventCode = DBGetFieldULong(hResult, iRow, 9);
   DBGetField(hResult, iRow, 1, szBuffer, MAX_DB_STRING);
   m_value = szBuffer;
   m_function = (BYTE)DBGetFieldLong(hResult, iRow, 3);
   m_operation = (BYTE)DBGetFieldLong(hResult, iRow, 4);
   m_dataType = pRelatedItem->getDataType();
   m_sampleCount = DBGetFieldLong(hResult, iRow, 5);
	if ((m_function == F_LAST) && (m_sampleCount < 1))
		m_sampleCount = 1;
   m_scriptSource = NULL;
   m_script = NULL;
   setScript(DBGetField(hResult, iRow, 6, NULL, 0));
   m_isReached = DBGetFieldLong(hResult, iRow, 8);
	m_repeatInterval = DBGetFieldLong(hResult, iRow, 10);
	m_currentSeverity = (BYTE)DBGetFieldLong(hResult, iRow, 11);
	m_lastEventTimestamp = (time_t)DBGetFieldULong(hResult, iRow, 12);
	m_numMatches = DBGetFieldLong(hResult, iRow, 13);
}

/**
 * Create threshold from import file
 */
Threshold::Threshold(ConfigEntry *config, DCItem *parentItem)
{
   createId();
   m_itemId = parentItem->getId();
   m_targetId = parentItem->getTarget()->getId();
   m_eventCode = EventCodeFromName(config->getSubEntryValue(_T("activationEvent"), 0, _T("SYS_THRESHOLD_REACHED")));
   m_rearmEventCode = EventCodeFromName(config->getSubEntryValue(_T("deactivationEvent"), 0, _T("SYS_THRESHOLD_REARMED")));
   m_function = (BYTE)config->getSubEntryValueAsInt(_T("function"), 0, F_LAST);
   m_operation = (BYTE)config->getSubEntryValueAsInt(_T("condition"), 0, OP_EQ);
   m_dataType = parentItem->getDataType();
	m_value = config->getSubEntryValue(_T("value"), 0, _T(""));
   m_sampleCount = (config->getSubEntryValue(_T("sampleCount")) != NULL) ? config->getSubEntryValueAsInt(_T("sampleCount"), 0, 1) : config->getSubEntryValueAsInt(_T("param1"), 0, 1);
   m_scriptSource = NULL;
   m_script = NULL;
   const TCHAR *script = config->getSubEntryValue(_T("script"));
   setScript(_tcsdup_ex(script));
   m_isReached = FALSE;
	m_currentSeverity = SEVERITY_NORMAL;
	m_repeatInterval = config->getSubEntryValueAsInt(_T("repeatInterval"), 0, -1);
	m_lastEventTimestamp = 0;
	m_numMatches = 0;
}

/**
 * Destructor
 */
Threshold::~Threshold()
{
   safe_free(m_scriptSource);
   delete m_script;
}

/**
 * Create new unique id for object
 */
void Threshold::createId()
{
   m_id = CreateUniqueId(IDG_THRESHOLD);
}

/**
 * Save threshold to database
 */
BOOL Threshold::saveToDB(DB_HANDLE hdb, UINT32 dwIndex)
{
   // Prepare and execute query
	DB_STATEMENT hStmt;
	if (!IsDatabaseRecordExist(hdb, _T("thresholds"), _T("threshold_id"), m_id))
	{
		hStmt = DBPrepare(hdb,
			_T("INSERT INTO thresholds (item_id,fire_value,rearm_value,")
			_T("check_function,check_operation,sample_count,script,event_code,")
			_T("sequence_number,current_state,rearm_event_code,repeat_interval,")
			_T("current_severity,last_event_timestamp,match_count,threshold_id) ")
			_T("VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
	}
	else
	{
		hStmt = DBPrepare(hdb,
			_T("UPDATE thresholds SET item_id=?,fire_value=?,rearm_value=?,check_function=?,")
         _T("check_operation=?,sample_count=?,script=?,event_code=?,")
         _T("sequence_number=?,current_state=?,rearm_event_code=?,")
			_T("repeat_interval=?,current_severity=?,last_event_timestamp=?,")
         _T("match_count=? WHERE threshold_id=?"));
	}
	if (hStmt == NULL)
		return FALSE;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_itemId);
	DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_value.getString(), DB_BIND_STATIC);
	DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, NULL, DB_BIND_STATIC);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (INT32)m_function);
	DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (INT32)m_operation);
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (INT32)m_sampleCount);
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, m_scriptSource, DB_BIND_STATIC);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_eventCode);
	DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, dwIndex);
	DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, (INT32)(m_isReached ? 1 : 0));
	DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, m_rearmEventCode);
	DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, (INT32)m_repeatInterval);
	DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, (INT32)m_currentSeverity);
	DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, (INT32)m_lastEventTimestamp);
	DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, (INT32)m_numMatches);
	DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, (INT32)m_id);

	BOOL success = DBExecute(hStmt);
	DBFreeStatement(hStmt);
	return success;
}

/**
 * Check threshold
 * Method will return the following codes:
 *    THRESHOLD_REACHED - when item's value match the threshold condition while previous check doesn't
 *    THRESHOLD_REARMED - when item's value doesn't match the threshold condition while previous check do
 *    NO_ACTION - when there are no changes in item's value match to threshold's condition
 */
ThresholdCheckResult Threshold::check(ItemValue &value, ItemValue **ppPrevValues, ItemValue &fvalue, NetObj *target, DCItem *dci)
{
   // check if there is enough cached data
   switch(m_function)
   {
      case F_DIFF:
         if (ppPrevValues[0]->getTimeStamp() == 1) // Timestamp 1 means placeholder value inserted by cache loader
            return m_isReached ? ALREADY_ACTIVE : ALREADY_INACTIVE;
         break;
      case F_AVERAGE:
      case F_SUM:
      case F_DEVIATION:
         for(int i = 0; i < m_sampleCount - 1; i++)
            if (ppPrevValues[i]->getTimeStamp() == 1) // Timestamp 1 means placeholder value inserted by cache loader
               return m_isReached ? ALREADY_ACTIVE : ALREADY_INACTIVE;
         break;
      default:
         break;
   }

   BOOL bMatch = FALSE;
   int iDataType = m_dataType;

   // Execute function on value
   switch(m_function)
   {
      case F_LAST:         // Check last value only
      case F_SCRIPT:
         fvalue = value;
         break;
      case F_AVERAGE:      // Check average value for last n polls
         calculateAverageValue(&fvalue, value, ppPrevValues);
         break;
		case F_SUM:
         calculateSumValue(&fvalue, value, ppPrevValues);
			break;
      case F_DEVIATION:    // Check mean absolute deviation
         calculateMDValue(&fvalue, value, ppPrevValues);
         break;
      case F_DIFF:
         calculateDiff(&fvalue, value, ppPrevValues);
         if (m_dataType == DCI_DT_STRING)
            iDataType = DCI_DT_INT;  // diff() for strings is an integer
         break;
      case F_ERROR:        // Check for collection error
         fvalue = (UINT32)0;
         break;
      default:
         break;
   }

   // Run comparision operation on function result and threshold value
   if (m_function == F_ERROR)
   {
      // Threshold::Check() can be called only for valid values, which
      // means that error thresholds cannot be active
      bMatch = FALSE;
   }
   else if (m_function == F_SCRIPT)
   {
      if (m_script != NULL)
      {
         NXSL_Value *parameters[2];
         parameters[0] = new NXSL_Value(value.getString());
         parameters[1] = new NXSL_Value(m_value.getString());
         m_script->setGlobalVariable(_T("$object"), new NXSL_Value(new NXSL_Object(&g_nxslNetObjClass, target)));
         if (target->getObjectClass() == OBJECT_NODE)
         {
            m_script->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, target)));
         }
         m_script->setGlobalVariable(_T("$dci"), new NXSL_Value(new NXSL_Object(&g_nxslDciClass, dci)));
         m_script->setGlobalVariable(_T("$isCluster"), new NXSL_Value((target->getObjectClass() == OBJECT_CLUSTER) ? 1 : 0));
         if (m_script->run(2, parameters))
         {
            NXSL_Value *result = m_script->getResult();
            if (result != NULL)
            {
               bMatch = (result->getValueAsInt32() != 0);
            }
         }
         else
         {
            TCHAR buffer[1024];
            _sntprintf(buffer, 1024, _T("DCI::%s::%d::%d::ThresholdScript"), target->getName(), dci->getId(), m_id);
            PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, m_script->getErrorText(), dci->getId());
         }
      }
      else
      {
         DbgPrintf(7, _T("Script not compiled for threshold %d of DCI %d of data collection target %s [%d]"),
                   m_id, dci->getId(), target->getName(), target->getId());
      }
   }
   else
   {
      switch(m_operation)
      {
         case OP_LE:    // Less
            switch(iDataType)
            {
               case DCI_DT_INT:
                  bMatch = ((INT32)fvalue < (INT32)m_value);
                  break;
               case DCI_DT_UINT:
                  bMatch = ((UINT32)fvalue < (UINT32)m_value);
                  break;
               case DCI_DT_INT64:
                  bMatch = ((INT64)fvalue < (INT64)m_value);
                  break;
               case DCI_DT_UINT64:
                  bMatch = ((UINT64)fvalue < (UINT64)m_value);
                  break;
               case DCI_DT_FLOAT:
                  bMatch = ((double)fvalue < (double)m_value);
                  break;
            }
            break;
         case OP_LE_EQ: // Less or equal
            switch(iDataType)
            {
               case DCI_DT_INT:
                  bMatch = ((INT32)fvalue <= (INT32)m_value);
                  break;
               case DCI_DT_UINT:
                  bMatch = ((UINT32)fvalue <= (UINT32)m_value);
                  break;
               case DCI_DT_INT64:
                  bMatch = ((INT64)fvalue <= (INT64)m_value);
                  break;
               case DCI_DT_UINT64:
                  bMatch = ((UINT64)fvalue <= (UINT64)m_value);
                  break;
               case DCI_DT_FLOAT:
                  bMatch = ((double)fvalue <= (double)m_value);
                  break;
            }
            break;
         case OP_EQ:    // Equal
            switch(iDataType)
            {
               case DCI_DT_INT:
                  bMatch = ((INT32)fvalue == (INT32)m_value);
                  break;
               case DCI_DT_UINT:
                  bMatch = ((UINT32)fvalue == (UINT32)m_value);
                  break;
               case DCI_DT_INT64:
                  bMatch = ((INT64)fvalue == (INT64)m_value);
                  break;
               case DCI_DT_UINT64:
                  bMatch = ((UINT64)fvalue == (UINT64)m_value);
                  break;
               case DCI_DT_FLOAT:
                  bMatch = ((double)fvalue == (double)m_value);
                  break;
               case DCI_DT_STRING:
                  bMatch = !_tcscmp(fvalue.getString(), m_value.getString());
                  break;
            }
            break;
         case OP_GT_EQ: // Greater or equal
            switch(iDataType)
            {
               case DCI_DT_INT:
                  bMatch = ((INT32)fvalue >= (INT32)m_value);
                  break;
               case DCI_DT_UINT:
                  bMatch = ((UINT32)fvalue >= (UINT32)m_value);
                  break;
               case DCI_DT_INT64:
                  bMatch = ((INT64)fvalue >= (INT64)m_value);
                  break;
               case DCI_DT_UINT64:
                  bMatch = ((UINT64)fvalue >= (UINT64)m_value);
                  break;
               case DCI_DT_FLOAT:
                  bMatch = ((double)fvalue >= (double)m_value);
                  break;
            }
            break;
         case OP_GT:    // Greater
            switch(iDataType)
            {
               case DCI_DT_INT:
                  bMatch = ((INT32)fvalue > (INT32)m_value);
                  break;
               case DCI_DT_UINT:
                  bMatch = ((UINT32)fvalue > (UINT32)m_value);
                  break;
               case DCI_DT_INT64:
                  bMatch = ((INT64)fvalue > (INT64)m_value);
                  break;
               case DCI_DT_UINT64:
                  bMatch = ((UINT64)fvalue > (UINT64)m_value);
                  break;
               case DCI_DT_FLOAT:
                  bMatch = ((double)fvalue > (double)m_value);
                  break;
            }
            break;
         case OP_NE:    // Not equal
            switch(iDataType)
            {
               case DCI_DT_INT:
                  bMatch = ((INT32)fvalue != (INT32)m_value);
                  break;
               case DCI_DT_UINT:
                  bMatch = ((UINT32)fvalue != (UINT32)m_value);
                  break;
               case DCI_DT_INT64:
                  bMatch = ((INT64)fvalue != (INT64)m_value);
                  break;
               case DCI_DT_UINT64:
                  bMatch = ((UINT64)fvalue != (UINT64)m_value);
                  break;
               case DCI_DT_FLOAT:
                  bMatch = ((double)fvalue != (double)m_value);
                  break;
               case DCI_DT_STRING:
                  bMatch = _tcscmp(fvalue.getString(), m_value.getString());
                  break;
            }
            break;
         case OP_LIKE:
            // This operation can be performed only on strings
            if (m_dataType == DCI_DT_STRING)
               bMatch = MatchString(m_value.getString(), fvalue.getString(), true);
            break;
         case OP_NOTLIKE:
            // This operation can be performed only on strings
            if (m_dataType == DCI_DT_STRING)
               bMatch = !MatchString(m_value.getString(), fvalue.getString(), true);
            break;
         default:
            break;
      }
   }

	// Check for number of consecutive matches
	if ((m_function == F_LAST) || (m_function == F_SCRIPT))
	{
		if (bMatch)
		{
			m_numMatches++;
			if (m_numMatches < m_sampleCount)
				bMatch = FALSE;
		}
		else
		{
			m_numMatches = 0;
		}
	}

   ThresholdCheckResult result = (bMatch & !m_isReached) ? ACTIVATED : ((!bMatch & m_isReached) ? DEACTIVATED : (m_isReached ? ALREADY_ACTIVE : ALREADY_INACTIVE));
   m_isReached = bMatch;
   if (result == ACTIVATED || result == DEACTIVATED)
   {
      // Update threshold status in database
      TCHAR szQuery[256];
      _sntprintf(szQuery, 256, _T("UPDATE thresholds SET current_state=%d WHERE threshold_id=%d"), (int)m_isReached, (int)m_id);
      QueueSQLRequest(szQuery);
   }
   return result;
}

/**
 * Mark last activation event
 */
void Threshold::markLastEvent(int severity)
{
	m_lastEventTimestamp = time(NULL);
	m_currentSeverity = (BYTE)severity;

	// Update threshold in database
	TCHAR query[256];
   _sntprintf(query, 256,
              _T("UPDATE thresholds SET current_severity=%d,last_event_timestamp=%d WHERE threshold_id=%d"),
              (int)m_currentSeverity, (int)m_lastEventTimestamp, (int)m_id);
	QueueSQLRequest(query);
}

/**
 * Check for collection error thresholds
 * Return same values as Check()
 */
ThresholdCheckResult Threshold::checkError(UINT32 dwErrorCount)
{
   if (m_function != F_ERROR)
      return m_isReached ? ALREADY_ACTIVE : ALREADY_INACTIVE;

   BOOL bMatch = ((UINT32)m_sampleCount <= dwErrorCount);
   ThresholdCheckResult result = (bMatch & !m_isReached) ? ACTIVATED : ((!bMatch & m_isReached) ? DEACTIVATED : (m_isReached ? ALREADY_ACTIVE : ALREADY_INACTIVE));
   m_isReached = bMatch;
   if (result == ACTIVATED || result == DEACTIVATED)
   {
      // Update threshold status in database
      TCHAR szQuery[256];
      _sntprintf(szQuery, 256, _T("UPDATE thresholds SET current_state=%d WHERE threshold_id=%d"), m_isReached, m_id);
      QueueSQLRequest(szQuery);
   }
   return result;
}

/**
 * Fill DCI_THRESHOLD with object's data ready to send over the network
 */
void Threshold::createMessage(NXCPMessage *msg, UINT32 baseId)
{
	UINT32 varId = baseId;

	msg->setField(varId++, m_id);
	msg->setField(varId++, m_eventCode);
	msg->setField(varId++, m_rearmEventCode);
	msg->setField(varId++, (WORD)m_function);
	msg->setField(varId++, (WORD)m_operation);
	msg->setField(varId++, (UINT32)m_sampleCount);
	msg->setField(varId++, CHECK_NULL_EX(m_scriptSource));
	msg->setField(varId++, (UINT32)m_repeatInterval);
	msg->setField(varId++, m_value.getString());
	msg->setField(varId++, (WORD)m_isReached);
	msg->setField(varId++, (WORD)m_currentSeverity);
	msg->setField(varId++, (UINT32)m_lastEventTimestamp);
}

/**
 * Update threshold object from NXCP message
 */
void Threshold::updateFromMessage(NXCPMessage *msg, UINT32 baseId)
{
	TCHAR buffer[MAX_DCI_STRING_VALUE];
	UINT32 varId = baseId + 1;	// Skip ID field

	m_eventCode = msg->getFieldAsUInt32(varId++);
	m_rearmEventCode = msg->getFieldAsUInt32(varId++);
   m_function = (BYTE)msg->getFieldAsUInt16(varId++);
   m_operation = (BYTE)msg->getFieldAsUInt16(varId++);
   m_sampleCount = (int)msg->getFieldAsUInt32(varId++);
   setScript(msg->getFieldAsString(varId++));
	m_repeatInterval = (int)msg->getFieldAsUInt32(varId++);
	m_value = msg->getFieldAsString(varId++, buffer, MAX_DCI_STRING_VALUE);
}

/**
 * Calculate average value for parameter
 */
#define CALC_AVG_VALUE(vtype) \
{ \
   vtype var; \
   var = (vtype)lastValue; \
   for(int i = 1; i < m_sampleCount; i++) \
   { \
      var += (vtype)(*ppPrevValues[i - 1]); \
   } \
   *pResult = var / (vtype)m_sampleCount; \
}

void Threshold::calculateAverageValue(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues)
{
   switch(m_dataType)
   {
      case DCI_DT_INT:
         CALC_AVG_VALUE(INT32);
         break;
      case DCI_DT_UINT:
         CALC_AVG_VALUE(UINT32);
         break;
      case DCI_DT_INT64:
         CALC_AVG_VALUE(INT64);
         break;
      case DCI_DT_UINT64:
         CALC_AVG_VALUE(UINT64);
         break;
      case DCI_DT_FLOAT:
         CALC_AVG_VALUE(double);
         break;
      case DCI_DT_STRING:
         *pResult = _T("");   // Average value for string is meaningless
         break;
      default:
         break;
   }
}

/**
 * Calculate sum value for values of given type
 */
#define CALC_SUM_VALUE(vtype) \
{ \
   vtype var; \
   var = (vtype)lastValue; \
   for(int i = 1; i < m_sampleCount; i++) \
   { \
      var += (vtype)(*ppPrevValues[i - 1]); \
   } \
   *pResult = var; \
}

/**
 * Calculate sum value for parameter
 */
void Threshold::calculateSumValue(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues)
{
   switch(m_dataType)
   {
      case DCI_DT_INT:
         CALC_SUM_VALUE(INT32);
         break;
      case DCI_DT_UINT:
         CALC_SUM_VALUE(UINT32);
         break;
      case DCI_DT_INT64:
         CALC_SUM_VALUE(INT64);
         break;
      case DCI_DT_UINT64:
         CALC_SUM_VALUE(UINT64);
         break;
      case DCI_DT_FLOAT:
         CALC_SUM_VALUE(double);
         break;
      case DCI_DT_STRING:
         *pResult = _T("");   // Sum value for string is meaningless
         break;
      default:
         break;
   }
}

/**
 * Calculate mean absolute deviation for values of given type
 */
#define CALC_MD_VALUE(vtype) \
{ \
   vtype mean, dev; \
   mean = (vtype)lastValue; \
   for(i = 1; i < m_sampleCount; i++) \
   { \
      mean += (vtype)(*ppPrevValues[i - 1]); \
   } \
   mean /= (vtype)m_sampleCount; \
   dev = ABS((vtype)lastValue - mean); \
   for(i = 1; i < m_sampleCount; i++) \
   { \
      dev += ABS((vtype)(*ppPrevValues[i - 1]) - mean); \
   } \
   *pResult = dev / (vtype)m_sampleCount; \
}

/**
 * Calculate mean absolute deviation for parameter
 */
void Threshold::calculateMDValue(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues)
{
   int i;

   switch(m_dataType)
   {
      case DCI_DT_INT:
#define ABS(x) ((x) < 0 ? -(x) : (x))
         CALC_MD_VALUE(INT32);
         break;
      case DCI_DT_INT64:
         CALC_MD_VALUE(INT64);
         break;
      case DCI_DT_FLOAT:
         CALC_MD_VALUE(double);
         break;
      case DCI_DT_UINT:
#undef ABS
#define ABS(x) (x)
         CALC_MD_VALUE(UINT32);
         break;
      case DCI_DT_UINT64:
         CALC_MD_VALUE(UINT64);
         break;
      case DCI_DT_STRING:
         *pResult = _T("");   // Mean deviation for string is meaningless
         break;
      default:
         break;
   }
}

#undef ABS

/**
 * Calculate difference between last and previous value
 */
void Threshold::calculateDiff(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues)
{
   CalculateItemValueDiff(*pResult, m_dataType, lastValue, *ppPrevValues[0]);
}

/**
 * Compare to another threshold
 */
BOOL Threshold::compare(Threshold *pThr)
{
   BOOL bMatch;

   switch(m_dataType)
   {
      case DCI_DT_INT:
         bMatch = ((INT32)pThr->m_value == (INT32)m_value);
         break;
      case DCI_DT_UINT:
         bMatch = ((UINT32)pThr->m_value == (UINT32)m_value);
         break;
      case DCI_DT_INT64:
         bMatch = ((INT64)pThr->m_value == (INT64)m_value);
         break;
      case DCI_DT_UINT64:
         bMatch = ((UINT64)pThr->m_value == (UINT64)m_value);
         break;
      case DCI_DT_FLOAT:
         bMatch = ((double)pThr->m_value == (double)m_value);
         break;
      case DCI_DT_STRING:
         bMatch = !_tcscmp(pThr->m_value.getString(), m_value.getString());
         break;
      default:
         bMatch = TRUE;
         break;
   }
   return bMatch &&
          (pThr->m_eventCode == m_eventCode) &&
          (pThr->m_rearmEventCode == m_rearmEventCode) &&
          (pThr->m_dataType == m_dataType) &&
          (pThr->m_function == m_function) &&
          (pThr->m_operation == m_operation) &&
          (pThr->m_sampleCount == m_sampleCount) &&
          !_tcscmp(CHECK_NULL_EX(pThr->m_scriptSource), CHECK_NULL_EX(m_scriptSource)) &&
			 (pThr->m_repeatInterval == m_repeatInterval);
}

/**
 * Create management pack record
 */
void Threshold::createNXMPRecord(String &str, int index)
{
   TCHAR activationEvent[MAX_EVENT_NAME], deactivationEvent[MAX_EVENT_NAME];

   EventNameFromCode(m_eventCode, activationEvent);
   EventNameFromCode(m_rearmEventCode, deactivationEvent);
   str.appendFormattedString(_T("\t\t\t\t\t\t<threshold id=\"%d\">\n")
                          _T("\t\t\t\t\t\t\t<function>%d</function>\n")
                          _T("\t\t\t\t\t\t\t<condition>%d</condition>\n")
                          _T("\t\t\t\t\t\t\t<value>%s</value>\n")
                          _T("\t\t\t\t\t\t\t<activationEvent>%s</activationEvent>\n")
                          _T("\t\t\t\t\t\t\t<deactivationEvent>%s</deactivationEvent>\n")
                          _T("\t\t\t\t\t\t\t<sampleCount>%d</sampleCount>\n")
                          _T("\t\t\t\t\t\t\t<repeatInterval>%d</repeatInterval>\n"),
								  index, m_function, m_operation,
								  (const TCHAR *)EscapeStringForXML2(m_value.getString()),
                          (const TCHAR *)EscapeStringForXML2(activationEvent),
								  (const TCHAR *)EscapeStringForXML2(deactivationEvent),
								  m_sampleCount, m_repeatInterval);
   if (m_scriptSource != NULL)
   {
      str.append(_T("\t\t\t\t\t\t\t<script>"));
      str.append(EscapeStringForXML2(m_scriptSource));
      str.append(_T("</script>\n"));
   }
   str.append(_T("\t\t\t\t\t\t</threshold>\n"));
}

/**
 * Make an association with DCI (used by management pack parser)
 */
void Threshold::associate(DCItem *pItem)
{
   m_itemId = pItem->getId();
   m_targetId = pItem->getTarget()->getId();
   m_dataType = pItem->getDataType();
}

/**
 * Set new script. Script source must be dynamically allocated
 * and will be deallocated by threshold object
 */
void Threshold::setScript(TCHAR *script)
{
   safe_free(m_scriptSource);
   delete m_script;
   if (script != NULL)
   {
      m_scriptSource = script;
      StrStrip(m_scriptSource);
      if (m_scriptSource[0] != 0)
      {
			/* TODO: add compilation error handling */
         m_script = NXSLCompileAndCreateVM(m_scriptSource, NULL, 0, new NXSL_ServerEnv);
      }
      else
      {
         m_script = NULL;
      }
   }
   else
   {
      m_scriptSource = NULL;
      m_script = NULL;
   }
}
