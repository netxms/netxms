/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2020 Victor Kirhenshtein
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

#include "nxagentd.h"


//
// Static data
//

static ACTION *m_pActionList = NULL;
static UINT32 m_dwNumActions = 0;

/**
 * Agent command executor default constructor
 */
AgentActionExecutor::AgentActionExecutor() : ProcessExecutor(NULL)
{
   m_requestId = 0;
   m_session = NULL;
   m_args = NULL;
}

/**
 * Agent command executor destructor
 */
AgentActionExecutor::~AgentActionExecutor()
{
   delete(m_args);
   if (m_session != NULL)
      m_session->decRefCount();
}

/**
 * Agent action executor creator
 */
AgentActionExecutor *AgentActionExecutor::createAgentExecutor(const TCHAR *cmd, const StringList *args)
{
   AgentActionExecutor *executor = new AgentActionExecutor();
   executor->m_cmd = MemCopyString(cmd);
   executor->m_args = new StringList(args);
   executor->m_session = new VirtualSession(0);

   UINT32 rcc = executor->findAgentAction();

   if (rcc == ERR_SUCCESS)
   {
      delete(executor);
      return NULL;
   }

   return executor;
}

/**
 * Agent action executor creator
 */
AgentActionExecutor *AgentActionExecutor::createAgentExecutor(NXCPMessage *request, AbstractCommSession *session, UINT32 *rcc)
{
   AgentActionExecutor *executor = new AgentActionExecutor();
   executor->m_sendOutput = request->getFieldAsBoolean(VID_RECEIVE_OUTPUT);
   executor->m_requestId = request->getId();
   executor->m_session = session;
   executor->m_session->incRefCount();
   executor->m_cmd = request->getFieldAsString(VID_ACTION_NAME);

   UINT32 count = request->getFieldAsUInt32(VID_NUM_ARGS);
   if (count > 0)
   {
      executor->m_args = new StringList();
      UINT32 fieldId = VID_ACTION_ARG_BASE;
      for(UINT32 i = 0; i < count; i++)
         executor->m_args->addPreallocated(request->getFieldAsString(fieldId++));
   }
   *rcc = executor->findAgentAction();

   if (*rcc == ERR_SUCCESS)
   {
      delete executor;
      return NULL;
   }

   return executor;
}

UINT32 AgentActionExecutor::findAgentAction()
{
   UINT32 rcc = ERR_UNKNOWN_PARAMETER;
   for(UINT32 i = 0; i < m_dwNumActions; i++)
   {
      if (!_tcsicmp(m_pActionList[i].szName, m_cmd))
      {
         MemFree(m_cmd);
         m_cmd = MemCopyString(m_pActionList[i].handler.pszCmdLine);
         switch(m_pActionList[i].iType)
         {
            case AGENT_ACTION_EXEC:
            case AGENT_ACTION_SHELLEXEC:
               rcc = ERR_PROCESSING;
               substituteArgs();
               break;
            case AGENT_ACTION_SUBAGENT:
               rcc = m_pActionList[i].handler.sa.fpHandler(m_cmd, m_args, m_pActionList[i].handler.sa.pArg, m_session);
               break;
            default:
               rcc = ERR_NOT_IMPLEMENTED;
               break;
         }
         DebugPrintf(4, _T("Executing action %s of type %d"), m_cmd, m_pActionList[i].iType);
         break;
      }
   }

   if (rcc == ERR_UNKNOWN_PARAMETER)
      rcc = ExecuteActionByExtSubagent(m_cmd, m_args, m_session, 0, false);

   return rcc;
}

void AgentActionExecutor::stopAction(AgentActionExecutor *executor)
{
   if (executor->isRunning())
   {
      executor->stop();
      DebugPrintf(6, _T("Agent action: %s timed out"), executor->getCommand());
   }
   delete(executor);
}

/**
 * Substitute agent action arguments
 */
void AgentActionExecutor::substituteArgs()
{
   if (m_args != NULL && m_args->size() > 0)
   {
      StringBuffer cmd(m_cmd);
      TCHAR macro[3];
      for(int i = 0; i < m_args->size() && i <= 9; i++)
      {
         _sntprintf(macro, 3, _T("$%d"), i + 1);
         cmd.replace(macro, m_args->get(i));
      }
      MemFree(m_cmd);
      m_cmd = MemCopyString(cmd);
   }
}

/**
 * Send output to console
 */
void AgentActionExecutor::onOutput(const char *text)
{
   NXCPMessage msg(m_session->getProtocolVersion());
   msg.setId(m_requestId);
   msg.setCode(CMD_COMMAND_OUTPUT);
#ifdef UNICODE
   TCHAR *buffer = WideStringFromMBStringSysLocale(text);
   msg.setField(VID_MESSAGE, buffer);
   m_session->sendMessage(&msg);
   MemFree(buffer);
#else
   msg.setField(VID_MESSAGE, text);
   m_session->sendMessage(&msg);
#endif
}

/**
 * Send message to make console stop listening to output
 */
void AgentActionExecutor::endOfOutput()
{
   NXCPMessage msg(m_session->getProtocolVersion());
   msg.setId(m_requestId);
   msg.setCode(CMD_COMMAND_OUTPUT);
   msg.setEndOfSequence();
   m_session->sendMessage(&msg);
}

/**
 * Add action
 */
