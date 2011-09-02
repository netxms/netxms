/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 NetXMS Team
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
** File: bizservice.cpp
**
**/

#include "nxcore.h"

#define QUERY_LENGTH		(512)

LONG BusinessService::logRecordId = -1;


//
// Service default constructor
//

BusinessService::BusinessService() : Container()
{
	m_busy = false;
	m_lastPollTime = time_t(0);
	m_lastPollStatus = m_prevUptimeUpdateStatus = STATUS_UNKNOWN;
	m_prevUptimeUpdateTime = time(NULL);
	_tcscpy(m_szName, _T("Default"));
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


//
// Constructor for new service object
//

BusinessService::BusinessService(const TCHAR *name) : Container(name, 0)
{
	m_busy = false;
	m_lastPollTime = time_t(0);
	m_lastPollStatus = m_prevUptimeUpdateStatus = STATUS_UNKNOWN;
	m_prevUptimeUpdateTime = time(NULL);
	nx_strncpy(m_szName, name, MAX_OBJECT_NAME);
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


//
// Service class destructor
//

BusinessService::~BusinessService()
{
}


//
// Calculate status for compound object based on childs status
//

void BusinessService::calculateCompoundStatus(BOOL bForcedRecalc)
{
	int i, iCount, iMostCriticalStatus;
	int iOldStatus = m_iStatus;

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

	if (iOldStatus != STATUS_UNKNOWN && iOldStatus != m_iStatus)
		addHistoryRecord();
}


//
// Create object from database data
//

BOOL BusinessService::CreateFromDB(DWORD id)
{
	if (!Container::CreateFromDB(id))
		return FALSE;

	// now it doesn't make any sense but hopefully will do in the future
	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT service_id FROM business_services WHERE service_id=?"));
	if (hStmt == NULL)
	{
		DbgPrintf(4, _T("Cannot prepare select from business_services"));
		return FALSE;
	}
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult == NULL)
	{
		DBFreeStatement(hStmt);
		return FALSE;
	}

	if (DBGetNumRows(hResult) == 0)
	{
		DBFreeResult(hResult);
		DBFreeStatement(hStmt);
		DbgPrintf(4, _T("Cannot load biz service object %ld - record missing"), (long)m_dwId);
		return FALSE;
	}

	DBFreeResult(hResult);
	DBFreeStatement(hStmt);

	initUptimeStats();

	return TRUE;
}


//
// Save service to database
//

BOOL BusinessService::SaveToDB(DB_HANDLE hdb)
{
	BOOL bNewObject = TRUE;

	LockData();

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT service_id FROM business_services WHERE service_id=?"));
	if (hStmt == NULL)
		return FALSE;
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult != NULL)
	{
		bNewObject = (DBGetNumRows(hResult) <= 0);
		DBFreeResult(hResult);
	}
	DBFreeStatement(hStmt);

	hStmt = DBPrepare(g_hCoreDB, bNewObject ? _T("INSERT INTO business_services (service_id) VALUES (?)") :
											  _T("UPDATE business_services SET service_id=service_id WHERE service_id=?"));
	if (hStmt == NULL)	
		return FALSE;
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
	if (!DBExecute(hStmt))
	{
		DBFreeStatement(hStmt);
		return FALSE;
	}

	DBFreeStatement(hStmt);

	saveACLToDB(hdb);

	// Unlock object and clear modification flag
	m_bIsModified = FALSE;
	UnlockData();

	return Container::SaveToDB(hdb);
}


//
// Delete object from database
//

BOOL BusinessService::DeleteFromDB()
{
   TCHAR szQuery[QUERY_LENGTH];
   BOOL bSuccess;

   bSuccess = Container::DeleteFromDB();
   if (bSuccess)
   {
      _sntprintf(szQuery, QUERY_LENGTH, _T("DELETE FROM business_services WHERE id=%d"), m_dwId);
      QueueSQLRequest(szQuery);
   }

   return bSuccess;
}


//
// Create CSCP message with object's data
//

void BusinessService::CreateMessage(CSCPMessage *pMsg)
{
   NetObj::CreateMessage(pMsg);
   pMsg->SetVariable(VID_UPTIME_DAY, m_uptimeDay);
   pMsg->SetVariable(VID_UPTIME_WEEK, m_uptimeWeek);
   pMsg->SetVariable(VID_UPTIME_MONTH, m_uptimeMonth);
}


