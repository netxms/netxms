/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2010 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: epp.cpp
**
**/

#include "libnxcl.h"


//
// Create copy of event policy rule
//

NXC_EPP_RULE LIBNXCL_EXPORTABLE *NXCCopyEventPolicyRule(NXC_EPP_RULE *src)
{
	NXC_EPP_RULE *dst;

	dst = (NXC_EPP_RULE *)nx_memdup(src, sizeof(NXC_EPP_RULE));
	dst->pszComment = (src->pszComment != NULL) ? _tcsdup(src->pszComment) : NULL;
	dst->pdwActionList = (UINT32 *)nx_memdup(src->pdwActionList, src->dwNumActions * sizeof(UINT32));
	dst->pdwSourceList = (UINT32 *)nx_memdup(src->pdwSourceList, src->dwNumSources * sizeof(UINT32));
	dst->pdwEventList = (UINT32 *)nx_memdup(src->pdwEventList, src->dwNumEvents * sizeof(UINT32));
	dst->pszScript = (src->pszScript != NULL) ? _tcsdup(src->pszScript) : NULL;
	return dst;
}


//
// Create copy of event policy rule in given buffer
//

void LIBNXCL_EXPORTABLE NXCCopyEventPolicyRuleToBuffer(NXC_EPP_RULE *dst, NXC_EPP_RULE *src)
{
	memcpy(dst, src, sizeof(NXC_EPP_RULE));
	dst->pszComment = (src->pszComment != NULL) ? _tcsdup(src->pszComment) : NULL;
	dst->pdwActionList = (UINT32 *)nx_memdup(src->pdwActionList, src->dwNumActions * sizeof(UINT32));
	dst->pdwSourceList = (UINT32 *)nx_memdup(src->pdwSourceList, src->dwNumSources * sizeof(UINT32));
	dst->pdwEventList = (UINT32 *)nx_memdup(src->pdwEventList, src->dwNumEvents * sizeof(UINT32));
	dst->pszScript = (src->pszScript != NULL) ? _tcsdup(src->pszScript) : NULL;
}


//
// Create empty event policy
//

NXC_EPP LIBNXCL_EXPORTABLE *NXCCreateEmptyEventPolicy()
{
	NXC_EPP *epp;

	epp = (NXC_EPP *)malloc(sizeof(NXC_EPP));
	memset(epp, 0, sizeof(NXC_EPP));
	return epp;
}


//
// Delete event policy structure
//

void LIBNXCL_EXPORTABLE NXCDestroyEventPolicy(NXC_EPP *pEventPolicy)
{
   UINT32 i;

	if (pEventPolicy == NULL)
		return;

   for(i = 0; i < pEventPolicy->dwNumRules; i++)
   {
      safe_free(pEventPolicy->pRuleList[i].pszComment);
      safe_free(pEventPolicy->pRuleList[i].pdwActionList);
      safe_free(pEventPolicy->pRuleList[i].pdwSourceList);
      safe_free(pEventPolicy->pRuleList[i].pdwEventList);
      safe_free(pEventPolicy->pRuleList[i].pszScript);
   }
   safe_free(pEventPolicy->pRuleList);
   free(pEventPolicy);
}


//
// Open and load to client event processing policy
//

