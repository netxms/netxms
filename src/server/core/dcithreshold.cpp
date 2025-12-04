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
** File: dcithreshold.cpp
**
**/

#include "nxcore.h"

/**
 * Create new threshold for given DCI
 */
Threshold::Threshold(DCItem *relatedItem)
{
   m_id = 0;
   m_itemId = relatedItem->getId();
   m_targetId = relatedItem->getOwnerId();
   m_eventCode = EVENT_THRESHOLD_REACHED;
   m_rearmEventCode = EVENT_THRESHOLD_REARMED;
   m_expandValue = false;
   m_function = F_LAST;
   m_operation = OP_EQ;
   m_dataType = relatedItem->getTransformedDataType();
   m_sampleCount = 1;
   m_scriptSource = nullptr;
   m_script = nullptr;
   m_lastScriptErrorReport = 0;
   m_isReached = false;
   m_wasReachedBeforeMaint = false;
   m_disabled = false;
	m_currentSeverity = SEVERITY_NORMAL;
	m_repeatInterval = -1;
	m_lastEventTimestamp = 0;
	m_lastEventMessage = nullptr;
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
   m_expandValue = false;
   m_function = F_LAST;
   m_operation = OP_EQ;
   m_dataType = 0;
   m_sampleCount = 1;
   m_scriptSource = nullptr;
   m_script = nullptr;
   m_lastScriptErrorReport = 0;
   m_isReached = false;
   m_wasReachedBeforeMaint = false;
   m_disabled = false;
	m_currentSeverity = SEVERITY_NORMAL;
	m_repeatInterval = -1;
	m_lastEventTimestamp = 0;
   m_lastEventMessage = nullptr;
	m_numMatches = 0;
}

/**
 * Create from another threshold object
 */
Threshold::Threshold(const Threshold& src, bool shadowCopy) : m_value(src.m_value)
{
   m_id = shadowCopy ? src.m_id : CreateUniqueId(IDG_THRESHOLD);
   m_itemId = src.m_itemId;
   m_targetId = src.m_targetId;
   m_eventCode = src.m_eventCode;
   m_rearmEventCode = src.m_rearmEventCode;
   m_expandValue = src.m_expandValue;
   m_function = src.m_function;
   m_operation = src.m_operation;
   m_dataType = src.m_dataType;
   m_sampleCount = src.m_sampleCount;
   m_scriptSource = nullptr;
   m_script = nullptr;
   setScript(MemCopyString(src.m_scriptSource));
   m_lastScriptErrorReport = shadowCopy ? src.m_lastScriptErrorReport : 0;
   m_isReached = shadowCopy ? src.m_isReached : false;
   m_wasReachedBeforeMaint = shadowCopy ? src.m_wasReachedBeforeMaint : false;
   m_disabled = src.m_disabled;
	m_currentSeverity = shadowCopy ? src.m_currentSeverity : SEVERITY_NORMAL;
	m_repeatInterval = src.m_repeatInterval;
	m_lastEventTimestamp = shadowCopy ? src.m_lastEventTimestamp : 0;
   m_lastEventMessage = nullptr;
	m_numMatches = shadowCopy ? src.m_numMatches : 0;
}

/**
 * Constructor for creating object from database
 * This constructor assumes that SELECT query look as following:
 * SELECT threshold_id,fire_value,rearm_value,check_function,check_operation,
 *        sample_count,script,event_code,current_state,rearm_event_code,
 *        repeat_interval,current_severity,last_event_timestamp,match_count,
 *        state_before_maint,last_checked_value,last_event_message,is_disabled FROM thresholds
 */
Threshold::Threshold(DB_RESULT hResult, int row, DCItem *relatedItem) : m_value(hResult, row, 1, Timestamp::fromMilliseconds(0), true)
{
   wchar_t textBuffer[MAX_EVENT_MSG_LENGTH];

   m_id = DBGetFieldULong(hResult, row, 0);
   m_function = (BYTE)DBGetFieldLong(hResult, row, 3);
   m_operation = (BYTE)DBGetFieldLong(hResult, row, 4);
   m_sampleCount = DBGetFieldLong(hResult, row, 5);
   m_scriptSource = nullptr;
   m_script = nullptr;
   setScript(DBGetField(hResult, row, 6, nullptr, 0));
   m_eventCode = DBGetFieldULong(hResult, row, 7);
   m_isReached = DBGetFieldLong(hResult, row, 8);
   m_rearmEventCode = DBGetFieldULong(hResult, row, 9);
   m_repeatInterval = DBGetFieldLong(hResult, row, 10);
   m_currentSeverity = (BYTE)DBGetFieldLong(hResult, row, 11);
   m_lastEventTimestamp = (time_t)DBGetFieldULong(hResult, row, 12);
   m_numMatches = DBGetFieldLong(hResult, row, 13);
   m_wasReachedBeforeMaint = DBGetFieldLong(hResult, row, 14) ? true : false;
   DBGetField(hResult, row, 15, textBuffer, MAX_DB_STRING);
   m_lastCheckValue = textBuffer;
   DBGetField(hResult, row, 16, textBuffer, MAX_EVENT_MSG_LENGTH);
   m_lastEventMessage = (textBuffer[0] != 0) ? MemCopyString(textBuffer) : nullptr;
   m_disabled = DBGetFieldLong(hResult, row, 17) ? true : false;

   m_lastScriptErrorReport = 0;
   m_itemId = relatedItem->getId();
   m_targetId = relatedItem->getOwnerId();
   m_dataType = relatedItem->getTransformedDataType();
   m_expandValue = (NumChars(m_value, '%') > 0);

	if ((m_function == F_LAST) && (m_sampleCount < 1))
		m_sampleCount = 1;
}

