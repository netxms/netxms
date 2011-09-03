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
** File: actions.cpp
**
**/

#include "nxcore.h"


//
// Static data
//

static DWORD m_dwNumActions = 0;
static NXC_ACTION *m_pActionList = NULL;
static RWLOCK m_rwlockActionListAccess;
static DWORD m_dwUpdateCode;


//
// Send updates to all connected clients
//

static void SendActionDBUpdate(ClientSession *pSession, void *pArg)
{
   pSession->onActionDBUpdate(m_dwUpdateCode, (NXC_ACTION *)pArg);
}


//
// Destroy action list
//

static void DestroyActionList()
{
   DWORD i;

   RWLockWriteLock(m_rwlockActionListAccess, INFINITE);
   if (m_pActionList != NULL)
   {
      for(i = 0; i < m_dwNumActions; i++)
         safe_free(m_pActionList[i].pszData);
      free(m_pActionList);
      m_pActionList = NULL;
      m_dwNumActions = 0;
   }
   RWLockUnlock(m_rwlockActionListAccess);
}


//
// Load actions list from database
//

static BOOL LoadActions()
{
   DB_RESULT hResult;
   BOOL bResult = FALSE;
   DWORD i;

   hResult = DBSelect(g_hCoreDB, _T("SELECT action_id,action_name,action_type,")
                                 _T("is_disabled,rcpt_addr,email_subject,action_data ")
                                 _T("FROM actions ORDER BY action_id"));
   if (hResult != NULL)
   {
      DestroyActionList();
      m_dwNumActions = (DWORD)DBGetNumRows(hResult);
      m_pActionList = (NXC_ACTION *)malloc(sizeof(NXC_ACTION) * m_dwNumActions);
      memset(m_pActionList, 0, sizeof(NXC_ACTION) * m_dwNumActions);
      for(i = 0; i < m_dwNumActions; i++)
      {
         m_pActionList[i].dwId = DBGetFieldULong(hResult, i, 0);
         DBGetField(hResult, i, 1, m_pActionList[i].szName, MAX_OBJECT_NAME);
         m_pActionList[i].iType = DBGetFieldLong(hResult, i, 2);
         m_pActionList[i].bIsDisabled = DBGetFieldLong(hResult, i, 3);

         DBGetField(hResult, i, 4, m_pActionList[i].szRcptAddr, MAX_RCPT_ADDR_LEN);
         DecodeSQLString(m_pActionList[i].szRcptAddr);

         DBGetField(hResult, i, 5, m_pActionList[i].szEmailSubject, MAX_EMAIL_SUBJECT_LEN);
         DecodeSQLString(m_pActionList[i].szEmailSubject);

         m_pActionList[i].pszData = DBGetField(hResult, i, 6, NULL, 0);
         DecodeSQLString(m_pActionList[i].pszData);
      }
      DBFreeResult(hResult);
      bResult = TRUE;
   }
   else
   {
      nxlog_write(MSG_ACTIONS_LOAD_FAILED, EVENTLOG_ERROR_TYPE, NULL);
   }
   return bResult;
}


//
// Initialize action-related stuff
//

BOOL InitActions()
{
   BOOL bSuccess = FALSE;

   m_rwlockActionListAccess = RWLockCreate();
   if (m_rwlockActionListAccess != NULL)
      bSuccess = LoadActions();
   return bSuccess;
}


//
// Cleanup action-related stuff
//

void CleanupActions()
{
   DestroyActionList();
   RWLockDestroy(m_rwlockActionListAccess);
}


//
// Save action record to database
//

