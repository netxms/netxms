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
 ** File: SubAgent.cpp
 **
 **/

#include "java_subagent.h"
#include "SubAgent.h"
#include "JNIException.h"

namespace org_netxms_agent
{

   // Static declarations (if any)

   // Returns the current env

   JNIEnv * SubAgent::getCurrentEnv()
   {
      JNIEnv * curEnv = NULL;
      jint res=this->jvm->AttachCurrentThread(reinterpret_cast<void **>(&curEnv), NULL);
      if (res != JNI_OK)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not retrieve the current JVM."));
         throw JNIException();

      }
      return curEnv;
   }
   // Destructor

   SubAgent::~SubAgent()
   {
      JNIEnv * curEnv = NULL;
      this->jvm->AttachCurrentThread(reinterpret_cast<void **>(&curEnv), NULL);

      curEnv->DeleteGlobalRef(this->instance);
      curEnv->DeleteGlobalRef(this->instanceClass);
      curEnv->DeleteGlobalRef(this->stringArrayClass);
   }
   // Constructors
   SubAgent::SubAgent(JavaVM * jvm_, jobject jconfig)
   {
      jmethodID constructObject = NULL ;
      jobject localInstance ;
      jclass localClass ;

      const char *construct="<init>";
      const char *param="(Lorg/netxms/agent/Config;)V";
      jvm=jvm_;

      JNIEnv * curEnv = getCurrentEnv();

      localClass = curEnv->FindClass( this->className() ) ;
      if (localClass == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not get the Class %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      this->instanceClass = static_cast<jclass>(curEnv->NewGlobalRef(localClass));

      /* localClass is not needed anymore */
      curEnv->DeleteLocalRef(localClass);

      if (this->instanceClass == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not create a Global Ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      constructObject = curEnv->GetMethodID( this->instanceClass, construct , param ) ;
      if(constructObject == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not retrieve the constructor of the class %s with the profile : %s%s"), WideStringFromMBString(this->className()), WideStringFromMBString(construct), WideStringFromMBString(param));

         throw JNIException();
      }

      localInstance = curEnv->NewObject( this->instanceClass, constructObject, jconfig ) ;
      if(localInstance == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not instantiate the object %s with the constructor : %s%s"), WideStringFromMBString(this->className()), WideStringFromMBString(construct), WideStringFromMBString(param));

         throw JNIException();
      }

      this->instance = curEnv->NewGlobalRef(localInstance) ;
      if(this->instance == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not create a new global ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }
      /* localInstance not needed anymore */
      curEnv->DeleteLocalRef(localInstance);

      /* Methods ID set to NULL */
      jbooleaninitID=NULL;
      voidshutdownID=NULL;
      jbooleanloadPluginjstringjava_lang_StringID=NULL;
      jstringparameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID=NULL;
      jobjectArray_listParameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID=NULL;
      jobjectArray__tableParameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID=NULL;
      voidactionHandlerjstringjava_lang_StringjobjectArray_java_lang_Stringjava_lang_Stringjstringjava_lang_StringID=NULL;

      jclass localStringArrayClass = curEnv->FindClass("java/lang/String");
      stringArrayClass = static_cast<jclass>(curEnv->NewGlobalRef(localStringArrayClass));
      curEnv->DeleteLocalRef(localStringArrayClass);
      jobjectArray_getActionIdsID=NULL;
      jobjectgetActionjstringjava_lang_StringID=NULL;
      jobjectArray_getParameterIdsID=NULL;
      jobjectgetParameterjstringjava_lang_StringID=NULL;
      jobjectArray_getListParameterIdsID=NULL;
      jobjectgetListParameterjstringjava_lang_StringID=NULL;
      jobjectArray_getPushParameterIdsID=NULL;
      jobjectgetPushParameterjstringjava_lang_StringID=NULL;
      jobjectArray_getTableParameterIdsID=NULL;
      jobjectgetTableParameterjstringjava_lang_StringID=NULL;

   }

   // Generic methods

   void SubAgent::synchronize()
   {
      if (getCurrentEnv()->MonitorEnter(instance) != JNI_OK)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Fail to enter monitor."));
         throw JNIException();

      }
   }

   void SubAgent::endSynchronize()
   {
      if ( getCurrentEnv()->MonitorExit(instance) != JNI_OK)
      {

         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Fail to exit monitor."));
         throw JNIException();
      }
   }
   // Method(s)

   bool SubAgent::init (jobject jconfig)
   {

      JNIEnv * curEnv = getCurrentEnv();

      if (jbooleaninitID==NULL)  /* Use the cache */
      {
         jbooleaninitID = curEnv->GetMethodID(this->instanceClass, "init", "(Lorg/netxms/agent/Config;)Z" ) ;
         if (jbooleaninitID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not access to the method init"));

            throw JNIException();
         }
      }
      jboolean res =  static_cast<jboolean>( curEnv->CallBooleanMethod( this->instance, jbooleaninitID, jconfig ));

      return (res == JNI_TRUE);

   }

   void SubAgent::shutdown ()
   {
      JNIEnv * curEnv = getCurrentEnv();

      if (voidshutdownID==NULL)  /* Use the cache */
      {
         voidshutdownID = curEnv->GetMethodID(this->instanceClass, "shutdown", "()V" ) ;
         if (voidshutdownID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not access to the method shutdown"));

            throw JNIException();
         }
      }
      curEnv->CallVoidMethod( this->instance, voidshutdownID );

   }

   bool SubAgent::loadPlugin (TCHAR const* path)
   {

      JNIEnv * curEnv = getCurrentEnv();

      /* Use the cache */
      if (jbooleanloadPluginjstringjava_lang_StringID==NULL)
      {
         jbooleanloadPluginjstringjava_lang_StringID = curEnv->GetMethodID(this->instanceClass, "loadPlugin", "(Ljava/lang/String;)Z" ) ;
         if (jbooleanloadPluginjstringjava_lang_StringID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not access to the method loadPlugin"));

            throw JNIException();
         }
      }
      jstring path_ = JavaStringFromCString(curEnv, path);
      if (path != NULL && path_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not convert C string to Java string, memory full."));
         throw JNIException();
      }

      jboolean res =  static_cast<jboolean>(curEnv->CallBooleanMethod( this->instance, jbooleanloadPluginjstringjava_lang_StringID ,path_));
      curEnv->DeleteLocalRef(path_);

      return (res == JNI_TRUE);
   }

   TCHAR *SubAgent::parameterHandler(TCHAR const* param, TCHAR const* id)
   {
      JNIEnv * curEnv = getCurrentEnv();

      /* Use the cache */
      if (jstringparameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID == NULL)
      {
         jstringparameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID = curEnv->GetMethodID(this->instanceClass, "parameterHandler", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
         if (jstringparameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not access to the method parameterHandler"));

            throw JNIException();
         }
      }

      jstring param_ = JavaStringFromCString(curEnv, param);
      if (param != NULL && param_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not convert C string to Java string, memory full."));
         throw JNIException();
      }

      jstring id_ = JavaStringFromCString(curEnv, id);
      if (id != NULL && id_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not convert C string to Java string, memory full."));
         throw JNIException();
      }

      jstring res = static_cast<jstring>(curEnv->CallObjectMethod(this->instance, jstringparameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID ,param_, id_));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe();
      }

      curEnv->DeleteLocalRef(param_);
      curEnv->DeleteLocalRef(id_);

      if (res != NULL)
      {
         TCHAR *myStringBuffer = CStringFromJavaString(curEnv, res);
         AgentWriteLog(NXLOG_DEBUG, _T("SubAgent::parameterHandler(param=%s, id=%s) returning %s"), param, id, myStringBuffer);
         curEnv->DeleteLocalRef(res);
         return myStringBuffer;
      }
      return NULL;
   }

   TCHAR** SubAgent::listParameterHandler (TCHAR const* param, TCHAR const* id, int *lenRow)
   {

      JNIEnv * curEnv = getCurrentEnv();

      /* Use the cache */
      if (jobjectArray_listParameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID==NULL)
      {
         jobjectArray_listParameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID = curEnv->GetMethodID(this->instanceClass, "listParameterHandler", "(Ljava/lang/String;Ljava/lang/String;)[Ljava/lang/String;" ) ;
         if (jobjectArray_listParameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not access to the method listParameterHandler"));

            throw JNIException();
         }
      }
      jstring param_ = JavaStringFromCString(curEnv, param);
      if (param != NULL && param_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not convert C string to Java  string, memory full."));
         throw JNIException();
      }

      jstring id_ = JavaStringFromCString(curEnv, id);
      if (id != NULL && id_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not convert C string to Java  string, memory full."));
         throw JNIException();
      }

      jobjectArray res =  static_cast<jobjectArray>( curEnv->CallObjectMethod( this->instance, jobjectArray_listParameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID ,param_, id_));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe() ;
      }

      curEnv->DeleteLocalRef(param_);
      curEnv->DeleteLocalRef(id_);

      if (res != NULL)
      {
         * lenRow = curEnv->GetArrayLength(res);

         TCHAR **arrayOfString;
         arrayOfString = new TCHAR *[*lenRow];
         for (jsize i = 0; i < *lenRow; i++)
         {
            jstring resString = reinterpret_cast<jstring>(curEnv->GetObjectArrayElement(res, i));
            arrayOfString[i] = CStringFromJavaString(curEnv, resString);
            curEnv->DeleteLocalRef(resString);
         }
         curEnv->DeleteLocalRef(res);
         return arrayOfString;
      }
      return NULL;
   }

   TCHAR*** SubAgent::tableParameterHandler (TCHAR const* param, TCHAR const* id, int *lenRow, int *lenCol)
   {

      JNIEnv * curEnv = getCurrentEnv();

      /* Use the cache */
      if (jobjectArray__tableParameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID==NULL)
      {
         jobjectArray__tableParameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID = curEnv->GetMethodID(this->instanceClass, "tableParameterHandler", "(Ljava/lang/String;Ljava/lang/String;)[[Ljava/lang/String;" ) ;
         if (jobjectArray__tableParameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not access to the method tableParameterHandler"));

            throw JNIException();
         }
      }
      jstring param_ = JavaStringFromCString(curEnv, param);
      if (param != NULL && param_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not convert C string to Java  string, memory full."));
         throw JNIException();
      }

      jstring id_ = JavaStringFromCString(curEnv, id);
      if (id != NULL && id_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not convert C string to Java  string, memory full."));
         throw JNIException();
      }

      jobjectArray res =  static_cast<jobjectArray>( curEnv->CallObjectMethod( this->instance, jobjectArray__tableParameterHandlerjstringjava_lang_Stringjstringjava_lang_StringID ,param_, id_));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe() ;
      }

      curEnv->DeleteLocalRef(param_);
      curEnv->DeleteLocalRef(id_);

      if (res != NULL)
      {
         * lenRow = curEnv->GetArrayLength(res);

         TCHAR ***arrayOfString;
         arrayOfString = new TCHAR **[*lenRow];
         /* Line of the array */
         for (jsize i = 0; i < *lenRow; i++)
         {
            jobjectArray resStringLine = reinterpret_cast<jobjectArray>(curEnv->GetObjectArrayElement(res, i));
            *lenCol = curEnv->GetArrayLength(resStringLine);
            arrayOfString[i]=new TCHAR*[*lenCol];
            for (jsize j = 0; j < *lenCol; j++)
            {
               jstring resString = reinterpret_cast<jstring>(curEnv->GetObjectArrayElement(resStringLine, j));
               arrayOfString[i][j] = CStringFromJavaString(curEnv, resString);
               curEnv->DeleteLocalRef(resString);
            }
            curEnv->DeleteLocalRef(resStringLine);
         }

         curEnv->DeleteLocalRef(res);
         return arrayOfString;
      }
      return NULL;
   }

   void SubAgent::actionHandler (TCHAR const* param, TCHAR const* const* args, int argsSize, TCHAR const* id)
   {

      JNIEnv * curEnv = getCurrentEnv();

      /* Use the cache */
      if (voidactionHandlerjstringjava_lang_StringjobjectArray_java_lang_Stringjava_lang_Stringjstringjava_lang_StringID==NULL)
      {
         voidactionHandlerjstringjava_lang_StringjobjectArray_java_lang_Stringjava_lang_Stringjstringjava_lang_StringID = curEnv->GetMethodID(this->instanceClass, "actionHandler", "(Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)V" ) ;
         if (voidactionHandlerjstringjava_lang_StringjobjectArray_java_lang_Stringjava_lang_Stringjstringjava_lang_StringID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not access to the method actionHandler"));

            throw JNIException();
         }
      }
      jstring param_ = JavaStringFromCString(curEnv, param);
      if (param != NULL && param_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not convert C string to Java  string, memory full."));
         throw JNIException();
      }

      jclass stringArrayClass = curEnv->FindClass("java/lang/String");

      // create java array of strings.
      jobjectArray args_ = curEnv->NewObjectArray( argsSize, stringArrayClass, NULL);
      if (args_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not allocate Java string array, memory full."));
         throw JNIException();
      }

      // convert each TCHAR * to java strings and fill the java array.
      for ( int i = 0; i < argsSize; i++)
      {
         jstring TempString = JavaStringFromCString(curEnv, args[i]);
         if (TempString == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not convert C string to Java  string, memory full."));
            throw JNIException();
         }

         curEnv->SetObjectArrayElement( args_, i, TempString);

         // avoid keeping reference on to many strings
         curEnv->DeleteLocalRef(TempString);
      }
      jstring id_ = JavaStringFromCString(curEnv, id);
      if (id != NULL && id_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not convert C string to Java  string, memory full."));
         throw JNIException();
      }

      curEnv->CallVoidMethod( this->instance, voidactionHandlerjstringjava_lang_StringjobjectArray_java_lang_Stringjava_lang_Stringjstringjava_lang_StringID ,param_, args_, id_);
      curEnv->DeleteLocalRef(stringArrayClass);
      curEnv->DeleteLocalRef(param_);
      curEnv->DeleteLocalRef(args_);
      curEnv->DeleteLocalRef(id_);

   }

   TCHAR** SubAgent::getActionIds (int *lenRow)
   {

      JNIEnv * curEnv = getCurrentEnv();

      /* Use the cache */
      if (jobjectArray_getActionIdsID==NULL)
      {
         jobjectArray_getActionIdsID = curEnv->GetMethodID(this->instanceClass, "getActionIds", "()[Ljava/lang/String;" ) ;
         if (jobjectArray_getActionIdsID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not access to the method getActionIds"));

            throw JNIException();
         }
      }
      jobjectArray res =  static_cast<jobjectArray>( curEnv->CallObjectMethod( this->instance, jobjectArray_getActionIdsID ));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe() ;
      }
      if (res != NULL)
      {
         * lenRow = curEnv->GetArrayLength(res);

         TCHAR **arrayOfString;
         arrayOfString = new TCHAR *[*lenRow];
         for (jsize i = 0; i < *lenRow; i++)
         {
            jstring resString = reinterpret_cast<jstring>(curEnv->GetObjectArrayElement(res, i));
            arrayOfString[i] = CStringFromJavaString(curEnv, resString);
            curEnv->DeleteLocalRef(resString);
         }

         curEnv->DeleteLocalRef(res);
         return arrayOfString;
      }
      return NULL;
   }

   Action SubAgent::getAction (TCHAR const* id)
   {

      JNIEnv * curEnv = getCurrentEnv();

      /* Use the cache */
      if (jobjectgetActionjstringjava_lang_StringID==NULL)
      {
         jobjectgetActionjstringjava_lang_StringID = curEnv->GetMethodID(this->instanceClass, "getAction", "(Ljava/lang/String;)Lorg/netxms/agent/Action;" ) ;
         if (jobjectgetActionjstringjava_lang_StringID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not access to the method getAction"));

            throw JNIException();
         }
      }
      jstring id_ = JavaStringFromCString(curEnv, id);
      if (id != NULL && id_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not convert C string to Java  string, memory full."));
         throw JNIException();
      }

      jobject res =  static_cast<jobject>( curEnv->CallObjectMethod( this->instance, jobjectgetActionjstringjava_lang_StringID ,id_));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe() ;
      }
      if (res != NULL)
      {

         return *(new Action(jvm, res));
      }
      else
      {
         curEnv->DeleteLocalRef(res);
         return NULL;
      }
   }

   TCHAR** SubAgent::getParameterIds (int *lenRow)
   {

      JNIEnv * curEnv = getCurrentEnv();

      /* Use the cache */
      if (jobjectArray_getParameterIdsID==NULL)
      {
         jobjectArray_getParameterIdsID = curEnv->GetMethodID(this->instanceClass, "getParameterIds", "()[Ljava/lang/String;" ) ;
         if (jobjectArray_getParameterIdsID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not access to the method getParameterIds"));

            throw JNIException();
         }
      }
      jobjectArray res =  static_cast<jobjectArray>( curEnv->CallObjectMethod( this->instance, jobjectArray_getParameterIdsID ));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe() ;
      }
      if (res != NULL)
      {
         * lenRow = curEnv->GetArrayLength(res);

         TCHAR **arrayOfString;
         arrayOfString = new TCHAR *[*lenRow];
         for (jsize i = 0; i < *lenRow; i++)
         {
            jstring resString = reinterpret_cast<jstring>(curEnv->GetObjectArrayElement(res, i));
            arrayOfString[i] = CStringFromJavaString(curEnv, resString);
            curEnv->DeleteLocalRef(resString);
         }

         curEnv->DeleteLocalRef(res);
         return arrayOfString;
      }
      return NULL;
   }

   Parameter *SubAgent::getParameter(TCHAR const* id)
   {
      JNIEnv * curEnv = getCurrentEnv();

      /* Use the cache */
      if (jobjectgetParameterjstringjava_lang_StringID==NULL)
      {
         jobjectgetParameterjstringjava_lang_StringID = curEnv->GetMethodID(this->instanceClass, "getParameter", "(Ljava/lang/String;)Lorg/netxms/agent/Parameter;" ) ;
         if (jobjectgetParameterjstringjava_lang_StringID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not access to the method getParameter"));

            throw JNIException();
         }
      }
      jstring id_ = JavaStringFromCString(curEnv, id);
      if (id != NULL && id_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not convert C string to Java  string, memory full."));
         throw JNIException();
      }

      jobject res =  static_cast<jobject>(curEnv->CallObjectMethod(instance, jobjectgetParameterjstringjava_lang_StringID ,id_));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe();
      }
      if (res != NULL)
      {
         return new Parameter(jvm, res);
      }
      return NULL;
   }

   TCHAR **SubAgent::getListParameterIds(int *lenRow)
   {
      JNIEnv * curEnv = getCurrentEnv();

      /* Use the cache */
      if (jobjectArray_getListParameterIdsID==NULL)
      {
         jobjectArray_getListParameterIdsID = curEnv->GetMethodID(this->instanceClass, "getListParameterIds", "()[Ljava/lang/String;" ) ;
         if (jobjectArray_getListParameterIdsID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not access to the method getListParameterIds"));

            throw JNIException();
         }
      }
      jobjectArray res =  static_cast<jobjectArray>( curEnv->CallObjectMethod( this->instance, jobjectArray_getListParameterIdsID ));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe() ;
      }
      if (res != NULL)
      {
         * lenRow = curEnv->GetArrayLength(res);

         TCHAR **arrayOfString;
         arrayOfString = new TCHAR *[*lenRow];
         for (jsize i = 0; i < *lenRow; i++)
         {
            jstring resString = reinterpret_cast<jstring>(curEnv->GetObjectArrayElement(res, i));
            arrayOfString[i] = CStringFromJavaString(curEnv, resString);
            curEnv->DeleteLocalRef(resString);
         }
         curEnv->DeleteLocalRef(res);
         return arrayOfString;
      }
      return NULL;
   }

   ListParameter SubAgent::getListParameter (TCHAR const* id)
   {

      JNIEnv * curEnv = getCurrentEnv();

      /* Use the cache */
      if (jobjectgetListParameterjstringjava_lang_StringID==NULL)
      {
         jobjectgetListParameterjstringjava_lang_StringID = curEnv->GetMethodID(this->instanceClass, "getListParameter", "(Ljava/lang/String;)Lorg/netxms/agent/ListParameter;" ) ;
         if (jobjectgetListParameterjstringjava_lang_StringID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not access to the method getListParameter"));

            throw JNIException();
         }
      }
      jstring id_ = JavaStringFromCString(curEnv, id);
      if (id != NULL && id_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not convert C string to Java  string, memory full."));
         throw JNIException();
      }

      jobject res =  static_cast<jobject>( curEnv->CallObjectMethod( this->instance, jobjectgetListParameterjstringjava_lang_StringID ,id_));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe() ;
      }
      if (res != NULL)
      {

         return *(new ListParameter(jvm, res));
      }
      else
      {
         curEnv->DeleteLocalRef(res);
         return NULL;
      }
   }

   TCHAR** SubAgent::getPushParameterIds (int *lenRow)
   {

      JNIEnv * curEnv = getCurrentEnv();

      /* Use the cache */
      if (jobjectArray_getPushParameterIdsID==NULL)
      {
         jobjectArray_getPushParameterIdsID = curEnv->GetMethodID(this->instanceClass, "getPushParameterIds", "()[Ljava/lang/String;" ) ;
         if (jobjectArray_getPushParameterIdsID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not access to the method getPushParameterIds"));

            throw JNIException();
         }
      }
      jobjectArray res =  static_cast<jobjectArray>( curEnv->CallObjectMethod( this->instance, jobjectArray_getPushParameterIdsID ));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe() ;
      }
      if (res != NULL)
      {
         * lenRow = curEnv->GetArrayLength(res);

         TCHAR **arrayOfString;
         arrayOfString = new TCHAR *[*lenRow];
         for (jsize i = 0; i < *lenRow; i++)
         {
            jstring resString = reinterpret_cast<jstring>(curEnv->GetObjectArrayElement(res, i));
            arrayOfString[i] = CStringFromJavaString(curEnv, resString);
            curEnv->DeleteLocalRef(resString);
         }

         curEnv->DeleteLocalRef(res);
         return arrayOfString;
      }
      return NULL;
   }

   PushParameter SubAgent::getPushParameter (TCHAR const* id)
   {

      JNIEnv * curEnv = getCurrentEnv();

      /* Use the cache */
      if (jobjectgetPushParameterjstringjava_lang_StringID==NULL)
      {
         jobjectgetPushParameterjstringjava_lang_StringID = curEnv->GetMethodID(this->instanceClass, "getPushParameter", "(Ljava/lang/String;)Lorg/netxms/agent/PushParameter;" ) ;
         if (jobjectgetPushParameterjstringjava_lang_StringID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not access to the method getPushParameter"));

            throw JNIException();
         }
      }
      jstring id_ = JavaStringFromCString(curEnv, id);
      if (id != NULL && id_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not convert C string to Java  string, memory full."));
         throw JNIException();
      }

      jobject res =  static_cast<jobject>( curEnv->CallObjectMethod( this->instance, jobjectgetPushParameterjstringjava_lang_StringID ,id_));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe() ;
      }
      if (res != NULL)
      {

         return *(new PushParameter(jvm, res));
      }
      else
      {
         curEnv->DeleteLocalRef(res);
         return NULL;
      }
   }

   TCHAR** SubAgent::getTableParameterIds (int *lenRow)
   {
      JNIEnv * curEnv = getCurrentEnv();

      /* Use the cache */
      if (jobjectArray_getTableParameterIdsID==NULL)
      {
         jobjectArray_getTableParameterIdsID = curEnv->GetMethodID(this->instanceClass, "getTableParameterIds", "()[Ljava/lang/String;" ) ;
         if (jobjectArray_getTableParameterIdsID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not access to the method getTableParameterIds"));

            throw JNIException();
         }
      }
      jobjectArray res =  static_cast<jobjectArray>( curEnv->CallObjectMethod( this->instance, jobjectArray_getTableParameterIdsID ));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe() ;
      }
      if (res != NULL)
      {
         * lenRow = curEnv->GetArrayLength(res);

         TCHAR **arrayOfString;
         arrayOfString = new TCHAR *[*lenRow];
         for (jsize i = 0; i < *lenRow; i++)
         {
            jstring resString = reinterpret_cast<jstring>(curEnv->GetObjectArrayElement(res, i));
            arrayOfString[i] = CStringFromJavaString(curEnv, resString);
            curEnv->DeleteLocalRef(resString);
         }
         curEnv->DeleteLocalRef(res);
         return arrayOfString;
      }
      return NULL;
   }

   TableParameter SubAgent::getTableParameter(TCHAR const* id)
   {

      JNIEnv * curEnv = getCurrentEnv();

      /* Use the cache */
      if (jobjectgetTableParameterjstringjava_lang_StringID==NULL)
      {
         jobjectgetTableParameterjstringjava_lang_StringID = curEnv->GetMethodID(this->instanceClass, "getTableParameter", "(Ljava/lang/String;)Lorg/netxms/agent/TableParameter;" ) ;
         if (jobjectgetTableParameterjstringjava_lang_StringID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not access to the method getTableParameter"));

            throw JNIException();
         }
      }
      jstring id_ = JavaStringFromCString(curEnv, id);
      if (id != NULL && id_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("SubAgent: Could not convert C string to Java  string, memory full."));
         throw JNIException();
      }

      jobject res =  static_cast<jobject>( curEnv->CallObjectMethod( this->instance, jobjectgetTableParameterjstringjava_lang_StringID ,id_));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe() ;
      }
      curEnv->DeleteLocalRef(id_);
      if (res != NULL)
      {
         return *(new TableParameter(jvm, res));
      }
      return NULL;
   }
}