/**
 * Create threshold from import file
 */
Threshold::Threshold(ConfigEntry *config, DCItem *parentItem, bool nxslV5)
{
   createId();
   m_itemId = parentItem->getId();
   m_targetId = parentItem->getOwnerId();
   m_eventCode = EventCodeFromName(config->getSubEntryValue(_T("activationEvent"), 0, _T("SYS_THRESHOLD_REACHED")), EVENT_THRESHOLD_REACHED);
   m_rearmEventCode = EventCodeFromName(config->getSubEntryValue(_T("deactivationEvent"), 0, _T("SYS_THRESHOLD_REARMED")), EVENT_THRESHOLD_REARMED);
   m_function = (BYTE)config->getSubEntryValueAsInt(_T("function"), 0, F_LAST);
   m_operation = (BYTE)config->getSubEntryValueAsInt(_T("condition"), 0, OP_EQ);
   m_dataType = parentItem->getTransformedDataType();
	m_value.set(config->getSubEntryValue(_T("value"), 0, _T("")), true);
   m_expandValue = (NumChars(m_value, '%') > 0);
   m_sampleCount = (config->getSubEntryValue(_T("sampleCount")) != nullptr) ? config->getSubEntryValueAsInt(_T("sampleCount"), 0, 1) : config->getSubEntryValueAsInt(_T("param1"), 0, 1);
   m_scriptSource = nullptr;
   m_script = nullptr;
   m_lastScriptErrorReport = 0;
   const TCHAR *script = config->getSubEntryValue(_T("script"), 0, _T(""));
   if (nxslV5)
   {
      setScript(MemCopyString(script));
   }
   else
   {
      StringBuffer output = NXSLConvertToV5(script);
      setScript(MemCopyString(output));
   }
   m_isReached = false;
   m_wasReachedBeforeMaint = false;
   m_disabled = false;
	m_currentSeverity = SEVERITY_NORMAL;
	m_repeatInterval = config->getSubEntryValueAsInt(_T("repeatInterval"), 0, -1);
	m_lastEventTimestamp = 0;
	m_lastEventMessage = nullptr;
	m_numMatches = 0;
}

/**
 * Destructor
 */
