/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: dcobject.cpp
**
**/

#include "nxcore.h"


//
// Default constructor for DCObject
//

DCObject::DCObject()
{
   m_dwId = 0;
   m_dwTemplateId = 0;
   m_dwTemplateItemId = 0;
   m_busy = 0;
	m_scheduledForDeletion = 0;
   m_iPollingInterval = 3600;
   m_iRetentionTime = 0;
   m_source = DS_INTERNAL;
   m_status = ITEM_STATUS_NOT_SUPPORTED;
   m_szName[0] = 0;
   m_szDescription[0] = 0;
	m_systemTag[0] = 0;
   m_tLastPoll = 0;
   m_pNode = NULL;
   m_hMutex = MutexCreateRecursive();
   m_dwNumSchedules = 0;
   m_ppScheduleList = NULL;
   m_tLastCheck = 0;
	m_flags = 0;
   m_dwErrorCount = 0;
	m_dwResourceId = 0;
	m_dwProxyNode = 0;
	m_pszPerfTabSettings = NULL;
	m_snmpPort = 0;	// use default
}


//
// Create DCObject from another DCObject
//

DCObject::DCObject(const DCObject *pSrc)
{
   DWORD i;

   m_dwId = pSrc->m_dwId;
   m_dwTemplateId = pSrc->m_dwTemplateId;
   m_dwTemplateItemId = pSrc->m_dwTemplateItemId;
   m_busy = 0;
	m_scheduledForDeletion = 0;
   m_iPollingInterval = pSrc->m_iPollingInterval;
   m_iRetentionTime = pSrc->m_iRetentionTime;
   m_source = pSrc->m_source;
   m_status = pSrc->m_status;
   m_tLastPoll = 0;
	_tcscpy(m_szName, pSrc->m_szName);
	_tcscpy(m_szDescription, pSrc->m_szDescription);
	_tcscpy(m_systemTag, pSrc->m_systemTag);
   m_pNode = NULL;
   m_hMutex = MutexCreateRecursive();
   m_tLastCheck = 0;
   m_dwErrorCount = 0;
	m_flags = pSrc->m_flags;
	m_dwResourceId = pSrc->m_dwResourceId;
	m_dwProxyNode = pSrc->m_dwProxyNode;
	m_pszPerfTabSettings = (pSrc->m_pszPerfTabSettings != NULL) ? _tcsdup(pSrc->m_pszPerfTabSettings) : NULL;
	m_snmpPort = pSrc->m_snmpPort;

   // Copy schedules
   m_dwNumSchedules = pSrc->m_dwNumSchedules;
   m_ppScheduleList = (TCHAR **)malloc(sizeof(TCHAR *) * m_dwNumSchedules);
   for(i = 0; i < m_dwNumSchedules; i++)
      m_ppScheduleList[i] = _tcsdup(pSrc->m_ppScheduleList[i]);
}


//
// Constructor for creating new DCObject from scratch
//

DCObject::DCObject(DWORD dwId, const TCHAR *szName, int iSource, 
               int iPollingInterval, int iRetentionTime, Template *pNode,
               const TCHAR *pszDescription, const TCHAR *systemTag)
{
   m_dwId = dwId;
   m_dwTemplateId = 0;
   m_dwTemplateItemId = 0;
   nx_strncpy(m_szName, szName, MAX_ITEM_NAME);
   if (pszDescription != NULL)
      nx_strncpy(m_szDescription, pszDescription, MAX_DB_STRING);
   else
      _tcscpy(m_szDescription, m_szName);
	nx_strncpy(m_systemTag, CHECK_NULL_EX(systemTag), MAX_DB_STRING);
   m_source = iSource;
   m_iPollingInterval = iPollingInterval;
   m_iRetentionTime = iRetentionTime;
   m_status = ITEM_STATUS_ACTIVE;
   m_busy = 0;
	m_scheduledForDeletion = 0;
   m_tLastPoll = 0;
   m_pNode = pNode;
   m_hMutex = MutexCreateRecursive();
   m_flags = 0;
   m_dwNumSchedules = 0;
   m_ppScheduleList = NULL;
   m_tLastCheck = 0;
   m_dwErrorCount = 0;
	m_dwResourceId = 0;
	m_dwProxyNode = 0;
	m_pszPerfTabSettings = NULL;
	m_snmpPort = 0;	// use default
}


