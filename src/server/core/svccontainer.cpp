/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Raden Solutions
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
** File: svccontainer.cpp
**
**/

#include "nxcore.h"

#define QUERY_LENGTH		(512)

/**
 * Service log record ID
 */
LONG ServiceContainer::logRecordId = -1;

/**
 * Default constructor for service service object
 */
ServiceContainer::ServiceContainer() : Container()
{
	initServiceContainer();
}

/**
 * Create new service container object
 */
ServiceContainer::ServiceContainer(const TCHAR *pszName) : Container(pszName, 0)
{
	initServiceContainer();
}

void ServiceContainer::initServiceContainer()
{
	m_prevUptimeUpdateStatus = STATUS_UNKNOWN;
	m_prevUptimeUpdateTime = time(NULL);
	m_uptimeDay = 100.0;
	m_uptimeWeek = 100.0;
	m_uptimeMonth = 100.0;
	m_downtimeDay = 0;
	m_downtimeWeek = 0;
	m_downtimeMonth = 0;
	m_prevDiffDay = 0;
	m_prevDiffWeek = 0;
	m_prevDiffMonth = 0;
}

/**
 * Create object from database data
 */
BOOL ServiceContainer::CreateFromDB(UINT32 id)
{
	if (!Container::CreateFromDB(id))
		return FALSE;

	initUptimeStats();
	return TRUE;
}

/**
 * Save object to database
 */
BOOL ServiceContainer::SaveToDB(DB_HANDLE hdb)
{
	return Container::SaveToDB(hdb);
}

/**
 * Delete object from database
 */
bool ServiceContainer::deleteFromDB(DB_HANDLE hdb)
{
	return Container::deleteFromDB(hdb);
}

/**
 * Create NXCP message with object's data
 */
void ServiceContainer::CreateMessage(CSCPMessage *pMsg)
{
   Container::CreateMessage(pMsg);
   pMsg->SetVariable(VID_UPTIME_DAY, m_uptimeDay);
   pMsg->SetVariable(VID_UPTIME_WEEK, m_uptimeWeek);
   pMsg->SetVariable(VID_UPTIME_MONTH, m_uptimeMonth);
}

/**
 * Modify object from message
 */
UINT32 ServiceContainer::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

   return Container::ModifyFromMessage(pRequest, TRUE);
}

/**
 * Calculate status for compound object based on childs status
 */
void ServiceContainer::calculateCompoundStatus(BOOL bForcedRecalc)
{
	int i, iCount, iMostCriticalStatus;
	int iOldStatus = m_iStatus;

	DbgPrintf(7, _T("#### CalculateCompoundStatus for id %d"), m_dwId);

	// Calculate own status by selecting the most critical status of the kids
	LockChildList(FALSE);
	for(i = 0, iCount = 0, iMostCriticalStatus = -1; i < int(m_dwChildCount); i++)
	{
		int iChildStatus = m_pChildList[i]->Status();
		if ((iChildStatus < STATUS_UNKNOWN) &&
			(iChildStatus > iMostCriticalStatus))
		{
			iMostCriticalStatus = iChildStatus;
			iCount++;
		}
	}
	// Set status and update uptime counters
	setStatus((iCount > 0) ? iMostCriticalStatus : STATUS_UNKNOWN);
	UnlockChildList();

	// Cause parent object(s) to recalculate it's status
	if ((iOldStatus != m_iStatus) || bForcedRecalc)
	{
		LockParentList(FALSE);
		for(i = 0; i < int(m_dwParentCount); i++)
			m_pParentList[i]->calculateCompoundStatus();
		UnlockParentList();
		Modify();   /* LOCK? */
	}

	DbgPrintf(6, _T("ServiceContainer::calculateCompoundStatus(%s [%d]): old_status=%d new_status=%d"), m_szName, m_dwId, iOldStatus, m_iStatus);

	if (iOldStatus != STATUS_UNKNOWN && iOldStatus != m_iStatus)
		addHistoryRecord();
}

/**
 * Set service status - use this instead of direct assignment;
 */
void ServiceContainer::setStatus(int newStatus)
{
	m_iStatus = newStatus;
}

/**
 * Add a record to slm_service_history table
 */
