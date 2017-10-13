/* 
 ** NetXMS Java Bridge
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
 ** File: config.cpp
 **
 **/

#include "libnxjava.h"

/**
 * Java class names
 */
static const char *s_configClassName = "org/netxms/bridge/Config";
static const char *s_configEntryClassName = "org/netxms/bridge/ConfigEntry";

/**
 * Global class references
 */
static jclass s_configClass = NULL;
static jclass s_configEntryClass = NULL;
static jmethodID s_configConstructor = NULL;
static jmethodID s_configEntryConstructor = NULL;

/**
 * Retrieves pointer to native Config from Java object
 */
static Config *RetrieveConfigNativePointer(JNIEnv *env, jobject obj)
{
   jclass objClass = env->GetObjectClass(obj);
   if (objClass == NULL)
   {
      nxlog_write_generic(NXLOG_ERROR, _T("JavaBridge: Could not access to the class Config"));
      return NULL;
   }
   jfieldID nativePointerFieldId = env->GetFieldID(objClass, "configHandle", "J");
   if (nativePointerFieldId == NULL)
   {
      nxlog_write_generic(NXLOG_ERROR, _T("JavaBridge: Could not access to the field Config.configHandle"));
      return NULL;
   }
   jlong result = env->GetLongField(obj, nativePointerFieldId);
   return (Config *)result;
}

/**
 * Retrieves pointer to native ConfigEntry from Java object
 */
static ConfigEntry *RetrieveConfigEntryNativePointer(JNIEnv *env, jobject obj)
{
   jclass objClass = env->GetObjectClass(obj);
   if (objClass == NULL)
   {
      nxlog_write_generic(NXLOG_ERROR, _T("JavaBridge: Could not access to the class ConfigEntry"));
      return NULL;
   }
   jfieldID nativePointerFieldId = env->GetFieldID(objClass, "configEntryHandle", "J");
   if (nativePointerFieldId == NULL)
   {
      nxlog_write_generic(NXLOG_ERROR, _T("JavaBridge: Could not access to the field ConfigEntry.configHandle"));
      return NULL;
   }
   jlong result = env->GetLongField(obj, nativePointerFieldId);
   return (ConfigEntry *) result;
}

/**
 * Create instance of ConfigEntry object
 */
static jobject CreateConfigEntryInstance(JNIEnv *curEnv, ConfigEntry *configEntry)
{
   if (s_configEntryConstructor == NULL)
      return NULL;

   jobject localInstance = curEnv->NewObject(s_configEntryClass, s_configEntryConstructor, CAST_FROM_POINTER(configEntry, jlong));
   if (localInstance == NULL)
   {
      nxlog_write_generic(NXLOG_ERROR, _T("JavaBridge: Could not instantiate object of class %hs"), s_configEntryClassName);
      return NULL;
   }

   jobject instance = curEnv->NewGlobalRef(localInstance);
   if (instance == NULL)
   {
      nxlog_write_generic(NXLOG_ERROR, _T("JavaBridge: Could not create a new global reference of %hs"), s_configEntryClassName);
   }

   curEnv->DeleteLocalRef(localInstance);
   return instance;
}

/*
 * Class:     org_netxms_agent_Config
 * Method:    lock
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_netxms_agent_Config_lock(JNIEnv *jenv, jobject jobj)
{
   Config *config = RetrieveConfigNativePointer(jenv, jobj);
   if (config != NULL)
   {
      config->lock();
   }
}

/*
 * Class:     org_netxms_agent_Config
 * Method:    unlock
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_netxms_agent_Config_unlock(JNIEnv *jenv, jobject jobj)
{
   Config *config = RetrieveConfigNativePointer(jenv, jobj);
   if (config != NULL)
   {
      config->unlock();
   }
}

/**
 * Class:     org_netxms_agent_Config
 * Method:    deleteEntry
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_netxms_agent_Config_deleteEntry(JNIEnv *jenv, jobject jobj, jstring jpath)
{
   Config *config = RetrieveConfigNativePointer(jenv, jobj);
   if (config == NULL)
   {
      return;
   }
   if (jpath != NULL)
   {
      TCHAR *path = CStringFromJavaString(jenv, jpath);
      config->deleteEntry(path);
      free(path);
   }
}

/**
 * Class:     org_netxms_agent_Config
 * Method:    getEntry
 * Signature: (Ljava/lang/String;)Lorg/netxms/bridge/ConfigEntry;
 */
