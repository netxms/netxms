/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2025 Victor Kirhenshtein
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

#define DEBUG_TAG _T("actions")

/**
 * External action executor
 */
class ExternalActionExecutor : public ProcessExecutor
{
private:
   uint32_t m_requestId;
   shared_ptr<AbstractCommSession> m_session;

protected:
   virtual void onOutput(const char *text, size_t length) override;
   virtual void endOfOutput() override;

public:
   ExternalActionExecutor(const TCHAR *command, const StringList& args, const shared_ptr<AbstractCommSession>& session, uint32_t requestId, bool sendOutput);
   virtual ~ExternalActionExecutor();
};

/**
 * Configured actions
 */
static StructArray<ACTION> s_actions(0, 16);

/**
 * Find action by name
 */
static ACTION *FindAction(const TCHAR *name)
{
   for(int i = 0; i < s_actions.size(); i++)
   {
      if (!_tcsicmp(s_actions.get(i)->name, name))
         return s_actions.get(i);
   }
   return nullptr;
}

/**
 * Add action
 */
bool AddAction(const TCHAR *name, bool isExternal, const void *arg,
         uint32_t (*handler)(const shared_ptr<ActionExecutionContext>&),
         const TCHAR *subAgent, const TCHAR *description)
{
   ACTION *action = FindAction(name);
   if (action != nullptr)
   {
      // Update existing entry in action list
      _tcslcpy(action->description, description, MAX_DB_STRING);
      if (action->isExternal)
         MemFree(action->handler.cmdLine);
      action->isExternal = isExternal;
      if (isExternal)
      {
         action->handler.cmdLine = MemCopyString((const TCHAR*)arg);
      }
      else
      {
         action->handler.sa.handler = handler;
         action->handler.sa.arg = arg;
         action->handler.sa.subagentName = subAgent;
      }
   }
   else
   {
      // Create new entry in action list
      ACTION newAction;
      _tcslcpy(newAction.name, name, MAX_PARAM_NAME);
      newAction.isExternal = isExternal;
      _tcslcpy(newAction.description, description, MAX_DB_STRING);
      if (isExternal)
      {
         newAction.handler.cmdLine = MemCopyString(static_cast<const TCHAR*>(arg));
      }
      else
      {
         newAction.handler.sa.handler = handler;
         newAction.handler.sa.arg = arg;
         newAction.handler.sa.subagentName = subAgent;
      }
      s_actions.add(newAction);
   }
   return true;
}

/**
 * Add action from config record
 * Accepts string of format <action_name>:<command_line>
 */
bool AddActionFromConfig(const TCHAR *config)
{
   StringBuffer sb(config);
   ssize_t index = sb.find(_T(":"));
   if (index == String::npos)
      return false;

   TCHAR *name = Trim(sb.substring(0, index, nullptr));
   TCHAR *cmdLine = Trim(sb.substring(index + 1, -1, nullptr));
   bool result = AddAction(name, true, cmdLine, nullptr, nullptr, _T(""));
   MemFree(name);
   MemFree(cmdLine);
   return result;
}

/**
 * List of available actions
 */
LONG H_ActionList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   TCHAR buffer[1024];
   for(int i = 0; i < s_actions.size(); i++)
   {
      ACTION *action = s_actions.get(i);
      _sntprintf(buffer, 1024, _T("%s %s \"%s\""), action->name, action->isExternal ? _T("external") : _T("internal"),
            action->isExternal ? action->handler.cmdLine : action->handler.sa.subagentName);
      value->add(buffer);
   }
   ListActionsFromExtSubagents(value);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Put list of supported actions into NXCP message
 */
void GetActionList(NXCPMessage *msg)
{
   uint32_t fieldId = VID_ACTION_LIST_BASE;
   for(int i = 0; i < s_actions.size(); i++)
   {
      ACTION *action = s_actions.get(i);
      msg->setField(fieldId++, action->name);
      msg->setField(fieldId++, action->description);
      msg->setField(fieldId++, action->isExternal);
      msg->setField(fieldId++, action->isExternal ? action->handler.cmdLine : action->handler.sa.subagentName);
   }

   uint32_t count = static_cast<uint32_t>(s_actions.size());
   ListActionsFromExtSubagents(msg, &fieldId, &count);
   msg->setField(VID_NUM_ACTIONS, count);
}

/**
 * Delete executor after completion or timeout
 */