Threshold::~Threshold()
{
   MemFree(m_scriptSource);
   delete m_script;
   MemFree(m_lastEventMessage);
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
bool Threshold::saveToDB(DB_HANDLE hdb, uint32_t index)
{
   static const TCHAR *columns[] = {
      _T("item_id"), _T("fire_value"), _T("rearm_value"), _T("check_function"), _T("check_operation"), _T("sample_count"),
      _T("script"), _T("event_code"), _T("sequence_number"), _T("current_state"), _T("state_before_maint"), _T("rearm_event_code"),
      _T("repeat_interval"), _T("current_severity"), _T("last_event_timestamp"), _T("match_count"), _T("last_checked_value"),
      _T("last_event_message"), _T("is_disabled"), nullptr
   };
	DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("thresholds"), _T("threshold_id"), m_id, columns);
	if (hStmt == nullptr)
		return false;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_itemId);
	DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_value.getString(), DB_BIND_STATIC);
	DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, _T(""), DB_BIND_STATIC);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_function));
	DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_operation));
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_sampleCount);
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, m_scriptSource, DB_BIND_STATIC);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_eventCode);
	DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, index);
	DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_isReached ? 1 : 0));
   DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, (m_wasReachedBeforeMaint ? _T("1") : _T("0")), DB_BIND_STATIC);
	DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, m_rearmEventCode);
	DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_repeatInterval);
	DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_currentSeverity));
	DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastEventTimestamp));
	DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, m_numMatches);
   DBBind(hStmt, 17, DB_SQLTYPE_VARCHAR, m_lastCheckValue.getString(), DB_BIND_STATIC);
   DBBind(hStmt, 18, DB_SQLTYPE_VARCHAR, m_lastEventMessage, DB_BIND_STATIC);
   DBBind(hStmt, 19, DB_SQLTYPE_VARCHAR, (m_disabled ? _T("1") : _T("0")), DB_BIND_STATIC);
	DBBind(hStmt, 20, DB_SQLTYPE_INTEGER, m_id);

	bool success = DBExecute(hStmt);
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
ThresholdCheckResult Threshold::check(ItemValue &value, ItemValue **ppPrevValues, ItemValue &fvalue, ItemValue &tvalue, shared_ptr<NetObj> target, DCItem *dci)
{
   if (m_disabled)
   {
      if (!m_isReached)
         return ThresholdCheckResult::ALREADY_INACTIVE;

      m_isReached = false;
      // Update threshold status in database
      TCHAR query[256];
      _sntprintf(query, 256, _T("UPDATE thresholds SET current_state=0 WHERE threshold_id=%u"), m_id);
      QueueSQLRequest(query);
      return ThresholdCheckResult::DEACTIVATED;
   }

   // check if there is enough cached data
   switch(m_function)
   {
      case F_DIFF:
         if (ppPrevValues[0]->getTimeStamp().asMilliseconds() == 1) // Timestamp 1 means placeholder value inserted by cache loader
            return m_isReached ? ThresholdCheckResult::ALREADY_ACTIVE : ThresholdCheckResult::ALREADY_INACTIVE;
         break;
      case F_AVERAGE:
      case F_SUM:
      case F_MEAN_DEVIATION:
         for(int i = 0; i < m_sampleCount - 1; i++)
            if (ppPrevValues[i]->getTimeStamp().asMilliseconds() == 1) // Timestamp 1 means placeholder value inserted by cache loader
               return m_isReached ? ThresholdCheckResult::ALREADY_ACTIVE : ThresholdCheckResult::ALREADY_INACTIVE;
         break;
      default:
         break;
   }

   bool match = false;
   int dataType = m_dataType;

   // Execute function on value
   switch(m_function)
   {
      case F_LAST:         // Check last value only
      case F_SCRIPT:
         fvalue = value;
         break;
      case F_AVERAGE:      // Check average value for last n polls
         calculateAverage(&fvalue, value, ppPrevValues);
         break;
		case F_SUM:
         calculateTotal(&fvalue, value, ppPrevValues);
			break;
      case F_MEAN_DEVIATION:    // Check mean absolute deviation
         calculateMeanDeviation(&fvalue, value, ppPrevValues);
         break;
      case F_ABS_DEVIATION:    // Check absolute deviation for last point
         calculateAbsoluteDeviation(&fvalue, value, ppPrevValues);
         break;
      case F_DIFF:
         CalculateItemValueDiff(&fvalue, m_dataType, value, *ppPrevValues[0]);
         switch(m_dataType)
         {
            case DCI_DT_STRING:
               dataType = DCI_DT_INT;  // diff() for strings is an integer
               break;
            case DCI_DT_UINT:
            case DCI_DT_UINT64:
            case DCI_DT_COUNTER64:
               dataType = DCI_DT_INT64;  // diff() always signed
               break;
         }
         break;
      case F_ANOMALY:        // Check for data anomaly
         fvalue = static_cast<uint32_t>(dci->isAnomalyDetected() ? 1 : 0);
         break;
      case F_ERROR:        // Check for collection error
         fvalue = static_cast<uint32_t>(0);
         break;
      default:
         break;
   }

   // Run comparison operation on function result and threshold value
   if (m_function == F_ERROR)
   {
      // Threshold::Check() can be called only for valid values, which
      // means that error thresholds cannot be active
      match = false;
   }
   else if (m_function == F_ANOMALY)
   {
      match = dci->isAnomalyDetected();
   }
   else if (m_function == F_SCRIPT)
   {
      tvalue = m_expandValue ?
               ItemValue(target->expandText(m_value.getString(), nullptr, nullptr, dci->createDescriptor(), nullptr, nullptr, dci->getInstanceName(), nullptr, nullptr), m_value.getTimeStamp(), true) :
               m_value;
      if ((m_script != nullptr) && !m_script->isEmpty())
      {
         NXSL_VM *vm = CreateServerScriptVM(m_script, target, dci->createDescriptor());
         if (vm != nullptr)
         {
            NXSL_Value *parameters[2];
            parameters[0] = vm->createValue(value.getString());
            parameters[1] = vm->createValue(tvalue.getString());
            if (vm->run(2, parameters))
            {
               match = vm->getResult()->getValueAsBoolean();
            }
            else
            {
               time_t now = time(nullptr);
               if (m_lastScriptErrorReport + ConfigReadInt(_T("DataCollection.ScriptErrorReportInterval"), 86400) < now)
               {
                  nxlog_debug_tag(DEBUG_TAG_DC_THRESHOLDS, 5, _T("Failed to execute threshold script for node %s [%u] DCI \"%s\" [%u] threshold [%u] (%s)"),
                           target->getName(), target->getId(), dci->getName().cstr(), dci->getId(), m_id, vm->getErrorText());
                  ReportScriptError(SCRIPT_CONTEXT_DCI, target.get(), dci->getId(), vm->getErrorText(), _T("DCI::%s::%d::%d::ThresholdScript"), target->getName(), dci->getId(), m_id);
                  m_lastScriptErrorReport = now;
               }
            }
            delete vm;
         }
         else
         {
            time_t now = time(nullptr);
            if (m_lastScriptErrorReport + ConfigReadInt(_T("DataCollection.ScriptErrorReportInterval"), 86400) < now)
            {
               nxlog_debug_tag(DEBUG_TAG_DC_THRESHOLDS, 5, _T("Failed to load threshold script for node %s [%u] DCI \"%s\" [%u] threshold [%u]"),
                        target->getName(), target->getId(), dci->getName().cstr(), dci->getId(), m_id);
               ReportScriptError(SCRIPT_CONTEXT_DCI, target.get(), dci->getId(), _T("Script load failed"), _T("DCI::%s::%d::%d::ThresholdScript"), target->getName(), dci->getId(), m_id);
               m_lastScriptErrorReport = now;
            }
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_DC_THRESHOLDS, 7, _T("Script not compiled for threshold [%u] of DCI \"%s\" [%u] of data collection target %s [%u]"),
               m_id, dci->getName().cstr(), dci->getId(), target->getName(), target->getId());
      }
   }
   else
   {
      tvalue = m_expandValue ?
               ItemValue(target->expandText(m_value.getString(), nullptr, nullptr, dci->createDescriptor(), nullptr, nullptr, dci->getInstanceName(), nullptr, nullptr), m_value.getTimeStamp(), true) :
               m_value;
      switch(m_operation)
      {
         case OP_LE:    // Less
            switch(dataType)
            {
               case DCI_DT_INT:
                  match = (fvalue.getInt32() < tvalue.getInt32());
                  break;
               case DCI_DT_UINT:
               case DCI_DT_COUNTER32:
                  match = (fvalue.getUInt32() < tvalue.getUInt32());
                  break;
               case DCI_DT_INT64:
                  match = (fvalue.getInt64() < tvalue.getInt64());
                  break;
               case DCI_DT_UINT64:
               case DCI_DT_COUNTER64:
                  match = (fvalue.getUInt64() < tvalue.getUInt64());
                  break;
               case DCI_DT_FLOAT:
                  match = (fvalue.getDouble() < tvalue.getDouble());
                  break;
            }
            break;
         case OP_LE_EQ: // Less or equal
            switch(dataType)
            {
               case DCI_DT_INT:
                  match = (fvalue.getInt32() <= tvalue.getInt32());
                  break;
               case DCI_DT_UINT:
               case DCI_DT_COUNTER32:
                  match = (fvalue.getUInt32() <= tvalue.getUInt32());
                  break;
               case DCI_DT_INT64:
                  match = (fvalue.getInt64() <= tvalue.getInt64());
                  break;
               case DCI_DT_UINT64:
               case DCI_DT_COUNTER64:
                  match = (fvalue.getUInt64() <= tvalue.getUInt64());
                  break;
               case DCI_DT_FLOAT:
                  match = (fvalue.getDouble() <= tvalue.getDouble());
                  break;
            }
            break;
         case OP_EQ:    // Equal
            switch(dataType)
            {
               case DCI_DT_INT:
                  match = (fvalue.getInt32() == tvalue.getInt32());
                  break;
               case DCI_DT_UINT:
               case DCI_DT_COUNTER32:
                  match = (fvalue.getUInt32() == tvalue.getUInt32());
                  break;
               case DCI_DT_INT64:
                  match = (fvalue.getInt64() == tvalue.getInt64());
                  break;
               case DCI_DT_UINT64:
               case DCI_DT_COUNTER64:
                  match = (fvalue.getUInt64() == tvalue.getUInt64());
                  break;
               case DCI_DT_FLOAT:
                  match = (fabs(fvalue.getDouble() - tvalue.getDouble()) < 0.000001);
                  break;
               case DCI_DT_STRING:
                  match = (_tcscmp(fvalue.getString(), tvalue.getString()) == 0);
                  break;
            }
            break;
         case OP_GT_EQ: // Greater or equal
            switch(dataType)
            {
               case DCI_DT_INT:
                  match = (fvalue.getInt32() >= tvalue.getInt32());
                  break;
               case DCI_DT_UINT:
               case DCI_DT_COUNTER32:
                  match = (fvalue.getUInt32() >= tvalue.getUInt32());
                  break;
               case DCI_DT_INT64:
                  match = (fvalue.getInt64() >= tvalue.getInt64());
                  break;
               case DCI_DT_UINT64:
               case DCI_DT_COUNTER64:
                  match = (fvalue.getUInt64() >= tvalue.getUInt64());
                  break;
               case DCI_DT_FLOAT:
                  match = (fvalue.getDouble() >= tvalue.getDouble());
                  break;
            }
            break;
         case OP_GT:    // Greater
            switch(dataType)
            {
               case DCI_DT_INT:
                  match = (fvalue.getInt32() > tvalue.getInt32());
                  break;
               case DCI_DT_UINT:
               case DCI_DT_COUNTER32:
                  match = (fvalue.getUInt32() > tvalue.getUInt32());
                  break;
               case DCI_DT_INT64:
                  match = (fvalue.getInt64() > tvalue.getInt64());
                  break;
               case DCI_DT_UINT64:
               case DCI_DT_COUNTER64:
                  match = (fvalue.getUInt64() > tvalue.getUInt64());
                  break;
               case DCI_DT_FLOAT:
                  match = (fvalue.getDouble() > tvalue.getDouble());
                  break;
            }
            break;
         case OP_NE:    // Not equal
            switch(dataType)
            {
               case DCI_DT_INT:
                  match = (fvalue.getInt32() != tvalue.getInt32());
                  break;
               case DCI_DT_UINT:
               case DCI_DT_COUNTER32:
                  match = (fvalue.getUInt32() != tvalue.getUInt32());
                  break;
               case DCI_DT_INT64:
                  match = (fvalue.getInt64() != tvalue.getInt64());
                  break;
               case DCI_DT_UINT64:
               case DCI_DT_COUNTER64:
                  match = (fvalue.getUInt64() != tvalue.getUInt64());
                  break;
               case DCI_DT_FLOAT:
                  match = (fvalue.getDouble() != tvalue.getDouble());
                  break;
               case DCI_DT_STRING:
                  match = (_tcscmp(fvalue.getString(), tvalue.getString()) != 0);
                  break;
            }
            break;
         case OP_LIKE:
            // This operation can be performed only on strings
            if (m_dataType == DCI_DT_STRING)
               match = MatchString(tvalue.getString(), fvalue.getString(), true);
            break;
         case OP_NOTLIKE:
            // This operation can be performed only on strings
            if (m_dataType == DCI_DT_STRING)
               match = !MatchString(tvalue.getString(), fvalue.getString(), true);
            break;
         case OP_ILIKE:
            // This operation can be performed only on strings
            if (m_dataType == DCI_DT_STRING)
               match = MatchString(tvalue.getString(), fvalue.getString(), false);
            break;
         case OP_INOTLIKE:
            // This operation can be performed only on strings
            if (m_dataType == DCI_DT_STRING)
               match = !MatchString(tvalue.getString(), fvalue.getString(), false);
            break;
         default:
            break;
      }
   }

	// Check for number of consecutive matches
	if ((m_function == F_LAST) || (m_function == F_DIFF) || (m_function == F_SCRIPT) || (m_function == F_ANOMALY))
	{
		if (match)
		{
			m_numMatches++;
			if (m_numMatches < m_sampleCount)
				match = false;
		}
		else
		{
			m_numMatches = 0;
		}
	}

   ThresholdCheckResult result = (match && !m_isReached) ?
            ThresholdCheckResult::ACTIVATED :
                  ((!match && m_isReached) ? ThresholdCheckResult::DEACTIVATED :
                        (m_isReached ? ThresholdCheckResult::ALREADY_ACTIVE : ThresholdCheckResult::ALREADY_INACTIVE));
   m_isReached = match;
   if (result == ThresholdCheckResult::ACTIVATED || result == ThresholdCheckResult::DEACTIVATED)
   {
      // Update threshold status in database
      TCHAR query[256];
      _sntprintf(query, 256, _T("UPDATE thresholds SET current_state=%d WHERE threshold_id=%u"), (int)m_isReached, m_id);
      QueueSQLRequest(query);
   }
   return result;
}

