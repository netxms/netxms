/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Raden Solutions
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
** File: ap_jobs.cpp
**
**/

#include "nxcore.h"

/**
 * Scheduled file upload
 */
void ScheduleDeployPolicy(const ScheduledTaskParameters *parameters)
{
   Node *object = (Node *)FindObjectById(parameters->m_objectId, OBJECT_NODE);
   if (object != NULL)
   {
      if (object->checkAccessRights(parameters->m_userId, OBJECT_ACCESS_CONTROL))
      {
         ServerJob *job = new PolicyInstallJob(parameters->m_persistentData, parameters->m_objectId, parameters->m_userId);
         if (!AddJob(job))
         {
            delete job;
            DbgPrintf(4, _T("ScheduleDeployPolicy: Failed to add job(incorrect parameters or no such object)."));
         }
      }
      else
      {
         DbgPrintf(4, _T("ScheduleDeployPolicy: Access to node %s denied"), object->getName());
      }
   }
   else
   {
      DbgPrintf(4, _T("ScheduleDeployPolicy: Node with id=\'%d\' not found"), parameters->m_userId);
   }
}

/**
 * Constructor
 */
PolicyInstallJob::PolicyInstallJob(DataCollectionTarget *node, UINT32 templateId, uuid policyGuid, const TCHAR *policyName, UINT32 userId)
                    : ServerJob(_T("DEPLOY_AGENT_POLICY"), _T("Deploy agent policy"), node->getId(), userId, false, 0)
{
   m_templateId = templateId;
   m_policyGuid = policyGuid;

	TCHAR buffer[1024];
	_sntprintf(buffer, 1024, _T("Deploy policy %s"), policyName);
	setDescription(buffer);

   setAutoCancelDelay(getRetryDelay() + 30);
}

/**
 * Constructor
 */
PolicyInstallJob::PolicyInstallJob(const TCHAR *params, UINT32 nodeId, UINT32 userId)
                    : ServerJob(_T("DEPLOY_AGENT_POLICY"), _T("Deploy agent policy"), nodeId, userId, false, 0)
{
   StringList paramList(params, _T(","));
   if (paramList.size() < 2)
   {
      m_templateId = 0;
      invalidate();
      return;
   }

   TCHAR name[MAX_DB_STRING];
	NetObj *obj = FindObjectById(_tcstol(paramList.get(0), NULL, 0));
	if (obj != NULL && obj->getObjectClass() == OBJECT_TEMPLATE)
	{
	   m_templateId = obj->getId();
	   m_policyGuid = uuid::parse(paramList.get(1));
   }
	else
	{
      m_templateId = 0;
	   invalidate();
	   return;
	}

	GenericAgentPolicy *policy = ((Template *)obj)->getAgentPolicyCopy(m_policyGuid);
	if(policy != NULL)
	{
	   _tcscpy(name,policy->getName());
	   delete policy;
	}
	else
	{

	}

   m_retryCount = (paramList.size() >= 2) ? _tcstol(paramList.get(2), NULL, 0) : 0;

   TCHAR buffer[1024];
   _sntprintf(buffer, 1024, _T("Deploy policy %s"), name);
   setDescription(buffer);

   setAutoCancelDelay(getRetryDelay() + 30);
}

/**
 * Destructor
 */
PolicyInstallJob::~PolicyInstallJob()
{
}

/**
 * Run job
 */
ServerJobResult PolicyInstallJob::run()
{
   ServerJobResult result = JOB_RESULT_FAILED;
   GenericAgentPolicy *policy = NULL;
   NetObj *obj = FindObjectById(m_templateId);
   if (obj != NULL && obj->getObjectClass() == OBJECT_TEMPLATE)
   {
      policy = ((Template *)obj)->getAgentPolicyCopy(m_policyGuid);
   }

   TCHAR jobName[1024];
   _sntprintf(jobName, 1024, _T("Deploy policy %s"), policy->getName());

   setDescription(jobName);
   AgentConnectionEx *conn = getNode()->createAgentConnection(true);
   if (conn != NULL)
   {
      UINT32 rcc = conn->deployPolicy(policy, getNode()->supportNewTypeFormat());
      conn->decRefCount();
      if (rcc == ERR_SUCCESS)
      {
         result = JOB_RESULT_SUCCESS;
      }
      else
      {
         setFailureMessage(AgentErrorCodeToText(rcc));
      }
   }
   else
   {
      setFailureMessage(_T("Agent connection not available"));
   }

   if ((result == JOB_RESULT_FAILED) && (m_retryCount-- > 0))
   {
      TCHAR description[256];
      _sntprintf(description, 256, _T("Policy installation failed. Waiting %d minutes to restart job."), getRetryDelay() / 60);
      setDescription(description);
      result = JOB_RESULT_RESCHEDULE;
   }
	return result;
}

