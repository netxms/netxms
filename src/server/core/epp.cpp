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
** File: epp.cpp
**
**/

#include "nxcore.h"


//
// Default event policy rule constructor
//

EPRule::EPRule(DWORD dwId)
{
   m_dwId = dwId;
   m_dwFlags = 0;
   m_dwNumSources = 0;
   m_pdwSourceList = NULL;
   m_dwNumEvents = 0;
   m_pdwEventList = NULL;
   m_dwNumActions = 0;
   m_pdwActionList = NULL;
   m_pszComment = NULL;
   m_iAlarmSeverity = 0;
   m_pszScript = NULL;
   m_pScript = NULL;
	m_dwAlarmTimeout = 0;
	m_dwAlarmTimeoutEvent = EVENT_ALARM_TIMEOUT;
	m_dwSituationId = 0;
	m_szSituationInstance[0] = 0;
}


//
// Construct event policy rule from database record
// Assuming the following field order:
// rule_id,flags,comments,alarm_message,alarm_severity,alarm_key,script,
// alarm_timeout,alarm_timeout_event,situation_id,situation_instance
//

EPRule::EPRule(DB_RESULT hResult, int iRow)
{
   m_dwId = DBGetFieldULong(hResult, iRow, 0);
   m_dwFlags = DBGetFieldULong(hResult, iRow, 1);
   m_pszComment = DBGetField(hResult, iRow, 2, NULL, 0);
   DecodeSQLString(m_pszComment);
   DBGetField(hResult, iRow, 3, m_szAlarmMessage, MAX_DB_STRING);
   DecodeSQLString(m_szAlarmMessage);
   m_iAlarmSeverity = DBGetFieldLong(hResult, iRow, 4);
   DBGetField(hResult, iRow, 5, m_szAlarmKey, MAX_DB_STRING);
   DecodeSQLString(m_szAlarmKey);
   m_pszScript = DBGetField(hResult, iRow, 6, NULL, 0);
   DecodeSQLString(m_pszScript);
   if ((m_pszScript != NULL) && (*m_pszScript != 0))
   {
      TCHAR szError[256];

      m_pScript = (NXSL_Program *)NXSLCompile(m_pszScript, szError, 256);
      if (m_pScript != NULL)
      {
      	m_pScript->setGlobalVariable(_T("CUSTOM_MESSAGE"), new NXSL_Value(_T("")));
      }
      else
      {
         nxlog_write(MSG_EPRULE_SCRIPT_COMPILATION_ERROR, EVENTLOG_ERROR_TYPE,
                  "ds", m_dwId, szError);
      }
   }
   else
   {
      m_pScript = NULL;
   }
	m_dwAlarmTimeout = DBGetFieldULong(hResult, iRow, 7);
	m_dwAlarmTimeoutEvent = DBGetFieldULong(hResult, iRow, 8);
	m_dwSituationId = DBGetFieldULong(hResult, iRow, 9);
   DBGetField(hResult, iRow, 10, m_szSituationInstance, MAX_DB_STRING);
	DecodeSQLString(m_szSituationInstance);
}


//
// Construct event policy rule from CSCP message
//

