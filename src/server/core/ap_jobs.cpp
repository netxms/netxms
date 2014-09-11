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
 * Constructor
 */
PolicyDeploymentJob::PolicyDeploymentJob(Node *node, AgentPolicy *policy, UINT32 userId)
                    : ServerJob(_T("DEPLOY_AGENT_POLICY"), _T("Deploy agent policy"), node->Id(), userId, false)
{
	m_node = node;
	m_policy = policy;
	node->incRefCount();
	policy->incRefCount();

	TCHAR buffer[1024];
	_sntprintf(buffer, 1024, _T("Deploy policy %s"), policy->Name());
	setDescription(buffer);
}

/**
 * Destructor
 */
PolicyDeploymentJob::~PolicyDeploymentJob()
{
	m_node->decRefCount();
	m_policy->decRefCount();
}

/**
 * Run job
 */
bool PolicyDeploymentJob::run()
{
   bool success = false;

   TCHAR jobName[1024];
   _sntprintf(jobName, 1024, _T("Deploy policy %s"), m_policy->Name());

   do
   {
      setDescription(jobName);
      AgentConnectionEx *conn = m_node->createAgentConnection();
      if (conn != NULL)
      {
         UINT32 rcc = conn->deployPolicy(m_policy);
         if (rcc == ERR_SUCCESS)
         {
            m_policy->linkNode(m_node);
            success = true;
            break;
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
      
      setDescription(_T("Policy deploy failed. Wainting 10 minutes to restart job."));
      success = SleepAndCheckForShutdown(600);
   } while (!success);

	return success;
}

/**
 * Constructor
 */
PolicyUninstallJob::PolicyUninstallJob(Node *node, AgentPolicy *policy, UINT32 userId)
                   : ServerJob(_T("UNINSTALL_AGENT_POLICY"), _T("Uninstall agent policy"), node->Id(), userId, false)
{
	m_node = node;
	m_policy = policy;
	node->incRefCount();
	policy->incRefCount();

	TCHAR buffer[1024];
	_sntprintf(buffer, 1024, _T("Uninstall policy %s"), policy->Name());
	setDescription(buffer);
}

/**
 * Destructor
 */
PolicyUninstallJob::~PolicyUninstallJob()
{
	m_node->decRefCount();
	m_policy->decRefCount();
}

/**
 * Run job
 */
bool PolicyUninstallJob::run()
{
	bool success = false;

	AgentConnectionEx *conn = m_node->createAgentConnection();
	if (conn != NULL)
	{
		UINT32 rcc = conn->uninstallPolicy(m_policy);
		if (rcc == ERR_SUCCESS)
		{
			m_policy->unlinkNode(m_node);
			success = true;
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
	return success;
}