/**
 * Mark last activation event
 */
void Threshold::markLastEvent(int severity, const wchar_t *message)
{
	m_lastEventTimestamp = time(nullptr);
	m_currentSeverity = (BYTE)severity;
	MemFree(m_lastEventMessage);
	m_lastEventMessage = MemCopyStringW(message);

	// Update threshold in database
	StringBuffer query(L"UPDATE thresholds SET current_severity=");
	query.append(severity);
	query.append(L",last_event_timestamp=");
	query.append(static_cast<int64_t>(m_lastEventTimestamp));
	query.append(L",last_event_message=");
	query.append(DBPrepareString(g_dbDriver, message, 2000));
	query.append(L"WHERE threshold_id=");
	query.append(m_id);
	QueueSQLRequest(query);
}

/**
 * Check for collection error thresholds
 * Return same values as Check()
 */
ThresholdCheckResult Threshold::checkError(uint32_t errorCount)
{
   if (m_function != F_ERROR)
      return m_isReached ? ThresholdCheckResult::ALREADY_ACTIVE : ThresholdCheckResult::ALREADY_INACTIVE;

   bool match = m_disabled ? false : (static_cast<uint32_t>(m_sampleCount) <= errorCount);
   ThresholdCheckResult result = (match && !m_isReached) ?
            ThresholdCheckResult::ACTIVATED :
                  ((!match && m_isReached) ? ThresholdCheckResult::DEACTIVATED :
                           (m_isReached ? ThresholdCheckResult::ALREADY_ACTIVE : ThresholdCheckResult::ALREADY_INACTIVE));
   m_isReached = match;
   if (result == ThresholdCheckResult::ACTIVATED || result == ThresholdCheckResult::DEACTIVATED)
   {
      // Update threshold status in database
      TCHAR query[256];
      _sntprintf(query, 256, _T("UPDATE thresholds SET current_state=%d WHERE threshold_id=%d"), m_isReached, m_id);
      QueueSQLRequest(query);
   }
   return result;
}