EPRule::EPRule(CSCPMessage *pMsg)
{
	DWORD i, id, count;
	TCHAR *name, *value;
	
   m_dwFlags = pMsg->GetVariableLong(VID_FLAGS);
   m_dwId = pMsg->GetVariableLong(VID_RULE_ID);
   m_pszComment = pMsg->GetVariableStr(VID_COMMENTS);

   m_dwNumActions = pMsg->GetVariableLong(VID_NUM_ACTIONS);
   m_pdwActionList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumActions);
   pMsg->GetVariableInt32Array(VID_RULE_ACTIONS, m_dwNumActions, m_pdwActionList);

   m_dwNumEvents = pMsg->GetVariableLong(VID_NUM_EVENTS);
   m_pdwEventList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumEvents);
   pMsg->GetVariableInt32Array(VID_RULE_EVENTS, m_dwNumEvents, m_pdwEventList);

   m_dwNumSources = pMsg->GetVariableLong(VID_NUM_SOURCES);
   m_pdwSourceList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumSources);
   pMsg->GetVariableInt32Array(VID_RULE_SOURCES, m_dwNumSources, m_pdwSourceList);

   pMsg->GetVariableStr(VID_ALARM_KEY, m_szAlarmKey, MAX_DB_STRING);
   pMsg->GetVariableStr(VID_ALARM_MESSAGE, m_szAlarmMessage, MAX_DB_STRING);
   m_iAlarmSeverity = pMsg->GetVariableShort(VID_ALARM_SEVERITY);
	m_dwAlarmTimeout = pMsg->GetVariableLong(VID_ALARM_TIMEOUT);
	m_dwAlarmTimeoutEvent = pMsg->GetVariableLong(VID_ALARM_TIMEOUT_EVENT);

	m_dwSituationId = pMsg->GetVariableLong(VID_SITUATION_ID);
   pMsg->GetVariableStr(VID_SITUATION_INSTANCE, m_szSituationInstance, MAX_DB_STRING);
   count = pMsg->GetVariableLong(VID_SITUATION_NUM_ATTRS);
   for(i = 0, id = VID_SITUATION_ATTR_LIST_BASE; i < count; i++)
   {
   	name = pMsg->GetVariableStr(id++);
   	value = pMsg->GetVariableStr(id++);
   	m_situationAttrList.setPreallocated(name, value);
   }

   m_pszScript = pMsg->GetVariableStr(VID_SCRIPT);
   if ((m_pszScript != NULL) && (*m_pszScript != 0))
   {
      TCHAR szError[256];

      m_pScript = (NXSL_Program *)NXSLCompile(m_pszScript, szError, 256);
      if (m_pScript != NULL)
      {
      	m_pScript->setGlobalVariable(_T("CUSTOM_MESSAGE"), new NXSL_Value(_T("")));
      }
      else
      {
         nxlog_write(MSG_EPRULE_SCRIPT_COMPILATION_ERROR, EVENTLOG_ERROR_TYPE,
                  "ds", m_dwId, szError);
      }
   }
   else
   {
      m_pScript = NULL;
   }
}


//
// Event policy rule destructor
//

EPRule::~EPRule()
{
   safe_free(m_pdwSourceList);
   safe_free(m_pdwEventList);
   safe_free(m_pdwActionList);
   safe_free(m_pszComment);
   safe_free(m_pszScript);
   delete m_pScript;
}


//
// Check if source object's id match to the rule
//

BOOL EPRule::MatchSource(DWORD dwObjectId)
{
   DWORD i;
   NetObj *pObject;
   BOOL bMatch = FALSE;

   if (m_dwNumSources == 0)   // No sources in list means "any"
   {
      bMatch = TRUE;
   }
   else
   {
      for(i = 0; i < m_dwNumSources; i++)
         if (m_pdwSourceList[i] == dwObjectId)
         {
            bMatch = TRUE;
            break;
         }
         else
         {
            pObject = FindObjectById(m_pdwSourceList[i]);
            if (pObject != NULL)
            {
               if (pObject->IsChild(dwObjectId))
               {
                  bMatch = TRUE;
                  break;
               }
            }
            else
            {
               nxlog_write(MSG_INVALID_EPP_OBJECT, EVENTLOG_ERROR_TYPE, "d", m_pdwSourceList[i]);
            }
         }
   }
   return (m_dwFlags & RF_NEGATED_SOURCE) ? !bMatch : bMatch;
}


//
// Check if event's id match to the rule
//

BOOL EPRule::MatchEvent(DWORD dwEventCode)
{
   DWORD i;
   BOOL bMatch = FALSE;

   if (m_dwNumEvents == 0)   // No sources in list means "any"
   {
      bMatch = TRUE;
   }
   else
   {
      for(i = 0; i < m_dwNumEvents; i++)
         if (m_pdwEventList[i] & GROUP_FLAG_BIT)
         {
            /* TODO: check group membership */
         }
         else
         {
            if (m_pdwEventList[i] == dwEventCode)
            {
               bMatch = TRUE;
               break;
            }
         }
   }
   return (m_dwFlags & RF_NEGATED_EVENTS) ? !bMatch : bMatch;
}