BOOL ServiceContainer::addHistoryRecord()
{
	DB_RESULT hResult;
	DB_STATEMENT hStmt;

	if (ServiceContainer::logRecordId < 0)
	{
		hResult = DBSelect(g_hCoreDB, _T("SELECT max(record_id) FROM slm_service_history"));
		if (hResult == NULL)
			return FALSE;
		ServiceContainer::logRecordId = DBGetNumRows(hResult) > 0 ? DBGetFieldLong(hResult, 0, 0) : 0;
		DBFreeResult(hResult);
	}

	ServiceContainer::logRecordId++;

	hStmt = DBPrepare(g_hCoreDB, _T("INSERT INTO slm_service_history (record_id,service_id,change_timestamp,new_status) ")
		_T("VALUES (?,?,?,?)"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, ServiceContainer::logRecordId);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_dwId);
		DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, UINT32(time(NULL)));
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, UINT32(m_iStatus));
		if (!DBExecute(hStmt))
		{
			DBFreeStatement(hStmt);
			return FALSE;
		}
		DbgPrintf(9, _T("ServiceContainer::addHistoryRecord() ok with id %ld"), ServiceContainer::logRecordId);
	}
	else
	{
		return FALSE;
	}

	DBFreeStatement(hStmt);
	return TRUE;
}


//
// Initialize uptime statistics (daily, weekly, monthly) by examining slm_service_history
//

void ServiceContainer::initUptimeStats()
{
	LockData();
	m_prevUptimeUpdateStatus = m_iStatus;
	m_uptimeDay = getUptimeFromDBFor(DAY, &m_downtimeDay);
	m_uptimeWeek = getUptimeFromDBFor(WEEK, &m_downtimeWeek);
	m_uptimeMonth = getUptimeFromDBFor(MONTH, &m_downtimeMonth);
	UnlockData();
	DbgPrintf(7, _T("++++ ServiceContainer::initUptimeStats() id=%d %lf %lf %lf"), m_dwId, m_uptimeDay, m_uptimeWeek, m_uptimeMonth);
}


//
// Calculate uptime for given period using data in database
//

double ServiceContainer::getUptimeFromDBFor(Period period, LONG *downtime)
{
	time_t beginTime;
	LONG timediffTillNow	= ServiceContainer::getSecondsSinceBeginningOf(period, &beginTime);
	double percentage = 0;

	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT change_timestamp,new_status FROM slm_service_history ")
	                                          _T("WHERE service_id=? AND change_timestamp>?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (UINT32)beginTime);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult == NULL)
		{
			DBFreeStatement(hStmt);
			return percentage;
		}

		time_t changeTimestamp, prevChangeTimestamp = beginTime;
		int newStatus = STATUS_UNKNOWN, i, realRows;
		int numRows = DBGetNumRows(hResult);
		*downtime = 0;
		for (i = 0, realRows = 0; i < numRows; i++)
		{
			changeTimestamp = DBGetFieldLong(hResult, i, 0);
			newStatus = DBGetFieldLong(hResult, i, 1);
			if (newStatus == STATUS_UNKNOWN) // Malawi hotfix - ignore unknown status
				continue;
			if (newStatus == STATUS_NORMAL)
				*downtime += (LONG)(changeTimestamp - prevChangeTimestamp);
			else 
				prevChangeTimestamp = changeTimestamp;
			realRows++;
		}
		if (newStatus == STATUS_CRITICAL) // the service is still down, add period till now
			*downtime += LONG(time(NULL) - prevChangeTimestamp);
		// no rows for period && critical status -> downtime from beginning till now
		if (realRows == 0 && m_iStatus == STATUS_CRITICAL)  
			*downtime = timediffTillNow;
		percentage = 100.0 - (double)(*downtime * 100) / (double)getSecondsInPeriod(period);
		DbgPrintf(7, _T("++++ ServiceContainer::getUptimeFromDBFor(), downtime %ld"), *downtime);

		DBFreeResult(hResult);
		DBFreeStatement(hStmt);
	}

	return percentage;
}


//
// Update uptime counters 
//