//
// Create DCObject from import file
//

DCObject::DCObject(ConfigEntry *config, Template *owner)
{
   m_dwId = CreateUniqueId(IDG_ITEM);
   m_dwTemplateId = 0;
   m_dwTemplateItemId = 0;
	nx_strncpy(m_szName, config->getSubEntryValue(_T("name"), 0, _T("unnamed")), MAX_ITEM_NAME);
   nx_strncpy(m_szDescription, config->getSubEntryValue(_T("description"), 0, m_szName), MAX_DB_STRING);
	nx_strncpy(m_systemTag, config->getSubEntryValue(_T("systemTag"), 0, _T("")), MAX_DB_STRING);
	m_source = (BYTE)config->getSubEntryValueInt(_T("origin"));
   m_iPollingInterval = config->getSubEntryValueInt(_T("interval"));
   m_iRetentionTime = config->getSubEntryValueInt(_T("retention"));
   m_status = ITEM_STATUS_ACTIVE;
   m_busy = 0;
	m_scheduledForDeletion = 0;
	m_flags = 0;
   m_tLastPoll = 0;
   m_pNode = owner;
   m_hMutex = MutexCreateRecursive();
   m_tLastCheck = 0;
   m_dwErrorCount = 0;
	m_dwResourceId = 0;
	m_dwProxyNode = 0;
	m_pszPerfTabSettings = NULL;
	m_snmpPort = (WORD)config->getSubEntryValueInt(_T("snmpPort"));

	if (config->getSubEntryValueInt(_T("advancedSchedule")))
		m_flags |= DCF_ADVANCED_SCHEDULE;

	ConfigEntry *schedules = config->findEntry(_T("schedules"));
	if (schedules != NULL)
		schedules = schedules->findEntry(_T("schedule"));
	if (schedules != NULL)
	{
		m_dwNumSchedules = (DWORD)schedules->getValueCount();
		m_ppScheduleList = (TCHAR **)malloc(sizeof(TCHAR *) * m_dwNumSchedules);
		for(int i = 0; i < (int)m_dwNumSchedules; i++)
		{
			m_ppScheduleList[i] = _tcsdup(schedules->getValue(i));
		}
	}
	else
	{
		m_dwNumSchedules = 0;
		m_ppScheduleList = NULL;
	}
}


//
// Destructor
//

DCObject::~DCObject()
{
   for(DWORD i = 0; i < m_dwNumSchedules; i++)
      free(m_ppScheduleList[i]);
   safe_free(m_ppScheduleList);
	safe_free(m_pszPerfTabSettings);
   MutexDestroy(m_hMutex);
}


//
// Load custom schedules from database
// (assumes that no schedules was created before this call)
//

BOOL DCObject::loadCustomSchedules()
{
   if (!(m_flags & DCF_ADVANCED_SCHEDULE))
		return TRUE;

	TCHAR query[256];

   _sntprintf(query, 256, _T("SELECT schedule FROM dci_schedules WHERE item_id=%d"), m_dwId);
   DB_RESULT hResult = DBSelect(g_hCoreDB, query);
   if (hResult != NULL)
   {
      m_dwNumSchedules = (DWORD)DBGetNumRows(hResult);
      m_ppScheduleList = (TCHAR **)malloc(sizeof(TCHAR *) * m_dwNumSchedules);
      for(DWORD i = 0; i < m_dwNumSchedules; i++)
      {
         m_ppScheduleList[i] = DBGetField(hResult, i, 0, NULL, 0);
      }
      DBFreeResult(hResult);
   }

	return hResult != NULL;
}


//
// Check if associated cluster resource is active. Returns true also if
// DCI has no resource association
//

