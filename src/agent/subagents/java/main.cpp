/* 
 ** Java-Bridge NetXMS subagent
 ** Copyright (C) 2013 TEMPEST a.s.
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
#include "ConfigHelper.h"
#include "JNIException.h"

#define MAX_STR   (256)
#define MAX_LONG_STR (1024)

using namespace org_netxms_agent;
using namespace std;

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
 * Subagent initialization
 */
static BOOL SubAgentInit(Config *config)
{
   s_subAgent->init(ConfigHelper::createInstance(s_jniEnv, config));
   return TRUE;
}

/**
 * Subagent shutdown
 */
static void SubAgentShutdown()
{
   try
   {
      s_subAgent->shutdown();
   }
   catch(JNIException const& e)
   {
      AgentWriteDebugLog(3, _T("JAVA: JNI exception in SubAgentShutdown(): %hs"), e.what());
   }

   if (s_jvm != NULL)
   {
      s_jvm->DestroyJavaVM();
      s_jvm = NULL;
   }

   if (s_jvmModule != NULL)
   {
      DLClose(s_jvmModule);
      s_jvmModule = NULL;
   }

   AgentWriteDebugLog(1, _T("JAVA: shutdown comple"));
}

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
 * Action handler
 */
static LONG ActionHandler(const TCHAR *action, StringList *pArgList, const TCHAR *id, AbstractCommSession *session)
{
   LONG result = SYSINFO_RC_SUCCESS;
   // route the call to SubAgent
   AgentWriteDebugLog(6, _T("JAVA: ActionHandler(action=%s, id=%s)"), action, id);
   int len = pArgList->size();
   TCHAR const** args = new TCHAR const* [len];
   for (int i=0; i < len; i++)
   {
      args[i] = pArgList->get(i);
   }

   try
   {
      s_subAgent->actionHandler(action, args, len, id);
   }
   catch (JNIException const& e)
   {
      AgentWriteDebugLog(3, _T("JAVA: JNI exception in ActionHandler(%s): %hs"), action, e.what());
      result = SYSINFO_RC_ERROR;
   }
   free(args);
   return result;
}

/**
 * Single value parameter handlers
 */
static LONG ParameterHandler(const TCHAR *param, const TCHAR *id, TCHAR *value, AbstractCommSession *session)
{
   try
   {
      TCHAR *resultString = s_subAgent->parameterHandler(param, id);
      nx_strncpy(value, resultString, MAX_RESULT_LENGTH);
      free(resultString);
      return SYSINFO_RC_SUCCESS;
   }
   catch (JNIException const& e)
   {
      AgentWriteDebugLog(3, _T("JAVA: JNI exception in ParameterHandler(%s): %hs"), param, e.what());
      return SYSINFO_RC_ERROR;
   }
}

/**
 * Handler for list parameters
 */
static LONG ListParameterHandler(const TCHAR *cmd, const TCHAR *id, StringList *value, AbstractCommSession *session)
{
   try
   {
      // route the call to SubAgent
      int len = 0;
      TCHAR **res = s_subAgent->listParameterHandler(cmd, id, &len);
      for (int i=0; i<len; i++)
      {
         value->addPreallocated(res[i]);
      }
      return SYSINFO_RC_SUCCESS;
   }
   catch (JNIException const& e)
   {
      AgentWriteDebugLog(3, _T("JAVA: JNI exception in ListParameterHandler(%s): %hs"), cmd, e.what());
      return SYSINFO_RC_ERROR;
   }
}

/**
 * Handler for table parameters
 */
