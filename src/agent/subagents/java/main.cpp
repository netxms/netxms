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

static JavaVM *pJvm = NULL;

static HMODULE jvmModule = NULL;
static JNIEnv *jniEnv = NULL;

static SubAgent *subAgent = NULL;

static BOOL SubAgentInit(Config *config)
{
   // initialize
   subAgent->init(ConfigHelper::createInstance(jniEnv, config));
   return TRUE;
}


static void SubAgentShutdown()
{
   try
   {
      subAgent->shutdown();
   }
   catch(JNIException const& e)
   {
      // not much to be done here
   }
   if (pJvm)
   {
      pJvm->DestroyJavaVM();
      pJvm = NULL;
   }
   if (jvmModule)
   {
      DLClose(jvmModule);
      jvmModule = NULL;
   }
   AgentWriteLog(NXLOG_DEBUG, _T("SubAgentShutdown(void)"));
}


static NETXMS_SUBAGENT_INFO g_subAgentInfo =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("JAVA"),
   NETXMS_VERSION_STRING,
   SubAgentInit,
   SubAgentShutdown,
   NULL,                         // BOOL (*commandhandler)(UINT32 dwCommand, CSCPMessage *pRequest, CSCPMessage *pResponse, void *session)
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
}


;

LONG actionHandler (const TCHAR *pszAction, StringList *pArgList, const TCHAR *id)
{
   LONG result = SYSINFO_RC_SUCCESS;
   // route the call to SubAgent
   AgentWriteLog(NXLOG_DEBUG, _T("actionHandler(action=%s, id=%s)"), pszAction, id);
   int len = pArgList->getSize();
   TCHAR const** args = new TCHAR const* [len];
   for (int i=0; i<len; i++)
   {
                                 // TODO should I use a copy?
      args[i] = pArgList->getValue(i);
   }
   try
   {
      subAgent->actionHandler(pszAction, args, len, id);
   }
   catch (JNIException const& e)
   {
      result = SYSINFO_RC_ERROR;
   }
   free(args);
   return result;
}


LONG parameterHandler (const TCHAR *pszParam, const TCHAR *id, TCHAR *pValue)
{
   try
   {
      // route the call to SubAgent
      TCHAR *resultString = subAgent->parameterHandler(pszParam, id);
      nx_strncpy(pValue, resultString, MAX_RESULT_LENGTH);
      free(resultString);
      return SYSINFO_RC_SUCCESS;
   }
   catch (JNIException const& e)
   {
      return SYSINFO_RC_ERROR;
   }
}


LONG listParameterHandler (const TCHAR *cmd, const TCHAR *id, StringList *value)
{
   try
   {
      // route the call to SubAgent
      int len = 0;
      TCHAR **res = subAgent->listParameterHandler(cmd, id, &len);
      for (int i=0; i<len; i++)
      {
         value->addPreallocated(res[i]);
      }
      return SYSINFO_RC_SUCCESS;
   }
   catch (JNIException const& e)
   {
      return SYSINFO_RC_ERROR;
   }
}


LONG tableParameterHandler (const TCHAR *cmd, const TCHAR *id, Table *table)
{
   try
   {
      // route the call to SubAgent
      // retrieve the TableColumn descriptors first and populate resulting table's columns
      int numTableColumns = 0;
      TableColumn **tableColumns = subAgent->getTableParameter(id).getColumns(&numTableColumns);
      for (int i=0; i<numTableColumns; i++)
      {
         table->addColumn(tableColumns[i]->getName(), tableColumns[i]->getType().getValue(), NULL, tableColumns[i]->isInstance());
      }

      // retrieve actuall values and fill them into resulting table
      int numColumns = 0;
      int numRows = 0;
      TCHAR ***res = subAgent->tableParameterHandler(cmd, id, &numRows, &numColumns);
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
      return SYSINFO_RC_ERROR;
   }
}

/**
 * Class:     org_netxms_agent_SubAgent
 * Method:    AgentGetParameterArg
 * Signature: (Ljava/lang/String;I)Ljava/lang/String;
 */
