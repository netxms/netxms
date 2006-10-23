/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: epp.cpp
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
}


//
// Construct event policy rule from database record
// Assuming the following field order:
// rule_id,flags,comments,alarm_message,alarm_severity,alarm_key,alarm_ack_key,script
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
   DBGetField(hResult, iRow, 6, m_szAlarmAckKey, MAX_DB_STRING);
   DecodeSQLString(m_szAlarmAckKey);
   m_pszScript = DBGetField(hResult, iRow, 7, NULL, 0);
   DecodeSQLString(m_pszScript);
   if ((m_pszScript != NULL) && (*m_pszScript != 0))
   {
      TCHAR szError[256];

      m_pScript = (NXSL_Program *)NXSLCompile(m_pszScript, szError, 256);
      if (m_pScript == NULL)
      {
         WriteLog(MSG_EPRULE_SCRIPT_COMPILATION_ERROR, EVENTLOG_ERROR_TYPE,
                  "ds", m_dwId, szError);
      }
   }
   else
   {
      m_pScript = NULL;
   }
}


//
// Construct event policy rule from CSCP message
//

EPRule::EPRule(CSCPMessage *pMsg)
{
   m_dwFlags = pMsg->GetVariableLong(VID_FLAGS);
   m_dwId = pMsg->GetVariableLong(VID_RULE_ID);
   m_pszComment = pMsg->GetVariableStr(VID_COMMENT);

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
   pMsg->GetVariableStr(VID_ALARM_ACK_KEY, m_szAlarmAckKey, MAX_DB_STRING);
   pMsg->GetVariableStr(VID_ALARM_MESSAGE, m_szAlarmMessage, MAX_DB_STRING);
   m_iAlarmSeverity = pMsg->GetVariableShort(VID_ALARM_SEVERITY);

   m_pszScript = pMsg->GetVariableStr(VID_SCRIPT);
   if ((m_pszScript != NULL) && (*m_pszScript != 0))
   {
      TCHAR szError[256];

      m_pScript = (NXSL_Program *)NXSLCompile(m_pszScript, szError, 256);
      if (m_pScript == NULL)
      {
         WriteLog(MSG_EPRULE_SCRIPT_COMPILATION_ERROR, EVENTLOG_ERROR_TYPE,
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
               WriteLog(MSG_INVALID_EPP_OBJECT, EVENTLOG_ERROR_TYPE, "d", m_pdwSourceList[i]);
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
   NXSL_Environment *pEnv;
   NXSL_Value **ppValueList, *pValue;
   NXSL_VariableSystem *pLocals;
   BOOL bRet = FALSE;
   DWORD i;

   if (m_pScript == NULL)
      return FALSE;

   pEnv = new NXSL_Environment;
   pEnv->SetLibrary(g_pScriptLibrary);

   // Pass event's parameters as arguments and
   // other information as variables
   ppValueList = (NXSL_Value **)malloc(sizeof(NXSL_Value *) * pEvent->GetParametersCount());
   memset(ppValueList, 0, sizeof(NXSL_Value *) * pEvent->GetParametersCount());
   for(i = 0; i < pEvent->GetParametersCount(); i++)
      ppValueList[i] = new NXSL_Value(pEvent->GetParameter(i));

   pLocals = new NXSL_VariableSystem;
   pLocals->Create(_T("EVENT_CODE"), new NXSL_Value(pEvent->Code()));
   pLocals->Create(_T("SEVERITY"), new NXSL_Value(pEvent->Severity()));
   pLocals->Create(_T("SEVERITY_TEXT"), new NXSL_Value(g_szStatusText[pEvent->Severity()]));
   pLocals->Create(_T("OBJECT_ID"), new NXSL_Value(pEvent->SourceId()));
   pLocals->Create(_T("EVENT_TEXT"), new NXSL_Value((TCHAR *)pEvent->Message()));

   // Run script
   if (m_pScript->Run(pEnv, pEvent->GetParametersCount(), ppValueList, pLocals) == 0)
   {
      pValue = m_pScript->GetResult();
      if (pValue != NULL)
         bRet = pValue->GetValueAsInt32() ? TRUE : FALSE;
   }
   else
   {
      WriteLog(MSG_EPRULE_SCRIPT_EXECUTION_ERROR, EVENTLOG_ERROR_TYPE,
               "ds", m_dwId, m_pScript->GetErrorText());
   }
   free(ppValueList);

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
   TCHAR *pszAlarmMsg;

   // Check disable flag
   if (!(m_dwFlags & RF_DISABLED))
   {
      // Check if event match
      if (MatchSource(pEvent->SourceId()) && MatchEvent(pEvent->Code()) &&
          MatchSeverity(pEvent->Severity()) && MatchScript(pEvent))
      {
         // Generate alarm if requested
         if (m_dwFlags & RF_GENERATE_ALARM)
            GenerateAlarm(pEvent);

         // Event matched, perform actions
         if (m_dwNumActions > 0)
         {
            pszAlarmMsg = pEvent->ExpandText(m_szAlarmMessage);
            for(i = 0; i < m_dwNumActions; i++)
               ExecuteAction(m_pdwActionList[i], pEvent, pszAlarmMsg);
            free(pszAlarmMsg);
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
   char *pszAckKey;
   int iSeverity;

   // Terminate alarms with key == our ack_key
   pszAckKey = pEvent->ExpandText(m_szAlarmAckKey);
   if (pszAckKey[0] != 0)
      g_alarmMgr.TerminateByKey(pszAckKey);
   free(pszAckKey);

   // Generate new alarm
   switch(m_iAlarmSeverity)
   {
      case SEVERITY_FROM_EVENT:
         iSeverity = pEvent->Severity();
         break;
      case SEVERITY_NONE:
         iSeverity = SEVERITY_NORMAL;
         break;
      default:
         iSeverity = m_iAlarmSeverity;
         break;
   }
   g_alarmMgr.NewAlarm(m_szAlarmMessage, m_szAlarmKey, 
                       (m_iAlarmSeverity == SEVERITY_NONE) ? ALARM_STATE_TERMINATED : ALARM_STATE_OUTSTANDING,
                       iSeverity, pEvent);
}


//
// Load rule from database
//

BOOL EPRule::LoadFromDB(void)
{
   DB_RESULT hResult;
   char szQuery[256];
   BOOL bSuccess = TRUE;
   DWORD i;
   
   // Load rule's sources
   sprintf(szQuery, "SELECT object_id FROM policy_source_list WHERE rule_id=%d", m_dwId);
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
   sprintf(szQuery, "SELECT event_code FROM policy_event_list WHERE rule_id=%d", m_dwId);
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
   sprintf(szQuery, "SELECT action_id FROM policy_action_list WHERE rule_id=%d", m_dwId);
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

   return bSuccess;
}


//
// Save rule to database
//

void EPRule::SaveToDB(void)
{
   TCHAR *pszComment, *pszEscKey, *pszEscAck, *pszEscMessage, *pszEscScript, *pszQuery;
   DWORD i;

   pszQuery = (TCHAR *)malloc((_tcslen(CHECK_NULL(m_pszComment)) +
                               _tcslen(CHECK_NULL(m_pszScript)) + 4096) * sizeof(TCHAR));

   // General attributes
   pszComment = EncodeSQLString(m_pszComment);
   pszEscKey = EncodeSQLString(m_szAlarmKey);
   pszEscAck = EncodeSQLString(m_szAlarmAckKey);
   pszEscMessage = EncodeSQLString(m_szAlarmMessage);
   pszEscScript = EncodeSQLString(m_pszScript);
   _stprintf(pszQuery, _T("INSERT INTO event_policy (rule_id,flags,comments,alarm_message,")
                       _T("alarm_severity,alarm_key,alarm_ack_key,script) ")
                       _T("VALUES (%d,%d,'%s','%s',%d,'%s','%s','%s')"),
             m_dwId, m_dwFlags, pszComment, pszEscMessage, m_iAlarmSeverity,
             pszEscKey, pszEscAck, pszEscScript);
   free(pszComment);
   free(pszEscMessage);
   free(pszEscKey);
   free(pszEscAck);
   free(pszEscScript);
   DBQuery(g_hCoreDB, pszQuery);

   // Actions
   for(i = 0; i < m_dwNumActions; i++)
   {
      _stprintf(pszQuery, _T("INSERT INTO policy_action_list (rule_id,action_id) VALUES (%d,%d)"),
                m_dwId, m_pdwActionList[i]);
      DBQuery(g_hCoreDB, pszQuery);
   }

   // Events
   for(i = 0; i < m_dwNumEvents; i++)
   {
      _stprintf(pszQuery, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"),
                m_dwId, m_pdwEventList[i]);
      DBQuery(g_hCoreDB, pszQuery);
   }

   // Sources
   for(i = 0; i < m_dwNumSources; i++)
   {
      _stprintf(pszQuery, _T("INSERT INTO policy_source_list (rule_id,object_id) VALUES (%d,%d)"),
                m_dwId, m_pdwSourceList[i]);
      DBQuery(g_hCoreDB, pszQuery);
   }

   free(pszQuery);
}


//
// Create CSCP message with rule's data
//

void EPRule::CreateMessage(CSCPMessage *pMsg)
{
   pMsg->SetVariable(VID_FLAGS, m_dwFlags);
   pMsg->SetVariable(VID_RULE_ID, m_dwId);
   pMsg->SetVariable(VID_ALARM_SEVERITY, (WORD)m_iAlarmSeverity);
   pMsg->SetVariable(VID_ALARM_KEY, m_szAlarmKey);
   pMsg->SetVariable(VID_ALARM_ACK_KEY, m_szAlarmAckKey);
   pMsg->SetVariable(VID_ALARM_MESSAGE, m_szAlarmMessage);
   pMsg->SetVariable(VID_COMMENT, CHECK_NULL_EX(m_pszComment));
   pMsg->SetVariable(VID_NUM_ACTIONS, m_dwNumActions);
   pMsg->SetVariableToInt32Array(VID_RULE_ACTIONS, m_dwNumActions, m_pdwActionList);
   pMsg->SetVariable(VID_NUM_EVENTS, m_dwNumEvents);
   pMsg->SetVariableToInt32Array(VID_RULE_EVENTS, m_dwNumEvents, m_pdwEventList);
   pMsg->SetVariable(VID_NUM_SOURCES, m_dwNumSources);
   pMsg->SetVariableToInt32Array(VID_RULE_SOURCES, m_dwNumSources, m_pdwSourceList);
   pMsg->SetVariable(VID_SCRIPT, CHECK_NULL_EX(m_pszScript));
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

void EventPolicy::Clear(void)
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

BOOL EventPolicy::LoadFromDB(void)
{
   DB_RESULT hResult;
   BOOL bSuccess = FALSE;

   hResult = DBSelect(g_hCoreDB, _T("SELECT rule_id,flags,comments,alarm_message,")
                                 _T("alarm_severity,alarm_key,alarm_ack_key,script ")
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

void EventPolicy::SaveToDB(void)
{
   DWORD i;

   ReadLock();
   DBBegin(g_hCoreDB);
   DBQuery(g_hCoreDB, _T("DELETE FROM event_policy"));
   DBQuery(g_hCoreDB, _T("DELETE FROM policy_action_list"));
   DBQuery(g_hCoreDB, _T("DELETE FROM policy_event_list"));
   DBQuery(g_hCoreDB, _T("DELETE FROM policy_source_list"));
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

   ReadLock();
   for(i = 0; i < m_dwNumRules; i++)
      if (m_ppRuleList[i]->ProcessEvent(pEvent))
         break;   // EPRule::ProcessEvent() return TRUE if we should stop processing this event
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
      pSession->SendMessage(&msg);
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