static LONG TableParameterHandler(const TCHAR *cmd, const TCHAR *id, Table *table, AbstractCommSession *session)
{
   try
   {
      // route the call to SubAgent
      // retrieve the TableColumn descriptors first and populate resulting table's columns
      int numTableColumns = 0;
      TableColumn **tableColumns = s_subAgent->getTableParameter(id).getColumns(&numTableColumns);
      for (int i=0; i<numTableColumns; i++)
      {
         table->addColumn(tableColumns[i]->getName(), tableColumns[i]->getType().getValue(), NULL, tableColumns[i]->isInstance());
      }

      // retrieve actuall values and fill them into resulting table
      int numColumns = 0;
      int numRows = 0;
      TCHAR ***res = s_subAgent->tableParameterHandler(cmd, id, &numRows, &numColumns);
      // TODO assert that numTableColumns == numColumns
      for (int i=0; i<numRows; i++)
      {
         table->addRow();
         for (int j=0; j<numColumns; j++)
         {
            table->setPreallocatedAt(i, j, res[i][j]);
         }
      }
      return SYSINFO_RC_SUCCESS;
   }
   catch (JNIException const& e)
   {
      AgentWriteDebugLog(3, _T("JAVA: JNI exception in TableParameterHandler(%s): %hs"), cmd, e.what());
      return SYSINFO_RC_ERROR;
   }
}

/**
 * Class:     org_netxms_agent_SubAgent
 * Method:    AgentGetParameterArg
 * Signature: (Ljava/lang/String;I)Ljava/lang/String;
 */
static jstring JNICALL Java_org_netxms_agent_SubAgent_AgentGetParameterArg(JNIEnv *jenv, jclass jcls, jstring jparam, jint jindex)
{
   jstring jresult = NULL;
   if (jparam)
   {
      TCHAR *param = CStringFromJavaString(jenv, jparam);
      TCHAR arg[MAX_PATH];
      if (AgentGetParameterArg(param, (int)jindex, arg, MAX_PATH))
      {
         jresult = JavaStringFromCString(jenv, arg);
      }
      free(param);
   }
   return jresult;
}

/**
 * Class:     org_netxms_agent_SubAgent
 * Method:    AgentSendTrap
 * Signature: (ILjava/lang/String;[Ljava/lang/String;)V
 */
static void JNICALL Java_org_netxms_agent_SubAgent_AgentSendTrap(JNIEnv *jenv, jclass jcls, jint event, jstring jname, jobjectArray jargs)
{
   if ((jname != NULL) && (jargs != NULL))
   {
      TCHAR *name = CStringFromJavaString(jenv, jname);
      int numArgs = jenv->GetArrayLength(jargs);

      TCHAR **arrayOfString = (TCHAR **)malloc(sizeof(TCHAR *) * numArgs);
      for(jsize i = 0; i < numArgs; i++)
      {
         jstring resString = reinterpret_cast<jstring>(jenv->GetObjectArrayElement(jargs, i));
         arrayOfString[i] = CStringFromJavaString(jenv, resString);
         jenv->DeleteLocalRef(resString);
      }
      AgentSendTrap2((UINT32)event, name, numArgs, arrayOfString);
      free(name);
      for(jsize i = 0; i < numArgs; i++)
         safe_free(arrayOfString[i]);
      safe_free(arrayOfString);
   }
}

/**
 * Class:     org_netxms_agent_SubAgent
 * Method:    AgentPushParameterData
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
static jboolean JNICALL Java_org_netxms_agent_SubAgent_AgentPushParameterData (JNIEnv *jenv, jclass jcls, jstring jname, jstring jvalue)
{
   jboolean res = false;
   if ((jname != NULL) && (jvalue != NULL))
   {
      TCHAR *name = CStringFromJavaString(jenv, jname);
      TCHAR *value = CStringFromJavaString(jenv, jvalue);
      res = (jboolean)AgentPushParameterData(name, value);
      free(name);
      free(value);
   }
   return res;
}

/**
 * Class:     org_netxms_agent_SubAgent
 * Method:    AgentWriteLog
 * Signature: (ILjava/lang/String;)V
 */
static void JNICALL Java_org_netxms_agent_SubAgent_AgentWriteLog (JNIEnv *jenv, jclass jcls, jint level, jstring jmessage)
{
   if (jmessage != NULL)
   {
      TCHAR *message = CStringFromJavaString(jenv, jmessage);
      AgentWriteLog((int)level, message);
      free(message);
   }
}