bool DCObject::matchClusterResource()
{
	Cluster *pCluster;

	if (m_dwResourceId == 0)
		return true;

	pCluster = ((Node *)m_pNode)->getMyCluster();
	if (pCluster == NULL)
		return false;	// Has association, but cluster object cannot be found

	return pCluster->isResourceOnNode(m_dwResourceId, m_pNode->Id());
}


//
// Expand macros in text
//

void DCObject::expandMacros(const TCHAR *src, TCHAR *dst, size_t dstLen)
{
	String temp;
	TCHAR *head, *rest, *macro;
	int index = 0, index2;

	temp = src;
	while((index = temp.find(_T("%{"), index)) != String::npos)
	{
		head = temp.subStr(0, index);
		index2 = temp.find(_T("}"), index);
		if (index2 == String::npos)
		{
			free(head);
			break;	// Missing closing }
		}
		rest = temp.subStr(index2 + 1, -1);
		macro = temp.subStr(index + 2, index2 - index - 2);
		StrStrip(macro);

		temp = head;
		if (!_tcscmp(macro, _T("node_id")))
		{
			if (m_pNode != NULL)
			{
				temp.addFormattedString(_T("%d"), m_pNode->Id());
			}
			else
			{
				temp += _T("(error)");
			}
		}
		else if (!_tcscmp(macro, _T("node_name")))
		{
			if (m_pNode != NULL)
			{
				temp += m_pNode->Name();
			}
			else
			{
				temp += _T("(error)");
			}
		}
		else if (!_tcscmp(macro, _T("node_primary_ip")))
		{
			if (m_pNode != NULL)
			{
				TCHAR ipAddr[32];

				temp += IpToStr(m_pNode->IpAddr(), ipAddr);
			}
			else
			{
				temp += _T("(error)");
			}
		}
		else if (!_tcsncmp(macro, _T("script:"), 7))
		{
			NXSL_Program *script;
			NXSL_ServerEnv *pEnv;

	      g_pScriptLibrary->lock();
			script = g_pScriptLibrary->findScript(&macro[7]);
			if (script != NULL)
			{
				pEnv = new NXSL_ServerEnv;
				if (m_pNode != NULL)
					script->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, m_pNode)));

				if (script->run(pEnv) == 0)
				{
					NXSL_Value *result = script->getResult();
					if (result != NULL)
						temp += CHECK_NULL_EX(result->getValueAsCString());
		         DbgPrintf(4, _T("DCItem::expandMacros(%d,\"%s\"): Script %s executed successfully"), m_dwId, src, &macro[7]);
				}
				else
				{
		         DbgPrintf(4, _T("DCItem::expandMacros(%d,\"%s\"): Script %s execution error: %s"),
					          m_dwId, src, &macro[7], script->getErrorText());
					PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", &macro[7],
								 script->getErrorText(), m_dwId);
				}
			}
			else
			{
	         DbgPrintf(4, _T("DCItem::expandMacros(%d,\"%s\"): Cannot find script %s"), m_dwId, src, &macro[7]);
			}
	      g_pScriptLibrary->unlock();
		}
		temp += rest;
		
		free(head);
		free(rest);
		free(macro);
	}
	nx_strncpy(dst, temp, dstLen);
}


//
// Delete all collected data
//

bool DCObject::deleteAllData()
{
	return false;
}


//
// Clean expired data
//

void DCObject::deleteExpiredData()
{
}


//
// Add schedule
//

void DCObject::addSchedule(const TCHAR *pszSchedule)
{
	m_dwNumSchedules++;
	m_ppScheduleList = (TCHAR **)realloc(m_ppScheduleList, sizeof(TCHAR *) * m_dwNumSchedules);
	m_ppScheduleList[m_dwNumSchedules - 1] = _tcsdup(pszSchedule);
}


//
// Set new ID and node/template association
//

void DCObject::changeBinding(DWORD dwNewId, Template *pNewNode, BOOL doMacroExpansion)
{
   lock();
   m_pNode = pNewNode;
	if (dwNewId != 0)
		m_dwId = dwNewId;

	if (doMacroExpansion)
	{
		expandMacros(m_szName, m_szName, MAX_ITEM_NAME);
		expandMacros(m_szDescription, m_szDescription, MAX_DB_STRING);
	}

   unlock();
}