//
// Check if event's severity match to the rule
//

BOOL EPRule::MatchSeverity(DWORD dwSeverity)
{
   static DWORD dwSeverityFlag[] = { RF_SEVERITY_INFO, RF_SEVERITY_WARNING,
                                     RF_SEVERITY_MINOR, RF_SEVERITY_MAJOR,
                                     RF_SEVERITY_CRITICAL };
   return dwSeverityFlag[dwSeverity] & m_dwFlags;
}


//
// Check if event match to the script
//

BOOL EPRule::MatchScript(Event *pEvent)
{
   NXSL_ServerEnv *pEnv;
   NXSL_Value **ppValueList, *pValue;
   NXSL_VariableSystem *pLocals, *pGlobals = NULL;
   BOOL bRet = TRUE;
   DWORD i;
	NetObj *pObject;

   if (m_pScript == NULL)
      return TRUE;

   pEnv = new NXSL_ServerEnv;

   // Pass event's parameters as arguments and
   // other information as variables
   ppValueList = (NXSL_Value **)malloc(sizeof(NXSL_Value *) * pEvent->getParametersCount());
   memset(ppValueList, 0, sizeof(NXSL_Value *) * pEvent->getParametersCount());
   for(i = 0; i < pEvent->getParametersCount(); i++)
      ppValueList[i] = new NXSL_Value(pEvent->getParameter(i));

   pLocals = new NXSL_VariableSystem;
   pLocals->create(_T("EVENT_CODE"), new NXSL_Value(pEvent->getCode()));
   pLocals->create(_T("SEVERITY"), new NXSL_Value(pEvent->getSeverity()));
   pLocals->create(_T("SEVERITY_TEXT"), new NXSL_Value(g_szStatusText[pEvent->getSeverity()]));
   pLocals->create(_T("OBJECT_ID"), new NXSL_Value(pEvent->getSourceId()));
   pLocals->create(_T("EVENT_TEXT"), new NXSL_Value((TCHAR *)pEvent->getMessage()));
   pLocals->create(_T("USER_TAG"), new NXSL_Value((TCHAR *)pEvent->getUserTag()));
	pObject = FindObjectById(pEvent->getSourceId());
	if (pObject != NULL)
	{
		if (pObject->Type() == OBJECT_NODE)
			m_pScript->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, pObject)));
	}
	m_pScript->setGlobalVariable(_T("$event"), new NXSL_Value(new NXSL_Object(&g_nxslEventClass, pEvent)));
	m_pScript->setGlobalVariable(_T("CUSTOM_MESSAGE"), new NXSL_Value);

   // Run script
   if (m_pScript->run(pEnv, pEvent->getParametersCount(), ppValueList, pLocals, &pGlobals) == 0)
   {
      pValue = m_pScript->getResult();
      if (pValue != NULL)
      {
         bRet = pValue->getValueAsInt32() ? TRUE : FALSE;
         if (bRet)
         {
         	NXSL_Variable *var;
         	
         	var = pGlobals->find(_T("CUSTOM_MESSAGE"));
         	if (var != NULL)
         	{
         		// Update custom message in event
         		pEvent->setCustomMessage(CHECK_NULL_EX(var->getValue()->getValueAsCString()));
         	}
         }
      }
   }
   else
   {
      nxlog_write(MSG_EPRULE_SCRIPT_EXECUTION_ERROR, EVENTLOG_ERROR_TYPE, "ds", m_dwId, m_pScript->getErrorText());
   }
   free(ppValueList);
   delete pGlobals;

   return bRet;
}


//
// Check if event match to rule and perform required actions if yes
// Method will return TRUE if event matched and RF_STOP_PROCESSING flag is set
//