/**
 * Class:     org_netxms_agent_SubAgent
 * Method:    AgentWriteDebugLog
 * Signature: (ILjava/lang/String;)V
 */
static void JNICALL Java_org_netxms_agent_SubAgent_AgentWriteDebugLog (JNIEnv *jenv, jclass jcls, jint level, jstring jmessage)
{
   if (jmessage != NULL)
   {
      TCHAR *message = CStringFromJavaString(jenv, jmessage);
      AgentWriteDebugLog((int)level, message);
      free(message);
   }
}

/**
 * Native methods
 */
static JNINativeMethod s_jniNativeMethods[] =
{
   { "AgentGetParameterArg", "(Ljava/lang/String;I)Ljava/lang/String;", (void *) Java_org_netxms_agent_SubAgent_AgentGetParameterArg },
   { "AgentPushParameterData", "(Ljava/lang/String;Ljava/lang/String;)Z", (void *) Java_org_netxms_agent_SubAgent_AgentPushParameterData },
   { "AgentWriteLog", "(ILjava/lang/String;)V", (void *) Java_org_netxms_agent_SubAgent_AgentWriteLog },
   { "AgentWriteDebugLog", "(ILjava/lang/String;)V", (void *) Java_org_netxms_agent_SubAgent_AgentWriteDebugLog }
};

/**
 * Regsiter native methods
 */
