/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: columnfilter.cpp
**
**/

#include "nxcore.h"
#include <nxcore_logs.h>

/**
 * Filter type names as used in JSON representation. Element index is FILTER_* code.
 */
static const char *s_filterTypeNames[] = { "equals", "range", "set", "like", "less", "greater", "childOf", "relative", "currentPeriod", nullptr };

/**
 * Time unit names as used in JSON representation. Element index is TIME_UNIT_* code.
 */
static const char *s_timeUnitNames[] = { "minute", "hour", "day", "week", nullptr };

/**
 * Calendar period names as used in JSON representation. Element index is CALENDAR_PERIOD_* code.
 */
static const char *s_calendarPeriodNames[] = { "today", "yesterday", "thisWeek", "thisMonth", nullptr };

/**
 * Get code for given symbolic name from name table. Returns -1 if name is unknown or not set.
 */
static int CodeFromName(json_t *json, const char *tag, const char **names)
{
   const char *name = json_object_get_string_utf8(json, tag, nullptr);
   if (name == nullptr)
      return -1;
   for(int i = 0; names[i] != nullptr; i++)
      if (!stricmp(name, names[i]))
         return i;
   return -1;
}

/**
 * Set filter column and read column type information from log definition
 */
void ColumnFilter::setColumn(const wchar_t *column, LogHandle *log)
{
   m_column = MemCopyStringW(column);
   const LOG_COLUMN *cd = log->getColumnDefinition(m_column);
   m_columnType = (cd != nullptr) ? cd->type : LC_TEXT;
   m_columnFlags = (cd != nullptr) ? cd->flags : 0;
}

/**
 * Read numeric filter value from JSON document. String values are interpreted as timestamps
 * (accepting ISO 8601, UNIX timestamp, relative offset, or "now") and converted to the unit
 * expected by the column.
 */
bool ColumnFilter::readNumericValue(json_t *json, const char *tag, int64_t *value)
{
   json_t *v = json_object_get(json, tag);
   if (json_is_integer(v))
   {
      *value = json_integer_value(v);
      return true;
   }
   if (json_is_string(v))
   {
      time_t t = ParseTimestamp(json_string_value(v));
      if (t == 0)
         return false;
      *value = (m_columnType == LC_TIMESTAMP_MS) ? static_cast<int64_t>(t) * 1000 : static_cast<int64_t>(t);
      return true;
   }
   return false;
}

/**
 * Create column filter object from JSON document
 */
ColumnFilter::ColumnFilter(json_t *json, const wchar_t *column, LogHandle *log)
{
   setColumn(column, log);
   m_varCount = 0;
   m_negated = json_object_get_boolean(json, "negated", false);
   m_valid = true;

   m_type = CodeFromName(json, "type", s_filterTypeNames);
   switch(m_type)
   {
      case FILTER_EQUALS:
      case FILTER_LESS:
      case FILTER_GREATER:
      case FILTER_CHILDOF:
         m_valid = readNumericValue(json, "value", &m_value.numericValue);
         break;
      case FILTER_RANGE:
         m_valid = readNumericValue(json, "from", &m_value.range.start) && readNumericValue(json, "to", &m_value.range.end);
         break;
      case FILTER_LIKE:
         m_value.like = json_object_get_string_w(json, "value", nullptr);
         m_valid = (m_value.like != nullptr);
         break;
      case FILTER_RELATIVE:
         m_value.relative.value = json_object_get_int32(json, "value", 0);
         m_value.relative.unit = CodeFromName(json, "unit", s_timeUnitNames);
         m_valid = (m_value.relative.value > 0) && (m_value.relative.unit != -1);
         break;
      case FILTER_CURRENT_PERIOD:
         m_value.currentPeriod.period = CodeFromName(json, "period", s_calendarPeriodNames);
         m_value.currentPeriod.tzOffset = json_object_get_int32(json, "timeZoneOffset", 0);
         m_valid = (m_value.currentPeriod.period != -1);
         break;
      case FILTER_SET:
         {
            const char *operation = json_object_get_string_utf8(json, "operation", "and");
            m_value.set.operation = stricmp(operation, "or") ? SET_OPERATION_AND : SET_OPERATION_OR;

            json_t *elements = json_object_get(json, "filters");
            m_value.set.count = json_is_array(elements) ? static_cast<int>(json_array_size(elements)) : 0;
            m_value.set.filters = MemAllocArray<ColumnFilter*>(m_value.set.count);
            for(int i = 0; i < m_value.set.count; i++)
            {
               ColumnFilter *filter = new ColumnFilter(json_array_get(elements, i), column, log);
               m_value.set.filters[i] = filter;
               if (!filter->isValid())
                  m_valid = false;
            }
         }
         break;
      default:
         nxlog_debug_tag(DEBUG_TAG_LOGS, 4, L"ColumnFilter: invalid or missing filter type for column \"%s\"", column);
         m_valid = false;
         break;
   }

   if (!m_valid)
      nxlog_debug_tag(DEBUG_TAG_LOGS, 4, L"ColumnFilter: cannot create filter of type %d for column \"%s\"", m_type, column);
}