static void SaveActionToDB(NXC_ACTION *pAction)
{
   DB_RESULT hResult;
   BOOL bExist = FALSE;
   TCHAR *pszEscRcpt, *pszEscSubj, *pszEscData, szQuery[4096];

   // Check if action with given ID already exist in database
   _sntprintf(szQuery, 4096, _T("SELECT action_id FROM actions WHERE action_id=%d"), pAction->dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      bExist = (DBGetNumRows(hResult) > 0);
      DBFreeResult(hResult);
   }

   // Prepare and execute INSERT or UPDATE query
   pszEscRcpt = EncodeSQLString(pAction->szRcptAddr);
   pszEscSubj = EncodeSQLString(pAction->szEmailSubject);
   pszEscData = EncodeSQLString(CHECK_NULL_EX(pAction->pszData));
   if (bExist)
      _sntprintf(szQuery, 4096, 
		           _T("UPDATE actions SET action_name='%s',action_type=%d,is_disabled=%d,")
                 _T("rcpt_addr='%s',email_subject='%s',action_data='%s'")
                 _T("WHERE action_id=%d"),
              pAction->szName, pAction->iType, pAction->bIsDisabled,
              pszEscRcpt, pszEscSubj, pszEscData, pAction->dwId);
   else
      _sntprintf(szQuery, 4096,
		           _T("INSERT INTO actions (action_id,action_name,action_type,")
                 _T("is_disabled,rcpt_addr,email_subject,action_data) VALUES")
                 _T(" (%d,'%s',%d,%d,'%s','%s','%s')"),
              pAction->dwId,pAction->szName, pAction->iType, pAction->bIsDisabled,
              pszEscRcpt, pszEscSubj, pszEscData);
   DBQuery(g_hCoreDB, szQuery);
   free(pszEscRcpt);
   free(pszEscSubj);
   free(pszEscData);
}


//
// Compare action's id for bsearch()
//

static int CompareId(const void *key, const void *elem)
{
   return CAST_FROM_POINTER(key, DWORD) < ((NXC_ACTION *)elem)->dwId ? -1 : 
            (CAST_FROM_POINTER(key, DWORD) > ((NXC_ACTION *)elem)->dwId ? 1 : 0);
}


//
// Compare action id for qsort()
//

static int CompareElements(const void *p1, const void *p2)
{
   return ((NXC_ACTION *)p1)->dwId < ((NXC_ACTION *)p2)->dwId ? -1 : 
            (((NXC_ACTION *)p1)->dwId > ((NXC_ACTION *)p2)->dwId ? 1 : 0);
}


//
// Execute remote action
//

static BOOL ExecuteRemoteAction(TCHAR *pszTarget, TCHAR *pszAction)
{
   Node *pNode;
   AgentConnection *pConn;
   DWORD dwAddr, dwError;
   int i, nLen, nState, nCount = 0;
   TCHAR *pCmd[128], *pTmp;

   // Resolve hostname
   dwAddr = ResolveHostName(pszTarget);
   if ((dwAddr == INADDR_ANY) || (dwAddr == INADDR_NONE))
      return FALSE;

   pNode = FindNodeByIP(0, ntohl(dwAddr));	/* TODO: add possibility to specify action target by object id */
   if (pNode != NULL)
   {
      pConn = pNode->createAgentConnection();
      if (pConn == NULL)
         return FALSE;
   }
   else
   {
      // Target node is not in our database, try default communication settings
      pConn = new AgentConnection(dwAddr, AGENT_LISTEN_PORT, AUTH_NONE, _T(""));
      if (!pConn->connect(g_pServerKey))
      {
         delete pConn;
         return FALSE;
      }
   }

	pTmp = _tcsdup(pszAction);
	pCmd[0] = pTmp;
	nLen = (int)_tcslen(pTmp);
	for(i = 0, nState = 0, nCount = 1; (i < nLen) && (nCount < 127); i++)
	{
		switch(pTmp[i])
		{
			case _T(' '):
				if (nState == 0)
				{
					pTmp[i] = 0;
					if (pTmp[i + 1] != 0)
					{
						pCmd[nCount++] = pTmp + i + 1;
					}
				}
				break;
			case _T('"'):
				nState == 0 ? nState++ : nState--;

				memmove(pTmp + i, pTmp + i + 1, (nLen - i) * sizeof(TCHAR));
				i--;
				break;
			case _T('\\'):
				if (pTmp[i + 1] == _T('"'))
				{
					memmove(pTmp + i, pTmp + i + 1, (nLen - i) * sizeof(TCHAR));
				}
				break;
			default:
				break;
		}
	}
	pCmd[nCount] = NULL;

   dwError = pConn->execAction(pCmd[0], nCount - 1, &pCmd[1]);
   pConn->disconnect();
   delete pConn;
   free(pTmp);
   return dwError == ERR_SUCCESS;
}


