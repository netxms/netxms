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

#include "Action.h"
#include "Parameter.h"
#include "ListParameter.h"
#include "PushParameter.h"
#include "TableParameter.h"

namespace org_netxms_agent
{
   class SubAgent
   {

      private:
         JavaVM * jvm;

      protected:
                                 // cache method id
         jmethodID jbooleaninitID;
                                 // cache method id
         jmethodID voidshutdownID;
                                 // cache method id
         jmethodID jbooleanloadPluginjstringjava_lang_StringID;
                                 // cache method id
         jmethodID jstringparameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID;
                                 // cache method id
         jmethodID jobjectArray_listParameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID;
                                 // cache method id
         jmethodID jobjectArray__tableParameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID;
                                 // cache method id
         jmethodID voidactionHandlerjstringjava_lang_StringjobjectArray_java_lang_Stringjava_lang_Stringjstringjava_lang_StringID;
         jclass stringArrayClass;
                                 // cache method id
         jmethodID jobjectArray_getActionIdsID;
                                 // cache method id
         jmethodID jobjectgetActionjstringjava_lang_StringID;
                                 // cache method id
         jmethodID jobjectArray_getParameterIdsID;
                                 // cache method id
         jmethodID jobjectgetParameterjstringjava_lang_StringID;
                                 // cache method id
         jmethodID jobjectArray_getListParameterIdsID;
                                 // cache method id
         jmethodID jobjectgetListParameterjstringjava_lang_StringID;
                                 // cache method id
         jmethodID jobjectArray_getPushParameterIdsID;
                                 // cache method id
         jmethodID jobjectgetPushParameterjstringjava_lang_StringID;
                                 // cache method id
         jmethodID jobjectArray_getTableParameterIdsID;
                                 // cache method id
         jmethodID jobjectgetTableParameterjstringjava_lang_StringID;

         jobject instance;
         jclass instanceClass;   // cache class

         // Caching (if any)

         /**
          * Get the environment matching to the current thread.
          */
         virtual JNIEnv * getCurrentEnv();

      public:
         // Constructor
         /**
          * Create a wrapping of the object from a JNIEnv.
          * It will call the default constructor
          * @param JEnv_ the Java Env
          */
         SubAgent(JavaVM * jvm_, jobject jconfig);

         // Destructor
         ~SubAgent();

         // Generic method
         // Synchronization methods
         /**
          * Enter monitor associated with the object.
          * Equivalent of creating a "synchronized(obj)" scope in Java.
          */
         void synchronize();

         /**
          * Exit monitor associated with the object.
          * Equivalent of ending a "synchronized(obj)" scope.
          */
         void endSynchronize();

         // Methods
         bool init(jobject jconfig);

         void shutdown();

         bool loadPlugin(TCHAR const* path);

         TCHAR* parameterHandler(TCHAR const* param, TCHAR const* id);

         TCHAR** listParameterHandler(TCHAR const* param, TCHAR const* id, int *lenRow);

         TCHAR*** tableParameterHandler(TCHAR const* param, TCHAR const* id, int *lenRow, int *lenCol);

         void actionHandler(TCHAR const* param, TCHAR const* const* args, int argsSize, TCHAR const* id);

         TCHAR** getActionIds(int *lenRow);

         Action getAction(TCHAR const* id);

         TCHAR **getParameterIds(int *lenRow);

         Parameter *getParameter(TCHAR const* id);

         TCHAR** getListParameterIds(int *lenRow);

         ListParameter getListParameter(TCHAR const* id);

         TCHAR** getPushParameterIds(int *lenRow);

         PushParameter getPushParameter(TCHAR const* id);

         TCHAR** getTableParameterIds(int *lenRow);

         TableParameter getTableParameter(TCHAR const* id);

         /**
          * Get class name to use for static methods
          * @return class name to use for static methods
          */

         static const char* className()
         {
            return "org/netxms/agent/SubAgent";
         }

   };

}
#endif