/**
 * Create column filter object from NXCP message
 */
ColumnFilter::ColumnFilter(const NXCPMessage& msg, const wchar_t *column, uint32_t baseId, LogHandle *log)
{
	uint32_t fieldId;

	setColumn(column, log);
	m_valid = true;

	m_type = msg.getFieldAsInt16(baseId);
	switch(m_type)
	{
		case FILTER_EQUALS:
		case FILTER_LESS:
		case FILTER_GREATER:
		case FILTER_CHILDOF:
			m_value.numericValue = msg.getFieldAsInt64(baseId + 1);
			m_negated = msg.getFieldAsBoolean(baseId + 2);
			m_varCount = 3;
			break;
		case FILTER_RANGE:
			m_value.range.start = msg.getFieldAsInt64(baseId + 1);
			m_value.range.end = msg.getFieldAsInt64(baseId + 2);
			m_negated = msg.getFieldAsBoolean(baseId + 3);
			m_varCount = 4;
			break;
		case FILTER_LIKE:
			m_value.like = msg.getFieldAsString(baseId + 1);
			m_negated = msg.getFieldAsBoolean(baseId + 2);
			m_varCount = 3;
			break;
		case FILTER_RELATIVE:
			m_value.relative.value = msg.getFieldAsInt32(baseId + 1);
			m_value.relative.unit = msg.getFieldAsInt16(baseId + 2);
			m_negated = msg.getFieldAsBoolean(baseId + 3);
			m_varCount = 4;
			break;
		case FILTER_CURRENT_PERIOD:
			m_value.currentPeriod.period = msg.getFieldAsInt16(baseId + 1);
			m_value.currentPeriod.tzOffset = msg.getFieldAsInt32(baseId + 2);
			m_negated = msg.getFieldAsBoolean(baseId + 3);
			m_varCount = 4;
			break;
		case FILTER_SET:
			m_value.set.operation = msg.getFieldAsInt16(baseId + 1);
			m_value.set.count = msg.getFieldAsInt16(baseId + 2);
			m_varCount = 3;

			m_value.set.filters = MemAllocArray<ColumnFilter*>(m_value.set.count);
			fieldId = baseId + 3;
			for(int i = 0; i < m_value.set.count; i++)
			{
				ColumnFilter *filter = new ColumnFilter(msg, column, fieldId, log);
				m_value.set.filters[i] = filter;
				fieldId += filter->getVariableCount();
				m_varCount += filter->getVariableCount();
			}
			break;
		default:
			break;
	}
}

/**
 * Destructor
 */
ColumnFilter::~ColumnFilter()
{
	MemFree(m_column);
	switch(m_type)
	{
		case FILTER_LIKE:
			MemFree(m_value.like);
			break;
		case FILTER_SET:
			for(int i = 0; i < m_value.set.count; i++)
				delete m_value.set.filters[i];
			MemFree(m_value.set.filters);
			break;
	}
}

/**
 * Generate SQL for column filter
 */
