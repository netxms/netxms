/* 
 ** Java-Bridge NetXMS subagent
 ** Copyright (c) 2013 TEMPEST a.s.
 ** Copyright (c) 2015 Raden Solutions SIA
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

/**
 * Classpath separator character
 */
#ifdef _WIN32
#define CLASSPATH_SEPARATOR   _T(';')
#else
#define CLASSPATH_SEPARATOR   _T(':')
#endif

/**
 * JVM instance
 */
static JavaVM *s_jvm = NULL;
static HMODULE s_jvmModule = NULL;
static JNIEnv *s_jniEnv = NULL;

/**
 * Java subagent instance
 */
static SubAgent *s_subAgent = NULL;

/**
 * Create global reference to Java class
 */
jclass CreateClassGlobalRef(JNIEnv *curEnv, const char *className)
{
   jclass c = curEnv->FindClass(className);
   if (c == NULL)
   {
      AgentWriteLog(NXLOG_ERROR, _T("JAVA: Could not find class %hs"), className);
      return NULL;
   }

   jclass gc = static_cast<jclass>(curEnv->NewGlobalRef(c));
   curEnv->DeleteLocalRef(c);

   if (gc == NULL)
   {
      AgentWriteLog(NXLOG_ERROR, _T("JAVA: Could not create global reference of class %s"), className);
      return NULL;
   }

   return gc;
}

/**
 * Action handler
 */
static LONG ActionHandler(const TCHAR *action, StringList *args, const TCHAR *id, AbstractCommSession *session)
{
    if (s_subAgent == NULL)
      return SYSINFO_RC_ERROR;

   AgentWriteDebugLog(6, _T("JAVA: ActionHandler(action=%s, id=%s)"), action, id);
   LONG rc = s_subAgent->actionHandler(action, args, id);
   s_jvm->DetachCurrentThread();
   return rc;
}

/**
 * Single value parameter handlers
 */
static LONG ParameterHandler(const TCHAR *param, const TCHAR *id, TCHAR *value, AbstractCommSession *session)
{
    if (s_subAgent == NULL)
      return SYSINFO_RC_ERROR;

   LONG rc = s_subAgent->parameterHandler(param, id, value);
   s_jvm->DetachCurrentThread();
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
   s_jvm->DetachCurrentThread();
   return rc;
}

/**
 * Handler for table parameters
 */
static LONG TableHandler(const TCHAR *cmd, const TCHAR *id, Table *value, AbstractCommSession *session)
{
   if (s_subAgent == NULL)
      return SYSINFO_RC_ERROR;

   LONG rc = s_subAgent->tableHandler(cmd, id, value);
   s_jvm->DetachCurrentThread();
   return rc;
}

/**
 * Subagent initialization
 */
static BOOL SubAgentInit(Config *config)
{
   jobject jconfig = CreateConfigInstance(s_jniEnv, config);
   if (jconfig == NULL)
   {
      return FALSE;
   }
   bool success = s_subAgent->init(jconfig);
   s_jniEnv->DeleteGlobalRef(jconfig);
   return success ? TRUE : FALSE;
}

/**
 * Subagent shutdown
 */
static void SubAgentShutdown()
{
   if (s_subAgent != NULL)
   {
      s_subAgent->shutdown();
   }

   if (s_jvm != NULL)
   {
      AgentWriteDebugLog(6, _T("JAVA: destroying Java VM"));
      s_jvm->DestroyJavaVM();
      s_jvm = NULL;
   }

   if (s_jvmModule != NULL)
   {
      AgentWriteDebugLog(6, _T("JAVA: unloading JVM library"));
      DLClose(s_jvmModule);
      s_jvmModule = NULL;
   }

   AgentWriteDebugLog(1, _T("JAVA: shutdown comple"));
}

/**
 * Prototype for JNI_CreateJavaVM
 */
typedef jint (JNICALL *T_JNI_CreateJavaVM)(JavaVM **, void **, void *);

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
static TCHAR *s_jvmOptions = NULL;

/**
 * User classpath
 */
static TCHAR *s_userClasspath = NULL;

/**
 * Configuration template
 */