BOOL AddAction(const TCHAR *pszName, int iType, const TCHAR *pArg,
               LONG (*fpHandler)(const TCHAR *, const StringList *, const TCHAR *, AbstractCommSession *),
               const TCHAR *pszSubAgent, const TCHAR *pszDescription)
{
   UINT32 i;

   // Check if action with given name already registered
   for(i = 0; i < m_dwNumActions; i++)
      if (!_tcsicmp(m_pActionList[i].szName, pszName))
         break;

   if (i == m_dwNumActions)
   {
      // Create new entry in action list
      m_dwNumActions++;
      m_pActionList = (ACTION *)realloc(m_pActionList, sizeof(ACTION) * m_dwNumActions);
      _tcslcpy(m_pActionList[i].szName, pszName, MAX_PARAM_NAME);
      m_pActionList[i].iType = iType;
      nx_strncpy(m_pActionList[i].szDescription, pszDescription, MAX_DB_STRING);
      switch(iType)
      {
         case AGENT_ACTION_EXEC:
         case AGENT_ACTION_SHELLEXEC:
            m_pActionList[i].handler.pszCmdLine = _tcsdup(pArg);
            break;
         case AGENT_ACTION_SUBAGENT:
            m_pActionList[i].handler.sa.fpHandler = fpHandler;
            m_pActionList[i].handler.sa.pArg = pArg;
            _tcslcpy(m_pActionList[i].handler.sa.szSubagentName, pszSubAgent,MAX_PATH);
            break;
         default:
            break;
      }
   }
   else
   {
      // Update existing entry in action list
      _tcslcpy(m_pActionList[i].szDescription, pszDescription, MAX_DB_STRING);
      if ((m_pActionList[i].iType == AGENT_ACTION_EXEC) || (m_pActionList[i].iType == AGENT_ACTION_SHELLEXEC))
         MemFree(m_pActionList[i].handler.pszCmdLine);
      m_pActionList[i].iType = iType;
      switch(iType)
      {
         case AGENT_ACTION_EXEC:
         case AGENT_ACTION_SHELLEXEC:
            m_pActionList[i].handler.pszCmdLine = _tcsdup(pArg);
            break;
         case AGENT_ACTION_SUBAGENT:
            m_pActionList[i].handler.sa.fpHandler = fpHandler;
            m_pActionList[i].handler.sa.pArg = pArg;
            _tcslcpy(m_pActionList[i].handler.sa.szSubagentName, pszSubAgent,MAX_PATH);
            break;
         default:
            break;
      }
   }
   return TRUE;
}

/**
 * Add action from config record
 * Accepts string of format <action_name>:<command_line>
 */
BOOL AddActionFromConfig(TCHAR *pszLine, BOOL bShellExec) //to be TCHAR
{
   TCHAR *pCmdLine;

   pCmdLine = _tcschr(pszLine, _T(':'));
   if (pCmdLine == NULL)
      return FALSE;
   *pCmdLine = 0;
   pCmdLine++;
   StrStrip(pszLine);
   StrStrip(pCmdLine);
   return AddAction(pszLine, bShellExec ? AGENT_ACTION_SHELLEXEC : AGENT_ACTION_EXEC, pCmdLine, NULL, NULL, _T(""));
}

/**
 * Action executor data
 */
class ActionExecutorData
{
public:
   TCHAR *m_cmdLine;
   StringList *m_args;
   AbstractCommSession *m_session;
   UINT32 m_requestId;

   ActionExecutorData(const TCHAR *cmd, StringList *args, AbstractCommSession *session, UINT32 requestId)
   {
      m_cmdLine = _tcsdup(cmd);
      m_args = args;
      m_session = session;
      m_session->incRefCount();
      m_requestId = requestId;
   }

   ~ActionExecutorData()
   {
      m_session->decRefCount();
      free(m_cmdLine);
      delete m_args;
   }
};

/**
 * List of available actions
 */
LONG H_ActionList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   TCHAR szBuffer[1024];
   for(UINT32 i = 0; i < m_dwNumActions; i++)
   {
      _sntprintf(szBuffer, 1024, _T("%s %d \"%s\""), m_pActionList[i].szName, m_pActionList[i].iType,
                 m_pActionList[i].iType == AGENT_ACTION_SUBAGENT ?
                    m_pActionList[i].handler.sa.szSubagentName :    
                    m_pActionList[i].handler.pszCmdLine);
		value->add(szBuffer);
   }
   ListActionsFromExtSubagents(value);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Put list of supported actions into NXCP message
 */
void GetActionList(NXCPMessage *msg)
{
   UINT32 fieldId = VID_ACTION_LIST_BASE;
   for(UINT32 i = 0; i < m_dwNumActions; i++)
   {
      msg->setField(fieldId++, m_pActionList[i].szName);
      msg->setField(fieldId++, m_pActionList[i].szDescription);
      msg->setField(fieldId++, (INT16)m_pActionList[i].iType);
      msg->setField(fieldId++, (m_pActionList[i].iType == AGENT_ACTION_SUBAGENT) ? m_pActionList[i].handler.sa.szSubagentName : m_pActionList[i].handler.pszCmdLine);
   }

   UINT32 count = m_dwNumActions;
	ListActionsFromExtSubagents(msg, &fieldId, &count);
   msg->setField(VID_NUM_ACTIONS, count);
}