JNIEXPORT jobject JNICALL Java_org_netxms_agent_Config_getEntry(JNIEnv *jenv, jobject jobj, jstring jpath)
{
   jobject result = NULL;
   Config *config = RetrieveConfigNativePointer(jenv, jobj);
   if (config == NULL)
   {
      return result;
   }
   if (jpath != NULL)
   {
      TCHAR *path = CStringFromJavaString(jenv, jpath);
      ConfigEntry *configEntry = config->getEntry(path);
      if (configEntry)
      {
         result = CreateConfigEntryInstance(jenv, configEntry);
      }
      free(path);
   }
   return result;
}

/*
 * Class:     org_netxms_agent_Config
 * Method:    getSubEntries
 * Signature: (Ljava/lang/String;Ljava/lang/String;)[Lorg/netxms/bridge/ConfigEntry;
 */
JNIEXPORT jobjectArray JNICALL Java_org_netxms_agent_Config_getSubEntries(JNIEnv *jenv, jobject jobj, jstring jpath, jstring jmask)
{
   jobjectArray jresult = NULL;
   Config *config = RetrieveConfigNativePointer(jenv, jobj);
   if (config == NULL)
   {
      return jresult;
   }
   if ((jmask != NULL) && (jpath != NULL))
   {
      TCHAR *path = CStringFromJavaString(jenv, jpath);
      TCHAR *mask = CStringFromJavaString(jenv, jmask);
      ObjectArray<ConfigEntry> *configEntryList = config->getSubEntries(path, mask);
      if (configEntryList != NULL)
      {
         for (int i = 0; i < configEntryList->size(); i++)
         {
            ConfigEntry *configEntry = configEntryList->get(i);
            jobject jconfigEntry = CreateConfigEntryInstance(jenv, configEntry);
            if (i == 0)
            {
               jresult = jenv->NewObjectArray((jsize)configEntryList->size(), jenv->GetObjectClass(jconfigEntry), jconfigEntry);
            }
            jenv->SetObjectArrayElement(jresult, (jsize) i, jconfigEntry);
         }
         delete configEntryList;
      }
      free(path);
      free(mask);
   }
   return jresult;
}

/*
 * Class:     org_netxms_agent_Config
 * Method:    getOrderedSubEntries
 * Signature: (Ljava/lang/String;Ljava/lang/String;)[Lorg/netxms/bridge/ConfigEntry;
 */
JNIEXPORT jobjectArray JNICALL Java_org_netxms_agent_Config_getOrderedSubEntries(JNIEnv *jenv, jobject jobj, jstring jpath, jstring jmask)
{
   jobjectArray jresult = NULL;
   Config *config = RetrieveConfigNativePointer(jenv, jobj);
   if (config == NULL)
   {
      return jresult;
   }
   if ((jmask != NULL) && (jpath != NULL))
   {
      TCHAR *path = CStringFromJavaString(jenv, jpath);
      TCHAR *mask = CStringFromJavaString(jenv, jmask);
      ObjectArray<ConfigEntry> *configEntryList = config->getOrderedSubEntries(path, mask);
      for (int i = 0; i < configEntryList->size(); i++)
      {
         ConfigEntry *configEntry = configEntryList->get(i);
         jobject jconfigEntry = CreateConfigEntryInstance(jenv, configEntry);
         if (i == 0)
         {
            jresult = jenv->NewObjectArray((jsize) configEntryList->size(), jenv->GetObjectClass(jconfigEntry), jconfigEntry);
         }
         jenv->SetObjectArrayElement(jresult, (jsize) i, jconfigEntry);
      }
      free(path);
      free(mask);
   }
   return jresult;
}

