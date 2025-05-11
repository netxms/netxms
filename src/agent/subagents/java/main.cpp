/* 
 ** Java-Bridge NetXMS subagent
 ** Copyright (c) 2013 TEMPEST a.s.
 ** Copyright (c) 2015-2024 Raden Solutions SIA
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
 ** File: main.cpp
 **
 **/

#include "java_subagent.h"
#include "SubAgent.h"
#include <netxms-version.h>

/**
 * Java subagent instance
 */
static SubAgent *s_subAgent = nullptr;

/**
 * Action handler
 */
static uint32_t ActionHandler(const shared_ptr<ActionExecutionContext>& context)
{
   if (s_subAgent == nullptr)
      return SYSINFO_RC_ERROR;

   nxlog_debug_tag(JAVA_AGENT_DEBUG_TAG, 6, _T("ActionHandler(action=%s, id=%s)"), context->getName(), context->getData<TCHAR>());
   uint32_t rc = s_subAgent->actionHandler(context->getName(), context->getArgs(), context->getData<TCHAR>());
   DetachThreadFromJavaVM();
   return rc;
}

/**
 * Single value parameter handlers
 */
static LONG ParameterHandler(const TCHAR *param, const TCHAR *id, TCHAR *value, AbstractCommSession *session)
{
    if (s_subAgent == nullptr)
      return SYSINFO_RC_ERROR;

   LONG rc = s_subAgent->parameterHandler(param, id, value);
   DetachThreadFromJavaVM();
   return rc;
}

/**
 * Handler for list parameters
 */
static LONG ListHandler(const TCHAR *cmd, const TCHAR *id, StringList *value, AbstractCommSession *session)
{
   if (s_subAgent == NULL)
      return SYSINFO_RC_ERROR;

   LONG rc = s_subAgent->listHandler(cmd, id, value);
   DetachThreadFromJavaVM();
   return rc;
}

/**
 * Handler for table parameters
 */
static LONG TableHandler(const TCHAR *cmd, const TCHAR *id, Table *value, AbstractCommSession *session)
{
   if (s_subAgent == nullptr)
      return SYSINFO_RC_ERROR;

   LONG rc = s_subAgent->tableHandler(cmd, id, value);
   DetachThreadFromJavaVM();
   return rc;
}

/**
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
   return s_subAgent->init();
}

/**
 * Subagent shutdown
 */
static void SubAgentShutdown()
{
   if (s_subAgent != nullptr)
   {
      s_subAgent->shutdown();
   }
   DestroyJavaVM();
   nxlog_debug_tag(JAVA_AGENT_DEBUG_TAG, 1, _T("Java VM destroyed"));
}

/**
 * JVM path
 */
#ifdef _WIN32
static TCHAR s_jvmPath[MAX_PATH] = _T("jvm.dll");
#else
static TCHAR s_jvmPath[MAX_PATH] = _T("libjvm.so");
#endif

/**
 * JVM options
 */
static TCHAR *s_jvmOptions = nullptr;

/**
 * User classpath
 */
static TCHAR *s_userClasspath = nullptr;

/**
 * Configuration template
 */
static NX_CFG_TEMPLATE s_configTemplate[] =
{
   { _T("JVM"), CT_STRING, 0, 0, MAX_PATH, 0,  s_jvmPath },
   { _T("JVMOptions"), CT_STRING_CONCAT, _T('\n'), 0, 0, 0, &s_jvmOptions },
   { _T("ClassPath"), CT_STRING_CONCAT, JAVA_CLASSPATH_SEPARATOR, 0, 0, 0, &s_userClasspath },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_subagentInfo =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("JAVA"),
   NETXMS_VERSION_STRING,
   SubAgentInit,
   SubAgentShutdown,
   NULL,                         // bool (*commandHandler)(UINT32 dwCommand, NXCPMessage *pRequest, NXCPMessage *pResponse, void *session)
   NULL,                         // bool (*notify)(UINT32 code, void *data)
   nullptr,                      // bool (*metricFilter)(const TCHAR*, const TCHAR*, AbstractCommSession*)
   0,                            // numParamaters
   NULL,                         // parameters
   0,                            // numLists
   NULL,                         // lists
   0,                            // numTables
   NULL,                         // tables
   0,                            // numActions
   NULL,                         // actions
   0,                            // numPushParameters
   NULL                          // pushParameters
};