BOOL EPRule::ProcessEvent(Event *pEvent)
{
   BOOL bStopProcessing = FALSE;
   DWORD i;
   TCHAR *pszText;

   // Check disable flag
   if (!(m_dwFlags & RF_DISABLED))
   {
      // Check if event match
      if (MatchSource(pEvent->getSourceId()) && MatchEvent(pEvent->getCode()) &&
          MatchSeverity(pEvent->getSeverity()) && MatchScript(pEvent))
      {
			DbgPrintf(6, _T("Event ") UINT64_FMT _T(" match EPP rule %d"), pEvent->getId(), (int)m_dwId);

         // Generate alarm if requested
         if (m_dwFlags & RF_GENERATE_ALARM)
            GenerateAlarm(pEvent);

         // Event matched, perform actions
         if (m_dwNumActions > 0)
         {
            pszText = pEvent->expandText(m_szAlarmMessage);
            for(i = 0; i < m_dwNumActions; i++)
               ExecuteAction(m_pdwActionList[i], pEvent, pszText);
            free(pszText);
         }

			// Update situation of needed
			if (m_dwSituationId != 0)
			{
				Situation *pSituation;
				TCHAR *pszAttr, *pszValue;

				pSituation = FindSituationById(m_dwSituationId);
				if (pSituation != NULL)
				{
					pszText = pEvent->expandText(m_szSituationInstance);
					for(i = 0; i < m_situationAttrList.getSize(); i++)
					{
						pszAttr = pEvent->expandText(m_situationAttrList.getKeyByIndex(i));
						pszValue = pEvent->expandText(m_situationAttrList.getValueByIndex(i));
						pSituation->UpdateSituation(pszText, pszAttr, pszValue);
						free(pszAttr);
						free(pszValue);
					}
					free(pszText);
				}
				else
				{
					DbgPrintf(3, _T("Event Policy: unable to find situation with ID=%d"), m_dwSituationId);
				}
			}

         bStopProcessing = m_dwFlags & RF_STOP_PROCESSING;
      }
   }

   return bStopProcessing;
}


//
// Generate alarm from event
//

void EPRule::GenerateAlarm(Event *pEvent)
{
   // Terminate alarms with key == our ack_key
	if ((m_iAlarmSeverity == SEVERITY_RESOLVE) || (m_iAlarmSeverity == SEVERITY_TERMINATE))
	{
		TCHAR *pszAckKey = pEvent->expandText(m_szAlarmKey);
		if (pszAckKey[0] != 0)
			g_alarmMgr.resolveByKey(pszAckKey, (m_dwFlags & RF_TERMINATE_BY_REGEXP) ? true : false, m_iAlarmSeverity == SEVERITY_TERMINATE);
		free(pszAckKey);
	}
	else	// Generate new alarm
	{
		g_alarmMgr.newAlarm(m_szAlarmMessage, m_szAlarmKey, ALARM_STATE_OUTSTANDING,
								  (m_iAlarmSeverity == SEVERITY_FROM_EVENT) ? pEvent->getSeverity() : m_iAlarmSeverity,
								  m_dwAlarmTimeout, m_dwAlarmTimeoutEvent, pEvent);
	}
}


//
// Load rule from database
//