/*
 * Class:     org_netxms_agent_Config
 * Method:    getValue
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_netxms_agent_Config_getValue(JNIEnv *jenv, jobject jobj, jstring jpath, jstring jvalue)
{
   jstring jresult = jvalue;
   Config *config = RetrieveConfigNativePointer(jenv, jobj);
   if ((config != NULL) && (jpath != NULL) && (jvalue != NULL))
   {
      TCHAR *path = CStringFromJavaString(jenv, jpath);
      TCHAR *value = CStringFromJavaString(jenv, jvalue);
      const TCHAR *result = config->getValue(path, value);
      if (result != NULL)
      {
         jresult = JavaStringFromCString(jenv, result);
      }
      free(path);
      free(value);
   }
   return jresult;
}

/*
 * Class:     org_netxms_agent_Config
 * Method:    getValueInt
 * Signature: (Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_org_netxms_agent_Config_getValueInt(JNIEnv *jenv, jobject jobj, jstring jpath, jint jvalue)
{
   Config *config = RetrieveConfigNativePointer(jenv, jobj);
   if ((config != NULL) && (jpath != NULL))
   {
      TCHAR *path = CStringFromJavaString(jenv, jpath);
      INT32 result = config->getValueAsInt(path, (INT32)jvalue);
      free(path);
      return (jint) result;
   }
   return jvalue;
}

/*
 * Class:     org_netxms_agent_Config
 * Method:    getValueLong
 * Signature: (Ljava/lang/String;J)J
 */
JNIEXPORT jlong JNICALL Java_org_netxms_agent_Config_getValueLong(JNIEnv *jenv, jobject jobj, jstring jpath, jlong jvalue)
{
   Config *config = RetrieveConfigNativePointer(jenv, jobj);
   if ((config != NULL) && (jpath != NULL))
   {
      TCHAR *path = CStringFromJavaString(jenv, jpath);
      INT64 result = config->getValueAsInt64(path, (long)jvalue);
      free(path);
      return (jlong)result;
   }
   return jvalue;
}

/*
 * Class:     org_netxms_agent_Config
 * Method:    getValueBoolean
 * Signature: (Ljava/lang/String;Z)Z
 */
JNIEXPORT jboolean JNICALL Java_org_netxms_agent_Config_getValueBoolean(JNIEnv *jenv, jobject jobj, jstring jpath, jboolean jvalue)
{
   Config *config = RetrieveConfigNativePointer(jenv, jobj);
   if ((config != NULL) && (jpath != NULL))
   {
      TCHAR *path = CStringFromJavaString(jenv, jpath);
      bool result = config->getValueAsBoolean(path, jvalue ? true : false);
      free(path);
      return (jboolean) result;
   }
   return (jboolean) jvalue;
}

/*
 * Class:     org_netxms_agent_Config
 * Method:    setValue
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_netxms_agent_Config_setValue__Ljava_lang_String_2Ljava_lang_String_2(JNIEnv *jenv, jobject jobj, jstring jpath, jstring jvalue)
{
   bool result = FALSE;
   Config *config = RetrieveConfigNativePointer(jenv, jobj);
   if ((config != NULL) && (jpath != NULL) && (jvalue != NULL))
   {
      TCHAR *path = CStringFromJavaString(jenv, jpath);
      TCHAR *value = CStringFromJavaString(jenv, jvalue);
      result = config->setValue(path, value);
      free(path);
      free(value);
   }
   return (jboolean) result;
}

/*
 * Class:     org_netxms_agent_Config
 * Method:    setValue
 * Signature: (Ljava/lang/String;I)Z
 */
JNIEXPORT jboolean JNICALL Java_org_netxms_agent_Config_setValue__Ljava_lang_String_2I(JNIEnv *jenv, jobject jobj, jstring jpath, jint jvalue)
{
   bool result = FALSE;
   Config *config = RetrieveConfigNativePointer(jenv, jobj);
   if ((config != NULL) && (jpath != NULL))
   {
      TCHAR *path = CStringFromJavaString(jenv, jpath);
      result = config->setValue(path, (INT32) jvalue);
      free(path);
   }
   return (jboolean) result;
}

/*
 * Class:     org_netxms_agent_Config
 * Method:    setValue
 * Signature: (Ljava/lang/String;J)Z
 */
JNIEXPORT jboolean JNICALL Java_org_netxms_agent_Config_setValue__Ljava_lang_String_2J(JNIEnv *jenv, jobject jobj, jstring jpath, jlong jvalue)
{
   bool result = FALSE;
   Config *config = RetrieveConfigNativePointer(jenv, jobj);
   if ((config != NULL) && (jpath != NULL))
   {
      TCHAR *path = CStringFromJavaString(jenv, jpath);
      result = config->setValue(path, (INT64) jvalue);
      free(path);
   }
   return (jboolean) result;
}

