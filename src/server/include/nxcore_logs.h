/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: nxcore_logs.h
**
**/

#ifndef _nxcore_logs_h_
#define _nxcore_logs_h_

#define MAX_COLUMN_NAME_LEN    64


//
// Column types
//

#define LC_TEXT            0
#define LC_SEVERITY        1
#define LC_OBJECT_ID       2
#define LC_USER_ID         3
#define LC_EVENT_CODE      4
#define LC_TIMESTAMP       5
#define LC_INTEGER         6
#define LC_ALARM_STATE     7
#define LC_ALARM_HD_STATE  8


//
// Column filter types
//

#define FILTER_EQUALS      0
#define FILTER_RANGE       1
#define FILTER_SET         2
#define FILTER_LIKE        3
#define FILTER_LESS        4
#define FILTER_GREATER     5


//
// Set operations
//

#define SET_OPERATION_AND  0
#define SET_OPERATION_OR   1


//
// Log definition structure
//

struct LOG_COLUMN
{
	const TCHAR *name;
	const TCHAR *description;
	int type;
};

struct NXCORE_LOG
{
	const TCHAR *name;
	const TCHAR *table;
	DWORD requiredAccess;
	LOG_COLUMN columns[32];
};


//
// Log filter
//

struct OrderingColumn
{
	TCHAR name[MAX_COLUMN_NAME_LEN];
	bool descending;
};

class ColumnFilter
{
private:
	int m_varCount;	// Number of variables read from NXCP message during construction
	int m_type;
	TCHAR *m_column;
	bool m_negated;
	union t_ColumnFilterValue
	{
		INT64 numericValue;	// numeric value for <, >, and = operations
		struct
		{
			INT64 start;
			INT64 end;
		} range;
		TCHAR *like;
		struct
		{
			int operation;
			int count;
			ColumnFilter **filters;
		} set;
	} m_value;

public:
	ColumnFilter(CSCPMessage *msg, const TCHAR *column, DWORD baseId);
	~ColumnFilter();

	int getVariableCount() { return m_varCount; }

	String generateSql();
};

class LogFilter
{
private:
	int m_numColumnFilters;
	ColumnFilter **m_columnFilters;
	int m_numOrderingColumns;
	OrderingColumn *m_orderingColumns;

public:
	LogFilter(CSCPMessage *msg);
	~LogFilter();

	String buildOrderClause();

	int getNumColumnFilter()
	{
		return m_numColumnFilters;
	}

	ColumnFilter *getColumnFilter(int offset)
	{
		return m_columnFilters[offset];
	}

	int getNumOrderingColumns()
	{
		return m_numOrderingColumns;
	}
};


//
// Log handle - object used to access log
//

class LogHandle
{
private:
	NXCORE_LOG *m_log;
	LogFilter *m_filter;
	MUTEX m_lock;
   String m_queryColumns;
	DWORD m_rowCountLimit;
	DB_RESULT m_resultSet;

	void buildQueryColumnList();
	void deleteQueryResults();
	bool queryInternal(INT64 *rowCount);
	Table *createTable();

public:
	LogHandle(NXCORE_LOG *log);
	~LogHandle();

	void lock() { MutexLock(m_lock); }
	void unlock() { MutexUnlock(m_lock); }

	bool query(LogFilter *filter, INT64 *rowCount);
	Table *getData(INT64 startRow, INT64 numRows);
	void getColumnInfo(CSCPMessage &msg);
};


//
// API functions
//

void InitLogAccess();
int OpenLog(const TCHAR *name, ClientSession *session, DWORD *rcc);
DWORD CloseLog(ClientSession *session, int logHandle);
LogHandle *AcquireLogHandleObject(ClientSession *session, int logHandle);


#endif
