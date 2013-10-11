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
 ** File: TableParameter.cpp
 **
 **/

#include "java_subagent.h"
#include "TableParameter.h"
#include "JNIException.h"

namespace org_netxms_agent
{

   // Static declarations (if any)

   // Returns the current env

   JNIEnv * TableParameter::getCurrentEnv()
   {
      JNIEnv * curEnv = NULL;
      jint res=this->jvm->AttachCurrentThread(reinterpret_cast<void **>(&curEnv), NULL);
      if (res != JNI_OK)
      {
         AgentWriteLog(NXLOG_ERROR, _T("TableParameter: Could not retrieve the current JVM."));
         throw JNIException();

      }
      return curEnv;
   }
   // Destructor

   TableParameter::~TableParameter()
   {
      JNIEnv * curEnv = NULL;
      this->jvm->AttachCurrentThread(reinterpret_cast<void **>(&curEnv), NULL);

      curEnv->DeleteGlobalRef(this->instance);
      curEnv->DeleteGlobalRef(this->instanceClass);
   }
   // Constructors
   TableParameter::TableParameter(JavaVM * jvm_)
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
         AgentWriteLog(NXLOG_ERROR, _T("TableParameter: Could not get the Class %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      this->instanceClass = static_cast<jclass>(curEnv->NewGlobalRef(localClass));

      /* localClass is not needed anymore */
      curEnv->DeleteLocalRef(localClass);

      if (this->instanceClass == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("TableParameter: Could not create a Global Ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }

      constructObject = curEnv->GetMethodID( this->instanceClass, construct , param ) ;
      if(constructObject == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("TableParameter: Could not retrieve the constructor of the class %s with the profile : %s%s"), WideStringFromMBString(this->className()), WideStringFromMBString(construct), WideStringFromMBString(param));

         throw JNIException();
      }

      localInstance = curEnv->NewObject( this->instanceClass, constructObject ) ;
      if(localInstance == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("TableParameter: Could not instantiate the object %s with the constructor : %s%s"), WideStringFromMBString(this->className()), WideStringFromMBString(construct), WideStringFromMBString(param));

         throw JNIException();
      }

      this->instance = curEnv->NewGlobalRef(localInstance) ;
      if(this->instance == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("TableParameter: Could not create a new global ref of %s"), WideStringFromMBString(this->className()));

         throw JNIException();
      }
      /* localInstance not needed anymore */
      curEnv->DeleteLocalRef(localInstance);

      /* Methods ID set to NULL */
      jstringgetNameID=NULL;
      jstringgetDescriptionID=NULL;
      jobjectArray_getColumnsID=NULL;
      jobjectArray__getValuesjstringjava_lang_StringID=NULL;

   }

   TableParameter::TableParameter(JavaVM * jvm_, jobject JObj)
   {
      jvm=jvm_;

      JNIEnv * curEnv = getCurrentEnv();

      jclass localClass = curEnv->GetObjectClass(JObj);
      this->instanceClass = static_cast<jclass>(curEnv->NewGlobalRef(localClass));
      curEnv->DeleteLocalRef(localClass);

      if (this->instanceClass == NULL)
      {

         AgentWriteLog(NXLOG_ERROR, _T("TableParameter: Could not create a Global Ref of %hs"), className());

         throw JNIException();
      }

      this->instance = curEnv->NewGlobalRef(JObj) ;
      if(this->instance == NULL)
      {

         AgentWriteLog(NXLOG_ERROR, _T("TableParameter: Could not create a new global ref of %hs"), className());

         throw JNIException();
      }
      /* Methods ID set to NULL */
      jstringgetNameID=NULL;
      jstringgetDescriptionID=NULL;
      jobjectArray_getColumnsID=NULL;
      jobjectArray__getValuesjstringjava_lang_StringID=NULL;

   }

   // Generic methods

   void TableParameter::synchronize()
   {
      if (getCurrentEnv()->MonitorEnter(instance) != JNI_OK)
      {
         AgentWriteLog(NXLOG_ERROR, _T("TableParameter: Fail to enter monitor."));
         throw JNIException();

      }
   }