static bool RegisterNatives(JNIEnv *curEnv)
{
   // register native methods exposed by Agent (see SubAgent.java implemented in org_netxms_agent_SubAgent.cpp)
   jclass clazz = curEnv->FindClass(SubAgent::className());
   if (clazz != NULL)
   {
      if (curEnv->RegisterNatives(clazz, s_jniNativeMethods, (jint)(sizeof(s_jniNativeMethods) / sizeof(JNINativeMethod))) == 0)
      {
      }
      else
      {
         AgentWriteLog(NXLOG_ERROR, _T("JAVA: Failed to register native methods"));
         curEnv->DeleteLocalRef(clazz);
         return false;
      }
      curEnv->DeleteLocalRef(clazz);
      return true;
   }
   else
   {
      AgentWriteLog(NXLOG_ERROR, _T("JAVA: Failed to find main class %hs"), SubAgent::className());
      return false;
   }
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
static TCHAR s_userClasspath[MAX_LONG_STR] = _T("");

/**
 * Configuration template
 */
static NX_CFG_TEMPLATE s_configTemplate[] =
{
   { _T("JVM"), CT_STRING, 0, 0, MAX_PATH, 0,  s_jvmPath },
   { _T("JVMOptions"), CT_STRING_LIST, _T('\n'), 0, 0, 0, &s_jvmOptions },
   { _T("ClassPath"), CT_STRING, 0, 0, MAX_LONG_STR, 0, s_userClasspath },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

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

   if (config->parseTemplate(_T("Java"), s_configTemplate))
   {
      AgentWriteDebugLog(1, _T("JAVA: using JVM %s"), s_jvmPath);
      
      BOOL initialized = FALSE;

      TCHAR szError[255];
      s_jvmModule = DLOpen(s_jvmPath, szError);
      if (s_jvmModule != NULL)
      {
         AgentWriteDebugLog(9, _T("JVM DLOpen success"));

         JavaVMInitArgs vmArgs;
         JavaVMOption vmOptions[1];
         memset(vmOptions, 0, sizeof(vmOptions));

         TCHAR libdir[MAX_PATH];
         GetNetXMSDirectory(nxDirLib, libdir);

         String classpath = _T("-Djava.class.path=");
         classpath.append(libdir);
         classpath.append(FS_PATH_SEPARATOR_CHAR);
         classpath.append(_T("netxms-agent-") NETXMS_VERSION_STRING _T(".jar"));
         if (s_userClasspath[0] != 0)
         {
#ifdef _WIN32
            classpath.append(_T(';'));
#else
            classpath.append(_T(':'));
#endif
            classpath.append(s_userClasspath);
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
            AgentWriteDebugLog(6, _T("Java VM created"));
            if (CreateJavaVM(&s_jvm, (void **)&s_jniEnv, &vmArgs) == JNI_OK)
            {
               // register native functions into JVM
               if (RegisterNatives(s_jniEnv))
               {
                  try
                  {
                     // create an instance of org.netxms.agent.Config
                     jobject jconfig = ConfigHelper::createInstance(s_jniEnv, config);
                     if (jconfig != NULL)
                     {
                        // create an instance of org.netxms.agent.SubAgent
                        s_subAgent = new SubAgent(s_jvm , jconfig);
                        if (s_subAgent != NULL)
                        {

                           AgentWriteDebugLog(7, _T("JAVA: Loading actions"));
                           // load all Actions
                           int numActions;
                           TCHAR **actionIds = s_subAgent->getActionIds(&numActions);
                           if (numActions > 0)
                           {
                              s_subagentInfo.numActions = numActions;
                              s_subagentInfo.actions = new NETXMS_SUBAGENT_ACTION[numActions];
                           }
                           for (int i=0; i<numActions; i++)
                           {
                              Action action = s_subAgent->getAction(actionIds[i]);
                              nx_strncpy(s_subagentInfo.actions[i].name, action.getName(), MAX_PARAM_NAME);
                              s_subagentInfo.actions[i].handler = ActionHandler;
                              s_subagentInfo.actions[i].arg = actionIds[i];
                              nx_strncpy(s_subagentInfo.actions[i].description, action.getDescription(), MAX_DB_STRING);
                              // TODO deallocate/destroy action
                           }

                           AgentWriteDebugLog(7, _T("JAVA: Loading parameters"));
                           // load all Parameters
                           int numParameters;
                           TCHAR **parameterIds = s_subAgent->getParameterIds(&numParameters);
                           if (numParameters > 0)
                           {
                              s_subagentInfo.numParameters = numParameters;
                              s_subagentInfo.parameters = new NETXMS_SUBAGENT_PARAM[numParameters];
                           }
                           for(int i = 0; i < numParameters; i++)
                           {
                              Parameter *parameter = s_subAgent->getParameter(parameterIds[i]);
                              nx_strncpy(s_subagentInfo.parameters[i].name, parameter->getName(), MAX_PARAM_NAME);
                              s_subagentInfo.parameters[i].dataType = parameter->getType().getValue();
                              s_subagentInfo.parameters[i].handler = ParameterHandler;
                              s_subagentInfo.parameters[i].arg = parameterIds[i];
                              nx_strncpy(s_subagentInfo.parameters[i].description, parameter->getDescription(), MAX_DB_STRING);
                              delete parameter;
                           }

                           AgentWriteDebugLog(7, _T("JAVA: Loading lists"));
                           // load all ListParameters
                           int numListParameters;
                           TCHAR **listParameterIds = s_subAgent->getListParameterIds(&numListParameters);
                           if (numListParameters > 0)
                           {
                              s_subagentInfo.numLists = numListParameters;
                              s_subagentInfo.lists = new NETXMS_SUBAGENT_LIST[numListParameters];
                           }
                           for (int i=0; i<numListParameters; i++)
                           {
                              ListParameter listParameter = s_subAgent->getListParameter(listParameterIds[i]);
                              nx_strncpy(s_subagentInfo.lists[i].name, listParameter.getName(), MAX_PARAM_NAME);
                              s_subagentInfo.lists[i].handler = ListParameterHandler;
                              s_subagentInfo.lists[i].arg = listParameterIds[i];
                              // TODO deallocate/destroy listParameter
                           }

                           AgentWriteDebugLog(7, _T("JAVA: Loading push parameters"));
                           // load all PushParameters
                           int numPushParameters;
                           TCHAR **pushParameterIds = s_subAgent->getPushParameterIds(&numPushParameters);
                           if (numPushParameters > 0)
                           {
                              s_subagentInfo.numPushParameters = numPushParameters;
                              s_subagentInfo.pushParameters = new NETXMS_SUBAGENT_PUSHPARAM[numPushParameters];
                           }
                           for (int i=0; i<numPushParameters; i++)
                           {
                              PushParameter pushParameter = s_subAgent->getPushParameter(pushParameterIds[i]);
                              nx_strncpy(s_subagentInfo.pushParameters[i].name, pushParameter.getName(), MAX_PARAM_NAME);
                              s_subagentInfo.pushParameters[i].dataType = pushParameter.getType().getValue();
                              nx_strncpy(s_subagentInfo.pushParameters[i].description, pushParameter.getDescription(), MAX_DB_STRING);
                              // TODO deallocate/destroy pushParameter
                           }

                           AgentWriteDebugLog(7, _T("JAVA: Loading tables"));
                           // load all TableParameters
                           int numTableParameters;
                           TCHAR **tableParameterIds = s_subAgent->getTableParameterIds(&numTableParameters);
                           if (numTableParameters > 0)
                           {
                              s_subagentInfo.tables = new NETXMS_SUBAGENT_TABLE[numTableParameters];
                              s_subagentInfo.numTables = numTableParameters;
                           }
                           for (int i=0; i<numTableParameters; i++)
                           {
                              TableParameter tableParameter = s_subAgent->getTableParameter(tableParameterIds[i]);
                              nx_strncpy(s_subagentInfo.tables[i].name, tableParameter.getName(), MAX_PARAM_NAME);
                              s_subagentInfo.tables[i].handler = TableParameterHandler;
                              s_subagentInfo.tables[i].arg = tableParameterIds[i];
                              nx_strncpy(s_subagentInfo.tables[i].instanceColumns, _T(""), MAX_PARAM_NAME);
                              int numTableColumns;
                              bool haveInstanceColumn = false;
                              TableColumn ** tableColumns = tableParameter.getColumns(&numTableColumns);
                              for (int j=0; j<numTableColumns; j++) {
                                 if (tableColumns[j]->isInstance()) {
                                    if (haveInstanceColumn) {
                                       _tcsncat(s_subagentInfo.tables[i].instanceColumns, _T("|"), MAX_PARAM_NAME);
                                    }
                                    _tcsncat(s_subagentInfo.tables[i].instanceColumns, tableColumns[j]->getName(), MAX_PARAM_NAME);
                                    haveInstanceColumn = true;
                                 }      
                              }
                              nx_strncpy(s_subagentInfo.tables[i].description, tableParameter.getDescription(), MAX_DB_STRING);
                              // TODO deallocate/destroy tableParameter
                           }
                           initialized = TRUE;
                        }
                        else
                        {
                           AgentWriteLog(NXLOG_ERROR, _T("JAVA: Failed to instantiate org.netxms.agent.SubAgent"));
                        }
                     }
                     else
                     {
                        AgentWriteLog(NXLOG_ERROR, _T("JAVA: Failed to instantiate org.netxms.agent.Config"));
                     }
                  }
                  catch (JNIException const& e)
                  {
                     AgentWriteDebugLog(3, _T("JAVA: JNI exception: %hs"), e.what());
                     AgentWriteLog(NXLOG_ERROR, _T("JAVA: Failed to intialize agent on JNI code"));
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
      }
      else
      {
         AgentWriteLog(NXLOG_ERROR, _T("JAVA: Unable to load JVM: %s"), szError);
      }

      if (!initialized)
      {
         if (s_jvmModule != NULL)
         {
            DLClose(s_jvmModule);
            s_jvmModule = NULL;
         }
         *ppInfo = NULL;
         return FALSE;
      }
   }
   else
   {
      AgentWriteLog(NXLOG_ERROR, _T("JAVA: Failed to parse configuration"));
   }

   *ppInfo = &s_subagentInfo;
   return TRUE;
}