/**
 * Fill NXCP message with threshold data
 */
void Threshold::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
	uint32_t fieldId = baseId;
	msg->setField(fieldId++, m_id);
	msg->setField(fieldId++, m_eventCode);
	msg->setField(fieldId++, m_rearmEventCode);
	msg->setField(fieldId++, static_cast<uint16_t>(m_function));
	msg->setField(fieldId++, static_cast<uint16_t>(m_operation));
	msg->setField(fieldId++, static_cast<uint32_t>(m_sampleCount));
	msg->setField(fieldId++, CHECK_NULL_EX(m_scriptSource));
	msg->setField(fieldId++, static_cast<uint32_t>(m_repeatInterval));
	msg->setField(fieldId++, m_value.getString());
	msg->setField(fieldId++, m_isReached);
	msg->setField(fieldId++, static_cast<uint16_t>(m_currentSeverity));
	msg->setFieldFromTime(fieldId++, m_lastEventTimestamp);
   msg->setField(fieldId++, CHECK_NULL_EX(m_lastEventMessage));
   msg->setField(fieldId++, m_disabled);
}

/**
 * Fill NXCP message with threshold data and expanded values
 */
void Threshold::fillMessage(NXCPMessage *msg, uint32_t baseId, DCItem *dci) const
{
   uint32_t fieldId = baseId;
   msg->setField(fieldId++, m_id);
   msg->setField(fieldId++, m_eventCode);
   msg->setField(fieldId++, m_rearmEventCode);
   msg->setField(fieldId++, static_cast<uint16_t>(m_function));
   msg->setField(fieldId++, static_cast<uint16_t>(m_operation));
   msg->setField(fieldId++, static_cast<uint32_t>(m_sampleCount));
   msg->setField(fieldId++, CHECK_NULL_EX(m_scriptSource));
   msg->setField(fieldId++, static_cast<uint32_t>(m_repeatInterval));
   ItemValue tvalue = m_expandValue ?
                  ItemValue(dci->getOwner()->expandText(m_value.getString(), nullptr, nullptr, dci->createDescriptor(), nullptr, nullptr, dci->getInstanceName(), nullptr, nullptr), m_value.getTimeStamp(), false) :
                  m_value;
   msg->setField(fieldId++, tvalue.getString());
   msg->setField(fieldId++, m_isReached);
   msg->setField(fieldId++, static_cast<uint16_t>(m_currentSeverity));
   msg->setFieldFromTime(fieldId++, m_lastEventTimestamp);
   msg->setField(fieldId++, CHECK_NULL_EX(m_lastEventMessage));
   msg->setField(fieldId++, m_disabled);
}

