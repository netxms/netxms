/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
void ScheduleDeployPolicy(const ScheduledTaskParameters *params)
{
   Node *object = (Node *)FindObjectById(params->m_objectId, OBJECT_NODE);
   if (object != NULL)
   {
      if (object->checkAccessRights(params->m_userId, OBJECT_ACCESS_CONTROL))
      {
         ServerJob *job = new PolicyDeploymentJob(params->m_params, params->m_objectId, params->m_userId);
         if (!AddJob(job))
         {
            delete job;
            DbgPrintf(4, _T("ScheduleDeployPolicy: Failed to add job(incorrect parameters or no such object)."));
         }
      }
      else
         DbgPrintf(4, _T("ScheduleDeployPolicy: Access to node %s denied"), object->getName());
   }
   else
      DbgPrintf(4, _T("ScheduleDeployPolicy: Node with id=\'%d\' not found"), params->m_userId);
}

/**
 * Constructor
 */
PolicyDeploymentJob::PolicyDeploymentJob(Node *node, AgentPolicy *policy, UINT32 userId)
                    : ServerJob(_T("DEPLOY_AGENT_POLICY"), _T("Deploy agent policy"), node->getId(), userId, false)
{
	m_node = node;
	m_policy = policy;
	node->incRefCount();
	policy->incRefCount();

	TCHAR buffer[1024];
	_sntprintf(buffer, 1024, _T("Deploy policy %s"), policy->getName());
	setDescription(buffer);
}


/**
 * Constructor
 */
PolicyDeploymentJob::PolicyDeploymentJob(const TCHAR* params, UINT32 node, UINT32 userId)
                    : ServerJob(_T("DEPLOY_AGENT_POLICY"), _T("Deploy agent policy"), node, userId, false)
{
   StringList paramList(params, _T(","));
   if(paramList.size() < 1)
   {
      setIsValid(false);
      return;
   }

	m_node = (Node *)FindObjectById(node, OBJECT_NODE);
	NetObj *obj = FindObjectById(_tcstol(paramList.get(0), NULL, 0));
	if(obj != NULL && (obj->getObjectClass() == OBJECT_AGENTPOLICY || obj->getObjectClass() == OBJECT_AGENTPOLICY_CONFIG
      || obj->getObjectClass() == OBJECT_AGENTPOLICY_LOGPARSER))
	{
      m_policy = (AgentPolicy *)obj;
   }

   if(m_node != NULL && m_policy != NULL)
   {
      m_node->incRefCount();
      m_policy->incRefCount();
   }
   else
   {
      setIsValid(false);
      return;
   }

   if(paramList.size() >= 2)
   {
      m_retryCount = _tcstol(paramList.get(1), NULL, 0);
   }
   else
      m_retryCount = 0;

   TCHAR buffer[1024];
   _sntprintf(buffer, 1024, _T("Deploy policy %s"), m_policy->getName());
   setDescription(buffer);
}

/**
 * Destructor
 */
PolicyDeploymentJob::~PolicyDeploymentJob()
{
   if(isValid())
   {
      m_node->decRefCount();
      m_policy->decRefCount();
   }
}

/**
 * Run job
 */
