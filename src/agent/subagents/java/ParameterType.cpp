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
 ** File: ParameterType.cpp
 **
 **/

#include "java_subagent.h"
#include "ParameterType.h"
#include "JNIException.h"

namespace org_netxms_agent
{

   // Static declarations (if any)

   // Returns the current env

   JNIEnv * ParameterType::getCurrentEnv()
   {
      JNIEnv * curEnv = NULL;
      jint res=this->jvm->AttachCurrentThread(reinterpret_cast<void **>(&curEnv), NULL);
      if (res != JNI_OK)
      {
         AgentWriteLog(NXLOG_ERROR, _T("ParameterType: Could not retrieve the current JVM."));
         throw JNIException();

      }
      return curEnv;
   }
   // Destructor

   ParameterType::~ParameterType()
   {
      JNIEnv * curEnv = NULL;
      this->jvm->AttachCurrentThread(reinterpret_cast<void **>(&curEnv), NULL);

      curEnv->DeleteGlobalRef(this->instance);
      curEnv->DeleteGlobalRef(this->instanceClass);
   }
   // Constructors
   ParameterType::ParameterType(JavaVM * jvm_)
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
         AgentWriteLog(NXLOG_ERROR, _T("ParameterType: Could not get the Class %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      this->instanceClass = static_cast<jclass>(curEnv->NewGlobalRef(localClass));

      /* localClass is not needed anymore */
      curEnv->DeleteLocalRef(localClass);

      if (this->instanceClass == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("ParameterType: Could not create a Global Ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      constructObject = curEnv->GetMethodID( this->instanceClass, construct , param ) ;
      if(constructObject == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("ParameterType: Could not retrieve the constructor of the class %s with the profile : %s%s"), WideStringFromMBString(this->className()), WideStringFromMBString(construct), WideStringFromMBString(param));

         throw JNIException();
      }

      localInstance = curEnv->NewObject( this->instanceClass, constructObject ) ;
      if(localInstance == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("ParameterType: Could not instantiate the object %s with the constructor : %s%s"), WideStringFromMBString(this->className()), WideStringFromMBString(construct), WideStringFromMBString(param));

         throw JNIException();
      }

      this->instance = curEnv->NewGlobalRef(localInstance) ;
      if(this->instance == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("ParameterType: Could not create a new global ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }
      /* localInstance not needed anymore */
      curEnv->DeleteLocalRef(localInstance);

      /* Methods ID set to NULL */
      jintgetValueID=NULL;

   }

   ParameterType::ParameterType(JavaVM * jvm_, jobject JObj)
   {
      jvm=jvm_;

      JNIEnv * curEnv = getCurrentEnv();

      jclass localClass = curEnv->GetObjectClass(JObj);
      this->instanceClass = static_cast<jclass>(curEnv->NewGlobalRef(localClass));
      curEnv->DeleteLocalRef(localClass);

      if (this->instanceClass == NULL)
      {

         AgentWriteLog(NXLOG_ERROR, _T("ParameterType: Could not create a Global Ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      this->instance = curEnv->NewGlobalRef(JObj) ;
      if(this->instance == NULL)
      {

         AgentWriteLog(NXLOG_ERROR, _T("ParameterType: Could not create a new global ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }
      /* Methods ID set to NULL */
      jintgetValueID=NULL;

   }

   // Generic methods

   void ParameterType::synchronize()
   {
      if (getCurrentEnv()->MonitorEnter(instance) != JNI_OK)
      {
         AgentWriteLog(NXLOG_ERROR, _T("ParameterType: Fail to enter monitor."));
         throw JNIException();

      }
   }

   void ParameterType::endSynchronize()
   {
      if ( getCurrentEnv()->MonitorExit(instance) != JNI_OK)
      {

         AgentWriteLog(NXLOG_ERROR, _T("ParameterType: Fail to exit monitor."));
         throw JNIException();
      }
   }
   // Method(s)

   int ParameterType::getValue ()
   {

      JNIEnv * curEnv = getCurrentEnv();

      if (jintgetValueID==NULL)  /* Use the cache */
      {
         jintgetValueID = curEnv->GetMethodID(this->instanceClass, "getValue", "()I" ) ;
         if (jintgetValueID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("ParameterType: Could not access to the method getValue"));

            throw JNIException();
         }
      }
      jint res =  static_cast<jint>( curEnv->CallIntMethod( this->instance, jintgetValueID ));

      return res;

   }

}