//
// Modify object from message
//

DWORD BusinessService::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

   return NetObj::ModifyFromMessage(pRequest, TRUE);
}


//
// Check if service is ready for poll
//

bool BusinessService::isReadyForPolling()
{
	return time(NULL) - m_lastPollTime > g_dwSlmPollingInterval && !m_busy;
}


//
// Lock service for polling
//

void BusinessService::lockForPolling()
{
	m_busy = true;
}


//
// A callback for poller threads
//

void BusinessService::poll(ClientSession *pSession, DWORD dwRqId, int nPoller)
{
	DbgPrintf(5, _T("Started polling of business service %s [%d]"), m_szName, (int)m_dwId);
	m_lastPollTime = time(NULL);

	// Loop through the kids and execute their either scripts or thresholds
   LockChildList(FALSE);
	for (int i = 0; i < int(m_dwChildCount); i++)
	{
		if (m_pChildList[i]->Type() == OBJECT_SLMCHECK)
			((SlmCheck *)m_pChildList[i])->execute();
		else if (m_pChildList[i]->Type() == OBJECT_NODELINK)
			((NodeLink *)m_pChildList[i])->execute();
	}
   UnlockChildList();

	// Set the status based on what the kids' been up to
	calculateCompoundStatus();

	m_lastPollStatus = m_iStatus;
	DbgPrintf(5, _T("Finished polling of business service %s [%d]"), m_szName, (int)m_dwId);
	m_busy = false;
}

//
// Set service status - use this instead of direct assignment;
//

void BusinessService::setStatus(int newStatus)
{
	m_iStatus = newStatus;
	updateUptimeStats();	
}

//
// Add a record to slm_service_history table
//

BOOL BusinessService::addHistoryRecord()
{
	DB_RESULT hResult;
	DB_STATEMENT hStmt;

	if (BusinessService::logRecordId < 0)
	{
		hResult = DBSelect(g_hCoreDB, _T("SELECT max(record_id) FROM slm_service_history"));
		if (hResult == NULL)
			return FALSE;
		BusinessService::logRecordId = DBGetNumRows(hResult) > 0 ? DBGetFieldLong(hResult, 0, 0) : 0;
		DBFreeResult(hResult);
	}

	BusinessService::logRecordId++;

	hStmt = DBPrepare(g_hCoreDB, _T("INSERT INTO slm_service_history (record_id,service_id,change_timestamp,new_status) ")
		_T("VALUES (?,?,?,?)"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, BusinessService::logRecordId);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_dwId);
		DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, DWORD(time(NULL)));
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, DWORD(m_iStatus));
		if (!DBExecute(hStmt))
		{
			DBFreeStatement(hStmt);
			return FALSE;
		}
		DbgPrintf(9, _T("BusinessService::addHistoryRecord() ok with id %ld"), BusinessService::logRecordId);
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

void BusinessService::initUptimeStats()
{
	m_uptimeDay = getUptimeFromDBFor(DAY, &m_downtimeDay);
	m_uptimeWeek = getUptimeFromDBFor(WEEK, &m_downtimeWeek);
	m_uptimeMonth = getUptimeFromDBFor(MONTH, &m_downtimeMonth);
	DbgPrintf(7, _T("++++ BusinessService::initUptimeStats() %lf %lf %lf"), m_uptimeDay, m_uptimeWeek, m_uptimeMonth);
}

//
// Calculate uptime for given period using data in database
//