ServerJobResult PolicyDeploymentJob::run()
{
   ServerJobResult success = JOB_RESULT_FAILED;

   TCHAR jobName[1024];
   _sntprintf(jobName, 1024, _T("Deploy policy %s"), m_policy->getName());

   setDescription(jobName);
   AgentConnectionEx *conn = m_node->createAgentConnection(true);
   if (conn != NULL)
   {
      UINT32 rcc = conn->deployPolicy(m_policy);
      conn->decRefCount();
      if (rcc == ERR_SUCCESS)
      {
         m_policy->linkNode(m_node);
         success = JOB_RESULT_SUCCESS;
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

   if(success == JOB_RESULT_FAILED && m_retryCount-- > 0)
   {
      TCHAR description[256];
      _sntprintf(description, 256, _T("Policy deploy failed. Wainting %d minutes to restart job."), getNextJobExecutionTime()/60);
      setDescription(description);
      success = JOB_RESULT_RESCHEDULE;
   }
	return success;
}

/**
 * Serializes job parameters into TCHAR line separated by ';'
 */
const String PolicyDeploymentJob::serializeParameters()
{
   String params;
   params.append(m_policy->getId());
   params.append(_T(','));
   params.append(m_retryCount);
   return params;
}

/**
 * Schedules execution in 10 minutes
 */
void PolicyDeploymentJob::rescheduleExecution()
{
   AddOneTimeScheduledTask(_T("Policy.Deploy"), time(NULL) + getNextJobExecutionTime(), serializeParameters(), 0, getRemoteNode(), SYSTEM_ACCESS_FULL, SCHEDULED_TASK_SYSTEM);//TODO: change to correct user
}


/**
 * Scheduled file upload
 */
void ScheduleUninstallPolicy(const ScheduledTaskParameters *params)
{
   Node *object = (Node *)FindObjectById(params->m_objectId, OBJECT_NODE);
   if (object != NULL)
   {
      if (object->checkAccessRights(params->m_userId, OBJECT_ACCESS_CONTROL))
      {
         ServerJob *job = new PolicyUninstallJob(params->m_params, params->m_objectId, params->m_userId);
         if (!AddJob(job))
         {
            delete job;
            DbgPrintf(4, _T("ScheduleUninstallPolicy: Failed to add job(incorrect parameters or no such object)."));
         }
      }
      else
         DbgPrintf(4, _T("ScheduleUninstallPolicy: Access to node %s denied"), object->getName());
   }
   else
      DbgPrintf(4, _T("ScheduleUninstallPolicy: Node with id=\'%d\' not found"), params->m_userId);
}

/**
 * Constructor
 */
PolicyUninstallJob::PolicyUninstallJob(Node *node, AgentPolicy *policy, UINT32 userId)
                   : ServerJob(_T("UNINSTALL_AGENT_POLICY"), _T("Uninstall agent policy"), node->getId(), userId, false)
{
	m_node = node;
	m_policy = policy;
	node->incRefCount();
	policy->incRefCount();

	TCHAR buffer[1024];
	_sntprintf(buffer, 1024, _T("Uninstall policy %s"), policy->getName());
	setDescription(buffer);
}

/**
 * Constructor
 */
PolicyUninstallJob::PolicyUninstallJob(const TCHAR* params, UINT32 node, UINT32 userId)
                    : ServerJob(_T("DEPLOY_AGENT_POLICY"), _T("Deploy agent policy"), node, userId, false)
{
   StringList paramList(params, _T(","));
   if(paramList.size() < 1)
   {
      setIsValid(false);
      return;
   }

	m_node = (Node *)FindObjectById(node, OBJECT_NODE);
	NetObj *obj = FindObjectById(_tcstol(paramList.get(0), NULL, 0));
	if(obj != NULL && (obj->getObjectClass() == OBJECT_AGENTPOLICY || obj->getObjectClass() == OBJECT_AGENTPOLICY_CONFIG
      || obj->getObjectClass() == OBJECT_AGENTPOLICY_LOGPARSER))
	{
      m_policy = (AgentPolicy *)obj;
   }

   if(m_node != NULL && m_policy != NULL)
   {
      m_node->incRefCount();
      m_policy->incRefCount();
   }
   else
   {
      setIsValid(false);
      return;
   }

   if(paramList.size() >= 2)
   {
      m_retryCount = _tcstol(paramList.get(1), NULL, 0);
   }
   else
      m_retryCount = 0;

   TCHAR buffer[1024];
   _sntprintf(buffer, 1024, _T("Uninstall policy %s"), m_policy->getName());
   setDescription(buffer);
}

/**
 * Destructor
 */
PolicyUninstallJob::~PolicyUninstallJob()
{
   if(isValid())
   {
      m_node->decRefCount();
      m_policy->decRefCount();
   }
}

/**
 * Run job
 */
ServerJobResult PolicyUninstallJob::run()
{
	ServerJobResult success = JOB_RESULT_FAILED;

	AgentConnectionEx *conn = m_node->createAgentConnection();
	if (conn != NULL)
	{
		UINT32 rcc = conn->uninstallPolicy(m_policy);
		conn->decRefCount();
		if (rcc == ERR_SUCCESS)
		{
			m_policy->unlinkNode(m_node);
			success = JOB_RESULT_SUCCESS;
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

   if(success == JOB_RESULT_FAILED && m_retryCount-- > 0)
   {
      TCHAR description[256];
      _sntprintf(description, 256, _T("Policy uninstall failed. Wainting %d minutes to restart job."), getNextJobExecutionTime()/60);
      setDescription(description);
      success = JOB_RESULT_RESCHEDULE;
   }

	return success;
}

/**
 * Serializes job parameters into TCHAR line separated by ';'
 */
const String PolicyUninstallJob::serializeParameters()
{
   String params;
   params.append(m_policy->getId());
   params.append(_T(','));
   params.append(m_retryCount);
   return params;
}

/**
 * Schedules execution in 10 minutes
 */
void PolicyUninstallJob::rescheduleExecution()
{
   AddOneTimeScheduledTask(_T("Policy.Uninstall"), time(NULL) + getNextJobExecutionTime(), serializeParameters(), 0, getRemoteNode(), SYSTEM_ACCESS_FULL, SCHEDULED_TASK_SYSTEM);//TODO: change to correct user
}
