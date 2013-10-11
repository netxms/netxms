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
 ** File: Action.cpp
 **
 **/

#include "java_subagent.h"
#include "Action.h"
#include "JNIException.h"

namespace org_netxms_agent
{

   // Static declarations (if any)

   // Returns the current env

   JNIEnv * Action::getCurrentEnv()
   {
      JNIEnv * curEnv = NULL;
      jint res=this->jvm->AttachCurrentThread(reinterpret_cast<void **>(&curEnv), NULL);
      if (res != JNI_OK)
      {
         AgentWriteLog(NXLOG_ERROR, _T("Action: Could not retrieve the current JVM."));
         throw JNIException();

      }
      return curEnv;
   }
   // Destructor

   Action::~Action()
   {
      JNIEnv * curEnv = NULL;
      this->jvm->AttachCurrentThread(reinterpret_cast<void **>(&curEnv), NULL);

      curEnv->DeleteGlobalRef(this->instance);
      curEnv->DeleteGlobalRef(this->instanceClass);
      curEnv->DeleteGlobalRef(this->stringArrayClass);
   }
   // Constructors
   Action::Action(JavaVM * jvm_)
   {
      jmethodID constructObject = NULL ;
      jobject localInstance ;
      jclass localClass ;

      const char *construct="<init>";
      const char *param="()V";
      jvm=jvm_;

      JNIEnv * curEnv = getCurrentEnv();

      localClass = curEnv->FindClass( this->className() ) ;
      if (localClass == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("Action: Could not get the Class %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      this->instanceClass = static_cast<jclass>(curEnv->NewGlobalRef(localClass));

      /* localClass is not needed anymore */
      curEnv->DeleteLocalRef(localClass);

      if (this->instanceClass == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("Action: Could not create a Global Ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      constructObject = curEnv->GetMethodID( this->instanceClass, construct , param ) ;
      if(constructObject == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("Action: Could not retrieve the constructor of the class %s with the profile : %s%s"), WideStringFromMBString(this->className()), WideStringFromMBString(construct), WideStringFromMBString(param));

         throw JNIException();
      }

      localInstance = curEnv->NewObject( this->instanceClass, constructObject ) ;
      if(localInstance == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("Action: Could not instantiate the object %s with the constructor : %s%s"), WideStringFromMBString(this->className()), WideStringFromMBString(construct), WideStringFromMBString(param));

         throw JNIException();
      }

      this->instance = curEnv->NewGlobalRef(localInstance) ;
      if(this->instance == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("Action: Could not create a new global ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }
      /* localInstance not needed anymore */
      curEnv->DeleteLocalRef(localInstance);

      /* Methods ID set to NULL */
      jstringgetNameID=NULL;
      jstringgetDescriptionID=NULL;
      voidexecutejstringjava_lang_StringjobjectArray_java_lang_Stringjava_lang_StringID=NULL;

      jclass localStringArrayClass = curEnv->FindClass("java/lang/String");
      stringArrayClass = static_cast<jclass>(curEnv->NewGlobalRef(localStringArrayClass));
      curEnv->DeleteLocalRef(localStringArrayClass);

   }

   Action::Action(JavaVM * jvm_, jobject JObj)
   {
      jvm=jvm_;

      JNIEnv * curEnv = getCurrentEnv();

      jclass localClass = curEnv->GetObjectClass(JObj);
      this->instanceClass = static_cast<jclass>(curEnv->NewGlobalRef(localClass));
      curEnv->DeleteLocalRef(localClass);

      if (this->instanceClass == NULL)
      {

         AgentWriteLog(NXLOG_ERROR, _T("Action: Could not create a Global Ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      this->instance = curEnv->NewGlobalRef(JObj) ;
      if(this->instance == NULL)
      {

         AgentWriteLog(NXLOG_ERROR, _T("Action: Could not create a new global ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }
      /* Methods ID set to NULL */
      jstringgetNameID=NULL;
      jstringgetDescriptionID=NULL;
      voidexecutejstringjava_lang_StringjobjectArray_java_lang_Stringjava_lang_StringID=NULL;

      jclass localStringArrayClass = curEnv->FindClass("java/lang/String");
      stringArrayClass = static_cast<jclass>(curEnv->NewGlobalRef(localStringArrayClass));
      curEnv->DeleteLocalRef(localStringArrayClass);

   }

   // Generic methods

   void Action::synchronize()
   {
      if (getCurrentEnv()->MonitorEnter(instance) != JNI_OK)
      {
         AgentWriteLog(NXLOG_ERROR, _T("Action: Fail to enter monitor."));
         throw JNIException();

      }
   }

   void Action::endSynchronize()
   {
      if ( getCurrentEnv()->MonitorExit(instance) != JNI_OK)
      {

         AgentWriteLog(NXLOG_ERROR, _T("Action: Fail to exit monitor."));
         throw JNIException();
      }
   }
   // Method(s)

   TCHAR* Action::getName ()
   {

      JNIEnv * curEnv = getCurrentEnv();

      if (jstringgetNameID==NULL)/* Use the cache */
      {
         jstringgetNameID = curEnv->GetMethodID(this->instanceClass, "getName", "()Ljava/lang/String;" ) ;
         if (jstringgetNameID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("Action: Could not access to the method getName"));

            throw JNIException();
         }
      }
      jstring res =  static_cast<jstring>( curEnv->CallObjectMethod( this->instance, jstringgetNameID ));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe() ;
      }
      if (res != NULL)
      {

         TCHAR *myStringBuffer = CStringFromJavaString(curEnv, res);
         curEnv->DeleteLocalRef(res);

         return myStringBuffer;
      }
      else
      {
         curEnv->DeleteLocalRef(res);
         return NULL;
      }
   }

   TCHAR* Action::getDescription ()
   {

      JNIEnv * curEnv = getCurrentEnv();

                                 /* Use the cache */
      if (jstringgetDescriptionID==NULL)
      {
         jstringgetDescriptionID = curEnv->GetMethodID(this->instanceClass, "getDescription", "()Ljava/lang/String;" ) ;
         if (jstringgetDescriptionID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("Action: Could not access to the method getDescription"));

            throw JNIException();
         }
      }
      jstring res =  static_cast<jstring>( curEnv->CallObjectMethod( this->instance, jstringgetDescriptionID ));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe() ;
      }
      if (res != NULL)
      {

         TCHAR *myStringBuffer = CStringFromJavaString(curEnv, res);
         curEnv->DeleteLocalRef(res);

         return myStringBuffer;
      }
      else
      {
         curEnv->DeleteLocalRef(res);
         return NULL;
      }
   }

   void Action::execute (TCHAR const* action, TCHAR const* const* args, int argsSize)
   {

      JNIEnv * curEnv = getCurrentEnv();

                                 /* Use the cache */
      if (voidexecutejstringjava_lang_StringjobjectArray_java_lang_Stringjava_lang_StringID==NULL)
      {
         voidexecutejstringjava_lang_StringjobjectArray_java_lang_Stringjava_lang_StringID = curEnv->GetMethodID(this->instanceClass, "execute", "(Ljava/lang/String;[Ljava/lang/String;)V" ) ;
         if (voidexecutejstringjava_lang_StringjobjectArray_java_lang_Stringjava_lang_StringID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("Action: Could not access to the method execute"));

            throw JNIException();
         }
      }
      jstring action_ = JavaStringFromCString(curEnv, action);
      if (action != NULL && action_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("Action: Could not convert C string to Java  string, memory full."));
         throw JNIException();
      }

      jclass stringArrayClass = curEnv->FindClass("java/lang/String");

      // create java array of strings.
      jobjectArray args_ = curEnv->NewObjectArray( argsSize, stringArrayClass, NULL);
      if (args_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("Action: Could not allocate Java string array, memory full."));
         throw JNIException();
      }

      // convert each TCHAR * to java strings and fill the java array.
      for ( int i = 0; i < argsSize; i++)
      {
         jstring tempString = JavaStringFromCString(curEnv, args[i]);
         if (tempString == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("Action: Could not convert C string to Java  string, memory full."));
            throw JNIException();
         }

         curEnv->SetObjectArrayElement(args_, i, tempString);

         // avoid keeping reference on to many strings
         curEnv->DeleteLocalRef(tempString);
      }
      curEnv->CallVoidMethod( this->instance, voidexecutejstringjava_lang_StringjobjectArray_java_lang_Stringjava_lang_StringID ,action_, args_);
      curEnv->DeleteLocalRef(stringArrayClass);
      curEnv->DeleteLocalRef(action_);
      curEnv->DeleteLocalRef(args_);

   }

}