//
// Run external command via system()
//

static THREAD_RESULT THREAD_CALL RunCommandThread(void *pArg)
{
	_tsystem((TCHAR *)pArg);
	free(pArg);
	return THREAD_OK;
}


//
// Forward event to other server
//

static BOOL ForwardEvent(const TCHAR *server, Event *event)
{
	ISC *isc;
	DWORD i, addr, rcc;

	addr = ResolveHostName(server);
	if (addr == INADDR_NONE)
	{
		DbgPrintf(2, _T("ForwardEvent: unable to resolve host name %s"), server);
		return FALSE;
	}

	isc = new ISC(addr);
	rcc = isc->Connect(ISC_SERVICE_EVENT_FORWARDER);
	if (rcc == ISC_ERR_SUCCESS)
	{
		CSCPMessage msg;
		NetObj *object;

		msg.SetId(1);
		msg.SetCode(CMD_FORWARD_EVENT);
		object = FindObjectById(event->getSourceId());
		if (object != NULL)
		{
			msg.SetVariable(VID_IP_ADDRESS, object->IpAddr());
			msg.SetVariable(VID_EVENT_CODE, event->getCode());
			msg.SetVariable(VID_EVENT_NAME, event->getName());
			if (event->getUserTag() != NULL)
				msg.SetVariable(VID_USER_TAG, event->getUserTag());
			msg.SetVariable(VID_NUM_ARGS, (WORD)event->getParametersCount());
			for(i = 0; i < event->getParametersCount(); i++)
				msg.SetVariable(VID_EVENT_ARG_BASE + i, event->getParameter(i));

			if (isc->SendMessage(&msg))
			{
				rcc = isc->WaitForRCC(1, 10000);
			}
			else
			{
				rcc = ISC_ERR_CONNECTION_BROKEN;
			}
		}
		else
		{
			rcc = ISC_ERR_INTERNAL_ERROR;
		}
		isc->Disconnect();
	}
	delete isc;
	if (rcc != ISC_ERR_SUCCESS)
		nxlog_write(MSG_EVENT_FORWARD_FAILED, EVENTLOG_WARNING_TYPE, "ss", server, ISCErrorCodeToText(rcc));
	return rcc == ISC_ERR_SUCCESS;
}


//
// Execute action on specific event
//