/**
 * Add contribution items provided by subagent's plugins
 */
static void AddContributionItems()
{
   // actions
   StringList *actions = s_subAgent->getActions();
   if ((actions != nullptr) && (actions->size() > 0))
   {
      s_subagentInfo.numActions = actions->size() / 3;
      s_subagentInfo.actions = (NETXMS_SUBAGENT_ACTION *)calloc(s_subagentInfo.numActions, sizeof(NETXMS_SUBAGENT_ACTION));
      for(int i = 0, j = 0; j < (int)s_subagentInfo.numActions; j++)
      {
         s_subagentInfo.actions[j].arg = _tcsdup(actions->get(i++));
         _tcslcpy(s_subagentInfo.actions[j].name, actions->get(i++), MAX_PARAM_NAME);
         _tcslcpy(s_subagentInfo.actions[j].description, actions->get(i++), MAX_DB_STRING);
         s_subagentInfo.actions[j].handler = ActionHandler;
      }
   }
   delete actions;

   // parameters
   StringList *parameters = s_subAgent->getParameters();
   if ((parameters != nullptr) && (parameters->size() > 0))
   {
      s_subagentInfo.numParameters = parameters->size() / 4;
      s_subagentInfo.parameters = (NETXMS_SUBAGENT_PARAM *)calloc(s_subagentInfo.numParameters, sizeof(NETXMS_SUBAGENT_PARAM));
      for(int i = 0, j = 0; j < (int)s_subagentInfo.numParameters; j++)
      {
         s_subagentInfo.parameters[j].arg = _tcsdup(parameters->get(i++));
         _tcslcpy(s_subagentInfo.parameters[j].name, parameters->get(i++), MAX_PARAM_NAME);
         _tcslcpy(s_subagentInfo.parameters[j].description, parameters->get(i++), MAX_DB_STRING);
         s_subagentInfo.parameters[j].dataType = (int)_tcstol(parameters->get(i++), NULL, 10);
         s_subagentInfo.parameters[j].handler = ParameterHandler;
      }
   }
   delete parameters;

   // lists
   StringList *lists = s_subAgent->getLists();
   if ((lists != nullptr) && (lists->size() > 0))
   {
      s_subagentInfo.numLists = lists->size() / 3;
      s_subagentInfo.lists = (NETXMS_SUBAGENT_LIST *)calloc(s_subagentInfo.numLists, sizeof(NETXMS_SUBAGENT_LIST));
      for(int i = 0, j = 0; j < (int)s_subagentInfo.numLists; j++)
      {
         s_subagentInfo.lists[j].arg = _tcsdup(lists->get(i++));
         _tcslcpy(s_subagentInfo.lists[j].name, lists->get(i++), MAX_PARAM_NAME);
         _tcslcpy(s_subagentInfo.lists[j].description, lists->get(i++), MAX_DB_STRING);
         s_subagentInfo.lists[j].handler = ListHandler;
      }
   }
   delete lists;

   // push parameters
   StringList *pushParameters = s_subAgent->getPushParameters();
   if ((pushParameters != nullptr) && (pushParameters->size() > 0))
   {
      s_subagentInfo.numPushParameters = pushParameters->size() / 4;
      s_subagentInfo.pushParameters = (NETXMS_SUBAGENT_PUSHPARAM *)calloc(s_subagentInfo.numPushParameters, sizeof(NETXMS_SUBAGENT_PUSHPARAM));
      for(int i = 0, j = 0; j < (int)s_subagentInfo.numPushParameters; j++)
      {
         i++;  // skip ID
         _tcslcpy(s_subagentInfo.pushParameters[j].name, pushParameters->get(i++), MAX_PARAM_NAME);
         _tcslcpy(s_subagentInfo.pushParameters[j].description, pushParameters->get(i++), MAX_DB_STRING);
         s_subagentInfo.pushParameters[j].dataType = (int)_tcstol(pushParameters->get(i++), NULL, 10);
      }
   }
   delete pushParameters;

   // tables
   StringList *tables = s_subAgent->getTables();
   if ((tables != nullptr) && (tables->size() > 0))
   {
      s_subagentInfo.numTables = _tcstoul(tables->get(0), NULL, 10);
      s_subagentInfo.tables = (NETXMS_SUBAGENT_TABLE *)calloc(s_subagentInfo.numTables, sizeof(NETXMS_SUBAGENT_TABLE));
      for(int i = 1, j = 0; j < (int)s_subagentInfo.numTables; j++)
      {
         s_subagentInfo.tables[j].arg = _tcsdup(tables->get(i++));
         _tcslcpy(s_subagentInfo.tables[j].name, tables->get(i++), MAX_PARAM_NAME);
         _tcslcpy(s_subagentInfo.tables[j].description, tables->get(i++), MAX_DB_STRING);
         s_subagentInfo.tables[j].handler = TableHandler;
         s_subagentInfo.tables[j].numColumns = (int)_tcstol(tables->get(i++), NULL, 10);
         if (s_subagentInfo.tables[j].numColumns > 0)
         {
            s_subagentInfo.tables[j].columns = (NETXMS_SUBAGENT_TABLE_COLUMN *)calloc(s_subagentInfo.tables[j].numColumns, sizeof(NETXMS_SUBAGENT_TABLE_COLUMN));
            for(int col = 0; col < s_subagentInfo.tables[j].numColumns; col++)
            {
               _tcslcpy(s_subagentInfo.tables[j].columns[col].name, tables->get(i++), MAX_COLUMN_NAME);
               _tcslcpy(s_subagentInfo.tables[j].columns[col].displayName, tables->get(i++), MAX_COLUMN_NAME);
               s_subagentInfo.tables[j].columns[col].dataType = (int)_tcstol(tables->get(i++), NULL, 10);
               s_subagentInfo.tables[j].columns[col].isInstance = !_tcscmp(tables->get(i++), _T("T"));
               if (s_subagentInfo.tables[j].columns[col].isInstance)
               {
                  if (s_subagentInfo.tables[j].instanceColumns[0] != 0)
                     _tcscat(s_subagentInfo.tables[j].instanceColumns, _T(","));
                  _tcscat(s_subagentInfo.tables[j].instanceColumns, s_subagentInfo.tables[j].columns[col].name);
               }
            }
         }
      }
   }
   delete tables;
}