UINT32 LIBNXCL_EXPORTABLE NXCOpenEventPolicy(NXC_SESSION hSession, NXC_EPP **ppEventPolicy)
{
   NXCPMessage msg, *pResponse;
   UINT32 i, j, dwRqId, dwRetCode, count, id;
	TCHAR *attr, *value;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Prepare message
   msg.setCode(CMD_OPEN_EPP);
   msg.setId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for reply
   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         // Prepare event policy structure
         *ppEventPolicy = (NXC_EPP *)malloc(sizeof(NXC_EPP));
         (*ppEventPolicy)->dwNumRules = pResponse->getFieldAsUInt32(VID_NUM_RULES);
         (*ppEventPolicy)->pRuleList = 
            (NXC_EPP_RULE *)malloc(sizeof(NXC_EPP_RULE) * (*ppEventPolicy)->dwNumRules);
         memset((*ppEventPolicy)->pRuleList, 0, sizeof(NXC_EPP_RULE) * (*ppEventPolicy)->dwNumRules);
         delete pResponse;

         // Receive policy rules, each in separate message
         for(i = 0; i < (*ppEventPolicy)->dwNumRules; i++)
         {
            pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_EPP_RECORD, dwRqId);
            if (pResponse != NULL)
            {
               (*ppEventPolicy)->pRuleList[i].dwFlags = pResponse->getFieldAsUInt32(VID_FLAGS);
               (*ppEventPolicy)->pRuleList[i].dwId = pResponse->getFieldAsUInt32(VID_RULE_ID);
               (*ppEventPolicy)->pRuleList[i].pszComment = pResponse->getFieldAsString(VID_COMMENTS);
               (*ppEventPolicy)->pRuleList[i].pszScript = pResponse->getFieldAsString(VID_SCRIPT);

               (*ppEventPolicy)->pRuleList[i].dwNumActions = 
                  pResponse->getFieldAsUInt32(VID_NUM_ACTIONS);
               (*ppEventPolicy)->pRuleList[i].pdwActionList = 
                  (UINT32 *)malloc(sizeof(UINT32) * (*ppEventPolicy)->pRuleList[i].dwNumActions);
               pResponse->getFieldAsInt32Array(VID_RULE_ACTIONS, 
                                                (*ppEventPolicy)->pRuleList[i].dwNumActions,
                                                (*ppEventPolicy)->pRuleList[i].pdwActionList);

               (*ppEventPolicy)->pRuleList[i].dwNumEvents = 
                  pResponse->getFieldAsUInt32(VID_NUM_EVENTS);
               (*ppEventPolicy)->pRuleList[i].pdwEventList = 
                  (UINT32 *)malloc(sizeof(UINT32) * (*ppEventPolicy)->pRuleList[i].dwNumEvents);
               pResponse->getFieldAsInt32Array(VID_RULE_EVENTS, 
                                                (*ppEventPolicy)->pRuleList[i].dwNumEvents,
                                                (*ppEventPolicy)->pRuleList[i].pdwEventList);

               (*ppEventPolicy)->pRuleList[i].dwNumSources = 
                  pResponse->getFieldAsUInt32(VID_NUM_SOURCES);
               (*ppEventPolicy)->pRuleList[i].pdwSourceList = 
                  (UINT32 *)malloc(sizeof(UINT32) * (*ppEventPolicy)->pRuleList[i].dwNumSources);
               pResponse->getFieldAsInt32Array(VID_RULE_SOURCES, 
                                                (*ppEventPolicy)->pRuleList[i].dwNumSources,
                                                (*ppEventPolicy)->pRuleList[i].pdwSourceList);

               pResponse->getFieldAsString(VID_ALARM_KEY, 
                                         (*ppEventPolicy)->pRuleList[i].szAlarmKey,
                                         MAX_DB_STRING);
               pResponse->getFieldAsString(VID_ALARM_MESSAGE, 
                                         (*ppEventPolicy)->pRuleList[i].szAlarmMessage,
                                         MAX_DB_STRING);
               (*ppEventPolicy)->pRuleList[i].wAlarmSeverity = pResponse->getFieldAsUInt16(VID_ALARM_SEVERITY);
					(*ppEventPolicy)->pRuleList[i].dwAlarmTimeout = pResponse->getFieldAsUInt32(VID_ALARM_TIMEOUT);
					(*ppEventPolicy)->pRuleList[i].dwAlarmTimeoutEvent = pResponse->getFieldAsUInt32(VID_ALARM_TIMEOUT_EVENT);

					(*ppEventPolicy)->pRuleList[i].dwSituationId = pResponse->getFieldAsUInt32(VID_SITUATION_ID);
               pResponse->getFieldAsString(VID_SITUATION_INSTANCE,
                                         (*ppEventPolicy)->pRuleList[i].szSituationInstance,
                                         MAX_DB_STRING);
					(*ppEventPolicy)->pRuleList[i].pSituationAttrList = new StringMap;
					count = pResponse->getFieldAsUInt32(VID_SITUATION_NUM_ATTRS);
					for(j = 0, id = VID_SITUATION_ATTR_LIST_BASE; j < count; j++)
					{
						attr = pResponse->getFieldAsString(id++);
						value = pResponse->getFieldAsString(id++);
						(*ppEventPolicy)->pRuleList[i].pSituationAttrList->setPreallocated(attr, value);
					}
               delete pResponse;
            }
            else
            {
               dwRetCode = RCC_TIMEOUT;
               break;
            }
         }

         // Delete allocated policy structure if we have failed somewhere in the middle
         if (dwRetCode != RCC_SUCCESS)
            NXCDestroyEventPolicy(*ppEventPolicy);
      }
      else
      {
         if (dwRetCode == RCC_COMPONENT_LOCKED)
         {
            if (pResponse->isFieldExist(VID_LOCKED_BY))
            {
               TCHAR szBuffer[MAX_LOCKINFO_LEN];

               pResponse->getFieldAsString(VID_LOCKED_BY, szBuffer, MAX_LOCKINFO_LEN);
               ((NXCL_Session *)hSession)->setLastLock(szBuffer);
            }
            else
            {
               ((NXCL_Session *)hSession)->setLastLock(_T("<unknown>"));
            }
         }
         delete pResponse;
      }
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}

/**
 * Close event policy (without saving)
 */
UINT32 LIBNXCL_EXPORTABLE NXCCloseEventPolicy(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->SimpleCommand(CMD_CLOSE_EPP);
}

/**
 * Save (and install) new event policy
 */