/**
 * Update threshold object from NXCP message
 */
void Threshold::updateFromMessage(const NXCPMessage& msg, uint32_t baseId)
{
	TCHAR buffer[MAX_DCI_STRING_VALUE];
	uint32_t fieldId = baseId + 1;	// Skip ID field

	m_eventCode = msg.getFieldAsUInt32(fieldId++);
	m_rearmEventCode = msg.getFieldAsUInt32(fieldId++);
   m_function = static_cast<uint8_t>(msg.getFieldAsUInt16(fieldId++));
   m_operation = static_cast<uint8_t>(msg.getFieldAsUInt16(fieldId++));
   m_sampleCount = msg.getFieldAsInt32(fieldId++);
   setScript(msg.getFieldAsString(fieldId++));
	m_repeatInterval = msg.getFieldAsInt32(fieldId++);
	m_value.set(msg.getFieldAsString(fieldId++, buffer, MAX_DCI_STRING_VALUE), true);
   m_expandValue = (NumChars(m_value, '%') > 0);
   m_disabled = msg.getFieldAsBoolean(fieldId++);
}

/**
 * Calculate average value for values of given type
 */
template<typename T> static T CalculateAverage(const ItemValue &lastValue, ItemValue **prevValues, int sampleCount)
{
   T sum = static_cast<T>(lastValue);
   for(int i = 1; i < sampleCount; i++)
      sum += static_cast<T>(*prevValues[i - 1]);
   return sum / static_cast<T>(sampleCount);
}

/**
 * Calculate average value for metric
 */
void Threshold::calculateAverage(ItemValue *result, const ItemValue &lastValue, ItemValue **prevValues)
{
   switch(m_dataType)
   {
      case DCI_DT_INT:
         *result = CalculateAverage<int32_t>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_UINT:
      case DCI_DT_COUNTER32:
         *result = CalculateAverage<uint32_t>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_INT64:
         *result = CalculateAverage<int64_t>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER64:
         *result = CalculateAverage<uint64_t>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_FLOAT:
         *result = CalculateAverage<double>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_STRING:
         *result = _T("");   // Average value for string is meaningless
         break;
      default:
         break;
   }
}

/**
 * Calculate sum value for values of given type
 */
template<typename T> static T CalculateSum(const ItemValue &lastValue, ItemValue **prevValues, int sampleCount)
{
   T sum = static_cast<T>(lastValue);
   for(int i = 1; i < sampleCount; i++)
      sum += static_cast<T>(*prevValues[i - 1]);
   return sum;
}

/**
 * Calculate sum value for metric
 */
void Threshold::calculateTotal(ItemValue *result, const ItemValue &lastValue, ItemValue **prevValues)
{
   switch(m_dataType)
   {
      case DCI_DT_INT:
         *result = CalculateSum<int32_t>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_UINT:
      case DCI_DT_COUNTER32:
         *result = CalculateSum<uint32_t>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_INT64:
         *result = CalculateSum<int64_t>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER64:
         *result = CalculateSum<uint64_t>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_FLOAT:
         *result = CalculateSum<double>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_STRING:
         *result = _T("");   // Sum value for string is meaningless
         break;
      default:
         break;
   }
}

/**
 * Calculate mean absolute deviation for values of given type
 */
template<typename T, T (*ABS)(T)> static T CalculateMeanDeviation(const ItemValue& lastValue, ItemValue **prevValues, int sampleCount)
{
   T mean = static_cast<T>(lastValue);
   for(int i = 1; i < sampleCount; i++)
   {
      mean += static_cast<T>(*prevValues[i - 1]);
   }
   mean /= static_cast<T>(sampleCount);
   T dev = ABS(static_cast<T>(lastValue) - mean);
   for(int i = 1; i < sampleCount; i++)
   {
      dev += ABS(static_cast<T>(*prevValues[i - 1]) - mean);
   }
   return dev / static_cast<T>(sampleCount);
}

/**
 * Get absolute value for 32 bit integer
 */
static int32_t abs32(int32_t v)
{
   return v < 0 ? -v : v;
}

