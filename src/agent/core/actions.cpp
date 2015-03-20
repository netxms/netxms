/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
 * Add action
 */
BOOL AddAction(const TCHAR *pszName, int iType, const TCHAR *pArg,
               LONG (*fpHandler)(const TCHAR *, StringList *, const TCHAR *, AbstractCommSession *),
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
      nx_strncpy(m_pActionList[i].szName, pszName, MAX_PARAM_NAME);
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
            nx_strncpy(m_pActionList[i].handler.sa.szSubagentName, pszSubAgent,MAX_PATH);
            break;
         default:
            break;
      }
   }
   else
   {
      // Update existing entry in action list
      nx_strncpy(m_pActionList[i].szDescription, pszDescription, MAX_DB_STRING);
      if ((m_pActionList[i].iType == AGENT_ACTION_EXEC) || (m_pActionList[i].iType == AGENT_ACTION_SHELLEXEC))
         safe_free(m_pActionList[i].handler.pszCmdLine);
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
            nx_strncpy(m_pActionList[i].handler.sa.szSubagentName, pszSubAgent,MAX_PATH);
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
 * Execute action
 */
UINT32 ExecAction(const TCHAR *action, StringList *args, AbstractCommSession *session)
{
   UINT32 rcc = ERR_UNKNOWN_PARAMETER;

   for(UINT32 i = 0; i < m_dwNumActions; i++)
      if (!_tcsicmp(m_pActionList[i].szName, action))
      {
         DebugPrintf(INVALID_INDEX, 4, _T("Executing action %s of type %d"), action, m_pActionList[i].iType);
         switch(m_pActionList[i].iType)
         {
            case AGENT_ACTION_EXEC:
               rcc = ExecuteCommand(m_pActionList[i].handler.pszCmdLine, args, NULL);
               break;
            case AGENT_ACTION_SHELLEXEC:
               rcc = ExecuteShellCommand(m_pActionList[i].handler.pszCmdLine, args);
               break;
            case AGENT_ACTION_SUBAGENT:
               rcc = m_pActionList[i].handler.sa.fpHandler(action, args, m_pActionList[i].handler.sa.pArg, session);
               break;
            default:
               rcc = ERR_NOT_IMPLEMENTED;
               break;
         }
         break;
      }
   return rcc;
}

/**
 * Action executor data
 */
class ActionExecutorData
{
public:
   TCHAR *m_cmdLine;
   StringList *m_args;
   CommSession *m_session;
   UINT32 m_requestId;

   ActionExecutorData(const TCHAR *cmd, StringList *args, CommSession *session, UINT32 requestId)
   {
      m_cmdLine = _tcsdup(cmd);
      m_args = args;
      m_session = session;
      m_requestId = requestId;
   }

   ~ActionExecutorData()
   {
      free(m_cmdLine);
      delete m_args;
   }
};

/**
 * Action executor (for actions with output)
 */
static THREAD_RESULT THREAD_CALL ActionExecutionThread(void *arg)
{
   ActionExecutorData *data = (ActionExecutorData *)arg;

   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(data->m_requestId);
   msg.setField(VID_RCC, ERR_SUCCESS);
   data->m_session->sendMessage(&msg);

   msg.setCode(CMD_COMMAND_OUTPUT);
   msg.deleteAllFields();

   DebugPrintf(INVALID_INDEX, 4, _T("ActionExecutionThread: Expanding command \"%s\""), data->m_cmdLine);

   // Substitute $1 .. $9 with actual arguments
   if (data->m_args != NULL)
   {
      String cmdLine;
      for(const TCHAR *sptr = data->m_cmdLine; *sptr != 0; sptr++)
         if (*sptr == _T('$'))
         {
            sptr++;
            if (*sptr == 0)
               break;   // Single $ character at the end of line
            if ((*sptr >= _T('1')) && (*sptr <= _T('9')))
            {
               int argNum = *sptr - _T('1');
               if (argNum < data->m_args->size())
               {
                  cmdLine.append(data->m_args->get(argNum));
               }
            }
            else
            {
               cmdLine.append(*sptr);
            }
         }
         else
         {
            cmdLine.append(*sptr);
         }
      free(data->m_cmdLine);
      data->m_cmdLine = _tcsdup(cmdLine.getBuffer());
   }

   DebugPrintf(INVALID_INDEX, 4, _T("ActionExecutionThread: Executing \"%s\""), data->m_cmdLine);
   FILE *pipe = _tpopen(data->m_cmdLine, _T("r"));
   if (pipe != NULL)
   {
      while (true)
      {
         TCHAR line[4096];

         TCHAR *ret = safe_fgetts(line, 4096, pipe);
         if (ret == NULL)
            break;

         msg.setField(VID_MESSAGE, line);
         data->m_session->sendMessage(&msg);
         msg.deleteAllFields();
      }
      pclose(pipe);
   }
   else
   {
      DebugPrintf(data->m_session->getIndex(), 4, _T("ActionExecutionThread: popen() failed"));

      TCHAR buffer[1024];
      _sntprintf(buffer, 1024, _T("Failed to execute command %s"), data->m_cmdLine);
      msg.setField(VID_MESSAGE, buffer);
   }

   msg.setEndOfSequence();
   data->m_session->sendMessage(&msg);

   delete data;
   return THREAD_OK;
}

/**
 * Execute action and send output to server
 */
UINT32 ExecActionWithOutput(CommSession *session, UINT32 requestId, const TCHAR *action, StringList *args)
{
   UINT32 rcc = ERR_UNKNOWN_PARAMETER;

   for(UINT32 i = 0; i < m_dwNumActions; i++)
   {
      if (!_tcsicmp(m_pActionList[i].szName, action))
      {
         DebugPrintf(INVALID_INDEX, 4, _T("Executing action %s of type %d with output"), action, m_pActionList[i].iType);
         switch(m_pActionList[i].iType)
         {
            case AGENT_ACTION_EXEC:
            case AGENT_ACTION_SHELLEXEC:
               ThreadCreate(ActionExecutionThread, 0, new ActionExecutorData(m_pActionList[i].handler.pszCmdLine, args, session, requestId));
               rcc = ERR_SUCCESS;
               break;
            case AGENT_ACTION_SUBAGENT:
               rcc = ERR_INTERNAL_ERROR;
               break;
            default:
               rcc = ERR_NOT_IMPLEMENTED;
               break;
         }
         break;
      }
   }
   if (rcc != ERR_SUCCESS)
      delete args;
   return rcc;
}

/**
 * List of available actions
 */
LONG H_ActionList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   UINT32 i;
   TCHAR szBuffer[1024];

   for(i = 0; i < m_dwNumActions; i++)
   {
      _sntprintf(szBuffer, 1024, _T("%s %d \"%s\""), m_pActionList[i].szName, m_pActionList[i].iType,
                 m_pActionList[i].iType == AGENT_ACTION_EXEC ?
                     m_pActionList[i].handler.pszCmdLine :
                     m_pActionList[i].handler.sa.szSubagentName);
		value->add(szBuffer);
   }
   return SYSINFO_RC_SUCCESS;
}