BOOL EPRule::LoadFromDB()
{
   DB_RESULT hResult;
   TCHAR szQuery[256], name[MAX_DB_STRING], value[MAX_DB_STRING];
   BOOL bSuccess = TRUE;
   DWORD i, count;
   
   // Load rule's sources
   _sntprintf(szQuery, 256, _T("SELECT object_id FROM policy_source_list WHERE rule_id=%d"), m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      m_dwNumSources = DBGetNumRows(hResult);
      m_pdwSourceList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumSources);
      for(i = 0; i < m_dwNumSources; i++)
         m_pdwSourceList[i] = DBGetFieldULong(hResult, i, 0);
      DBFreeResult(hResult);
   }
   else
   {
      bSuccess = FALSE;
   }

   // Load rule's events
   _sntprintf(szQuery, 256, _T("SELECT event_code FROM policy_event_list WHERE rule_id=%d"), m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      m_dwNumEvents = DBGetNumRows(hResult);
      m_pdwEventList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumEvents);
      for(i = 0; i < m_dwNumEvents; i++)
         m_pdwEventList[i] = DBGetFieldULong(hResult, i, 0);
      DBFreeResult(hResult);
   }
   else
   {
      bSuccess = FALSE;
   }

   // Load rule's actions
   _sntprintf(szQuery, 256, _T("SELECT action_id FROM policy_action_list WHERE rule_id=%d"), m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      m_dwNumActions = DBGetNumRows(hResult);
      m_pdwActionList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumActions);
      for(i = 0; i < m_dwNumActions; i++)
         m_pdwActionList[i] = DBGetFieldULong(hResult, i, 0);
      DBFreeResult(hResult);
   }
   else
   {
      bSuccess = FALSE;
   }
   
   // Load situation attributes
   _sntprintf(szQuery, 256, _T("SELECT attr_name,attr_value FROM policy_situation_attr_list WHERE rule_id=%d"), m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      count = DBGetNumRows(hResult);
      for(i = 0; i < count; i++)
      {
         DBGetField(hResult, i, 0, name, MAX_DB_STRING);
         DecodeSQLString(name);
         DBGetField(hResult, i, 1, value, MAX_DB_STRING);
         DecodeSQLString(value);
         m_situationAttrList.set(name, value);
      }
      DBFreeResult(hResult);
   }
   else
   {
      bSuccess = FALSE;
   }

   return bSuccess;
}


//
// Save rule to database
//

void EPRule::SaveToDB()
{
   TCHAR *pszComment, *pszEscKey, *pszEscMessage,
	      *pszEscScript, *pszQuery, *pszEscSituationInstance,
			*pszEscName, *pszEscValue;
   DWORD i, len;

	len = (DWORD)(_tcslen(CHECK_NULL(m_pszComment)) + _tcslen(CHECK_NULL(m_pszScript)) + 4096);
   pszQuery = (TCHAR *)malloc(len * sizeof(TCHAR));

   // General attributes
   pszComment = EncodeSQLString(m_pszComment);
   pszEscKey = EncodeSQLString(m_szAlarmKey);
   pszEscMessage = EncodeSQLString(m_szAlarmMessage);
   pszEscScript = EncodeSQLString(m_pszScript);
	pszEscSituationInstance = EncodeSQLString(m_szSituationInstance);
   _sntprintf(pszQuery, len, _T("INSERT INTO event_policy (rule_id,flags,comments,alarm_message,")
                       _T("alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event,")
							  _T("situation_id,situation_instance) ")
                       _T("VALUES (%d,%d,'%s','%s',%d,'%s','%s',%d,%d,%d,'%s')"),
             m_dwId, m_dwFlags, pszComment, pszEscMessage, m_iAlarmSeverity,
             pszEscKey, pszEscScript, m_dwAlarmTimeout, m_dwAlarmTimeoutEvent,
				 m_dwSituationId, pszEscSituationInstance);
   free(pszComment);
   free(pszEscMessage);
   free(pszEscKey);
   free(pszEscScript);
	free(pszEscSituationInstance);
   DBQuery(g_hCoreDB, pszQuery);

   // Actions
   for(i = 0; i < m_dwNumActions; i++)
   {
      _sntprintf(pszQuery, len, _T("INSERT INTO policy_action_list (rule_id,action_id) VALUES (%d,%d)"),
                m_dwId, m_pdwActionList[i]);
      DBQuery(g_hCoreDB, pszQuery);
   }

   // Events
   for(i = 0; i < m_dwNumEvents; i++)
   {
      _sntprintf(pszQuery, len, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"),
                m_dwId, m_pdwEventList[i]);
      DBQuery(g_hCoreDB, pszQuery);
   }

   // Sources
   for(i = 0; i < m_dwNumSources; i++)
   {
      _sntprintf(pszQuery, len, _T("INSERT INTO policy_source_list (rule_id,object_id) VALUES (%d,%d)"),
                m_dwId, m_pdwSourceList[i]);
      DBQuery(g_hCoreDB, pszQuery);
   }

	// Situation attributes
	for(i = 0; i < m_situationAttrList.getSize(); i++)
	{
		pszEscName = EncodeSQLString(m_situationAttrList.getKeyByIndex(i));
		pszEscValue = EncodeSQLString(m_situationAttrList.getValueByIndex(i));
      _sntprintf(pszQuery, len, _T("INSERT INTO policy_situation_attr_list (rule_id,situation_id,attr_name,attr_value) VALUES (%d,%d,'%s','%s')"),
                m_dwId, m_dwSituationId, pszEscName, pszEscValue);
		free(pszEscName);
		free(pszEscValue);
      DBQuery(g_hCoreDB, pszQuery);
	}

   free(pszQuery);
}


