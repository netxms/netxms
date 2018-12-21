/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Raden Solutions
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
INT32 __EXPORT ServiceContainer::logRecordId = -1;

/**
 * Default constructor for service service object
 */
ServiceContainer::ServiceContainer() : super()
{
	initServiceContainer();
}

/**
 * Create new service container object
 */
ServiceContainer::ServiceContainer(const TCHAR *pszName) : super(pszName, 0)
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
bool ServiceContainer::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
	if (!super::loadFromDatabase(hdb, id))
		return false;

	initUptimeStats();
	return true;
}

/**
 * Save object to database
 */
bool ServiceContainer::saveToDatabase(DB_HANDLE hdb)
{
	return super::saveToDatabase(hdb);
}

/**
 * Delete object from database
 */
bool ServiceContainer::deleteFromDatabase(DB_HANDLE hdb)
{
	return super::deleteFromDatabase(hdb);
}

/**
 * Create NXCP message with object's data
 */
void ServiceContainer::fillMessageInternal(NXCPMessage *pMsg, UINT32 userId)
{
   super::fillMessageInternal(pMsg, userId);
   pMsg->setField(VID_UPTIME_DAY, m_uptimeDay);
   pMsg->setField(VID_UPTIME_WEEK, m_uptimeWeek);
   pMsg->setField(VID_UPTIME_MONTH, m_uptimeMonth);
}

/**
 * Modify object from message
 */
UINT32 ServiceContainer::modifyFromMessageInternal(NXCPMessage *pRequest)
{
   return super::modifyFromMessageInternal(pRequest);
}

/**
 * Calculate status for compound object based on childs status
 */
void ServiceContainer::calculateCompoundStatus(BOOL bForcedRecalc)
{
	int i, iCount, iMostCriticalStatus;
	int iOldStatus = m_status;

	DbgPrintf(7, _T("ServiceContainer::calculateCompoundStatus() for %s [%d]"), m_name, m_id);

	// Calculate own status by selecting the most critical status of the kids
	lockChildList(false);
	for(i = 0, iCount = 0, iMostCriticalStatus = -1; i < m_childList->size(); i++)
	{
		int iChildStatus = m_childList->get(i)->getStatus();
		if ((iChildStatus < STATUS_UNKNOWN) &&
			(iChildStatus > iMostCriticalStatus))
		{
			iMostCriticalStatus = iChildStatus;
			iCount++;
		}
	}
	// Set status and update uptime counters
	setStatus((iCount > 0) ? iMostCriticalStatus : STATUS_UNKNOWN);
	unlockChildList();

	// Cause parent object(s) to recalculate it's status
	if ((iOldStatus != m_status) || bForcedRecalc)
	{
		lockParentList(false);
		for(i = 0; i < m_parentList->size(); i++)
			m_parentList->get(i)->calculateCompoundStatus();
		unlockParentList();
		lockProperties();
		setModified(MODIFY_COMMON_PROPERTIES);
		unlockProperties();
	}

	DbgPrintf(6, _T("ServiceContainer::calculateCompoundStatus(%s [%d]): old_status=%d new_status=%d"), m_name, m_id, iOldStatus, m_status);

	if (iOldStatus != STATUS_UNKNOWN && iOldStatus != m_status)
		addHistoryRecord();
}

/**
 * Set service status - use this instead of direct assignment;
 */
void ServiceContainer::setStatus(int newStatus)
{
	m_status = newStatus;
}

/**
 * Add a record to slm_service_history table
 */
BOOL ServiceContainer::addHistoryRecord()
{
	DB_RESULT hResult;
	DB_STATEMENT hStmt;

	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	if (ServiceContainer::logRecordId < 0)
	{
		hResult = DBSelect(hdb, _T("SELECT max(record_id) FROM slm_service_history"));
		if (hResult == NULL)
      {
         DBConnectionPoolReleaseConnection(hdb);
         return FALSE;
      }
		ServiceContainer::logRecordId = DBGetNumRows(hResult) > 0 ? DBGetFieldLong(hResult, 0, 0) : 0;
		DBFreeResult(hResult);
	}

	ServiceContainer::logRecordId++;

	hStmt = DBPrepare(hdb, _T("INSERT INTO slm_service_history (record_id,service_id,change_timestamp,new_status) ")
		_T("VALUES (?,?,?,?)"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, ServiceContainer::logRecordId);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
		DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, UINT32(time(NULL)));
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, UINT32(m_status));
		if (!DBExecute(hStmt))
		{
			DBFreeStatement(hStmt);
         DBConnectionPoolReleaseConnection(hdb);
			return FALSE;
		}
		DbgPrintf(9, _T("ServiceContainer::addHistoryRecord() ok with id %d"), ServiceContainer::logRecordId);
	}
	else
	{
      DBConnectionPoolReleaseConnection(hdb);
		return FALSE;
	}

	DBFreeStatement(hStmt);
	DBConnectionPoolReleaseConnection(hdb);
	return TRUE;
}