//
// Set DCI status
//

void DCObject::setStatus(int status, bool generateEvent)
{
	if (generateEvent && (m_pNode != NULL) && (m_status != (BYTE)status))
	{
		static DWORD eventCode[3] = { EVENT_DCI_ACTIVE, EVENT_DCI_DISABLED, EVENT_DCI_UNSUPPORTED };
		static const TCHAR *originName[5] = { _T("Internal"), _T("NetXMS Agent"), _T("SNMP"), _T("CheckPoint SNMP"), _T("Push") };

		PostEvent(eventCode[status], m_pNode->Id(), "dssds", m_dwId, m_szName, m_szDescription,
		          m_source, originName[m_source]);
	}
	m_status = (BYTE)status;
}


//
// Get step size for "%" and "/" crontab cases
//

static int GetStepSize(TCHAR *str)
{
  int step = 0;
  if (str != NULL)
  {
    *str = 0;
    str++;
    step = *str == _T('\0') ? 1 : _tcstol(str, NULL, 10);
  }

  if (step <= 0)
  {
    step = 1;
  }

  return step;
}

/**
 * Match schedule element
 * NOTE: We assume that pattern can be modified during processing
 */
static BOOL MatchScheduleElement(TCHAR *pszPattern, int nValue, time_t currTime = 0)
{
   TCHAR *ptr, *curr;
   int nStep, nCurr, nPrev;
   BOOL bRun = TRUE, bRange = FALSE;

	// Check if time() step was specified (% - special syntax)
	ptr = _tcschr(pszPattern, _T('%'));
	if (ptr != NULL)
		return (currTime % GetStepSize(ptr)) != 0;

   // Check if step was specified
   ptr = _tcschr(pszPattern, _T('/'));
   nStep = GetStepSize(ptr);

   if (*pszPattern == _T('*'))
      goto check_step;

   for(curr = pszPattern; bRun; curr = ptr + 1)
   {
      for(ptr = curr; (*ptr != 0) && (*ptr != '-') && (*ptr != ','); ptr++);
      switch(*ptr)
      {
         case '-':
            if (bRange)
               return FALSE;  // Form like 1-2-3 is invalid
            bRange = TRUE;
            *ptr = 0;
            nPrev = _tcstol(curr, NULL, 10);
            break;
         case 0:
            bRun = FALSE;
         case ',':
            *ptr = 0;
            nCurr = _tcstol(curr, NULL, 10);
            if (bRange)
            {
               if ((nValue >= nPrev) && (nValue <= nCurr))
                  goto check_step;
               bRange = FALSE;
            }
            else
            {
               if (nValue == nCurr)
                  return TRUE;
            }
            break;
      }
   }

   return FALSE;

check_step:
   return (nValue % nStep) == 0;
}

/**
 * Match schedule to current time
 */