/**
 * Get absolute value for 64 bit integer
 */
static int64_t abs64(int64_t v)
{
   return v < 0 ? -v : v;
}

/**
 * Do nothing with unsigned 32 bit value
 */
static uint32_t noop32(uint32_t v)
{
   return v;
}

/**
 * Do nothing with unsigned 64 bit value
 */
static uint64_t noop64(uint64_t v)
{
   return v;
}

/**
 * Calculate mean absolute deviation for metric
 */
void Threshold::calculateMeanDeviation(ItemValue *result, const ItemValue &lastValue, ItemValue **prevValues)
{
   switch(m_dataType)
   {
      case DCI_DT_INT:
         *result = CalculateMeanDeviation<int32_t, abs32>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_INT64:
         *result = CalculateMeanDeviation<int64_t, abs64>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_FLOAT:
         *result = CalculateMeanDeviation<double, fabs>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_UINT:
      case DCI_DT_COUNTER32:
         *result = CalculateMeanDeviation<uint32_t, noop32>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER64:
         *result = CalculateMeanDeviation<uint64_t, noop64>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_STRING:
         *result = _T("");   // Mean deviation for string is meaningless
         break;
      default:
         break;
   }
}

/**
 * Calculate mean absolute deviation for values of given type
 */
template<typename T, T (*ABS)(T)> static T CalculateAbsoluteDeviation(const ItemValue& lastValue, ItemValue **prevValues, int sampleCount)
{
   T mean = static_cast<T>(lastValue);
   for(int i = 1; i < sampleCount; i++)
   {
      mean += static_cast<T>(*prevValues[i - 1]);
   }
   mean /= static_cast<T>(sampleCount);
   return ABS(static_cast<T>(lastValue) - mean);
}

/**
 * Calculate absolute deviation for metric
 */
void Threshold::calculateAbsoluteDeviation(ItemValue *result, const ItemValue &lastValue, ItemValue **prevValues)
{
   switch(m_dataType)
   {
      case DCI_DT_INT:
         *result = CalculateAbsoluteDeviation<int32_t, abs32>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_INT64:
         *result = CalculateAbsoluteDeviation<int64_t, abs64>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_FLOAT:
         *result = CalculateAbsoluteDeviation<double, fabs>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_UINT:
      case DCI_DT_COUNTER32:
         *result = CalculateAbsoluteDeviation<uint32_t, noop32>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER64:
         *result = CalculateAbsoluteDeviation<uint64_t, noop64>(lastValue, prevValues, m_sampleCount);
         break;
      case DCI_DT_STRING:
         *result = _T("");   // Absolute deviation for string is meaningless
         break;
      default:
         break;
   }
}

/**
 * Check if given threshold is equal to this threshold
 */
bool Threshold::equals(const Threshold& t) const
{
   bool match;

   if ((m_function == F_SCRIPT) || t.m_expandValue || m_expandValue)
   {
      match = (_tcscmp(t.m_value.getString(), m_value.getString()) == 0);
   }
   else
   {
      switch(m_dataType)
      {
         case DCI_DT_INT:
            match = (t.m_value.getInt32() == m_value.getInt32());
            break;
         case DCI_DT_UINT:
         case DCI_DT_COUNTER32:
            match = (t.m_value.getUInt32() == m_value.getUInt32());
            break;
         case DCI_DT_INT64:
            match = (t.m_value.getInt64() == m_value.getInt64());
            break;
         case DCI_DT_UINT64:
         case DCI_DT_COUNTER64:
            match = (t.m_value.getUInt64() == m_value.getUInt64());
            break;
         case DCI_DT_FLOAT:
            match = (t.m_value.getDouble() == m_value.getDouble());
            break;
         case DCI_DT_STRING:
            match = (_tcscmp(t.m_value.getString(), m_value.getString()) == 0);
            break;
         default:
            match = true;
            break;
      }
   }
   return match &&
          (t.m_eventCode == m_eventCode) &&
          (t.m_rearmEventCode == m_rearmEventCode) &&
          (t.m_dataType == m_dataType) &&
          (t.m_function == m_function) &&
          (t.m_operation == m_operation) &&
          (t.m_sampleCount == m_sampleCount) &&
          (t.m_repeatInterval == m_repeatInterval) &&
          !_tcscmp(CHECK_NULL_EX(t.m_scriptSource), CHECK_NULL_EX(m_scriptSource));
}

/**
 * Create management pack record
 */