BOOL ExecuteAction(DWORD dwActionId, Event *pEvent, TCHAR *pszAlarmMsg)
{
   NXC_ACTION *pAction;
   BOOL bSuccess = FALSE;

   RWLockReadLock(m_rwlockActionListAccess, INFINITE);
   pAction = (NXC_ACTION *)bsearch(CAST_TO_POINTER(dwActionId, void *), m_pActionList, 
                                   m_dwNumActions, sizeof(NXC_ACTION), CompareId);
   if (pAction != NULL)
   {
      if (pAction->bIsDisabled)
      {
         DbgPrintf(3, _T("*actions* Action %d (%s) is disabled and will not be executed"),
                   dwActionId, pAction->szName);
         bSuccess = TRUE;
      }
      else
      {
         TCHAR *pszExpandedData, *pszExpandedSubject, *pszExpandedRcpt, *curr, *next;

         pszExpandedData = pEvent->expandText(CHECK_NULL_EX(pAction->pszData), pszAlarmMsg);
         pszExpandedRcpt = pEvent->expandText(pAction->szRcptAddr, pszAlarmMsg);
         switch(pAction->iType)
         {
            case ACTION_EXEC:
               DbgPrintf(3, _T("*actions* Executing command \"%s\""), pszExpandedData);
               //bSuccess = ExecCommand(pszExpandedData);
					ThreadCreate(RunCommandThread, 0, _tcsdup(pszExpandedData));
					bSuccess = TRUE;
               break;
            case ACTION_SEND_EMAIL:
               DbgPrintf(3, _T("*actions* Sending mail to %s: \"%s\""), pszExpandedRcpt, pszExpandedData);
               pszExpandedSubject = pEvent->expandText(pAction->szEmailSubject, pszAlarmMsg);
					curr = pszExpandedRcpt;
					do
					{
						next = _tcschr(curr, _T(';'));
						if (next != NULL)
							*next = 0;
						StrStrip(curr);
						PostMail(curr, pszExpandedSubject, pszExpandedData);
						curr = next + 1;
					} while(next != NULL);
               free(pszExpandedSubject);
               bSuccess = TRUE;
               break;
            case ACTION_SEND_SMS:
               DbgPrintf(3, _T("*actions* Sending SMS to %s: \"%s\""), pszExpandedRcpt, pszExpandedData);
					curr = pszExpandedRcpt;
					do
					{
						next = _tcschr(curr, _T(';'));
						if (next != NULL)
							*next = 0;
						StrStrip(curr);
	               PostSMS(curr, pszExpandedData);
						curr = next + 1;
					} while(next != NULL);
               bSuccess = TRUE;
               break;
            case ACTION_REMOTE:
               DbgPrintf(3, _T("*actions* Executing on \"%s\": \"%s\""), pszExpandedRcpt, pszExpandedData);
               bSuccess = ExecuteRemoteAction(pszExpandedRcpt, pszExpandedData);
               break;
				case ACTION_FORWARD_EVENT:
               DbgPrintf(3, _T("*actions* Forwarding event to \"%s\""), pszExpandedRcpt);
               bSuccess = ForwardEvent(pszExpandedRcpt, pEvent);
					break;
            default:
               break;
         }
         free(pszExpandedRcpt);
         free(pszExpandedData);
      }
   }
   RWLockUnlock(m_rwlockActionListAccess);
   return bSuccess;
}


//
// Create new action
//

DWORD CreateNewAction(const TCHAR *pszName, DWORD *pdwId)
{
   DWORD i, dwResult = RCC_SUCCESS;

   RWLockWriteLock(m_rwlockActionListAccess, INFINITE);

   // Check for duplicate name
   for(i = 0; i < m_dwNumActions; i++)
      if (!_tcsicmp(m_pActionList[i].szName, pszName))
      {
         dwResult = RCC_ALREADY_EXIST;
         break;
      }

   // If not exist, create it
   if (i == m_dwNumActions)
   {
      m_dwNumActions++;
      m_pActionList = (NXC_ACTION *)realloc(m_pActionList, sizeof(NXC_ACTION) * m_dwNumActions);
      m_pActionList[i].dwId = CreateUniqueId(IDG_ACTION);
      nx_strncpy(m_pActionList[i].szName, pszName, MAX_OBJECT_NAME);
      m_pActionList[i].bIsDisabled = TRUE;
      m_pActionList[i].iType = ACTION_EXEC;
      m_pActionList[i].szEmailSubject[0] = 0;
      m_pActionList[i].szRcptAddr[0] = 0;
      m_pActionList[i].pszData = NULL;

      qsort(m_pActionList, m_dwNumActions, sizeof(NXC_ACTION), CompareElements);

      SaveActionToDB(&m_pActionList[i]);
      m_dwUpdateCode = NX_NOTIFY_ACTION_CREATED;
      EnumerateClientSessions(SendActionDBUpdate, &m_pActionList[i]);
      *pdwId = m_pActionList[i].dwId;
   }

   RWLockUnlock(m_rwlockActionListAccess);
   return dwResult;
}


//
// Delete action
//

DWORD DeleteActionFromDB(DWORD dwActionId)
{
   DWORD i, dwResult = RCC_INVALID_ACTION_ID;
   TCHAR szQuery[256];

   RWLockWriteLock(m_rwlockActionListAccess, INFINITE);

   for(i = 0; i < m_dwNumActions; i++)
      if (m_pActionList[i].dwId == dwActionId)
      {
         m_dwUpdateCode = NX_NOTIFY_ACTION_DELETED;
         EnumerateClientSessions(SendActionDBUpdate, &m_pActionList[i]);

         m_dwNumActions--;
         safe_free(m_pActionList[i].pszData);
         memmove(&m_pActionList[i], &m_pActionList[i + 1], sizeof(NXC_ACTION) * (m_dwNumActions - i));
         _sntprintf(szQuery, 256, _T("DELETE FROM actions WHERE action_id=%d"), dwActionId);
         DBQuery(g_hCoreDB, szQuery);

         dwResult = RCC_SUCCESS;
         break;
      }

   RWLockUnlock(m_rwlockActionListAccess);
   return dwResult;
}