double BusinessService::getUptimeFromDBFor(Period period, LONG *downtime)
{
	time_t beginTime;
	LONG timediffTillNow	= BusinessService::getSecondsSinceBeginningOf(period, &beginTime);
	double percentage = 0;

	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT change_timestamp,new_status FROM slm_service_history ")
											  _T("WHERE service_id=? AND change_timestamp>?"));
	if (hStmt != NULL)
	{
		time_t changeTimestamp, prevChangeTimestamp;
		int newStatus;
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (DWORD)beginTime);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult == NULL)
		{
			DBFreeStatement(hStmt);
			return percentage;
		}

		int numRows = DBGetNumRows(hResult);
		*downtime = 0;
		prevChangeTimestamp = beginTime;
		if (numRows > 0) // if <= 0 then assume zero downtime   FIXME: it can be 100% downtime
		{
			for (int i = 0; i < numRows; i++)
			{
				changeTimestamp = DBGetFieldLong(hResult, i, 0);
				newStatus = DBGetFieldLong(hResult, i, 1);
				if (newStatus == STATUS_NORMAL)
					*downtime += (LONG)(changeTimestamp - prevChangeTimestamp);
				else 
					prevChangeTimestamp = changeTimestamp;
			}
			if (newStatus == STATUS_CRITICAL) // the service is still down, add period till now
				*downtime += LONG(time(NULL) - prevChangeTimestamp);
		}
		percentage = 100.0 - (double)(*downtime * 100) / (double)getSecondsInPeriod(period);
		DbgPrintf(7, _T("++++ BusinessService::getUptimeFromDBFor(), downtime %ld"), *downtime);

		DBFreeResult(hResult);
		DBFreeStatement(hStmt);
	}
	
	return percentage;
}


//
// Update uptime counters 
//

void BusinessService::updateUptimeStats()
{
	LONG timediffTillNow;
	LONG downtimeBetweenPolls = 0;
	time_t curTime = time(NULL);

	LockData();

	double prevUptimeDay = m_uptimeDay;
	double prevUptimeWeek = m_uptimeWeek;
	double prevUptimeMonth = m_uptimeMonth;

	if (m_iStatus == STATUS_CRITICAL && m_prevUptimeUpdateStatus == STATUS_CRITICAL)
	{
		downtimeBetweenPolls = LONG(curTime - m_prevUptimeUpdateTime);		
		DbgPrintf(7, _T("++++ BusinessService::updateUptimeStats() both statuses critical"));
	}

	timediffTillNow = BusinessService::getSecondsSinceBeginningOf(DAY, NULL);
	m_downtimeDay += downtimeBetweenPolls;
	if (timediffTillNow < m_prevDiffDay)
		m_downtimeDay = 0;
	m_uptimeDay = 100.0 - (double)(m_downtimeDay * 100) / (double)BusinessService::getSecondsInPeriod(DAY);
	m_prevDiffDay = timediffTillNow;
	DbgPrintf(7, _T("++++ BusinessService::updateUptimeStats() m_downtimeDay %ld, timediffTillNow %ld, downtimeBetweenPolls %ld"), 
				m_downtimeDay, timediffTillNow, downtimeBetweenPolls);

	timediffTillNow = BusinessService::getSecondsSinceBeginningOf(WEEK, NULL);
	m_downtimeWeek += downtimeBetweenPolls;
	if (timediffTillNow < m_prevDiffWeek)
		m_downtimeWeek = 0;
	m_uptimeWeek = 100.0 - (double)(m_downtimeWeek * 100) / (double)BusinessService::getSecondsInPeriod(WEEK);
	m_prevDiffWeek = timediffTillNow;

	timediffTillNow = BusinessService::getSecondsSinceBeginningOf(MONTH, NULL);
	m_downtimeMonth += downtimeBetweenPolls;
	if (timediffTillNow < m_prevDiffMonth)
		m_downtimeMonth = 0;
	m_uptimeMonth = 100.0 - (double)(m_downtimeMonth * 100) / (double)BusinessService::getSecondsInPeriod(MONTH);
	m_prevDiffMonth = timediffTillNow;

	if ((prevUptimeDay != m_uptimeDay) || (prevUptimeWeek != m_uptimeWeek) || (prevUptimeMonth != m_uptimeMonth))
	{
		Modify();
	}
	UnlockData();
	
	m_prevUptimeUpdateStatus = m_iStatus;
	m_prevUptimeUpdateTime = curTime;
	
	DbgPrintf(7, _T("++++ BusinessService::updateUptimeStats() %lf %lf %lf"), m_uptimeDay, m_uptimeWeek, m_uptimeMonth);
}


//
// Calculate number of seconds since the beginning of given period
//

LONG BusinessService::getSecondsSinceBeginningOf(Period period, time_t *beginTime)
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

LONG BusinessService::getSecondsInMonth()
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