/**
 * Initialize uptime statistics (daily, weekly, monthly) by examining slm_service_history
 */
void ServiceContainer::initUptimeStats()
{
	lockProperties();
	m_prevUptimeUpdateStatus = m_status;
	m_uptimeDay = getUptimeFromDBFor(DAY, &m_downtimeDay);
	m_uptimeWeek = getUptimeFromDBFor(WEEK, &m_downtimeWeek);
	m_uptimeMonth = getUptimeFromDBFor(MONTH, &m_downtimeMonth);
	unlockProperties();
	DbgPrintf(6, _T("ServiceContainer::initUptimeStats() %s [%d] %lf %lf %lf"), m_name, m_id, m_uptimeDay, m_uptimeWeek, m_uptimeMonth);
}

/**
 * Calculate uptime for given period using data in database
 */
double ServiceContainer::getUptimeFromDBFor(Period period, INT32 *downtime)
{
	time_t beginTime;
	INT32 timediffTillNow	= ServiceContainer::getSecondsSinceBeginningOf(period, &beginTime);
	double percentage = 0;

	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT change_timestamp,new_status FROM slm_service_history ")
	                                    _T("WHERE service_id=? AND change_timestamp>?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (UINT32)beginTime);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult == NULL)
		{
			DBFreeStatement(hStmt);
         DBConnectionPoolReleaseConnection(hdb);
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
		if (realRows == 0 && m_status == STATUS_CRITICAL)
			*downtime = timediffTillNow;
		percentage = 100.0 - (double)(*downtime * 100) / (double)getSecondsInPeriod(period);
		DbgPrintf(7, _T("++++ ServiceContainer::getUptimeFromDBFor(), downtime %ld"), *downtime);

		DBFreeResult(hResult);
		DBFreeStatement(hStmt);
	}

	DBConnectionPoolReleaseConnection(hdb);
	return percentage;
}

/**
 * Update uptime counters
 */
void ServiceContainer::updateUptimeStats(time_t currentTime, BOOL updateChilds)
{
	LONG timediffTillNow;
	LONG downtimeBetweenPolls = 0;

	if (currentTime == 0)
		currentTime = time(NULL);

	lockProperties();

	double prevUptimeDay = m_uptimeDay;
	double prevUptimeWeek = m_uptimeWeek;
	double prevUptimeMonth = m_uptimeMonth;

	if (m_status == STATUS_CRITICAL && m_prevUptimeUpdateStatus == STATUS_CRITICAL)
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
		setModified(MODIFY_OTHER);
	}
	unlockProperties();

	m_prevUptimeUpdateStatus = m_status;
	m_prevUptimeUpdateTime = currentTime;

	DbgPrintf(7, _T("++++ ServiceContainer::updateUptimeStats() [%d] %lf %lf %lf"), int(m_id), m_uptimeDay, m_uptimeWeek, m_uptimeMonth);

	if (updateChilds)
	{
		lockChildList(false);
		for(int i = 0; i < m_childList->size(); i++)
		{
			NetObj *child = m_childList->get(i);
			if (child->getObjectClass() == OBJECT_BUSINESSSERVICE || child->getObjectClass() == OBJECT_NODELINK)
				((ServiceContainer*)child)->updateUptimeStats(currentTime, TRUE);
		}
		unlockChildList();
	}
}

/**
 * Calculate number of seconds since the beginning of given period
 */
INT32 ServiceContainer::getSecondsSinceBeginningOf(Period period, time_t *beginTime)
{
	time_t curTime = time(NULL);
	struct tm tmBuffer;

#if HAVE_LOCALTIME_R
	localtime_r(&curTime, &tmBuffer);
#else
	struct tm *tms = localtime(&curTime);
	memcpy(&tmBuffer, tms, sizeof(struct tm));
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

	return (INT32)(curTime - beginTimeL);
}

/**
 * Calculate number of seconds in the current month
 */
INT32 ServiceContainer::getSecondsInMonth()
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

	return (INT32)(days * 24 * 3600);
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool ServiceContainer::showThresholdSummary()
{
	return false;
}
