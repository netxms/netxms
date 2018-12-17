/* 
 ** Java-Bridge NetXMS subagent
 ** Copyright (c) 2013 TEMPEST a.s.
 ** Copyright (c) 2015-2017 Raden Solutions SIA
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
 ** File: SubAgent.cpp
 **
 **/

#include "java_subagent.h"
#include "SubAgent.h"

/**
 * SubAgent class name
 */
static const char *s_subAgentClassName = "org/netxms/agent/SubAgent";

/**
 * SubAgent class reference
 */
jclass SubAgent::m_class = NULL;

/**
 * java.lang.String class reference
 */
jclass SubAgent::m_stringClass = NULL;

/**
 * SubAgent method IDs
 */
jmethodID SubAgent::m_actionHandler = NULL;
jmethodID SubAgent::m_constructor = NULL;
jmethodID SubAgent::m_getActions = NULL;
jmethodID SubAgent::m_getLists = NULL;
jmethodID SubAgent::m_getParameters = NULL;
jmethodID SubAgent::m_getPushParameters = NULL;
jmethodID SubAgent::m_getTables = NULL;
jmethodID SubAgent::m_init = NULL;
jmethodID SubAgent::m_listHandler = NULL;
jmethodID SubAgent::m_parameterHandler = NULL;
jmethodID SubAgent::m_shutdown = NULL;
jmethodID SubAgent::m_tableHandler = NULL;

/**
 * Initialization marker
 */
bool SubAgent::m_initialized = false;

/**
 * Class:     org.netxms.agent.SubAgent
 * Method:    getParameterArg
 * Signature: (Ljava/lang/String;I)Ljava/lang/String;
 */
static jstring JNICALL J_getParameterArg(JNIEnv *jenv, jclass jcls, jstring jparam, jint jindex)
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
 * Class:     org.netxms.agent.SubAgent
 * Method:    sendTrap
 * Signature: (ILjava/lang/String;[Ljava/lang/String;)V
 */
static void JNICALL J_sendTrap(JNIEnv *jenv, jclass jcls, jint event, jstring jname, jobjectArray jargs)
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
      AgentSendTrap2((UINT32)event, name, numArgs, const_cast<const TCHAR**>(arrayOfString));
      free(name);
      for(jsize i = 0; i < numArgs; i++)
         free(arrayOfString[i]);
      free(arrayOfString);
   }
}

/**
 * Class:     org.netxms.agent.SubAgent
 * Method:    pushParameterData
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
static jboolean JNICALL J_pushParameterData(JNIEnv *jenv, jclass jcls, jstring jname, jstring jvalue)
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
 * Native methods
 */
static JNINativeMethod s_jniNativeMethods[] =
{
   { (char *)"getParameterArg", (char *)"(Ljava/lang/String;I)Ljava/lang/String;", (void *)J_getParameterArg },
   { (char *)"pushParameterData", (char *)"(Ljava/lang/String;Ljava/lang/String;)Z", (void *)J_pushParameterData },
   { (char *)"sendTrap", (char *)"(ILjava/lang/String;[Ljava/lang/String;)V", (void *)J_sendTrap }
};

/**
 * Get method ID
 */
bool SubAgent::getMethodId(JNIEnv *curEnv, const char *name, const char *profile, jmethodID *id)
{
   *id = curEnv->GetMethodID(m_class, name, profile);
   if (*id == NULL)
   {
      AgentWriteLog(NXLOG_ERROR, _T("JAVA: Could not retrieve metod %hs%hs for class %hs"), name, profile, s_subAgentClassName);
      return false;
   }
   return true;
}

/**
 * Initialize SubAgent class wrapper
 */