void ServiceContainer::updateUptimeStats(time_t currentTime /* = 0*/, BOOL updateChilds /* = FALSE */)
{
	LONG timediffTillNow;
	LONG downtimeBetweenPolls = 0;

	if (currentTime == 0)
		currentTime = time(NULL);

	LockData();

	double prevUptimeDay = m_uptimeDay;
	double prevUptimeWeek = m_uptimeWeek;
	double prevUptimeMonth = m_uptimeMonth;

	if (m_iStatus == STATUS_CRITICAL && m_prevUptimeUpdateStatus == STATUS_CRITICAL)
	{
		downtimeBetweenPolls = LONG(currentTime - m_prevUptimeUpdateTime);		
		DbgPrintf(7, _T("++++ ServiceContainer::updateUptimeStats() both statuses critical"));
	}

	timediffTillNow = ServiceContainer::getSecondsSinceBeginningOf(DAY, NULL);
	m_downtimeDay += downtimeBetweenPolls;
	if (timediffTillNow < m_prevDiffDay)
		m_downtimeDay = 0;
	m_uptimeDay = 100.0 - (double)(m_downtimeDay * 100) / (double)ServiceContainer::getSecondsInPeriod(DAY);
	m_prevDiffDay = timediffTillNow;
	DbgPrintf(7, _T("++++ ServiceContainer::updateUptimeStats() m_downtimeDay %ld, timediffTillNow %ld, downtimeBetweenPolls %ld"), 
		m_downtimeDay, timediffTillNow, downtimeBetweenPolls);

	timediffTillNow = ServiceContainer::getSecondsSinceBeginningOf(WEEK, NULL);
	m_downtimeWeek += downtimeBetweenPolls;
	if (timediffTillNow < m_prevDiffWeek)
		m_downtimeWeek = 0;
	m_uptimeWeek = 100.0 - (double)(m_downtimeWeek * 100) / (double)ServiceContainer::getSecondsInPeriod(WEEK);
	m_prevDiffWeek = timediffTillNow;

	timediffTillNow = ServiceContainer::getSecondsSinceBeginningOf(MONTH, NULL);
	m_downtimeMonth += downtimeBetweenPolls;
	if (timediffTillNow < m_prevDiffMonth)
		m_downtimeMonth = 0;
	m_uptimeMonth = 100.0 - (double)(m_downtimeMonth * 100) / (double)ServiceContainer::getSecondsInPeriod(MONTH);
	m_prevDiffMonth = timediffTillNow;

	if ((prevUptimeDay != m_uptimeDay) || (prevUptimeWeek != m_uptimeWeek) || (prevUptimeMonth != m_uptimeMonth))
	{
		Modify();
	}
	UnlockData();

	m_prevUptimeUpdateStatus = m_iStatus;
	m_prevUptimeUpdateTime = currentTime;

	DbgPrintf(7, _T("++++ ServiceContainer::updateUptimeStats() [%d] %lf %lf %lf"), int(m_dwId), m_uptimeDay, m_uptimeWeek, m_uptimeMonth);

	if (updateChilds)
	{
		LockChildList(FALSE);
		for (int i = 0; i < int(m_dwChildCount); i++)
		{
			NetObj *child = m_pChildList[i];
			if (child->Type() == OBJECT_BUSINESSSERVICE || child->Type() == OBJECT_NODELINK)
				((ServiceContainer*)child)->updateUptimeStats(currentTime, TRUE);
		}
		UnlockChildList();
	}
}


//
// Calculate number of seconds since the beginning of given period
//

LONG ServiceContainer::getSecondsSinceBeginningOf(Period period, time_t *beginTime)
{
	time_t curTime = time(NULL);
	struct tm *tms;
	struct tm tmBuffer;

#if HAVE_LOCALTIME_R
	tms = localtime_r(&curTime, &tmBuffer);
#else
	tms = localtime(&curTime);
	memcpy((void*)&tmBuffer, (void*)tms, sizeof(struct tm));
#endif

	tmBuffer.tm_hour = 0;
	tmBuffer.tm_min = 0;
	tmBuffer.tm_sec = 0;
	if (period == MONTH)
		tmBuffer.tm_mday = 1;
	time_t beginTimeL = mktime(&tmBuffer);
	if (period == WEEK)
	{
		if (tmBuffer.tm_wday == 0)
			tmBuffer.tm_wday = 7;
		tmBuffer.tm_wday--;
		beginTimeL -= 3600 * 24 * tmBuffer.tm_wday;
	}

	if (beginTime != NULL)
		*beginTime = beginTimeL;

	return LONG(curTime - beginTimeL);
}


//
// Calculate number of seconds in the current month
//

LONG ServiceContainer::getSecondsInMonth()
{
	time_t curTime = time(NULL);
	struct tm *tms;

#if HAVE_LOCALTIME_R
	struct tm tmBuffer;
	tms = localtime_r(&curTime, &tmBuffer);
#else
	tms = localtime(&curTime);
#endif

	int& month = tms->tm_mon;
	int year = tms->tm_year + 1900;
	int days = 31;

	if (month == 3 || month == 5 || month == 8 || month == 10)
		days = 30;	
	else if (month == 1) /* February */
		days = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0) ? 29 : 28;

	return LONG(days * 24 * 3600);
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool ServiceContainer::showThresholdSummary()
{
	return false;
}