//
// Modify action record from message
//

DWORD ModifyActionFromMessage(CSCPMessage *pMsg)
{
   DWORD i, dwResult = RCC_INVALID_ACTION_ID;
   DWORD dwActionId;
	TCHAR name[MAX_OBJECT_NAME];

   pMsg->GetVariableStr(VID_ACTION_NAME, name, MAX_OBJECT_NAME);
	if (!IsValidObjectName(name, TRUE))
		return RCC_INVALID_OBJECT_NAME;

   dwActionId = pMsg->GetVariableLong(VID_ACTION_ID);
   RWLockWriteLock(m_rwlockActionListAccess, INFINITE);

   // Find action with given id
   for(i = 0; i < m_dwNumActions; i++)
      if (m_pActionList[i].dwId == dwActionId)
      {
         m_pActionList[i].bIsDisabled = pMsg->GetVariableShort(VID_IS_DISABLED);
         m_pActionList[i].iType = pMsg->GetVariableShort(VID_ACTION_TYPE);
         safe_free(m_pActionList[i].pszData);
         m_pActionList[i].pszData = pMsg->GetVariableStr(VID_ACTION_DATA);
         pMsg->GetVariableStr(VID_EMAIL_SUBJECT, m_pActionList[i].szEmailSubject, MAX_EMAIL_SUBJECT_LEN);
         pMsg->GetVariableStr(VID_RCPT_ADDR, m_pActionList[i].szRcptAddr, MAX_RCPT_ADDR_LEN);
			_tcscpy(m_pActionList[i].szName, name);

         SaveActionToDB(&m_pActionList[i]);

         m_dwUpdateCode = NX_NOTIFY_ACTION_MODIFIED;
         EnumerateClientSessions(SendActionDBUpdate, &m_pActionList[i]);

         dwResult = RCC_SUCCESS;
         break;
      }

   RWLockUnlock(m_rwlockActionListAccess);
   return dwResult;
}


//
// Fill CSCP message with action's data
//

void FillActionInfoMessage(CSCPMessage *pMsg, NXC_ACTION *pAction)
{
   pMsg->SetVariable(VID_IS_DISABLED, (WORD)pAction->bIsDisabled);
   pMsg->SetVariable(VID_ACTION_TYPE, (WORD)pAction->iType);
   pMsg->SetVariable(VID_ACTION_DATA, CHECK_NULL_EX(pAction->pszData));
   pMsg->SetVariable(VID_EMAIL_SUBJECT, pAction->szEmailSubject);
   pMsg->SetVariable(VID_ACTION_NAME, pAction->szName);
   pMsg->SetVariable(VID_RCPT_ADDR, pAction->szRcptAddr);
}


//
// Send all actions to client
//

void SendActionsToClient(ClientSession *pSession, DWORD dwRqId)
{
   DWORD i;
   CSCPMessage msg;

   // Prepare message
   msg.SetCode(CMD_ACTION_DATA);
   msg.SetId(dwRqId);

   RWLockReadLock(m_rwlockActionListAccess, INFINITE);

   for(i = 0; i < m_dwNumActions; i++)
   {
      msg.SetVariable(VID_ACTION_ID, m_pActionList[i].dwId);
      FillActionInfoMessage(&msg, &m_pActionList[i]);
      pSession->sendMessage(&msg);
      msg.DeleteAllVariables();
   }

   RWLockUnlock(m_rwlockActionListAccess);

   // Send end-of-list flag
   msg.SetVariable(VID_ACTION_ID, (DWORD)0);
   pSession->sendMessage(&msg);
}
