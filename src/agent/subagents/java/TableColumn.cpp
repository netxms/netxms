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
 ** File: TableColumn.cpp
 **
 **/

#include "java_subagent.h"
#include "TableColumn.h"
#include "JNIException.h"

namespace org_netxms_agent
{

   // Static declarations (if any)

   // Returns the current env

   JNIEnv * TableColumn::getCurrentEnv()
   {
      JNIEnv * curEnv = NULL;
      jint res=this->jvm->AttachCurrentThread(reinterpret_cast<void **>(&curEnv), NULL);
      if (res != JNI_OK)
      {
         AgentWriteLog(NXLOG_ERROR, _T("TableColumn: Could not retrieve the current JVM."));
         throw JNIException();

      }
      return curEnv;
   }
   // Destructor

   TableColumn::~TableColumn()
   {
      JNIEnv * curEnv = NULL;
      this->jvm->AttachCurrentThread(reinterpret_cast<void **>(&curEnv), NULL);

      curEnv->DeleteGlobalRef(this->instance);
      curEnv->DeleteGlobalRef(this->instanceClass);
   }
   // Constructors
   TableColumn::TableColumn(JavaVM * jvm_)
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
         AgentWriteLog(NXLOG_ERROR, _T("TableColumn: Could not get the Class %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      this->instanceClass = static_cast<jclass>(curEnv->NewGlobalRef(localClass));

      /* localClass is not needed anymore */
      curEnv->DeleteLocalRef(localClass);

      if (this->instanceClass == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("TableColumn: Could not create a Global Ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      constructObject = curEnv->GetMethodID( this->instanceClass, construct , param ) ;
      if(constructObject == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("TableColumn: Could not retrieve the constructor of the class %s with the profile : %s%s"), WideStringFromMBString(this->className()), WideStringFromMBString(construct), WideStringFromMBString(param));

         throw JNIException();
      }

      localInstance = curEnv->NewObject( this->instanceClass, constructObject ) ;
      if(localInstance == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("TableColumn: Could not instantiate the object %s with the constructor : %s%s"), WideStringFromMBString(this->className()), WideStringFromMBString(construct), WideStringFromMBString(param));

         throw JNIException();
      }

      this->instance = curEnv->NewGlobalRef(localInstance) ;
      if(this->instance == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("TableColumn: Could not create a new global ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }
      /* localInstance not needed anymore */
      curEnv->DeleteLocalRef(localInstance);

      /* Methods ID set to NULL */
      jstringgetNameID=NULL;
      jobjectgetTypeID=NULL;
      jbooleanisInstanceID=NULL;

   }

   TableColumn::TableColumn(JavaVM * jvm_, jobject JObj)
   {
      jvm=jvm_;

      JNIEnv * curEnv = getCurrentEnv();

      jclass localClass = curEnv->GetObjectClass(JObj);
      this->instanceClass = static_cast<jclass>(curEnv->NewGlobalRef(localClass));
      curEnv->DeleteLocalRef(localClass);

      if (this->instanceClass == NULL)
      {

         AgentWriteLog(NXLOG_ERROR, _T("TableColumn: Could not create a Global Ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      this->instance = curEnv->NewGlobalRef(JObj) ;
      if(this->instance == NULL)
      {

         AgentWriteLog(NXLOG_ERROR, _T("TableColumn: Could not create a new global ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }
      /* Methods ID set to NULL */
      jstringgetNameID=NULL;
      jobjectgetTypeID=NULL;
      jbooleanisInstanceID=NULL;

   }

   // Generic methods

   void TableColumn::synchronize()
   {
      if (getCurrentEnv()->MonitorEnter(instance) != JNI_OK)
      {
         AgentWriteLog(NXLOG_ERROR, _T("TableColumn: Fail to enter monitor."));
         throw JNIException();

      }
   }

   void TableColumn::endSynchronize()
   {
      if ( getCurrentEnv()->MonitorExit(instance) != JNI_OK)
      {

         AgentWriteLog(NXLOG_ERROR, _T("TableColumn: Fail to exit monitor."));
         throw JNIException();
      }
   }
   // Method(s)

   TCHAR* TableColumn::getName ()
   {

      JNIEnv * curEnv = getCurrentEnv();

      if (jstringgetNameID==NULL)/* Use the cache */
      {
         jstringgetNameID = curEnv->GetMethodID(this->instanceClass, "getName", "()Ljava/lang/String;" ) ;
         if (jstringgetNameID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("TableColumn: Could not access to the method getName"));

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
         TCHAR * myStringBuffer = CStringFromJavaString(curEnv, res);
         curEnv->DeleteLocalRef(res);
         return myStringBuffer;
      }
      return NULL;
   }

   ParameterType TableColumn::getType ()
   {

      JNIEnv * curEnv = getCurrentEnv();

      if (jobjectgetTypeID==NULL)/* Use the cache */
      {
         jobjectgetTypeID = curEnv->GetMethodID(this->instanceClass, "getType", "()Lorg/netxms/agent/ParameterType;" ) ;
         if (jobjectgetTypeID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("TableColumn: Could not access to the method getType"));

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

         return *(new ParameterType(jvm, res));
      }
      else
      {
         curEnv->DeleteLocalRef(res);
         return NULL;
      }
   }

   bool TableColumn::isInstance ()
   {

      JNIEnv * curEnv = getCurrentEnv();

                                 /* Use the cache */
      if (jbooleanisInstanceID==NULL)
      {
         jbooleanisInstanceID = curEnv->GetMethodID(this->instanceClass, "isInstance", "()Z" ) ;
         if (jbooleanisInstanceID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("TableColumn: Could not access to the method isInstance"));

            throw JNIException();
         }
      }
      jboolean res =  static_cast<jboolean>( curEnv->CallBooleanMethod( this->instance, jbooleanisInstanceID ));

      return (res == JNI_TRUE);

   }

}