BOOL DCObject::matchSchedule(struct tm *pCurrTime, TCHAR *pszSchedule, BOOL *bWithSeconds, time_t currTimestamp)
{
   const TCHAR *pszCurr;
	TCHAR szValue[256];
   TCHAR *realSchedule = pszSchedule;

   if (_tcslen(pszSchedule) > 4 && !_tcsncmp(pszSchedule, _T("%["), 2))
   {
      TCHAR *scriptName = _tcsdup(pszSchedule + 2);
      if (scriptName != NULL)
      {
         TCHAR *closingBracker = _tcschr(scriptName, _T(']'));
         if (closingBracker != NULL)
         {
            *closingBracker = 0;

            g_pScriptLibrary->lock();
            NXSL_Program *script = g_pScriptLibrary->findScript(scriptName);
            if (script != NULL)
            {
               NXSL_ServerEnv *env = new NXSL_ServerEnv;
               script->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, m_pNode)));
               script->setGlobalVariable(_T("$dci"), new NXSL_Value(new NXSL_Object(&g_nxslDciClass, this)));
               if (script->run(env) == 0)
               {
                  NXSL_Value *result = script->getResult();
                  if (result != NULL)
                  {
                     const TCHAR *temp = result->getValueAsCString();
                     if (temp != NULL)
                     {
                        DbgPrintf(7, _T("DCObject::matchSchedule(%%[%s]) expanded to \"%s\""), scriptName, temp);
                        realSchedule = _tcsdup(temp);
                     }
                  }
               }
               else
               {
                  DbgPrintf(4, _T("DCObject::matchSchedule(%%[%s]) script execution failed"), scriptName);
               }
            }
            g_pScriptLibrary->unlock();
         }
         free(scriptName);
      }
      else
      {
         // invalid syntax
         // TODO: add logging
         return FALSE;
      }
   }

   // Minute
   pszCurr = ExtractWord(realSchedule, szValue);
   if (!MatchScheduleElement(szValue, pCurrTime->tm_min))
         return FALSE;

   // Hour
   pszCurr = ExtractWord(pszCurr, szValue);
   if (!MatchScheduleElement(szValue, pCurrTime->tm_hour))
         return FALSE;

   // Day of month
   pszCurr = ExtractWord(pszCurr, szValue);
   if (!MatchScheduleElement(szValue, pCurrTime->tm_mday))
         return FALSE;

   // Month
   pszCurr = ExtractWord(pszCurr, szValue);
   if (!MatchScheduleElement(szValue, pCurrTime->tm_mon + 1))
         return FALSE;

   // Day of week
   pszCurr = ExtractWord(pszCurr, szValue);
	for(int i = 0; szValue[i] != 0; i++)
		if (szValue[i] == _T('7'))
			szValue[i] = _T('0');
   if (!MatchScheduleElement(szValue, pCurrTime->tm_wday))
		return FALSE;

	// Seconds
	szValue[0] = _T('\0');
	ExtractWord(pszCurr, szValue);
	if (szValue[0] != _T('\0'))
	{
		if (bWithSeconds)
			*bWithSeconds = TRUE;
		return MatchScheduleElement(szValue, pCurrTime->tm_sec, currTimestamp);
	}

	return TRUE;
}

/**
 * Check if data collection object have to be polled
 */
bool DCObject::isReadyForPolling(time_t currTime)
{
   bool result;

   lock();
   if ((m_status != ITEM_STATUS_DISABLED) && (!m_busy) &&
       isCacheLoaded() && (m_source != DS_PUSH_AGENT) &&
		 matchClusterResource())
   {
      if (m_flags & DCF_ADVANCED_SCHEDULE)
      {
         DWORD i;
         struct tm tmCurrLocal, tmLastLocal;
			BOOL bWithSeconds = FALSE;

         memcpy(&tmCurrLocal, localtime(&currTime), sizeof(struct tm));
         memcpy(&tmLastLocal, localtime(&m_tLastCheck), sizeof(struct tm));
         for(i = 0, result = false; i < m_dwNumSchedules; i++)
         {
            if (matchSchedule(&tmCurrLocal, m_ppScheduleList[i], &bWithSeconds, currTime))
            {
					// TODO: do we have to take care about the schedules with seconds
					// that trigger polling too often?
               if (bWithSeconds || (currTime - m_tLastCheck >= 60) ||
                   (tmCurrLocal.tm_min != tmLastLocal.tm_min))
               {
                  result = true;
                  break;
               }
            }
         }
         m_tLastCheck = currTime;
      }
      else
      {
			if (m_status == ITEM_STATUS_NOT_SUPPORTED)
		      result = (m_tLastPoll + m_iPollingInterval * 10 <= currTime);
			else
		      result = (m_tLastPoll + m_iPollingInterval <= currTime);
      }
   }
   else
   {
      result = false;
   }
   unlock();
   return result;
}

/**
 * Returns true if internal cache is loaded. If data collection object
 * does not have cache should return true
 */
bool DCObject::isCacheLoaded()
{
	return true;
}

/**
 * Prepare object for deletion
 */