UINT32 LIBNXCL_EXPORTABLE NXCSaveEventPolicy(NXC_SESSION hSession, NXC_EPP *pEventPolicy)
{
   NXCPMessage msg;
   UINT32 i, dwRqId, dwRetCode;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Prepare message
   msg.setCode(CMD_SAVE_EPP);
   msg.setId(dwRqId);
   msg.setField(VID_NUM_RULES, pEventPolicy->dwNumRules);

   // Send message and wait for response
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   dwRetCode = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   if (dwRetCode == RCC_SUCCESS)
   {
      // Send all event policy records, one per message
      msg.setCode(CMD_EPP_RECORD);
      for(i = 0; i < pEventPolicy->dwNumRules; i++)
      {
         msg.deleteAllFields();

         msg.setField(VID_FLAGS, pEventPolicy->pRuleList[i].dwFlags);
         msg.setField(VID_RULE_ID, pEventPolicy->pRuleList[i].dwId);
         msg.setField(VID_COMMENTS, (TCHAR *)CHECK_NULL_EX(pEventPolicy->pRuleList[i].pszComment));
         msg.setField(VID_SCRIPT, (TCHAR *)CHECK_NULL_EX(pEventPolicy->pRuleList[i].pszScript));
         msg.setField(VID_NUM_ACTIONS, pEventPolicy->pRuleList[i].dwNumActions);
         msg.setFieldFromInt32Array(VID_RULE_ACTIONS,
                                     pEventPolicy->pRuleList[i].dwNumActions,
                                     pEventPolicy->pRuleList[i].pdwActionList);
         msg.setField(VID_NUM_EVENTS, pEventPolicy->pRuleList[i].dwNumEvents);
         msg.setFieldFromInt32Array(VID_RULE_EVENTS,
                                     pEventPolicy->pRuleList[i].dwNumEvents,
                                     pEventPolicy->pRuleList[i].pdwEventList);
         msg.setField(VID_NUM_SOURCES, pEventPolicy->pRuleList[i].dwNumSources);
         msg.setFieldFromInt32Array(VID_RULE_SOURCES,
                                     pEventPolicy->pRuleList[i].dwNumSources,
                                     pEventPolicy->pRuleList[i].pdwSourceList);
         msg.setField(VID_ALARM_KEY, pEventPolicy->pRuleList[i].szAlarmKey);
         msg.setField(VID_ALARM_MESSAGE, pEventPolicy->pRuleList[i].szAlarmMessage);
         msg.setField(VID_ALARM_SEVERITY, pEventPolicy->pRuleList[i].wAlarmSeverity);
			msg.setField(VID_ALARM_TIMEOUT, pEventPolicy->pRuleList[i].dwAlarmTimeout);
			msg.setField(VID_ALARM_TIMEOUT_EVENT, pEventPolicy->pRuleList[i].dwAlarmTimeoutEvent);
			msg.setField(VID_SITUATION_ID, pEventPolicy->pRuleList[i].dwSituationId);
			msg.setField(VID_SITUATION_INSTANCE, pEventPolicy->pRuleList[i].szSituationInstance);
         if (pEventPolicy->pRuleList[i].pSituationAttrList != NULL)
         {
            pEventPolicy->pRuleList[i].pSituationAttrList->fillMessage(&msg, VID_SITUATION_NUM_ATTRS, VID_SITUATION_ATTR_LIST_BASE);
         }
         else
         {
   			msg.setField(VID_SITUATION_NUM_ATTRS, (UINT32)0);
         }

         ((NXCL_Session *)hSession)->SendMsg(&msg);
      }

      // Wait for final confirmation if we have sent some rules
      if (pEventPolicy->dwNumRules > 0)
         dwRetCode = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   }
   return dwRetCode;
}


//
// Add rule to policy
//

void LIBNXCL_EXPORTABLE NXCAddPolicyRule(NXC_EPP *policy, NXC_EPP_RULE *rule, BOOL dynAllocated)
{
	policy->dwNumRules++;
	policy->pRuleList = (NXC_EPP_RULE *)realloc(policy->pRuleList, sizeof(NXC_EPP_RULE) * policy->dwNumRules);
	memcpy(&policy->pRuleList[policy->dwNumRules - 1], rule, sizeof(NXC_EPP_RULE));
	if (dynAllocated)
		free(rule);
}


//
// Delete rule from policy
//

void LIBNXCL_EXPORTABLE NXCDeletePolicyRule(NXC_EPP *pEventPolicy, UINT32 dwRule)
{
   if (dwRule < pEventPolicy->dwNumRules)
   {
      safe_free(pEventPolicy->pRuleList[dwRule].pdwActionList);
      safe_free(pEventPolicy->pRuleList[dwRule].pdwEventList);
      safe_free(pEventPolicy->pRuleList[dwRule].pdwSourceList);
      safe_free(pEventPolicy->pRuleList[dwRule].pszComment);
      safe_free(pEventPolicy->pRuleList[dwRule].pszScript);
      pEventPolicy->dwNumRules--;
      memmove(&pEventPolicy->pRuleList[dwRule], &pEventPolicy->pRuleList[dwRule + 1],
              sizeof(NXC_EPP_RULE) * (pEventPolicy->dwNumRules - dwRule));
   }
}