static void ExecutorCleanup(ExternalActionExecutor *executor)
{
   if (executor->isRunning())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Force terminate external action %s"), executor->getCommand());
      executor->stop();
   }
   delete executor;
   nxlog_debug_tag(DEBUG_TAG, 7, _T("ExecutorCleanup call completed"));
}

/**
 * Wrapper for internal action handlers
 */
static void InternalActionExecutor(uint32_t (*handler)(const shared_ptr<ActionExecutionContext>&), shared_ptr<ActionExecutionContext> context)
{
   context->markAsCompleted(handler(context));
}

/**
 * Execute action
 */
static uint32_t ExecuteAction(const TCHAR *name, const StringList& args, const shared_ptr<AbstractCommSession>& session, uint32_t requestId, bool sendOutput)
{
   ACTION *action = FindAction(name);
   if (action == nullptr)
      return ExecuteActionByExtSubagent(name, args, session, requestId, sendOutput);

   uint32_t rcc;
   if (action->isExternal)
   {
      ExternalActionExecutor *executor = new ExternalActionExecutor(action->handler.cmdLine, args, session, requestId, sendOutput);
      if (executor->execute())
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Execution of external action %s (%s) started"), name, executor->getCommand());
         ThreadPoolScheduleRelative(g_executorThreadPool, g_externalCommandTimeout, ExecutorCleanup, executor);
         rcc = ERR_SUCCESS;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Execution of external action %s (%s) failed"), name, executor->getCommand());
         delete executor;
         rcc = ERR_EXEC_FAILED;
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Executing internal action %s"), name);
      auto context = make_shared<ActionExecutionContext>(name, args, action->handler.sa.arg, session, requestId, sendOutput);
      ThreadCreate(InternalActionExecutor, action->handler.sa.handler, context);
      rcc = context->waitForCompletion();
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Internal action execution result: %d"), rcc);
   }
   return rcc;
}

/**
 * Execute action (external interface)
 */
void ExecuteAction(const NXCPMessage& request, NXCPMessage *response, const shared_ptr<AbstractCommSession>& session)
{
   TCHAR name[MAX_PARAM_NAME];
   request.getFieldAsString(VID_ACTION_NAME, name, MAX_PARAM_NAME);
   StringList args(request, VID_ACTION_ARG_BASE, VID_NUM_ARGS);
   uint32_t rcc = ExecuteAction(name, args, session, request.getId(), request.getFieldAsBoolean(VID_RECEIVE_OUTPUT));
   nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteAction(%s): requestId=%u, RCC=%u"), name, request.getId(), rcc);
   response->setField(VID_RCC, rcc);
}

/**
 * Execute action (internal interface)
 */
void ExecuteAction(const TCHAR *name, const StringList& args)
{
   shared_ptr<VirtualSession> session = MakeSharedCommSession<VirtualSession>(0);
   uint32_t rcc = ExecuteAction(name, args, session, 0, false);
   nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteAction(%s): RCC=%u"), name, rcc);
}

/**
 * Expand macros in action's command line
 */
static StringBuffer ExpandCommandLineMacros(const TCHAR *command, const StringList& args)
{
   StringBuffer sb(command);
   TCHAR macro[3] = _T("$0");
   for(int i = 0; (i < args.size()) && (i <= 9); i++)
   {
      macro[1] = static_cast<TCHAR>(i) + 1 + '0';
      sb.replace(macro, args.get(i));
   }
   return sb;
}

/**
 * External action executor constructor
 */
ExternalActionExecutor::ExternalActionExecutor(const TCHAR *command, const StringList& args, const shared_ptr<AbstractCommSession>& session,
      uint32_t requestId, bool sendOutput) : ProcessExecutor(ExpandCommandLineMacros(command, args), true), m_session(session)
{
   m_sendOutput = sendOutput;
   m_replaceNullCharacters = true;
   m_requestId = requestId;
}

/**
 * External action executor destructor
 */
ExternalActionExecutor::~ExternalActionExecutor()
{
}

/**
 * Send output to console
 */
void ExternalActionExecutor::onOutput(const char *text, size_t length)
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
void ExternalActionExecutor::endOfOutput()
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("ExternalActionExecutor: end of output for command %s"), m_cmd);
   NXCPMessage msg(m_session->getProtocolVersion());
   msg.setId(m_requestId);
   msg.setCode(CMD_COMMAND_OUTPUT);
   msg.setEndOfSequence();
   m_session->sendMessage(&msg);
}
