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
 ** File: SubAgent.hxx
 **
 **/

#ifndef __ORG_NETXMS_AGENT_SUBAGENT__
#define __ORG_NETXMS_AGENT_SUBAGENT__

class SubAgent
{
private:
   static jclass m_class;
   static jclass m_stringClass;
   static jmethodID m_actionHandler;
   static jmethodID m_constructor;
   static jmethodID m_getActions;
   static jmethodID m_getLists;
   static jmethodID m_getParameters;
   static jmethodID m_getPushParameters;
   static jmethodID m_getTables;
   static jmethodID m_init;
   static jmethodID m_listHandler;
   static jmethodID m_parameterHandler;
   static jmethodID m_shutdown;
   static jmethodID m_tableHandler;
   static bool m_initialized;

   jobject m_instance;

   static bool getMethodId(JNIEnv *curEnv, const char *name, const char *profile, jmethodID *id);

   SubAgent(jobject instance);

   JNIEnv *getCurrentEnv();
   StringList *getContributionItems(jmethodID method, const TCHAR *methodName);

public:
   static bool initialize(JNIEnv *curEnv);

   /**
    * Create a wrapping of the object from a JNIEnv.
    * It will call the default constructor
    * @param jvm the Java VM
    */
   static SubAgent *createInstance(JNIEnv *curEnv, jobject config);

   ~SubAgent();

   // Methods
   bool init();
   void shutdown();
   bool loadPlugin(const TCHAR *path);

   LONG parameterHandler(const TCHAR *param, const TCHAR *id, TCHAR *value);
   LONG listHandler(const TCHAR *param, const TCHAR *id, StringList *value);
   LONG tableHandler(const TCHAR *param, const TCHAR *id, Table *value);
   LONG actionHandler(const TCHAR *action, const StringList *args, const TCHAR *id);

   StringList *getActions() { return getContributionItems(m_getActions, _T("getActions")); }
   StringList *getParameters() { return getContributionItems(m_getParameters, _T("getParameters")); }
   StringList *getLists() { return getContributionItems(m_getLists, _T("getLists")); }
   StringList *getPushParameters() { return getContributionItems(m_getPushParameters, _T("getPushParameters")); }
   StringList *getTables() { return getContributionItems(m_getTables, _T("getTables")); }
};

#endif