StringBuffer ColumnFilter::generateSql()
{
	StringBuffer sql;

	// Conversion from the client-supplied epoch value to TSDB timestamptz: millisecond
	// timestamp columns send epoch milliseconds, all other timestamp columns send epoch seconds.
	const wchar_t *tsToTimestamp = (m_columnType == LC_TIMESTAMP_MS) ? L"ms_to_timestamptz(" : L"to_timestamp(";

	// Append a timestamp boundary (given as epoch seconds) in the unit expected by the column,
	// wrapping it for TSDB timestamptz columns the same way as for client-supplied values.
	auto appendTimestampBoundary = [&](int64_t epochSeconds)
	{
		int64_t value = (m_columnType == LC_TIMESTAMP_MS) ? epochSeconds * 1000 : epochSeconds;
		if ((m_columnFlags & LCF_TSDB_TIMESTAMPTZ) && (g_dbSyntax == DB_SYNTAX_TSDB))
		{
			sql.append(tsToTimestamp);
			sql.append(value);
			sql.append(L")");
		}
		else
		{
			sql.append(value);
		}
	};

	switch(m_type)
	{
		case FILTER_EQUALS:
			if (m_negated)
				sql.append(_T("NOT "));
			sql.append(m_column);
			sql.append(_T(" = "));
			if ((m_columnFlags & LCF_TSDB_TIMESTAMPTZ) && (g_dbSyntax == DB_SYNTAX_TSDB))
			{
	         sql.append(tsToTimestamp);
            sql.append(m_value.numericValue);
            sql.append(_T(")"));
			}
         else if (m_columnFlags & LCF_CHAR_COLUMN)
         {
            sql.append(_T('\''));
            sql.append(m_value.numericValue);
            sql.append(_T('\''));
         }
			else
			{
			   sql.append(m_value.numericValue);
			}
			break;
		case FILTER_LESS:
			if (m_negated)
            sql.append(_T("NOT "));
         sql.append(m_column);
         sql.append(_T(" < "));
         if ((m_columnFlags & LCF_TSDB_TIMESTAMPTZ) && (g_dbSyntax == DB_SYNTAX_TSDB))
         {
            sql.append(tsToTimestamp);
            sql.append(m_value.numericValue);
            sql.append(_T(")"));
         }
         else
         {
            sql.append(m_value.numericValue);
         }
			break;
		case FILTER_GREATER:
			if (m_negated)
            sql.append(_T("NOT "));
         sql.append(m_column);
         sql.append(_T(" > "));
         if ((m_columnFlags & LCF_TSDB_TIMESTAMPTZ) && (g_dbSyntax == DB_SYNTAX_TSDB))
         {
            sql.append(tsToTimestamp);
            sql.append(m_value.numericValue);
            sql.append(_T(")"));
         }
         else
         {
            sql.append(m_value.numericValue);
         }
			break;
		case FILTER_RANGE:
			if (m_negated)
            sql.append(_T("NOT "));
         sql.append(m_column);
         sql.append(_T(" BETWEEN "));
         if ((m_columnFlags & LCF_TSDB_TIMESTAMPTZ) && (g_dbSyntax == DB_SYNTAX_TSDB))
         {
            sql.append(tsToTimestamp);
            sql.append(m_value.range.start);
            sql.append(L") AND ");
            sql.append(tsToTimestamp);
            sql.append(m_value.range.end);
            sql.append(_T(")"));
         }
         else
         {
            sql.append(m_value.range.start);
            sql.append(_T(" AND "));
            sql.append(m_value.range.end);
         }
			break;
		case FILTER_RELATIVE:
		{
			static const int64_t unitSeconds[] = { 60, 3600, 86400, 604800 };
			int unit = ((m_value.relative.unit >= 0) && (m_value.relative.unit <= TIME_UNIT_WEEK)) ? m_value.relative.unit : TIME_UNIT_HOUR;
			int64_t cutoff = static_cast<int64_t>(time(nullptr)) - static_cast<int64_t>(m_value.relative.value) * unitSeconds[unit];
			sql.append(m_column);
			sql.append(m_negated ? L" < " : L" >= ");   // negated => "older than N units"
			appendTimestampBoundary(cutoff);
			break;
		}
		case FILTER_CURRENT_PERIOD:
		{
			int32_t tzOffset = m_value.currentPeriod.tzOffset;
			time_t localNow = time(nullptr) + tzOffset;
			struct tm lt;
#if HAVE_GMTIME_R
			if (gmtime_r(&localNow, &lt) == nullptr)   // broken-down representation of "now" in client local time
                           memset(&lt, 0, sizeof(struct tm));
#else
			struct tm *gt = gmtime(&localNow);
			if (gt != nullptr)
                           memcpy(&lt, gmtime(&localNow), sizeof(struct tm));
			else
                           memset(&lt, 0, sizeof(struct tm));
#endif
			lt.tm_hour = 0;
			lt.tm_min = 0;
			lt.tm_sec = 0;
			int64_t startLocal, endLocal;   // period boundaries expressed in client local time
			switch(m_value.currentPeriod.period)
			{
				case CALENDAR_PERIOD_YESTERDAY:
					endLocal = timegm(&lt);   // local midnight starting today
					startLocal = endLocal - 86400;
					break;
				case CALENDAR_PERIOD_THIS_WEEK:
				{
					int wday = (lt.tm_wday == 0) ? 6 : lt.tm_wday - 1;   // ISO week: Monday = 0 ... Sunday = 6
					startLocal = static_cast<int64_t>(timegm(&lt)) - static_cast<int64_t>(wday) * 86400;
					endLocal = startLocal + 7 * 86400;
					break;
				}
				case CALENDAR_PERIOD_THIS_MONTH:
					lt.tm_mday = 1;
					startLocal = timegm(&lt);
					lt.tm_mon++;   // timegm() normalizes month overflow into the next year
					endLocal = timegm(&lt);
					break;
				default:   // CALENDAR_PERIOD_TODAY
					startLocal = timegm(&lt);
					endLocal = startLocal + 86400;
					break;
			}
			int64_t start = startLocal - tzOffset;   // convert local boundaries back to UTC epoch
			int64_t end = endLocal - tzOffset;
			if (m_negated)
				sql.append(L"NOT ");
			sql.append(L"(");
			sql.append(m_column);
			sql.append(L" >= ");
			appendTimestampBoundary(start);
			sql.append(L" AND ");
			sql.append(m_column);
			sql.append(L" < ");
			appendTimestampBoundary(end);
			sql.append(L")");
			break;
		}
		case FILTER_LIKE:
			if (m_value.like[0] == 0)
			{
				if (m_negated)
					sql.appendFormattedString(_T("(%s IS NOT NULL) AND (%s <> '')"), m_column, m_column);
				else
					sql.appendFormattedString(_T("(%s IS NULL) OR (%s = '')"), m_column, m_column);
			}
			else
			{
				if (m_negated)
					sql.append(_T("NOT "));
				sql.append(m_column);
            sql.append(_T(" LIKE "));
            sql.append(DBPrepareString(g_dbDriver, m_value.like));
			}
			break;
		case FILTER_SET:
			if (m_value.set.count > 0)
			{
				bool first = true;
				for(int i = 0; i < m_value.set.count; i++)
				{
					StringBuffer subExpr = m_value.set.filters[i]->generateSql();
					if (!subExpr.isEmpty())
					{
						if (first)
						{
							first = false;
						}
						else
						{
							sql.append((m_value.set.operation == SET_OPERATION_AND) ? _T(" AND ") : _T(" OR "));
						}
						sql.append(_T("("));
						sql.append(subExpr);
						sql.append(_T(")"));
					}
				}
			}
			break;
		case FILTER_CHILDOF:
			if (m_negated)
				sql.append(_T("NOT "));

			{
				shared_ptr<NetObj> object = FindObjectById(static_cast<uint32_t>(m_value.numericValue));
				if (object != NULL)
				{
					// For any-class columns (e.g. audit_log.object_id) include non-event-source descendants like interfaces
					unique_ptr<SharedObjectArray<NetObj>> children = object->getAllChildren((m_columnFlags & LCF_ANY_OBJECT_CLASS) == 0);
					if (children->size() > 0)
					{
						sql.append(m_column);
						sql.append(_T(" IN ("));
						for(int i = 0; i < children->size(); i++)
						{
							if (i > 0)
								sql.append(_T(", "));
							sql.append(children->get(i)->getId());
						}
						sql.append(_T(")"));
					}
					else
					{
						sql.append(_T("0=1"));
					}
				}
				else
				{
					sql.append(_T("0=1"));
				}
			}
			break;
	}

	return sql;
}