jstring JNICALL Java_org_netxms_agent_SubAgent_AgentGetParameterArg(JNIEnv *jenv, jclass jcls, jstring jparam, jint jindex)
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
void JNICALL Java_org_netxms_agent_SubAgent_AgentSendTrap(JNIEnv *jenv, jclass jcls, jint event, jstring jname, jobjectArray jargs)
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
jboolean JNICALL Java_org_netxms_agent_SubAgent_AgentPushParameterData (JNIEnv *jenv, jclass jcls, jstring jname, jstring jvalue)
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
void JNICALL Java_org_netxms_agent_SubAgent_AgentWriteLog (JNIEnv *jenv, jclass jcls, jint level, jstring jmessage)
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
void JNICALL Java_org_netxms_agent_SubAgent_AgentWriteDebugLog (JNIEnv *jenv, jclass jcls, jint level, jstring jmessage)
{
   if (jmessage != NULL)
   {
      TCHAR *message = CStringFromJavaString(jenv, jmessage);
      AgentWriteDebugLog((int)level, message);
      free(message);
   }
}


static JNINativeMethod jniNativeMethods[] =
{
   { "AgentGetParameterArg", "(Ljava/lang/String;I)Ljava/lang/String;", (void *) Java_org_netxms_agent_SubAgent_AgentGetParameterArg },
   { "AgentPushParameterData", "(Ljava/lang/String;Ljava/lang/String;)Z", (void *) Java_org_netxms_agent_SubAgent_AgentPushParameterData },
   { "AgentWriteLog", "(ILjava/lang/String;)V", (void *) Java_org_netxms_agent_SubAgent_AgentWriteLog },
   { "AgentWriteDebugLog", "(ILjava/lang/String;)V", (void *) Java_org_netxms_agent_SubAgent_AgentWriteDebugLog }
};

static bool RegisterNatives(JNIEnv *curEnv)
{
   // register native methods exposed by Agent (see SubAgent.java implemented in org_netxms_agent_SubAgent.cpp)
   jclass clazz = curEnv->FindClass( SubAgent::className() );
   if (clazz)
   {
      if (curEnv->RegisterNatives(clazz, jniNativeMethods, (jint) (sizeof (jniNativeMethods) / sizeof (jniNativeMethods[0])) ) == 0)
      {
      }
      else
      {
         AgentWriteLog(NXLOG_ERROR, _T("Failed to register native methods"));
         curEnv->DeleteLocalRef(clazz);
         return false;
      }
      curEnv->DeleteLocalRef(clazz);
      return true;
   }
   else
   {
      AgentWriteLog(NXLOG_ERROR, _T("Failed to find main class %hs"), SubAgent::className());;
      return false;
   }
}


typedef jint (JNICALL *T_JNI_CreateJavaVM)(JavaVM **, void **, void *);

