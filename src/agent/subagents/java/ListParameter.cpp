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
 ** File: ListParameter.cpp
 **
 **/

#include "java_subagent.h"
#include "ListParameter.h"
#include "JNIException.h"

namespace org_netxms_agent
{

   // Static declarations (if any)

   // Returns the current env

   JNIEnv * ListParameter::getCurrentEnv()
   {
      JNIEnv * curEnv = NULL;
      jint res=this->jvm->AttachCurrentThread(reinterpret_cast<void **>(&curEnv), NULL);
      if (res != JNI_OK)
      {
         AgentWriteLog(NXLOG_ERROR, _T("ListParameter: Could not retrieve the current JVM."));
         throw JNIException();

      }
      return curEnv;
   }
   // Destructor

   ListParameter::~ListParameter()
   {
      JNIEnv * curEnv = NULL;
      this->jvm->AttachCurrentThread(reinterpret_cast<void **>(&curEnv), NULL);

      curEnv->DeleteGlobalRef(this->instance);
      curEnv->DeleteGlobalRef(this->instanceClass);
   }
   // Constructors
   ListParameter::ListParameter(JavaVM * jvm_)
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
         AgentWriteLog(NXLOG_ERROR, _T("ListParameter: Could not get the Class %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      this->instanceClass = static_cast<jclass>(curEnv->NewGlobalRef(localClass));

      /* localClass is not needed anymore */
      curEnv->DeleteLocalRef(localClass);

      if (this->instanceClass == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("ListParameter: Could not create a Global Ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      constructObject = curEnv->GetMethodID( this->instanceClass, construct , param ) ;
      if(constructObject == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("ListParameter: Could not retrieve the constructor of the class %s with the profile : %s%s"), WideStringFromMBString(this->className()), WideStringFromMBString(construct), WideStringFromMBString(param));

         throw JNIException();
      }

      localInstance = curEnv->NewObject( this->instanceClass, constructObject ) ;
      if(localInstance == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("ListParameter: Could not instantiate the object %s with the constructor : %s%s"), WideStringFromMBString(this->className()), WideStringFromMBString(construct), WideStringFromMBString(param));

         throw JNIException();
      }

      this->instance = curEnv->NewGlobalRef(localInstance) ;
      if(this->instance == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("ListParameter: Could not create a new global ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }
      /* localInstance not needed anymore */
      curEnv->DeleteLocalRef(localInstance);

      /* Methods ID set to NULL */
      jstringgetNameID=NULL;
      jobjectArray_getValuesjstringjava_lang_StringID=NULL;

   }

   ListParameter::ListParameter(JavaVM * jvm_, jobject JObj)
   {
      jvm=jvm_;

      JNIEnv * curEnv = getCurrentEnv();

      jclass localClass = curEnv->GetObjectClass(JObj);
      this->instanceClass = static_cast<jclass>(curEnv->NewGlobalRef(localClass));
      curEnv->DeleteLocalRef(localClass);

      if (this->instanceClass == NULL)
      {

         AgentWriteLog(NXLOG_ERROR, _T("ListParameter: Could not create a Global Ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      this->instance = curEnv->NewGlobalRef(JObj) ;
      if(this->instance == NULL)
      {

         AgentWriteLog(NXLOG_ERROR, _T("ListParameter: Could not create a new global ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }
      /* Methods ID set to NULL */
      jstringgetNameID=NULL;
      jobjectArray_getValuesjstringjava_lang_StringID=NULL;

   }

   // Generic methods

   void ListParameter::synchronize()
   {
      if (getCurrentEnv()->MonitorEnter(instance) != JNI_OK)
      {
         AgentWriteLog(NXLOG_ERROR, _T("ListParameter: Fail to enter monitor."));
         throw JNIException();

      }
   }

   void ListParameter::endSynchronize()
   {
      if ( getCurrentEnv()->MonitorExit(instance) != JNI_OK)
      {

         AgentWriteLog(NXLOG_ERROR, _T("ListParameter: Fail to exit monitor."));
         throw JNIException();
      }
   }
   // Method(s)

   TCHAR* ListParameter::getName ()
   {

      JNIEnv * curEnv = getCurrentEnv();

      if (jstringgetNameID==NULL)/* Use the cache */
      {
         jstringgetNameID = curEnv->GetMethodID(this->instanceClass, "getName", "()Ljava/lang/String;" ) ;
         if (jstringgetNameID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("ListParameter: Could not access to the method getName"));

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

   TCHAR **ListParameter::getValues(TCHAR const* param, int *lenRow)
   {

      JNIEnv * curEnv = getCurrentEnv();

                                 /* Use the cache */
      if (jobjectArray_getValuesjstringjava_lang_StringID==NULL)
      {
         jobjectArray_getValuesjstringjava_lang_StringID = curEnv->GetMethodID(this->instanceClass, "getValues", "(Ljava/lang/String;)[Ljava/lang/String;" ) ;
         if (jobjectArray_getValuesjstringjava_lang_StringID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("ListParameter: Could not access to the method getValues"));

            throw JNIException();
         }
      }
      jstring param_ = JavaStringFromCString(curEnv, param);
      if (param != NULL && param_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("ListParameter: Could not convert C string to Java  string, memory full."));
         throw JNIException();
      }

      jobjectArray res =  static_cast<jobjectArray>( curEnv->CallObjectMethod( this->instance, jobjectArray_getValuesjstringjava_lang_StringID ,param_));
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
         curEnv->DeleteLocalRef(param_);

         curEnv->DeleteLocalRef(res);
         return arrayOfString;
      }
      else
      {
         curEnv->DeleteLocalRef(res);
         return NULL;
      }
   }

}