//
// Create CSCP message with rule's data
//

void EPRule::CreateMessage(CSCPMessage *pMsg)
{
	DWORD i, id;

   pMsg->SetVariable(VID_FLAGS, m_dwFlags);
   pMsg->SetVariable(VID_RULE_ID, m_dwId);
   pMsg->SetVariable(VID_ALARM_SEVERITY, (WORD)m_iAlarmSeverity);
   pMsg->SetVariable(VID_ALARM_KEY, m_szAlarmKey);
   pMsg->SetVariable(VID_ALARM_MESSAGE, m_szAlarmMessage);
   pMsg->SetVariable(VID_ALARM_TIMEOUT, m_dwAlarmTimeout);
   pMsg->SetVariable(VID_ALARM_TIMEOUT_EVENT, m_dwAlarmTimeoutEvent);
   pMsg->SetVariable(VID_COMMENTS, CHECK_NULL_EX(m_pszComment));
   pMsg->SetVariable(VID_NUM_ACTIONS, m_dwNumActions);
   pMsg->SetVariableToInt32Array(VID_RULE_ACTIONS, m_dwNumActions, m_pdwActionList);
   pMsg->SetVariable(VID_NUM_EVENTS, m_dwNumEvents);
   pMsg->SetVariableToInt32Array(VID_RULE_EVENTS, m_dwNumEvents, m_pdwEventList);
   pMsg->SetVariable(VID_NUM_SOURCES, m_dwNumSources);
   pMsg->SetVariableToInt32Array(VID_RULE_SOURCES, m_dwNumSources, m_pdwSourceList);
   pMsg->SetVariable(VID_SCRIPT, CHECK_NULL_EX(m_pszScript));
	pMsg->SetVariable(VID_SITUATION_ID, m_dwSituationId);
	pMsg->SetVariable(VID_SITUATION_INSTANCE, m_szSituationInstance);
	pMsg->SetVariable(VID_SITUATION_NUM_ATTRS, m_situationAttrList.getSize());
	for(i = 0, id = VID_SITUATION_ATTR_LIST_BASE; i < m_situationAttrList.getSize(); i++)
	{
		pMsg->SetVariable(id++, m_situationAttrList.getKeyByIndex(i));
		pMsg->SetVariable(id++, m_situationAttrList.getValueByIndex(i));
	}
}


//
// Check if given action is used within rule
//

BOOL EPRule::ActionInUse(DWORD dwActionId)
{
   DWORD i;

   for(i = 0; i < m_dwNumActions; i++)
      if (m_pdwActionList[i] == dwActionId)
         return TRUE;
   return FALSE;
}


//
// Event processing policy constructor
//

EventPolicy::EventPolicy()
{
   m_dwNumRules = 0;
   m_ppRuleList = NULL;
   m_rwlock = RWLockCreate();
}


//
// Event processing policy destructor
//

EventPolicy::~EventPolicy()
{
   Clear();
   RWLockDestroy(m_rwlock);
}


//
// Clear existing policy
//

