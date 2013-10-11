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
 ** File: Parameter.cpp
 **
 **/

#include "java_subagent.h"
#include "Parameter.h"
#include "JNIException.h"

namespace org_netxms_agent
{

   // Static declarations (if any)

   // Returns the current env

   JNIEnv * Parameter::getCurrentEnv()
   {
      JNIEnv * curEnv = NULL;
      jint res=this->jvm->AttachCurrentThread(reinterpret_cast<void **>(&curEnv), NULL);
      if (res != JNI_OK)
      {
         AgentWriteLog(NXLOG_ERROR, _T("Parameter: Could not retrieve the current JVM."));
         throw JNIException();

      }
      return curEnv;
   }
   // Destructor

   Parameter::~Parameter()
   {
      JNIEnv * curEnv = NULL;
      this->jvm->AttachCurrentThread(reinterpret_cast<void **>(&curEnv), NULL);

      curEnv->DeleteGlobalRef(this->instance);
      curEnv->DeleteGlobalRef(this->instanceClass);
   }
   // Constructors
   Parameter::Parameter(JavaVM * jvm_)
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
         AgentWriteLog(NXLOG_ERROR, _T("Parameter: Could not get the Class %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      this->instanceClass = static_cast<jclass>(curEnv->NewGlobalRef(localClass));

      /* localClass is not needed anymore */
      curEnv->DeleteLocalRef(localClass);

      if (this->instanceClass == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("Parameter: Could not create a Global Ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      constructObject = curEnv->GetMethodID( this->instanceClass, construct , param ) ;
      if(constructObject == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("Parameter: Could not retrieve the constructor of the class %s with the profile : %s%s"), WideStringFromMBString(this->className()), WideStringFromMBString(construct), WideStringFromMBString(param));

         throw JNIException();
      }

      localInstance = curEnv->NewObject( this->instanceClass, constructObject ) ;
      if(localInstance == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("Parameter: Could not instantiate the object %s with the constructor : %s%s"), WideStringFromMBString(this->className()), WideStringFromMBString(construct), WideStringFromMBString(param));

         throw JNIException();
      }

      this->instance = curEnv->NewGlobalRef(localInstance) ;
      if(this->instance == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("Parameter: Could not create a new global ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }
      /* localInstance not needed anymore */
      curEnv->DeleteLocalRef(localInstance);

      /* Methods ID set to NULL */
      jstringgetNameID=NULL;
      jstringgetDescriptionID=NULL;
      jobjectgetTypeID=NULL;
      jstringgetValuejstringjava_lang_StringID=NULL;

   }

   Parameter::Parameter(JavaVM * jvm_, jobject JObj)
   {
      jvm=jvm_;

      JNIEnv * curEnv = getCurrentEnv();

      jclass localClass = curEnv->GetObjectClass(JObj);
      this->instanceClass = static_cast<jclass>(curEnv->NewGlobalRef(localClass));
      curEnv->DeleteLocalRef(localClass);

      if (this->instanceClass == NULL)
      {

         AgentWriteLog(NXLOG_ERROR, _T("Parameter: Could not create a Global Ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      this->instance = curEnv->NewGlobalRef(JObj) ;
      if(this->instance == NULL)
      {

         AgentWriteLog(NXLOG_ERROR, _T("Parameter: Could not create a new global ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }
      /* Methods ID set to NULL */
      jstringgetNameID=NULL;
      jstringgetDescriptionID=NULL;
      jobjectgetTypeID=NULL;
      jstringgetValuejstringjava_lang_StringID=NULL;

   }

   // Generic methods

   void Parameter::synchronize()
   {
      if (getCurrentEnv()->MonitorEnter(instance) != JNI_OK)
      {
         AgentWriteLog(NXLOG_ERROR, _T("Parameter: Fail to enter monitor."));
         throw JNIException();

      }
   }

   void Parameter::endSynchronize()
   {
      if ( getCurrentEnv()->MonitorExit(instance) != JNI_OK)
      {

         AgentWriteLog(NXLOG_ERROR, _T("Parameter: Fail to exit monitor."));
         throw JNIException();
      }
   }
   // Method(s)

   TCHAR* Parameter::getName ()
   {

      JNIEnv * curEnv = getCurrentEnv();

      if (jstringgetNameID==NULL)/* Use the cache */
      {
         jstringgetNameID = curEnv->GetMethodID(this->instanceClass, "getName", "()Ljava/lang/String;" ) ;
         if (jstringgetNameID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("Parameter: Could not access to the method getName"));

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

   TCHAR* Parameter::getDescription ()
   {

      JNIEnv * curEnv = getCurrentEnv();

                                 /* Use the cache */
      if (jstringgetDescriptionID==NULL)
      {
         jstringgetDescriptionID = curEnv->GetMethodID(this->instanceClass, "getDescription", "()Ljava/lang/String;" ) ;
         if (jstringgetDescriptionID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("Parameter: Could not access to the method getDescription"));

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
         TCHAR * myStringBuffer = CStringFromJavaString(curEnv, res);
         curEnv->DeleteLocalRef(res);
         return myStringBuffer;
      }
      else
      {
         curEnv->DeleteLocalRef(res);
         return NULL;
      }
   }

   ParameterType Parameter::getType ()
   {

      JNIEnv * curEnv = getCurrentEnv();

      if (jobjectgetTypeID==NULL)/* Use the cache */
      {
         jobjectgetTypeID = curEnv->GetMethodID(this->instanceClass, "getType", "()Lorg/netxms/agent/ParameterType;" ) ;
         if (jobjectgetTypeID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("Parameter: Could not access to the method getType"));

            throw JNIException();
         }
      }
      jobject res =  static_cast<jobject>( curEnv->CallObjectMethod( this->instance, jobjectgetTypeID ));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe() ;
      }
      if (res != NULL)
      {

         return * (new ParameterType(jvm, res));
      }
      else
      {
         curEnv->DeleteLocalRef(res);
         return NULL;
      }
   }

   TCHAR *Parameter::getValue(const TCHAR *param)
   {

      JNIEnv * curEnv = getCurrentEnv();

                                 /* Use the cache */
      if (jstringgetValuejstringjava_lang_StringID==NULL)
      {
         jstringgetValuejstringjava_lang_StringID = curEnv->GetMethodID(this->instanceClass, "getValue", "(Ljava/lang/String;)Ljava/lang/String;" ) ;
         if (jstringgetValuejstringjava_lang_StringID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("Parameter: Could not access to the method getValue"));

            throw JNIException();
         }
      }
      jstring param_ = JavaStringFromCString(curEnv, param);
      if (param != NULL && param_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("Parameter: Could not convert C string to Java  string, memory full."));
         throw JNIException();
      }

      jstring res =  static_cast<jstring>( curEnv->CallObjectMethod( this->instance, jstringgetValuejstringjava_lang_StringID ,param_));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe();
      }
      curEnv->DeleteLocalRef(param_);
      if (res != NULL)
      {
         TCHAR * myStringBuffer = CStringFromJavaString(curEnv, res);
         curEnv->DeleteLocalRef(res);
         return myStringBuffer;
      }
      return NULL;
   }
}