static NX_CFG_TEMPLATE s_configTemplate[] =
{
   { _T("JVM"), CT_STRING, 0, 0, MAX_PATH, 0,  s_jvmPath },
   { _T("JVMOptions"), CT_STRING_LIST, _T('\n'), 0, 0, 0, &s_jvmOptions },
   { _T("ClassPath"), CT_STRING_LIST, CLASSPATH_SEPARATOR, 0, 0, 0, &s_userClasspath },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/**
 * Sabagent information
 */
static NETXMS_SUBAGENT_INFO s_subagentInfo =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("JAVA"),
   NETXMS_VERSION_STRING,
   SubAgentInit,
   SubAgentShutdown,
   NULL,                         // BOOL (*commandhandler)(UINT32 dwCommand, NXCPMessage *pRequest, NXCPMessage *pResponse, void *session)
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
   if ((actions != NULL) && (actions->size() > 0))
   {
      s_subagentInfo.numActions = actions->size() / 3;
      s_subagentInfo.actions = (NETXMS_SUBAGENT_ACTION *)calloc(s_subagentInfo.numActions, sizeof(NETXMS_SUBAGENT_ACTION));
      for(int i = 0, j = 0; j < (int)s_subagentInfo.numActions; j++)
      {
         s_subagentInfo.actions[j].arg = _tcsdup(actions->get(i++));
         nx_strncpy(s_subagentInfo.actions[j].name, actions->get(i++), MAX_PARAM_NAME);
         nx_strncpy(s_subagentInfo.actions[j].description, actions->get(i++), MAX_DB_STRING);
         s_subagentInfo.actions[j].handler = ActionHandler;
      }
   }
   delete actions;

   // parameters
   StringList *parameters = s_subAgent->getParameters();
   if ((parameters != NULL) && (parameters->size() > 0))
   {
      s_subagentInfo.numParameters = parameters->size() / 4;
      s_subagentInfo.parameters = (NETXMS_SUBAGENT_PARAM *)calloc(s_subagentInfo.numParameters, sizeof(NETXMS_SUBAGENT_PARAM));
      for(int i = 0, j = 0; j < (int)s_subagentInfo.numParameters; j++)
      {
         s_subagentInfo.parameters[j].arg = _tcsdup(parameters->get(i++));
         nx_strncpy(s_subagentInfo.parameters[j].name, parameters->get(i++), MAX_PARAM_NAME);
         nx_strncpy(s_subagentInfo.parameters[j].description, parameters->get(i++), MAX_DB_STRING);
         s_subagentInfo.parameters[j].dataType = (int)_tcstol(parameters->get(i++), NULL, 10);
         s_subagentInfo.parameters[j].handler = ParameterHandler;
      }
   }
   delete parameters;

   // lists
   StringList *lists = s_subAgent->getLists();
   if ((lists != NULL) && (lists->size() > 0))
   {
      s_subagentInfo.numLists = lists->size() / 3;
      s_subagentInfo.lists = (NETXMS_SUBAGENT_LIST *)calloc(s_subagentInfo.numLists, sizeof(NETXMS_SUBAGENT_LIST));
      for(int i = 0, j = 0; j < (int)s_subagentInfo.numLists; j++)
      {
         s_subagentInfo.lists[j].arg = _tcsdup(lists->get(i++));
         nx_strncpy(s_subagentInfo.lists[j].name, lists->get(i++), MAX_PARAM_NAME);
         nx_strncpy(s_subagentInfo.lists[j].description, lists->get(i++), MAX_DB_STRING);
         s_subagentInfo.lists[j].handler = ListHandler;
      }
   }
   delete lists;

   // push parameters
   StringList *pushParameters = s_subAgent->getPushParameters();
   if ((pushParameters != NULL) && (pushParameters->size() > 0))
   {
      s_subagentInfo.numPushParameters = pushParameters->size() / 4;
      s_subagentInfo.pushParameters = (NETXMS_SUBAGENT_PUSHPARAM *)calloc(s_subagentInfo.numPushParameters, sizeof(NETXMS_SUBAGENT_PUSHPARAM));
      for(int i = 0, j = 0; j < (int)s_subagentInfo.numPushParameters; j++)
      {
         i++;  // skip ID
         nx_strncpy(s_subagentInfo.pushParameters[j].name, pushParameters->get(i++), MAX_PARAM_NAME);
         nx_strncpy(s_subagentInfo.pushParameters[j].description, pushParameters->get(i++), MAX_DB_STRING);
         s_subagentInfo.pushParameters[j].dataType = (int)_tcstol(pushParameters->get(i++), NULL, 10);
      }
   }
   delete pushParameters;

   // tables
   StringList *tables = s_subAgent->getTables();
   if ((tables != NULL) && (tables->size() > 0))
   {
      s_subagentInfo.numTables = _tcstoul(tables->get(0), NULL, 10);
      s_subagentInfo.tables = (NETXMS_SUBAGENT_TABLE *)calloc(s_subagentInfo.numTables, sizeof(NETXMS_SUBAGENT_TABLE));
      for(int i = 1, j = 0; j < (int)s_subagentInfo.numTables; j++)
      {
         s_subagentInfo.tables[j].arg = _tcsdup(tables->get(i++));
         nx_strncpy(s_subagentInfo.tables[j].name, tables->get(i++), MAX_PARAM_NAME);
         nx_strncpy(s_subagentInfo.tables[j].description, tables->get(i++), MAX_DB_STRING);
         s_subagentInfo.tables[j].handler = TableHandler;
         s_subagentInfo.tables[j].numColumns = (int)_tcstol(tables->get(i++), NULL, 10);
         if (s_subagentInfo.tables[j].numColumns > 0)
         {
            s_subagentInfo.tables[j].columns = (NETXMS_SUBAGENT_TABLE_COLUMN *)calloc(s_subagentInfo.tables[j].numColumns, sizeof(NETXMS_SUBAGENT_TABLE_COLUMN));
            for(int col = 0; col < s_subagentInfo.tables[j].numColumns; col++)
            {
               nx_strncpy(s_subagentInfo.tables[j].columns[col].name, tables->get(i++), MAX_COLUMN_NAME);
               nx_strncpy(s_subagentInfo.tables[j].columns[col].displayName, tables->get(i++), MAX_COLUMN_NAME);
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
   AgentWriteDebugLog(1, _T("Initializing Java subagent"));

   // Try to set default JVM
#ifdef _WIN32
   const TCHAR *javaHome = _tgetenv(_T("JAVA_HOME"));
   if ((javaHome != NULL) && (*javaHome != 0))
   {
      _sntprintf(s_jvmPath, MAX_PATH, _T("%s\\bin\\server\\jvm.dll"), javaHome);
      if (_taccess(s_jvmPath, 0) != 0)
      {
         _sntprintf(s_jvmPath, MAX_PATH, _T("%s\\jre\\bin\\server\\jvm.dll"), javaHome);
      }
      AgentWriteDebugLog(1, _T("JAVA: Default JVM set from JAVA_HOME: %s"), s_jvmPath);
   }
#endif

   if (!config->parseTemplate(_T("Java"), s_configTemplate))
   {
      AgentWriteLog(NXLOG_ERROR, _T("JAVA: error parsing configuration"));
      return FALSE;
   }

   AgentWriteDebugLog(1, _T("JAVA: using JVM %s"), s_jvmPath);
   
   TCHAR errorText[256];
   s_jvmModule = DLOpen(s_jvmPath, errorText);
   if (s_jvmModule == NULL)
   {
      AgentWriteLog(NXLOG_ERROR, _T("JAVA: Unable to load JVM: %s"), errorText);
      return FALSE;
   }

   BOOL success = FALSE;

   JavaVMInitArgs vmArgs;
   JavaVMOption vmOptions[1];
   memset(vmOptions, 0, sizeof(vmOptions));

   TCHAR libdir[MAX_PATH];
   GetNetXMSDirectory(nxDirLib, libdir);

   String classpath = _T("-Djava.class.path=");
   classpath.append(libdir);
   classpath.append(FS_PATH_SEPARATOR_CHAR);
   classpath.append(_T("netxms-agent.jar"));
   if (s_userClasspath != NULL)
   {
      classpath.append(CLASSPATH_SEPARATOR);
      classpath.append(s_userClasspath);
      free(s_userClasspath);
      s_userClasspath = NULL;
   }

#ifdef UNICODE
   vmOptions[0].optionString = classpath.getUTF8String();
#else
   vmOptions[0].optionString = strdup(classpath);
#endif

   // TODO JVM options

   vmArgs.version = JNI_VERSION_1_6;
   vmArgs.options = vmOptions;
   vmArgs.nOptions = 1;
   vmArgs.ignoreUnrecognized = JNI_TRUE;

   AgentWriteDebugLog(6, _T("JVM options:"));
   for(int i = 0; i < vmArgs.nOptions; i++)
      AgentWriteDebugLog(6, _T("    %hs"), vmArgs.options[i].optionString);

   T_JNI_CreateJavaVM CreateJavaVM = (T_JNI_CreateJavaVM)DLGetSymbolAddr(s_jvmModule, "JNI_CreateJavaVM", NULL);
   if (CreateJavaVM != NULL)
   {
      if (CreateJavaVM(&s_jvm, (void **)&s_jniEnv, &vmArgs) == JNI_OK)
      {
         AgentWriteDebugLog(6, _T("Java VM created"));
         if (SubAgent::initialize(s_jniEnv) && RegisterConfigHelperNatives(s_jniEnv))
         {
            // create an instance of org.netxms.agent.Config
            jobject jconfig = CreateConfigInstance(s_jniEnv, config);
            if (jconfig != NULL)
            {
               // create an instance of org.netxms.agent.SubAgent
               s_subAgent = SubAgent::createInstance(s_jvm, s_jniEnv, jconfig);
               if (s_subAgent != NULL)
               {
                  AddContributionItems();
                  success = TRUE;
               }
               else
               {
                  AgentWriteLog(NXLOG_ERROR, _T("JAVA: Failed to instantiate org.netxms.agent.SubAgent"));
               }
               s_jniEnv->DeleteGlobalRef(jconfig);
            }
            else
            {
               AgentWriteLog(NXLOG_ERROR, _T("JAVA: Failed to instantiate org.netxms.agent.Config"));
            }
         }
      }
      else
      {
         AgentWriteLog(NXLOG_ERROR, _T("JAVA: CreateJavaVM failed"));
      }
   }
   else
   {
      AgentWriteLog(NXLOG_ERROR, _T("JAVA: JNI_CreateJavaVM failed"));
   }

   if (!success)
   {
      if (s_jvm != NULL)
         s_jvm->DestroyJavaVM();

      if (s_jvmModule != NULL)
         DLClose(s_jvmModule);
      return FALSE;
   }

   *ppInfo = &s_subagentInfo;
   return TRUE;
}