/**
 * Serializes job parameters into TCHAR line separated by ';'
 */
const String PolicyInstallJob::serializeParameters()
{
   String params;
   params.append(m_templateId);
   params.append(_T(','));
   params.append(m_policyGuid);
   params.append(_T(','));
   params.append(m_retryCount);
   return params;
}

/**
 * Scheduled policy uninstall
 */
void ScheduleUninstallPolicy(const ScheduledTaskParameters *parameters)
{
   Node *object = (Node *)FindObjectById(parameters->m_objectId, OBJECT_NODE);
   if (object != NULL)
   {
      if (object->checkAccessRights(parameters->m_userId, OBJECT_ACCESS_CONTROL))
      {
         ServerJob *job = new PolicyUninstallJob(parameters->m_persistentData, parameters->m_objectId, parameters->m_userId);
         if (!AddJob(job))
         {
            delete job;
            DbgPrintf(4, _T("ScheduleUninstallPolicy: Failed to add job(incorrect parameters or no such object)."));
         }
      }
      else
      {
         DbgPrintf(4, _T("ScheduleUninstallPolicy: Access to node %s denied"), object->getName());
      }
   }
   else
   {
      DbgPrintf(4, _T("ScheduleUninstallPolicy: Node with id=\'%d\' not found"), parameters->m_userId);
   }
}

/**
 * Constructor
 */
PolicyUninstallJob::PolicyUninstallJob(DataCollectionTarget *node, const TCHAR *policyType, uuid policyGuid, UINT32 userId)
                   : ServerJob(_T("UNINSTALL_AGENT_POLICY"), _T("Uninstall agent policy"), node->getId(), userId, false, 0)
{
   m_policyGuid = policyGuid;
   _tcsncpy(m_policyType, policyType, 32);

	TCHAR buffer[1024];
	_sntprintf(buffer, 1024, _T("Uninstall policy %s"), (const TCHAR *)policyGuid.toString());
	setDescription(buffer);

   setAutoCancelDelay(getRetryDelay() + 30);
}

/**
 * Constructor
 */
PolicyUninstallJob::PolicyUninstallJob(const TCHAR* params, UINT32 node, UINT32 userId)
                    : ServerJob(_T("DEPLOY_AGENT_POLICY"), _T("Deploy agent policy"), node, userId, false, 0)
{
   StringList paramList(params, _T(","));
   if (paramList.size() < 2)
   {
      invalidate();
      return;
   }

   m_policyGuid = uuid::parse(paramList.get(0));
   _tcsncpy(m_policyType, paramList.get(1), 32);

   m_retryCount = (paramList.size() >= 2) ? _tcstol(paramList.get(1), NULL, 0) : 0;

   TCHAR buffer[1024];
   _sntprintf(buffer, 1024, _T("Uninstall policy %s"), (const TCHAR *)m_policyGuid.toString());
   setDescription(buffer);

   setAutoCancelDelay(getRetryDelay() + 30);
}

/**
 * Destructor
 */
PolicyUninstallJob::~PolicyUninstallJob()
{
}

/**
 * Run job
 */
ServerJobResult PolicyUninstallJob::run()
{
	ServerJobResult result = JOB_RESULT_FAILED;

	AgentConnectionEx *conn = getNode()->createAgentConnection();
	if (conn != NULL)
	{
		UINT32 rcc = conn->uninstallPolicy(m_policyGuid, m_policyType, getNode()->supportNewTypeFormat());
		conn->decRefCount();
		if (rcc == ERR_SUCCESS)
		{
			result = JOB_RESULT_SUCCESS;
		}
		else
		{
			setFailureMessage(AgentErrorCodeToText(rcc));
		}
	}
	else
	{
		setFailureMessage(_T("Agent connection not available"));
	}

   if ((result == JOB_RESULT_FAILED) && (m_retryCount-- > 0))
   {
      TCHAR description[256];
      _sntprintf(description, 256, _T("Policy uninstall failed. Waiting %d minutes to restart job."), getRetryDelay() / 60);
      setDescription(description);
      result = JOB_RESULT_RESCHEDULE;
   }

	return result;
}

/**
 * Serializes job parameters into TCHAR line separated by ';'
 */
const String PolicyUninstallJob::serializeParameters()
{
   String params;
   params.append(m_policyGuid);
   params.append(_T(','));
   params.append(m_policyType);
   params.append(_T(','));
   params.append(m_retryCount);
   return params;
}