// input parameters NETXMS_SUBAGENT_INFO **ppInfo, Config *config
DECLARE_SUBAGENT_ENTRY_POINT(JAVA)
{
#ifdef _WIN32
   static TCHAR szJvm[MAX_PATH] = _T("jvm.dll");
#else
   static TCHAR szJvm[MAX_PATH] = _T("");
#endif
   static TCHAR *szJvmOptions = NULL;
   static TCHAR szClasspath[MAX_LONG_STR] = _T("netxms-agent-") NETXMS_VERSION_STRING _T(".jar ");

   static NX_CFG_TEMPLATE configTemplate[] =
   {
      { _T("Jvm"), CT_STRING, 0, 0, MAX_PATH, 0,  szJvm },
      { _T("JvmOptions"), CT_STRING_LIST, _T('\n'), 0, 0, 0, &szJvmOptions },
      { _T("ClassPath"), CT_STRING, 0, 0, MAX_LONG_STR, 0,  szClasspath },
      { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
   };

   AgentWriteDebugLog(1, _T("Initializing Java subagent"));

   // Try to set default JVM
#ifdef _WIN32
   const TCHAR *javaHome = _tgetenv(_T("JAVA_HOME"));
   if ((javaHome != NULL) && (*javaHome != 0))
   {
      _sntprintf(szJvm, MAX_PATH, _T("%s\\bin\\server\\jvm.dll"), javaHome);
      if (_taccess(szJvm, 0) != 0)
      {
         _sntprintf(szJvm, MAX_PATH, _T("%s\\jre\\bin\\server\\jvm.dll"), javaHome);
      }
      AgentWriteDebugLog(1, _T("JAVA: Default JVM set from JAVA_HOME: %s"), szJvm);
   }
#endif

   if (config->parseTemplate(_T("Java"), configTemplate))
   {
      BOOL initialized = FALSE;

      TCHAR szError[255];
      jvmModule = DLOpen(szJvm, szError);
      if (jvmModule != NULL)
      {
         AgentWriteDebugLog(9, _T("JVM DLOpen success"));

         JavaVMInitArgs vmArgs;
         JavaVMOption vmOptions[1];

         String classpath = _T("-Djava.class.path=");
         classpath += szClasspath;

#ifdef UNICODE
         vmOptions[0].optionString = MBStringFromWideString(classpath);
#else
         vmOptions[0].optionString = strdup(classpath);
#endif

         // TODO JVM options

         vmArgs.version = JNI_VERSION_1_6;
         vmArgs.options = vmOptions;
         vmArgs.nOptions = 1;
         vmArgs.ignoreUnrecognized = JNI_TRUE;

         T_JNI_CreateJavaVM CreateJavaVM = (T_JNI_CreateJavaVM)DLGetSymbolAddr(jvmModule, "JNI_CreateJavaVM", NULL);
         if (CreateJavaVM != NULL)
         {
            AgentWriteDebugLog(9, _T("JNI_CreateJavaVM success"));
            if (CreateJavaVM(&pJvm, (void **)&jniEnv, &vmArgs) == JNI_OK)
            {
               // register native functions into JVM
               if (RegisterNatives(jniEnv))
               {
                  try
                  {
                     // create an instance of org.netxms.agent.Config
                     jobject jconfig = ConfigHelper::createInstance(jniEnv, config);
                     if (jconfig)
                     {

                        // create an instance of org.netxms.agent.SubAgent
                        subAgent = new SubAgent(pJvm , jconfig);
                        if (subAgent)
                        {

                           AgentWriteDebugLog(7, _T("JAVA: Loading actions"));
                           // load all Actions
                           int numActions;
                           TCHAR **actionIds = subAgent->getActionIds(&numActions);
                           if (numActions > 0)
                           {
                              g_subAgentInfo.numActions = numActions;
                              g_subAgentInfo.actions = new NETXMS_SUBAGENT_ACTION[numActions];
                           }
                           for (int i=0; i<numActions; i++)
                           {
                              Action action = subAgent->getAction(actionIds[i]);
                              nx_strncpy(g_subAgentInfo.actions[i].name, action.getName(), MAX_PARAM_NAME);
                              g_subAgentInfo.actions[i].handler = actionHandler;
                              g_subAgentInfo.actions[i].arg = actionIds[i];
                              nx_strncpy(g_subAgentInfo.actions[i].description, action.getDescription(), MAX_DB_STRING);
                              // TODO deallocate/destroy action
                           }

                           AgentWriteDebugLog(7, _T("JAVA: Loading parameters"));
                           // load all Parameters
                           int numParameters;
                           TCHAR **parameterIds = subAgent->getParameterIds(&numParameters);
                           if (numParameters > 0)
                           {
                              g_subAgentInfo.numParameters = numParameters;
                              g_subAgentInfo.parameters = new NETXMS_SUBAGENT_PARAM[numParameters];
                           }
                           for(int i = 0; i < numParameters; i++)
                           {
                              Parameter *parameter = subAgent->getParameter(parameterIds[i]);
                              nx_strncpy(g_subAgentInfo.parameters[i].name, parameter->getName(), MAX_PARAM_NAME);
                              g_subAgentInfo.parameters[i].dataType = parameter->getType().getValue();
                              g_subAgentInfo.parameters[i].handler = parameterHandler;
                              g_subAgentInfo.parameters[i].arg = parameterIds[i];
                              nx_strncpy(g_subAgentInfo.parameters[i].description, parameter->getDescription(), MAX_DB_STRING);
                              delete parameter;
                           }

                           AgentWriteDebugLog(7, _T("JAVA: Loading lists"));
                           // load all ListParameters
                           int numListParameters;
                           TCHAR **listParameterIds = subAgent->getListParameterIds(&numListParameters);
                           if (numListParameters > 0)
                           {
                              g_subAgentInfo.numLists = numListParameters;
                              g_subAgentInfo.lists = new NETXMS_SUBAGENT_LIST[numListParameters];
                           }
                           for (int i=0; i<numListParameters; i++)
                           {
                              ListParameter listParameter = subAgent->getListParameter(listParameterIds[i]);
                              nx_strncpy(g_subAgentInfo.lists[i].name, listParameter.getName(), MAX_PARAM_NAME);
                              g_subAgentInfo.lists[i].handler = listParameterHandler;
                              g_subAgentInfo.lists[i].arg = listParameterIds[i];
                              // TODO deallocate/destroy listParameter
                           }

                           AgentWriteDebugLog(7, _T("JAVA: Loading push parameters"));
                           // load all PushParameters
                           int numPushParameters;
                           TCHAR **pushParameterIds = subAgent->getPushParameterIds(&numPushParameters);
                           if (numPushParameters > 0)
                           {
                              g_subAgentInfo.numPushParameters = numPushParameters;
                              g_subAgentInfo.pushParameters = new NETXMS_SUBAGENT_PUSHPARAM[numPushParameters];
                           }
                           for (int i=0; i<numPushParameters; i++)
                           {
                              PushParameter pushParameter = subAgent->getPushParameter(pushParameterIds[i]);
                              nx_strncpy(g_subAgentInfo.pushParameters[i].name, pushParameter.getName(), MAX_PARAM_NAME);
                              g_subAgentInfo.pushParameters[i].dataType = pushParameter.getType().getValue();
                              nx_strncpy(g_subAgentInfo.pushParameters[i].description, pushParameter.getDescription(), MAX_DB_STRING);
                              // TODO deallocate/destroy pushParameter
                           }

                           AgentWriteDebugLog(7, _T("JAVA: Loading tables"));
                           // load all TableParameters
                           int numTableParameters;
                           TCHAR **tableParameterIds = subAgent->getTableParameterIds(&numTableParameters);
                           if (numTableParameters > 0)
                           {
                              g_subAgentInfo.tables = new NETXMS_SUBAGENT_TABLE[numTableParameters];
                              g_subAgentInfo.numTables = numTableParameters;
                           }
                           for (int i=0; i<numTableParameters; i++)
                           {
                              TableParameter tableParameter = subAgent->getTableParameter(tableParameterIds[i]);
                              nx_strncpy(g_subAgentInfo.tables[i].name, tableParameter.getName(), MAX_PARAM_NAME);
                              g_subAgentInfo.tables[i].handler = tableParameterHandler;
                              g_subAgentInfo.tables[i].arg = tableParameterIds[i];
                              nx_strncpy(g_subAgentInfo.tables[i].instanceColumns, _T(""), MAX_PARAM_NAME);
                              int numTableColumns;
                              bool haveInstanceColumn = false;
                              TableColumn ** tableColumns = tableParameter.getColumns(&numTableColumns);
                              for (int j=0; j<numTableColumns; j++) {
                                 if (tableColumns[j]->isInstance()) {
                                    if (haveInstanceColumn) {
                                       _tcsncat(g_subAgentInfo.tables[i].instanceColumns, _T("|"), MAX_PARAM_NAME);
                                    }
                                    _tcsncat(g_subAgentInfo.tables[i].instanceColumns, tableColumns[j]->getName(), MAX_PARAM_NAME);
                                    haveInstanceColumn = true;
                                 }      
                              }
                              nx_strncpy(g_subAgentInfo.tables[i].description, tableParameter.getDescription(), MAX_DB_STRING);
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
         if (jvmModule)
         {
            DLClose(jvmModule);
            jvmModule = NULL;
         }
         *ppInfo = NULL;
         return FALSE;
      }
   }
   else
   {
      AgentWriteLog(NXLOG_DEBUG, _T("Failed to parse template"));
   }

   *ppInfo = &g_subAgentInfo;
   return TRUE;
}