bool DCObject::prepareForDeletion()
{
	DbgPrintf(9, _T("DCObject::prepareForDeletion for DCO %d"), m_dwId);

	lock();
   m_status = ITEM_STATUS_DISABLED;   // Prevent future polls
	m_scheduledForDeletion = 1;
	bool canDelete = (m_busy ? false : true);
   unlock();
	DbgPrintf(9, _T("DCObject::prepareForDeletion: completed for DCO %d, canDelete=%d"), m_dwId, (int)canDelete);

	return canDelete;
}


//
// Create NXCP message with object data
//

void DCObject::createMessage(CSCPMessage *pMsg)
{
	lock();
   pMsg->SetVariable(VID_DCI_ID, m_dwId);
	pMsg->SetVariable(VID_DCOBJECT_TYPE, (WORD)getType());
   pMsg->SetVariable(VID_TEMPLATE_ID, m_dwTemplateId);
   pMsg->SetVariable(VID_NAME, m_szName);
   pMsg->SetVariable(VID_DESCRIPTION, m_szDescription);
   pMsg->SetVariable(VID_FLAGS, m_flags);
   pMsg->SetVariable(VID_SYSTEM_TAG, m_systemTag);
   pMsg->SetVariable(VID_POLLING_INTERVAL, (DWORD)m_iPollingInterval);
   pMsg->SetVariable(VID_RETENTION_TIME, (DWORD)m_iRetentionTime);
   pMsg->SetVariable(VID_DCI_SOURCE_TYPE, (WORD)m_source);
   pMsg->SetVariable(VID_DCI_STATUS, (WORD)m_status);
	pMsg->SetVariable(VID_RESOURCE_ID, m_dwResourceId);
	pMsg->SetVariable(VID_AGENT_PROXY, m_dwProxyNode);
	pMsg->SetVariable(VID_SNMP_PORT, m_snmpPort);
	if (m_pszPerfTabSettings != NULL)
		pMsg->SetVariable(VID_PERFTAB_SETTINGS, m_pszPerfTabSettings);
   if (m_flags & DCF_ADVANCED_SCHEDULE)
   {
      pMsg->SetVariable(VID_NUM_SCHEDULES, m_dwNumSchedules);
      for(DWORD i = 0, dwId = VID_DCI_SCHEDULE_BASE; i < m_dwNumSchedules; i++, dwId++)
         pMsg->SetVariable(dwId, m_ppScheduleList[i]);
   }
   unlock();
}


//
// Update data collection object from NXCP message
//

void DCObject::updateFromMessage(CSCPMessage *pMsg)
{
   lock();

   pMsg->GetVariableStr(VID_NAME, m_szName, MAX_ITEM_NAME);
   pMsg->GetVariableStr(VID_DESCRIPTION, m_szDescription, MAX_DB_STRING);
   pMsg->GetVariableStr(VID_SYSTEM_TAG, m_systemTag, MAX_DB_STRING);
	m_flags = pMsg->GetVariableShort(VID_FLAGS);
   m_source = (BYTE)pMsg->GetVariableShort(VID_DCI_SOURCE_TYPE);
   m_iPollingInterval = pMsg->GetVariableLong(VID_POLLING_INTERVAL);
   m_iRetentionTime = pMsg->GetVariableLong(VID_RETENTION_TIME);
   setStatus(pMsg->GetVariableShort(VID_DCI_STATUS), true);
	m_dwResourceId = pMsg->GetVariableLong(VID_RESOURCE_ID);
	m_dwProxyNode = pMsg->GetVariableLong(VID_AGENT_PROXY);
	safe_free(m_pszPerfTabSettings);
	m_pszPerfTabSettings = pMsg->GetVariableStr(VID_PERFTAB_SETTINGS);
	m_snmpPort = pMsg->GetVariableShort(VID_SNMP_PORT);

   // Update schedules
   for(DWORD i = 0; i < m_dwNumSchedules; i++)
      free(m_ppScheduleList[i]);

   if (m_flags & DCF_ADVANCED_SCHEDULE)
   {
      m_dwNumSchedules = pMsg->GetVariableLong(VID_NUM_SCHEDULES);
      m_ppScheduleList = (TCHAR **)realloc(m_ppScheduleList, sizeof(TCHAR *) * m_dwNumSchedules);
      for(DWORD i = 0, dwId = VID_DCI_SCHEDULE_BASE; i < m_dwNumSchedules; i++, dwId++)
      {
         TCHAR *pszStr = pMsg->GetVariableStr(dwId);
         if (pszStr != NULL)
         {
            m_ppScheduleList[i] = pszStr;
         }
         else
         {
            m_ppScheduleList[i] = _tcsdup(_T("(null)"));
         }
      }
   }
   else
   {
      if (m_ppScheduleList != NULL)
      {
         free(m_ppScheduleList);
         m_ppScheduleList = NULL;
      }
      m_dwNumSchedules = 0;
   }

	unlock();
}