bool SubAgent::initialize(JNIEnv *curEnv)
{
   m_class = CreateJavaClassGlobalRef(curEnv, s_subAgentClassName);
   if (m_class == NULL)
      return false;

   m_stringClass = CreateJavaClassGlobalRef(curEnv, "java/lang/String");
   if (m_stringClass == NULL)
      return false;

   if (!getMethodId(curEnv, "<init>", "(Lorg/netxms/bridge/Config;)V", &m_constructor))
      return false;
   if (!getMethodId(curEnv, "actionHandler", "(Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)Z", &m_actionHandler))
      return false;
   if (!getMethodId(curEnv, "getActions", "()[Ljava/lang/String;", &m_getActions))
      return false;
   if (!getMethodId(curEnv, "getLists", "()[Ljava/lang/String;", &m_getLists))
      return false;
   if (!getMethodId(curEnv, "getParameters", "()[Ljava/lang/String;", &m_getParameters))
      return false;
   if (!getMethodId(curEnv, "getPushParameters", "()[Ljava/lang/String;", &m_getPushParameters))
      return false;
   if (!getMethodId(curEnv, "getTables", "()[Ljava/lang/String;", &m_getTables))
      return false;
   if (!getMethodId(curEnv, "init", "()Z", &m_init))
      return false;
   if (!getMethodId(curEnv, "listHandler", "(Ljava/lang/String;Ljava/lang/String;)[Ljava/lang/String;", &m_listHandler))
      return false;
   if (!getMethodId(curEnv, "parameterHandler", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", &m_parameterHandler))
      return false;
   if (!getMethodId(curEnv, "shutdown", "()V", &m_shutdown))
      return false;
   if (!getMethodId(curEnv, "tableHandler", "(Ljava/lang/String;Ljava/lang/String;)[[Ljava/lang/String;", &m_tableHandler))
      return false;

   if (curEnv->RegisterNatives(m_class, s_jniNativeMethods, (jint)(sizeof(s_jniNativeMethods) / sizeof(JNINativeMethod))) != 0)
   {
      AgentWriteLog(NXLOG_ERROR, _T("JAVA: Failed to register native methods for class %hs"), s_subAgentClassName);
      return false;
   }

   m_initialized = true;
   return true;
}

/**
 * Create SubAgent object instance.
 * @param config Config object
 */
SubAgent *SubAgent::createInstance(JNIEnv *curEnv, jobject config)
{
   if (!m_initialized)
   {
      AgentWriteLog(NXLOG_ERROR, _T("JAVA: SubAgent class was not properly initialized"));
      return NULL;
   }

   jobject object = curEnv->NewObject(m_class, m_constructor, config);
   if (object == NULL)
   {
      AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not instantiate SubAgent object"));
      return NULL;
   }

   jobject instance = curEnv->NewGlobalRef(object);
   curEnv->DeleteLocalRef(object);
   if (instance == NULL)
   {
      AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not create a new global reference for SubAgent object"));
      return NULL;
   }

   return new SubAgent(instance);
}

/**
 * Internal constructor
 */
SubAgent::SubAgent(jobject instance)
{
   m_instance = instance;
}

/**
 * Destructor
 */
SubAgent::~SubAgent()
{
   JNIEnv *curEnv = AttachThreadToJavaVM();
   if (curEnv != NULL)
      curEnv->DeleteGlobalRef(m_instance);
   DetachThreadFromJavaVM();
}

/**
 * Get JNI environment
 */
JNIEnv *SubAgent::getCurrentEnv()
{
   JNIEnv *curEnv = AttachThreadToJavaVM();
   if (curEnv == NULL)
      AgentWriteLog(NXLOG_ERROR, _T("JAVA: SubAgent::getCurrentEnv(): cannot attach current thread to JVM"));
   return curEnv;
}

/**
 * Call Java method SubAgent.init()
 */
bool SubAgent::init()
{
   if (!m_initialized)
      return false;

   JNIEnv *curEnv = getCurrentEnv();
   if (curEnv == NULL)
      return false;

   jboolean ret = static_cast<jboolean>(curEnv->CallBooleanMethod(m_instance, m_init));
   m_initialized = (ret == JNI_TRUE);
   return m_initialized;
}

/**
 * Call Java method SubAgent.shutdown()
 */
void SubAgent::shutdown()
{
   if (!m_initialized)
      return;

   JNIEnv *curEnv = getCurrentEnv();
   if (curEnv == NULL)
      return;

   curEnv->CallVoidMethod(m_instance, m_shutdown);
}

/**
 * Call Java method SubAgent.parameterHandler()
 */
LONG SubAgent::parameterHandler(const TCHAR *param, const TCHAR *id, TCHAR *value)
{
   if (!m_initialized)
      return SYSINFO_RC_ERROR;

   JNIEnv *curEnv = getCurrentEnv();
   if (curEnv == NULL)
      return SYSINFO_RC_ERROR;

   LONG rc = SYSINFO_RC_ERROR;
   jstring jparam = JavaStringFromCString(curEnv, param);
   jstring jid = JavaStringFromCString(curEnv, id);
   if ((jparam != NULL) && (jid != NULL))
   {
      jstring ret = static_cast<jstring>(curEnv->CallObjectMethod(m_instance, m_parameterHandler, jparam, jid));
      if (!curEnv->ExceptionCheck())
      {
         if (ret != NULL)
         {
            CStringFromJavaString(curEnv, ret, value, MAX_RESULT_LENGTH);
            AgentWriteDebugLog(7, _T("JAVA: SubAgent::parameterHandler(\"%s\", \"%s\"): value \"%s\""), param, id, value);
            curEnv->DeleteLocalRef(ret);
            rc = SYSINFO_RC_SUCCESS;
         }
         else
         {
            rc = SYSINFO_RC_UNSUPPORTED;
         }
      }
      else
      {
         AgentWriteDebugLog(5, _T("JAVA: SubAgent::parameterHandler(\"%s\", \"%s\"): exception in Java code"), param, id);
         curEnv->ExceptionClear();
      }
   }
   else
   {
      AgentWriteLog(NXLOG_ERROR, _T("JAVA: SubAgent::parameterHandler: Could not convert C string to Java string"));
   }

   DeleteJavaLocalRef(curEnv, jparam);
   DeleteJavaLocalRef(curEnv, jid);
   return rc;
}

/**
 * Call Java method SubAgent.listHandler()
 */
LONG SubAgent::listHandler(const TCHAR *param, const TCHAR *id, StringList *value)
{
   if (!m_initialized)
      return SYSINFO_RC_ERROR;

   JNIEnv *curEnv = getCurrentEnv();
   if (curEnv == NULL)
      return SYSINFO_RC_ERROR;

   LONG rc = SYSINFO_RC_ERROR;
   jstring jparam = JavaStringFromCString(curEnv, param);
   jstring jid = JavaStringFromCString(curEnv, id);
   if ((jparam != NULL) && (jid != NULL))
   {
      jobjectArray ret = static_cast<jobjectArray>(curEnv->CallObjectMethod(m_instance, m_listHandler, jparam, jid));
      if (!curEnv->ExceptionCheck())
      {
         if (ret != NULL)
         {
            jsize count = curEnv->GetArrayLength(ret);
            for(jsize i = 0; i < count; i++)
            {
               jstring v = reinterpret_cast<jstring>(curEnv->GetObjectArrayElement(ret, i));
               value->addPreallocated(CStringFromJavaString(curEnv, v));
               curEnv->DeleteLocalRef(v);
            }
            curEnv->DeleteLocalRef(ret);
            rc = SYSINFO_RC_SUCCESS;
         }
         else
         {
            rc = SYSINFO_RC_UNSUPPORTED;
         }
      }
      else
      {
         AgentWriteDebugLog(5, _T("JAVA: SubAgent::listHandler(\"%s\", \"%s\"): exception in Java code"), param, id);
         curEnv->ExceptionClear();
      }
   }
   else
   {
      AgentWriteLog(NXLOG_ERROR, _T("JAVA: SubAgent::listHandler: Could not convert C string to Java string"));
   }

   DeleteJavaLocalRef(curEnv, jparam);
   DeleteJavaLocalRef(curEnv, jid);
   return rc;
}

/**
 * Call Java method SubAgent.tableHandler()
 */
LONG SubAgent::tableHandler(const TCHAR *param, const TCHAR *id, Table *value)
{
   if (!m_initialized)
      return SYSINFO_RC_ERROR;

   JNIEnv *curEnv = getCurrentEnv();
   if (curEnv == NULL)
      return SYSINFO_RC_ERROR;

   LONG rc = SYSINFO_RC_ERROR;
   jstring jparam = JavaStringFromCString(curEnv, param);
   jstring jid = JavaStringFromCString(curEnv, id);
   if ((jparam != NULL) && (jid != NULL))
   {
      jobjectArray ret = static_cast<jobjectArray>(curEnv->CallObjectMethod(m_instance, m_tableHandler, jparam, jid));
      if (!curEnv->ExceptionCheck())
      {
         if (ret != NULL)
         {
            jsize count = curEnv->GetArrayLength(ret);
            for(jsize i = 0; i < count; i++)
            {
               jobjectArray row = reinterpret_cast<jobjectArray>(curEnv->GetObjectArrayElement(ret, i));
               if (row != NULL)
               {
                  value->addRow();
                  jsize columns = curEnv->GetArrayLength(row);
                  for(jsize j = 0; j < columns; j++)
                  {
                     jstring s = reinterpret_cast<jstring>(curEnv->GetObjectArrayElement(row, j));
                     if (s != NULL)
                     {
                        value->setPreallocated(j, CStringFromJavaString(curEnv, s));
                        curEnv->DeleteLocalRef(s);
                     }
                  }
                  curEnv->DeleteLocalRef(row);
               }
            }
            curEnv->DeleteLocalRef(ret);
            rc = SYSINFO_RC_SUCCESS;
         }
         else
         {
            rc = SYSINFO_RC_UNSUPPORTED;
         }
      }
      else
      {
         AgentWriteDebugLog(5, _T("JAVA: SubAgent::tableHandler(\"%s\", \"%s\"): exception in Java code"), param, id);
         curEnv->ExceptionClear();
      }
   }
   else
   {
      AgentWriteLog(NXLOG_ERROR, _T("JAVA: SubAgent::tableHandler: Could not convert C string to Java string"));
   }

   DeleteJavaLocalRef(curEnv, jparam);
   DeleteJavaLocalRef(curEnv, jid);
   return rc;
}

/**
 * Call Java method SubAgent.actionHandler()
 */
LONG SubAgent::actionHandler(const TCHAR *action, const StringList *args, const TCHAR *id)
{
   if (!m_initialized)
      return SYSINFO_RC_ERROR;

   JNIEnv *curEnv = getCurrentEnv();
   if (curEnv == NULL)
      return SYSINFO_RC_ERROR;

   LONG rc = SYSINFO_RC_ERROR;
   jboolean ret = JNI_FALSE;
   jstring jaction = JavaStringFromCString(curEnv, action);
   jstring jid = JavaStringFromCString(curEnv, id);
   jobjectArray jargs = curEnv->NewObjectArray(args->size(), m_stringClass, NULL);
   if ((jaction == NULL) || (jid == NULL))
   {
      AgentWriteLog(NXLOG_ERROR, _T("JAVA: SubAgent::actionHandler: Could not convert C string to Java string"));
      goto cleanup;
   }
   if (jargs == NULL)
   {
      AgentWriteLog(NXLOG_ERROR, _T("JAVA: SubAgent::actionHandler: cannot allocate Java string array"));
      goto cleanup;
   }

   // convert arguments to java string array
   for(int i = 0; i < args->size(); i++)
   {
      jstring s = JavaStringFromCString(curEnv, args->get(i));
      if (s != NULL)
      {
         curEnv->SetObjectArrayElement(jargs, i, s);
         curEnv->DeleteLocalRef(s);
      }
   }

   ret = static_cast<jboolean>(curEnv->CallBooleanMethod(m_instance, m_actionHandler, jaction, jargs, jid));
   if (!curEnv->ExceptionCheck())
   {
      rc = ret ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
   }
   else
   {
      AgentWriteDebugLog(5, _T("JAVA: SubAgent::actionHandler(\"%s\", \"%s\"): exception in Java code"), action, id);
      curEnv->ExceptionClear();
   }

cleanup:
   DeleteJavaLocalRef(curEnv, jaction);
   DeleteJavaLocalRef(curEnv, jid);
   DeleteJavaLocalRef(curEnv, jargs);
   return rc;
}

/**
 * Generic getter for agent contribution items
 */
StringList *SubAgent::getContributionItems(jmethodID method, const TCHAR *methodName)
{
   if (!m_initialized)
      return NULL;

   JNIEnv *curEnv = getCurrentEnv();
   if (curEnv == NULL)
      return NULL;

   StringList *list = NULL;
   jobjectArray a = static_cast<jobjectArray>(curEnv->CallObjectMethod(m_instance, method));
   if (!curEnv->ExceptionCheck())
   {
      list = StringListFromJavaArray(curEnv, a);
   }
   else
   {
      AgentWriteDebugLog(5, _T("JAVA: SubAgent::%s(): exception in Java code"), methodName);
      curEnv->ExceptionClear();
   }
   DeleteJavaLocalRef(curEnv, a);
   return list;
}