/*
 * Class:     org_netxms_agent_Config
 * Method:    setValue
 * Signature: (Ljava/lang/String;D)Z
 */
JNIEXPORT jboolean JNICALL Java_org_netxms_agent_Config_setValue__Ljava_lang_String_2D(JNIEnv *jenv, jobject jobj, jstring jpath, jdouble jvalue)
{
   bool result = FALSE;
   Config *config = RetrieveConfigNativePointer(jenv, jobj);
   if ((config != NULL) && (jpath != NULL))
   {
      TCHAR *path = CStringFromJavaString(jenv, jpath);
      result = config->setValue(path, (double) jvalue);
      free(path);
   }
   return (jboolean) result;
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getNext
 * Signature: ()Lorg/netxms/bridge/ConfigEntry;
 */
JNIEXPORT jobject JNICALL Java_org_netxms_agent_ConfigEntry_getNext(JNIEnv *jenv, jobject jobj)
{
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return NULL;
   }
   return CreateConfigEntryInstance(jenv, configEntry->getNext());
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getParent
 * Signature: ()Lorg/netxms/bridge/ConfigEntry;
 */
JNIEXPORT jobject JNICALL Java_org_netxms_agent_ConfigEntry_getParent(JNIEnv *jenv, jobject jobj)
{
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return NULL;
   }
   return CreateConfigEntryInstance(jenv, configEntry->getParent());
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_netxms_agent_ConfigEntry_getName(JNIEnv *jenv, jobject jobj)
{
   jstring jresult = NULL;
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return jresult;
   }
   const TCHAR *result = configEntry->getName();
   if (result != NULL)
   {
      jresult = JavaStringFromCString(jenv, result);
   }
   return jresult;
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    setName
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_netxms_agent_ConfigEntry_setName(JNIEnv *jenv, jobject jobj, jstring jname)
{
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return;
   }
   if (jname != NULL)
   {
      TCHAR *name = CStringFromJavaString(jenv, jname);
      configEntry->setName(name);
      free(name);
   }
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getId
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_netxms_agent_ConfigEntry_getId(JNIEnv *jenv, jobject jobj)
{
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return 0;
   }
   return (jint) configEntry->getId();
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getValueCount
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_netxms_agent_ConfigEntry_getValueCount(JNIEnv *jenv, jobject jobj)
{
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return 0;
   }
   return (jint) configEntry->getValueCount();
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getLine
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_netxms_agent_ConfigEntry_getLine(JNIEnv *jenv, jobject jobj)
{
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return 0;
   }
   return (jint) configEntry->getLine();
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    addValue
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_netxms_agent_ConfigEntry_addValue(JNIEnv *jenv, jobject jobj, jstring jvalue)
{
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return;
   }
   if (jvalue != NULL)
   {
      TCHAR *value = CStringFromJavaString(jenv, jvalue);
      configEntry->addValue(value);
      free(value);
   }
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    setValue
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_netxms_agent_ConfigEntry_setValue(JNIEnv *jenv, jobject jobj, jstring jvalue)
{
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return;
   }
   if (jvalue != NULL)
   {
      TCHAR *value = CStringFromJavaString(jenv, jvalue);
      configEntry->setValue(value);
      free(value);
   }
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getFile
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_netxms_agent_ConfigEntry_getFile(JNIEnv *jenv, jobject jobj)
{
   jstring jresult = NULL;
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return jresult;
   }
   const TCHAR *result = configEntry->getFile();
   if (result != NULL)
   {
      jresult = JavaStringFromCString(jenv, result);
   }
   return jresult;
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    createEntry
 * Signature: (Ljava/lang/String;)Lorg/netxms/bridge/ConfigEntry;
 */
JNIEXPORT jobject JNICALL Java_org_netxms_agent_ConfigEntry_createEntry(JNIEnv *jenv, jobject jobj, jstring jname)
{
   jobject jresult = NULL;
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return jresult;
   }
   if (jname != NULL)
   {
      TCHAR *name = CStringFromJavaString(jenv, jname);
      jresult = CreateConfigEntryInstance(jenv, configEntry->createEntry(name));
      free(name);
   }
   return jresult;
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    findEntry
 * Signature: (Ljava/lang/String;)Lorg/netxms/bridge/ConfigEntry;
 */
JNIEXPORT jobject JNICALL Java_org_netxms_agent_ConfigEntry_findEntry(JNIEnv *jenv, jobject jobj, jstring jname)
{
   jobject jresult = NULL;
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return jresult;
   }
   if (jname != NULL)
   {
      TCHAR *name = CStringFromJavaString(jenv, jname);
      jresult = CreateConfigEntryInstance(jenv, configEntry->findEntry(name));
      free(name);
   }
   return jresult;
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getSubEntries
 * Signature: (Ljava/lang/String;)[Lorg/netxms/bridge/ConfigEntry;
 */
JNIEXPORT jobjectArray JNICALL Java_org_netxms_agent_ConfigEntry_getSubEntries(JNIEnv *jenv, jobject jobj, jstring jmask)
{
   jobjectArray jresult = NULL;
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return jresult;
   }
   if (jmask != NULL)
   {
      TCHAR *mask = CStringFromJavaString(jenv, jmask);
      ObjectArray<ConfigEntry> *configEntryList = configEntry->getSubEntries(mask);
      if (configEntryList != NULL)
      {
         for (int i = 0; i < configEntryList->size(); i++)
         {
            ConfigEntry *subConfigEntry = configEntryList->get(i);
            jobject jconfigEntry = CreateConfigEntryInstance(jenv, subConfigEntry);
            if (i == 0)
            {
               jresult = jenv->NewObjectArray((jsize) configEntryList->size(), jenv->GetObjectClass(jconfigEntry), jconfigEntry);
            }
            jenv->SetObjectArrayElement(jresult, (jsize) i, jconfigEntry);
         }
         delete configEntryList;
      }
      free(mask);
   }
   return jresult;
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getOrderedSubEntries
 * Signature: (Ljava/lang/String;)[Lorg/netxms/bridge/ConfigEntry;
 */
JNIEXPORT jobjectArray JNICALL Java_org_netxms_agent_ConfigEntry_getOrderedSubEntries(JNIEnv *jenv, jobject jobj, jstring jmask)
{
   jobjectArray jresult = NULL;
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return jresult;
   }
   if (jmask != NULL)
   {
      TCHAR *mask = CStringFromJavaString(jenv, jmask);
      ObjectArray<ConfigEntry> *configEntryList = configEntry->getSubEntries(mask);
      if (configEntryList != NULL)
      {
         for(int i = 0; i < configEntryList->size(); i++)
         {
            ConfigEntry *subConfigEntry = configEntryList->get(i);
            jobject jconfigEntry = CreateConfigEntryInstance(jenv, subConfigEntry);
            if (i == 0)
            {
               jresult = jenv->NewObjectArray((jsize) configEntryList->size(), jenv->GetObjectClass(jconfigEntry), jconfigEntry);
            }
            jenv->SetObjectArrayElement(jresult, (jsize) i, jconfigEntry);
         }
         delete configEntryList;
      }
      free(mask);
   }
   return jresult;
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    unlinkEntry
 * Signature: (Lorg/netxms/bridge/ConfigEntry;)V
 */
JNIEXPORT void JNICALL Java_org_netxms_agent_ConfigEntry_unlinkEntry(JNIEnv *jenv, jobject jobj, jobject jentry)
{
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return;
   }
   ConfigEntry *configEntryParam = RetrieveConfigEntryNativePointer(jenv, jentry);
   if (configEntryParam == NULL)
   {
      return;
   }
   configEntry->unlinkEntry(configEntryParam);
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getValue
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_netxms_agent_ConfigEntry_getValue__I(JNIEnv *jenv, jobject jobj, jint jindex)
{
   jstring jresult = NULL;
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return jresult;
   }
   const TCHAR *result = configEntry->getValue((INT32) jindex);
   if (result != NULL)
   {
      jresult = JavaStringFromCString(jenv, result);
   }
   return jresult;
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getValue
 * Signature: (ILjava/lang/String;)Ljava/lang/String;
 */
//  JNIEXPORT jstring JNICALL Java_org_netxms_agent_ConfigEntry_getValue__ILjava_lang_String_2(JNIEnv *, jobject, jint, jstring)
//  {
//  }

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getValueInt
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_org_netxms_agent_ConfigEntry_getValueInt(JNIEnv *jenv, jobject jobj, jint jindex, jint jdefaultValue)
{
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return jdefaultValue;
   }
   return (jint) configEntry->getValueAsInt((INT32) jindex, (INT32) jdefaultValue);
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getValueLong
 * Signature: (IJ)J
 */
JNIEXPORT jlong JNICALL Java_org_netxms_agent_ConfigEntry_getValueLong(JNIEnv *jenv, jobject jobj, jint jindex, jlong jdefaultValue)
{
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return jdefaultValue;
   }
   return (jlong) configEntry->getValueAsInt64((INT32) jindex, (INT64) jdefaultValue);
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getValueBoolean
 * Signature: (IZ)Z
 */
JNIEXPORT jboolean JNICALL Java_org_netxms_agent_ConfigEntry_getValueBoolean(JNIEnv *jenv, jobject jobj, jint jindex, jboolean jdefaultValue)
{
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return jdefaultValue;
   }
   return (jboolean)configEntry->getValueAsBoolean((INT32) jindex, jdefaultValue ? true : false);
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getSubEntryValue
 * Signature: (Ljava/lang/String;ILjava/lang/String;)Ljava/lang/String;
 */
//  JNIEXPORT jstring JNICALL Java_org_netxms_agent_ConfigEntry_getSubEntryValue(JNIEnv *, jobject, jstring, jint, jstring)
//  {
//  }

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getSubEntryValueInt
 * Signature: (Ljava/lang/String;II)I
 */
JNIEXPORT jint JNICALL Java_org_netxms_agent_ConfigEntry_getSubEntryValueInt(JNIEnv *jenv, jobject jobj, jstring jname, jint jindex, jint jdefaultValue)
{
   jint jresult = jdefaultValue;
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return jresult;
   }
   if (jname != NULL)
   {
      TCHAR *name = CStringFromJavaString(jenv, jname);
      jresult = (jint) configEntry->getSubEntryValueAsInt(name, (int) jindex, (INT32) jdefaultValue );
      free(name);
   }
   return jresult;
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getSubEntryValueLong
 * Signature: (Ljava/lang/String;IJ)J
 */
JNIEXPORT jlong JNICALL Java_org_netxms_agent_ConfigEntry_getSubEntryValueLong(JNIEnv *jenv, jobject jobj, jstring jname, jint jindex, jlong jdefaultValue)
{
   jlong jresult = jdefaultValue;
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return jresult;
   }
   if (jname != NULL)
   {
      TCHAR *name = CStringFromJavaString(jenv, jname);
      jresult = (jlong) configEntry->getSubEntryValueAsInt64(name, (int) jindex, (INT64) jdefaultValue );
      free(name);
   }
   return jresult;
}

/*
 * Class:     org_netxms_agent_ConfigEntry
 * Method:    getSubEntryValueBoolean
 * Signature: (Ljava/lang/String;IZ)Z
 */
JNIEXPORT jboolean JNICALL Java_org_netxms_agent_ConfigEntry_getSubEntryValueBoolean(JNIEnv *jenv, jobject jobj, jstring jname, jint jindex, jboolean jdefaultValue)
{
   jboolean jresult = jdefaultValue;
   ConfigEntry *configEntry = RetrieveConfigEntryNativePointer(jenv, jobj);
   if (configEntry == NULL)
   {
      return jresult;
   }
   if (jname != NULL)
   {
      TCHAR *name = CStringFromJavaString(jenv, jname);
      jresult = (jboolean) configEntry->getSubEntryValueAsBoolean(name, (int)jindex, jdefaultValue ? true : false);
      free(name);
   }
   return jresult;
}

/**
 * Config class native methods
 */
static JNINativeMethod s_jniMethodsConfig[] =
{
   { (char *)"lock", (char *)"()V", (void *)Java_org_netxms_agent_Config_lock },
   { (char *)"unlock", (char *)"()V", (void *)Java_org_netxms_agent_Config_unlock },
   { (char *)"deleteEntry", (char *)"(Ljava/lang/String;)V", (void *) Java_org_netxms_agent_Config_deleteEntry },
   { (char *)"getEntry", (char *)"(Ljava/lang/String;)Lorg/netxms/bridge/ConfigEntry;", (void *) Java_org_netxms_agent_Config_getEntry },
   { (char *)"getSubEntries", (char *)"(Ljava/lang/String;Ljava/lang/String;)[Lorg/netxms/bridge/ConfigEntry;", (void *) Java_org_netxms_agent_Config_getSubEntries },
   { (char *)"getOrderedSubEntries", (char *)"(Ljava/lang/String;Ljava/lang/String;)[Lorg/netxms/bridge/ConfigEntry;", (void *) Java_org_netxms_agent_Config_getOrderedSubEntries },
   { (char *)"getValue", (char *)"(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", (void *) Java_org_netxms_agent_Config_getValue },
   { (char *)"getValueInt", (char *)"(Ljava/lang/String;I)I", (void *) Java_org_netxms_agent_Config_getValueInt },
   { (char *)"getValueLong", (char *)"(Ljava/lang/String;J)J", (void *) Java_org_netxms_agent_Config_getValueLong },
   { (char *)"getValueBoolean", (char *)"(Ljava/lang/String;Z)Z", (void *) Java_org_netxms_agent_Config_getValueBoolean },
   { (char *)"setValue", (char *)"(Ljava/lang/String;Ljava/lang/String;)Z", (void *) Java_org_netxms_agent_Config_setValue__Ljava_lang_String_2Ljava_lang_String_2 },
   { (char *)"setValue", (char *)"(Ljava/lang/String;I)Z", (void *) Java_org_netxms_agent_Config_setValue__Ljava_lang_String_2I },
   { (char *)"setValue", (char *)"(Ljava/lang/String;J)Z", (void *) Java_org_netxms_agent_Config_setValue__Ljava_lang_String_2J },
   { (char *)"setValue", (char *)"(Ljava/lang/String;D)Z", (void *) Java_org_netxms_agent_Config_setValue__Ljava_lang_String_2D }
};

/**
 * ConfigEntry class native methods
 */
static JNINativeMethod s_jniMethodsConfigEntry[] =
{
   { (char *)"getNext", (char *)"()Lorg/netxms/bridge/ConfigEntry;", (void *)Java_org_netxms_agent_ConfigEntry_getNext },
   { (char *)"getParent", (char *)"()Lorg/netxms/bridge/ConfigEntry;", (void *)Java_org_netxms_agent_ConfigEntry_getParent },
   { (char *)"getName", (char *)"()Ljava/lang/String;", (void *)Java_org_netxms_agent_ConfigEntry_getName },
   { (char *)"setName", (char *)"(Ljava/lang/String;)V", (void *)Java_org_netxms_agent_ConfigEntry_setName },
   { (char *)"getId", (char *)"()I", (void *)Java_org_netxms_agent_ConfigEntry_getId },
   { (char *)"getValueCount", (char *)"()I", (void *)Java_org_netxms_agent_ConfigEntry_getValueCount },
   { (char *)"getLine", (char *)"()I", (void *) Java_org_netxms_agent_ConfigEntry_getLine },
   { (char *)"addValue", (char *)"(Ljava/lang/String;)V", (void *) Java_org_netxms_agent_ConfigEntry_addValue },
   { (char *)"setValue", (char *)"(Ljava/lang/String;)V", (void *) Java_org_netxms_agent_ConfigEntry_setValue },
   { (char *)"getFile", (char *)"()Ljava/lang/String;", (void *) Java_org_netxms_agent_ConfigEntry_getFile },
   { (char *)"createEntry", (char *)"(Ljava/lang/String;)Lorg/netxms/bridge/ConfigEntry;", (void *) Java_org_netxms_agent_ConfigEntry_createEntry },
   { (char *)"findEntry", (char *)"(Ljava/lang/String;)Lorg/netxms/bridge/ConfigEntry;", (void *) Java_org_netxms_agent_ConfigEntry_findEntry },
   { (char *)"getSubEntries", (char *)"(Ljava/lang/String;)[Lorg/netxms/bridge/ConfigEntry;", (void *) Java_org_netxms_agent_ConfigEntry_getSubEntries },
   { (char *)"getOrderedSubEntries", (char *)"(Ljava/lang/String;)[Lorg/netxms/bridge/ConfigEntry;", (void *) Java_org_netxms_agent_ConfigEntry_getOrderedSubEntries },
   { (char *)"unlinkEntry", (char *)"(Lorg/netxms/bridge/ConfigEntry;)V", (void *) Java_org_netxms_agent_ConfigEntry_unlinkEntry },
   { (char *)"getValue", (char *)"(I)Ljava/lang/String;", (void *) Java_org_netxms_agent_ConfigEntry_getValue__I },
   //    { "getValue", "(ILjava/lang/String;)Ljava/lang/String;", (void *) Java_org_netxms_agent_ConfigEntry_getValue__ILjava_lang_String_2 },
   { (char *)"getValueInt", (char *)"(II)I", (void *) Java_org_netxms_agent_ConfigEntry_getValueInt },
   { (char *)"getValueLong", (char *)"(IJ)J", (void *) Java_org_netxms_agent_ConfigEntry_getValueLong },
   { (char *)"getValueBoolean", (char *)"(IZ)Z", (void *) Java_org_netxms_agent_ConfigEntry_getValueBoolean },
   //{ "getSubEntryValue", "(Ljava/lang/String;ILjava/lang/String;)Ljava/lang/String;", (void *) Java_org_netxms_agent_ConfigEntry_getSubEntryValue },
   { (char *)"getSubEntryValueInt", (char *)"(Ljava/lang/String;II)I", (void *) Java_org_netxms_agent_ConfigEntry_getSubEntryValueInt },
   { (char *)"getSubEntryValueLong", (char *)"(Ljava/lang/String;IJ)J", (void *) Java_org_netxms_agent_ConfigEntry_getSubEntryValueLong },
   { (char *)"getSubEntryValueBoolean", (char *)"(Ljava/lang/String;IZ)Z", (void *) Java_org_netxms_agent_ConfigEntry_getSubEntryValueBoolean },
};

/**
 * Register native methods for Config related classes
 */
bool RegisterConfigHelperNatives(JNIEnv *env)
{
   s_configClass = CreateJavaClassGlobalRef(env, s_configClassName);
   if (s_configClass == NULL)
      return false;

   s_configEntryClass = CreateJavaClassGlobalRef(env, s_configEntryClassName);
   if (s_configEntryClass == NULL)
      return false;

   s_configConstructor = env->GetMethodID(s_configClass, "<init>", "(J)V");
   if (s_configConstructor == NULL)
   {
      nxlog_write_generic(NXLOG_ERROR, _T("JavaBridge: Could not retrieve constructor for class %hs"), s_configClassName);
      return false;
   }

   s_configEntryConstructor = env->GetMethodID(s_configEntryClass, "<init>", "(J)V");
   if (s_configEntryConstructor == NULL)
   {
      nxlog_write_generic(NXLOG_ERROR, _T("JavaBridge: Could not retrieve constructor for class %hs"), s_configEntryClass);
      return false;
   }

   // register native methods exposed by Config
   if (env->RegisterNatives(s_configClass, s_jniMethodsConfig, (jint)(sizeof(s_jniMethodsConfig) / sizeof (s_jniMethodsConfig[0]))) != 0)
   {
      nxlog_write_generic(NXLOG_ERROR, _T("JavaBridge: Failed to register native methods for %hs"), s_configClassName);
      return false;
   }

   // register native methods exposed by ConfigEntry
   if (env->RegisterNatives(s_configEntryClass, s_jniMethodsConfigEntry, (jint)(sizeof(s_jniMethodsConfigEntry) / sizeof(s_jniMethodsConfigEntry[0]))) != 0)
   {
      nxlog_write_generic(NXLOG_ERROR, _T("JavaBridge: Failed to register native methods for %hs"), s_configEntryClassName);
      return false;
   }

   return true;
}

/**
 * Create Java Config object (wrapper around C++ Config class)
 *
 * @param env JNI environment for current thread
 * @param config C++ Config object
 * @return Java wrapper object or NULL on failure
 */
jobject LIBNXJAVA_EXPORTABLE CreateConfigJavaInstance(JNIEnv *env, Config *config)
{
   if ((env == NULL) || (s_configConstructor == NULL))
      return NULL;

   jobject localInstance = env->NewObject(s_configClass, s_configConstructor, CAST_FROM_POINTER(config, jlong));
   if (localInstance == NULL)
   {
      nxlog_write_generic(NXLOG_ERROR, _T("JavaBridge: Could not instantiate object of class %s"), s_configClassName);
      return NULL;
   }

   jobject instance = env->NewGlobalRef(localInstance);
   if (instance == NULL)
   {
      nxlog_write_generic(NXLOG_ERROR, _T("JavaBridge: Could not create a new global reference of %s"), s_configClassName);
   }
   env->DeleteLocalRef(localInstance);
   return instance;
}