//
// Save to database
//

BOOL DCObject::saveToDB(DB_HANDLE hdb)
{
	TCHAR query[1024];

	lock();

   // Save schedules
   _sntprintf(query, 1024, _T("DELETE FROM dci_schedules WHERE item_id=%d"), (int)m_dwId);
   BOOL success = DBQuery(hdb, query);
	if (success && (m_flags & DCF_ADVANCED_SCHEDULE))
   {
      for(DWORD i = 0; i < m_dwNumSchedules; i++)
      {
         _sntprintf(query, 1024, _T("INSERT INTO dci_schedules (item_id,schedule_id,schedule) VALUES (%d,%d,%s)"),
                    m_dwId, i + 1, (const TCHAR *)DBPrepareString(hdb, m_ppScheduleList[i]));
         success = DBQuery(hdb, query);
			if (!success)
				break;
      }
   }

	unlock();

	return success;
}


//
// Delete object and collected data from database
//

void DCObject::deleteFromDB()
{
	TCHAR query[256];
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM dci_schedules WHERE item_id=%d"), (int)m_dwId);
   QueueSQLRequest(query);
}


//
// Get list of used events
//

void DCObject::getEventList(DWORD **ppdwList, DWORD *pdwSize)
{
	*ppdwList = NULL;
	*pdwSize = 0;
}


//
// Create management pack record
//

void DCObject::createNXMPRecord(String &str)
{
}


//
// Load data collection object thresholds from database
//

bool DCObject::loadThresholdsFromDB()
{
	return true;
}


//
// Update DC object from template object
//

void DCObject::updateFromTemplate(DCObject *src)
{
	lock();

   expandMacros(src->m_szName, m_szName, MAX_ITEM_NAME);
   expandMacros(src->m_szDescription, m_szDescription, MAX_DB_STRING);
	expandMacros(src->m_systemTag, m_systemTag, MAX_DB_STRING);

   m_iPollingInterval = src->m_iPollingInterval;
   m_iRetentionTime = src->m_iRetentionTime;
   m_source = src->m_source;
   setStatus(src->m_status, true);
	m_flags = src->m_flags;
	m_dwProxyNode = src->m_dwProxyNode;
	m_dwResourceId = src->m_dwResourceId;
	m_snmpPort = src->m_snmpPort;

	safe_free(m_pszPerfTabSettings);
	m_pszPerfTabSettings = (src->m_pszPerfTabSettings != NULL) ? _tcsdup(src->m_pszPerfTabSettings) : NULL;

   // Copy schedules
	DWORD i;
   for(i = 0; i < m_dwNumSchedules; i++)
      safe_free(m_ppScheduleList[i]);
   safe_free(m_ppScheduleList);
   m_dwNumSchedules = src->m_dwNumSchedules;
   m_ppScheduleList = (TCHAR **)malloc(sizeof(TCHAR *) * m_dwNumSchedules);
   for(i = 0; i < m_dwNumSchedules; i++)
      m_ppScheduleList[i] = _tcsdup(src->m_ppScheduleList[i]);

	unlock();
}


//
// Process new collected value
//

void DCObject::processNewValue(time_t nTimeStamp, void *value)
{
}


//
// Process new data collection error
//

void DCObject::processNewError()
{
}