/**
 * Subagent entry point
 */
DECLARE_SUBAGENT_ENTRY_POINT(JAVA)
{
   nxlog_debug_tag(JAVA_AGENT_DEBUG_TAG, 1, _T("Initializing Java subagent"));

   // Try to set default JVM
   if (FindJavaRuntime(s_jvmPath, MAX_PATH) != nullptr)
      nxlog_debug_tag(JAVA_AGENT_DEBUG_TAG, 1, _T("Default JVM: %s"), s_jvmPath);

   if (!config->parseTemplate(_T("Java"), s_configTemplate))
   {
      nxlog_write_tag(NXLOG_ERROR, JAVA_AGENT_DEBUG_TAG, _T("Error parsing Java subagent configuration"));
      return false;
   }

   nxlog_debug_tag(JAVA_AGENT_DEBUG_TAG, 1, _T("Using JVM %s"), s_jvmPath);

   JNIEnv *env;
   JavaBridgeError err = CreateJavaVM(s_jvmPath, _T("netxms-agent-") NETXMS_JAR_VERSION _T(".jar"), nullptr, s_userClasspath, nullptr, &env);
   if (err != NXJAVA_SUCCESS)
   {
      nxlog_write_tag(NXLOG_ERROR, JAVA_AGENT_DEBUG_TAG, _T("Unable to load JVM: %s"), GetJavaBridgeErrorMessage(err));
      return false;
   }

   bool success = false;
   if (SubAgent::initialize(env))
   {
      // create an instance of org.netxms.agent.Config
      jobject jconfig = CreateConfigJavaInstance(env, config);
      if (jconfig != nullptr)
      {
         // create an instance of org.netxms.agent.SubAgent
         s_subAgent = SubAgent::createInstance(env, jconfig);
         if (s_subAgent != nullptr)
         {
            AddContributionItems();
            success = true;
         }
         else
         {
            nxlog_write_tag(NXLOG_ERROR, JAVA_AGENT_DEBUG_TAG, _T("Failed to instantiate org.netxms.agent.SubAgent"));
         }
         env->DeleteGlobalRef(jconfig);
      }
      else
      {
         nxlog_write_tag(NXLOG_ERROR, JAVA_AGENT_DEBUG_TAG, _T("Failed to instantiate org.netxms.bridge.Config"));
      }
   }

   if (!success)
   {
      DestroyJavaVM();
      return false;
   }

   *ppInfo = &s_subagentInfo;
   return true;
}