   void TableParameter::endSynchronize()
   {
      if ( getCurrentEnv()->MonitorExit(instance) != JNI_OK)
      {

         AgentWriteLog(NXLOG_ERROR, _T("TableParameter: Fail to exit monitor."));
         throw JNIException();
      }
   }
   // Method(s)

   TCHAR* TableParameter::getName ()
   {

      JNIEnv * curEnv = getCurrentEnv();

      if (jstringgetNameID==NULL)/* Use the cache */
      {
         jstringgetNameID = curEnv->GetMethodID(this->instanceClass, "getName", "()Ljava/lang/String;" ) ;
         if (jstringgetNameID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("TableParameter: Could not access to the method getName"));

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

   TCHAR* TableParameter::getDescription ()
   {

      JNIEnv * curEnv = getCurrentEnv();

                                 /* Use the cache */
      if (jstringgetDescriptionID==NULL)
      {
         jstringgetDescriptionID = curEnv->GetMethodID(this->instanceClass, "getDescription", "()Ljava/lang/String;" ) ;
         if (jstringgetDescriptionID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("TableParameter: Could not access to the method getDescription"));

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
      return NULL;
   }

   TableColumn** TableParameter::getColumns (int *lenRow)
   {

      JNIEnv * curEnv = getCurrentEnv();

                                 /* Use the cache */
      if (jobjectArray_getColumnsID==NULL)
      {
         jobjectArray_getColumnsID = curEnv->GetMethodID(this->instanceClass, "getColumns", "()[Lorg/netxms/agent/TableColumn;" ) ;
         if (jobjectArray_getColumnsID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("TableParameter: Could not access to the method getColumns"));

            throw JNIException();
         }
      }
      jobjectArray res =  static_cast<jobjectArray>( curEnv->CallObjectMethod( this->instance, jobjectArray_getColumnsID ));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe() ;
      }
      if (res != NULL)
      {
         * lenRow = curEnv->GetArrayLength(res);

         TableColumn** arrayOfColumn;
         arrayOfColumn = new TableColumn * [*lenRow];
         for (jsize i = 0; i < *lenRow; i++)
         {
            jobject resColumn = reinterpret_cast<jobject>(curEnv->GetObjectArrayElement(res, i));
            arrayOfColumn[i] = new TableColumn(jvm, resColumn);
         }

         curEnv->DeleteLocalRef(res);
         return arrayOfColumn;
      }
      else
      {
         curEnv->DeleteLocalRef(res);
         return NULL;
      }
   }

   TCHAR*** TableParameter::getValues (TCHAR const* param, int *lenRow, int *lenCol)
   {

      JNIEnv * curEnv = getCurrentEnv();

                                 /* Use the cache */
      if (jobjectArray__getValuesjstringjava_lang_StringID==NULL)
      {
         jobjectArray__getValuesjstringjava_lang_StringID = curEnv->GetMethodID(this->instanceClass, "getValues", "(Ljava/lang/String;)[[Ljava/lang/String;" ) ;
         if (jobjectArray__getValuesjstringjava_lang_StringID == NULL)
         {
            AgentWriteLog(NXLOG_ERROR, _T("TableParameter: Could not access to the method getValues"));

            throw JNIException();
         }
      }
      jstring param_ = JavaStringFromCString(curEnv, param);
      if (param != NULL && param_ == NULL)
      {
         AgentWriteLog(NXLOG_ERROR, _T("TableParameter: Could not convert C string to Java  string, memory full."));
         throw JNIException();
      }

      jobjectArray res =  static_cast<jobjectArray>( curEnv->CallObjectMethod( this->instance, jobjectArray__getValuesjstringjava_lang_StringID ,param_));
      if (curEnv->ExceptionCheck())
      {
         curEnv->ExceptionDescribe() ;
      }
      curEnv->DeleteLocalRef(param_);
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
}