void Threshold::createExportRecord(TextFileWriter& xml, int index) const
{
   TCHAR activationEvent[MAX_EVENT_NAME], deactivationEvent[MAX_EVENT_NAME];

   EventNameFromCode(m_eventCode, activationEvent);
   EventNameFromCode(m_rearmEventCode, deactivationEvent);
   xml.appendFormattedString(_T("\t\t\t\t\t\t<threshold id=\"%d\">\n")
                          _T("\t\t\t\t\t\t\t<function>%d</function>\n")
                          _T("\t\t\t\t\t\t\t<condition>%d</condition>\n")
                          _T("\t\t\t\t\t\t\t<value>%s</value>\n")
                          _T("\t\t\t\t\t\t\t<activationEvent>%s</activationEvent>\n")
                          _T("\t\t\t\t\t\t\t<deactivationEvent>%s</deactivationEvent>\n")
                          _T("\t\t\t\t\t\t\t<sampleCount>%d</sampleCount>\n")
                          _T("\t\t\t\t\t\t\t<repeatInterval>%d</repeatInterval>\n"),
								  index, m_function, m_operation,
								  EscapeStringForXML2(m_value.getString()).cstr(),
                          EscapeStringForXML2(activationEvent).cstr(),
								  EscapeStringForXML2(deactivationEvent).cstr(),
								  m_sampleCount, m_repeatInterval);
   if (m_scriptSource != nullptr)
   {
      xml.appendUtf8String("\t\t\t\t\t\t\t<script>");
      xml.append(EscapeStringForXML2(m_scriptSource));
      xml.appendUtf8String("</script>\n");
   }
   xml.appendUtf8String("\t\t\t\t\t\t</threshold>\n");
}

/**
 * Serialize to JSON
 */
json_t *Threshold::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "targetId", json_integer(m_targetId));
   json_object_set_new(root, "eventCode", json_integer(m_eventCode));
   json_object_set_new(root, "rearmEventCode", json_integer(m_rearmEventCode));
   json_object_set_new(root, "value", json_string_t(m_value));
   json_object_set_new(root, "function", json_integer(m_function));
   json_object_set_new(root, "operation", json_integer(m_operation));
   json_object_set_new(root, "dataType", json_integer(m_dataType));
   json_object_set_new(root, "currentSeverity", json_integer(m_currentSeverity));
   json_object_set_new(root, "sampleCount", json_integer(m_sampleCount));
   json_object_set_new(root, "script", json_string_t(CHECK_NULL_EX(m_scriptSource)));
   json_object_set_new(root, "isReached", json_boolean(m_isReached));
   json_object_set_new(root, "disabled", json_boolean(m_disabled));
   json_object_set_new(root, "numMatches", json_integer(m_numMatches));
   json_object_set_new(root, "repeatInterval", json_integer(m_repeatInterval));
   json_object_set_new(root, "lastEventTimestamp", json_integer(m_lastEventTimestamp));
   json_object_set_new(root, "lastEventMessage", json_string_t(m_lastEventMessage));
   return root;
}

/**
 * Make an association with DCI (used by management pack parser)
 */
void Threshold::associate(DCItem *pItem)
{
   m_itemId = pItem->getId();
   m_targetId = pItem->getOwnerId();
   m_dataType = pItem->getTransformedDataType();
}

/**
 * Set new script. Script source must be dynamically allocated
 * and will be deallocated by threshold object
 */
void Threshold::setScript(TCHAR *script)
{
   MemFree(m_scriptSource);
   delete m_script;
   if (script != nullptr)
   {
      m_scriptSource = Trim(script);
      if (m_scriptSource[0] != 0)
      {
         NXSL_CompilationDiagnostic diag;
         NXSL_ServerEnv env;
         m_script = NXSLCompile(m_scriptSource, &env, &diag);
         if (m_script == nullptr)
         {
            TCHAR defaultName[32];
            _sntprintf(defaultName, 32, _T("[%u]"), m_targetId);
            ReportScriptError(SCRIPT_CONTEXT_DCI, FindObjectById(m_targetId).get(), m_itemId, diag.errorText, _T("DCI::%s::%d::%d::ThresholdScript"), GetObjectName(m_targetId, defaultName), m_itemId, m_id);
         }
      }
      else
      {
         m_script = nullptr;
      }
   }
   else
   {
      m_scriptSource = nullptr;
      m_script = nullptr;
   }
   m_lastScriptErrorReport = 0;  // allow immediate error report after script change
}

/**
 * Reconcile changes in threshold copy
 */
void Threshold::reconcile(const Threshold& src)
{
   m_numMatches = src.m_numMatches;
   m_isReached = src.m_isReached;
   m_wasReachedBeforeMaint = src.m_wasReachedBeforeMaint;
   m_disabled = src.m_disabled;
   m_currentSeverity = src.m_currentSeverity;
   m_lastScriptErrorReport = src.m_lastScriptErrorReport;
   m_lastCheckValue = src.m_lastCheckValue;
}

/**
 * Get textual definition of this threshold
 */
String Threshold::getTextualDefinition() const
{
   static const TCHAR *functionNames[] = { _T("last("), _T("average("), _T("mean-deviation("), _T("diff("), _T("error("), _T("sum("), _T("script("), _T("abs-deviation("), _T("anomaly(") };
   static const TCHAR *operationNames[] = { _T("<"), _T("<="), _T("=="), _T(">="), _T(">"), _T("!="), _T("LIKE"), _T("NOT LIKE"), _T("ICASE LIKE"), _T("ICASE NOT LIKE") };

   StringBuffer text(functionNames[m_function]);
   text.append(m_sampleCount);
   text.append(_T(") "));
   if ((m_function != F_SCRIPT) && (m_function != F_ERROR) && (m_function != F_ANOMALY))
   {
      text.append(operationNames[m_operation]);
      text.append(_T(' '));
      text.append(m_value.getString());
   }
   return text;
}

/**
 * Get script dependencies
 */
void Threshold::getScriptDependencies(StringSet *dependencies) const
{
   if (m_expandValue)
   {
      FindScriptMacrosInText(m_value.getString(), dependencies);
   }
}