void EventPolicy::Clear()
{
   DWORD i;

   for(i = 0; i < m_dwNumRules; i++)
      delete m_ppRuleList[i];
   safe_free(m_ppRuleList);
   m_ppRuleList = NULL;
}


//
// Load event processing policy from database
//

BOOL EventPolicy::LoadFromDB()
{
   DB_RESULT hResult;
   BOOL bSuccess = FALSE;

   hResult = DBSelect(g_hCoreDB, _T("SELECT rule_id,flags,comments,alarm_message,")
                                 _T("alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event,")
											_T("situation_id,situation_instance ")
                                 _T("FROM event_policy ORDER BY rule_id"));
   if (hResult != NULL)
   {
      DWORD i;

      bSuccess = TRUE;
      m_dwNumRules = DBGetNumRows(hResult);
      m_ppRuleList = (EPRule **)malloc(sizeof(EPRule *) * m_dwNumRules);
      for(i = 0; i < m_dwNumRules; i++)
      {
         m_ppRuleList[i] = new EPRule(hResult, i);
         bSuccess = bSuccess && m_ppRuleList[i]->LoadFromDB();
      }
      DBFreeResult(hResult);
   }

   return bSuccess;
}


//
// Save event processing policy to database
//

void EventPolicy::SaveToDB()
{
   DWORD i;

   ReadLock();
   DBBegin(g_hCoreDB);
   DBQuery(g_hCoreDB, _T("DELETE FROM event_policy"));
   DBQuery(g_hCoreDB, _T("DELETE FROM policy_action_list"));
   DBQuery(g_hCoreDB, _T("DELETE FROM policy_event_list"));
   DBQuery(g_hCoreDB, _T("DELETE FROM policy_source_list"));
   DBQuery(g_hCoreDB, _T("DELETE FROM policy_situation_attr_list"));
   for(i = 0; i < m_dwNumRules; i++)
      m_ppRuleList[i]->SaveToDB();
   DBCommit(g_hCoreDB);
   Unlock();
}


//
// Pass event through policy
//

void EventPolicy::ProcessEvent(Event *pEvent)
{
   DWORD i;

	DbgPrintf(7, _T("EPP: processing event ") UINT64_FMT, pEvent->getId());
   ReadLock();
   for(i = 0; i < m_dwNumRules; i++)
      if (m_ppRuleList[i]->ProcessEvent(pEvent))
		{
			DbgPrintf(7, _T("EPP: got \"stop processing\" flag for event ") UINT64_FMT _T(" at rule %d"), pEvent->getId(), i + 1);
         break;   // EPRule::ProcessEvent() return TRUE if we should stop processing this event
		}
   Unlock();
}


//
// Send event policy to client
//

void EventPolicy::SendToClient(ClientSession *pSession, DWORD dwRqId)
{
   DWORD i;
   CSCPMessage msg;

   ReadLock();
   msg.SetCode(CMD_EPP_RECORD);
   msg.SetId(dwRqId);
   for(i = 0; i < m_dwNumRules; i++)
   {
      m_ppRuleList[i]->CreateMessage(&msg);
      pSession->sendMessage(&msg);
      msg.DeleteAllVariables();
   }
   Unlock();
}


//
// Replace policy with new one
//

void EventPolicy::ReplacePolicy(DWORD dwNumRules, EPRule **ppRuleList)
{
   DWORD i;

   WriteLock();

   // Replace rule list
   Clear();
   m_dwNumRules = dwNumRules;
   m_ppRuleList = ppRuleList;

   // Replace id's in rules
   for(i = 0; i < m_dwNumRules; i++)
      m_ppRuleList[i]->SetId(i);
   
   Unlock();
}


//
// Check if given action is used in policy
//

BOOL EventPolicy::ActionInUse(DWORD dwActionId)
{
   BOOL bResult = FALSE;
   DWORD i;

   ReadLock();

   for(i = 0; i < m_dwNumRules; i++)
      if (m_ppRuleList[i]->ActionInUse(dwActionId))
      {
         bResult = TRUE;
         break;
      }

   Unlock();
   return bResult;
}
