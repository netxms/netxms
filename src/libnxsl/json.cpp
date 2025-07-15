/*
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2025 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: json.cpp
**
**/

#include "libnxsl.h"

/**
 * Instance of NXSL_JsonObjectClass
 */
NXSL_JsonObjectClass LIBNXSL_EXPORTABLE g_nxslJsonObjectClass;

/**
 * Instance of NXSL_JsonArrayClass
 */
NXSL_JsonArrayClass LIBNXSL_EXPORTABLE g_nxslJsonArrayClass;

/**
 * Create NXSL value from json_t
 */
static NXSL_Value *ValueFromJson(NXSL_VM *vm, json_t *json)
{
   NXSL_Value *value = nullptr;
   if (json != nullptr)
   {
      switch(json_typeof(json))
      {
         case JSON_OBJECT:
            json_incref(json);
            value = vm->createValue(vm->createObject(&g_nxslJsonObjectClass, json));
            break;
         case JSON_ARRAY:
            json_incref(json);
            value = vm->createValue(vm->createObject(&g_nxslJsonArrayClass, json));
            break;
         case JSON_STRING:
            value = vm->createValue(json_string_value(json));
            break;
         case JSON_INTEGER:
            value = vm->createValue(static_cast<int64_t>(json_integer_value(json)));
            break;
         case JSON_REAL:
            value = vm->createValue(json_real_value(json));
            break;
         case JSON_TRUE:
            value = vm->createValue(true);
            break;
         case JSON_FALSE:
            value = vm->createValue(false);
            break;
         default:
            value = vm->createValue();
            break;
      }
   }
   else
   {
      value = vm->createValue();
   }
   return value;
}

/**
 * Create json_t from NXSL value
 */
json_t *NXSL_Value::toJson(int depth)
{
   if (isInteger())
      return json_integer(getValueAsInt64());
   if (m_dataType == NXSL_DT_REAL)
      return json_real(getValueAsReal());
   if (m_dataType == NXSL_DT_BOOLEAN)
      return json_boolean(isTrue());
   if (isString())
      return json_string_t(getValueAsCString());
   if (m_dataType == NXSL_DT_ARRAY)
   {
      json_t *jarray = json_array();
      if (depth > 0)
      {
         NXSL_Array *a = m_value.arrayHandle->getObject();
         for(int i = 0; i < a->size(); i++)
         {
            NXSL_Value *v = a->getByPosition(i);
            if (v->isObject(_T("JsonObject")) || v->isObject(_T("JsonArray")))
            {
               json_array_append(jarray, static_cast<json_t*>(v->getValueAsObject()->getData()));
            }
            else
            {
               json_array_append_new(jarray, v->toJson(depth - 1));
            }
         }
      }
      return jarray;
   }
   if (m_dataType == NXSL_DT_HASHMAP)
   {
      json_t *jobject = json_object();
      if (depth > 0)
      {
         NXSL_HashMap *m = m_value.hashMapHandle->getObject();
         StringList keys = m->getKeysAsList();
         for(int i = 0; i < keys.size(); i++)
         {
            const TCHAR *key = keys.get(i);
            NXSL_Value *v = m->get(key);

            char jkey[1024];
            tchar_to_utf8(key, -1, jkey, 1024);
            jkey[1023] = 0;

            if (v->isObject(_T("JsonObject")) || v->isObject(_T("JsonArray")))
            {
               json_object_set(jobject, jkey, static_cast<json_t*>(v->getValueAsObject()->getData()));
            }
            else
            {
               json_object_set_new(jobject, jkey, v->toJson(depth - 1));
            }
         }
      }
      return jobject;
   }
   if (m_dataType == NXSL_DT_OBJECT)
   {
      return m_value.object->getClass()->toJson(m_value.object, depth);
   }
   return json_null();
}

/**
 * Set json object attribute from NXSL value object
 */
static void SetAttribute(json_t *json, const char *attr, NXSL_Value *value)
{
   if (value->isObject(_T("JsonObject")) || value->isObject(_T("JsonArray")))
   {
      json_object_set(json, attr, static_cast<json_t*>(value->getValueAsObject()->getData()));
   }
   else
   {
      json_object_set_new(json, attr, value->toJson());
   }
}

/**
 * JsonObject method keys()
 */
NXSL_METHOD_DEFINITION(JsonObject, keys)
{
   NXSL_Array *keys = new NXSL_Array(vm);
   json_t *json = static_cast<json_t*>(object->getData());
   void *it = json_object_iter(json);
   while(it != nullptr)
   {
      keys->append(vm->createValue(json_object_iter_key(it)));
      it = json_object_iter_next(json, it);
   }
   *result = vm->createValue(keys);
   return 0;
}

/**
 * JsonObject method serialize()
 */
NXSL_METHOD_DEFINITION(JsonObject, serialize)
{
   char *s = json_dumps(static_cast<json_t*>(object->getData()), JSON_INDENT(3));
   *result = vm->createValue(s);
   MemFree(s);
   return 0;
}

/**
 * JsonObject method get(key)
 */
NXSL_METHOD_DEFINITION(JsonObject, get)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   char attr[256];
#ifdef UNICODE
   wchar_to_utf8(argv[0]->getValueAsCString(), -1, attr, 256);
#else
   mb_to_utf8(argv[0]->getValueAsCString(), -1, attr, 256);
#endif
   attr[255] = 0;

   *result = ValueFromJson(vm, json_object_get(static_cast<json_t*>(object->getData()), attr));
   return 0;
}

/**
 * JsonObject method set(key, value)
 */
NXSL_METHOD_DEFINITION(JsonObject, set)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   char attr[256];
#ifdef UNICODE
   wchar_to_utf8(argv[0]->getValueAsCString(), -1, attr, 256);
#else
   mb_to_utf8(argv[0]->getValueAsCString(), -1, attr, 256);
#endif
   attr[255] = 0;

   SetAttribute(static_cast<json_t*>(object->getData()), attr, argv[1]);
   *result = vm->createValue();
   return 0;
}

/**
 * Implementation of "JsonObject" class: constructor
 */
NXSL_JsonObjectClass::NXSL_JsonObjectClass() : NXSL_Class()
{
   setName(_T("JsonObject"));

   NXSL_REGISTER_METHOD(JsonObject, get, 1);
   NXSL_REGISTER_METHOD(JsonObject, keys, 0);
   NXSL_REGISTER_METHOD(JsonObject, serialize, 0);
   NXSL_REGISTER_METHOD(JsonObject, set, 2);
}

/**
 * Object delete
 */
void NXSL_JsonObjectClass::onObjectDelete(NXSL_Object *object)
{
   json_decref(static_cast<json_t*>(object->getData()));
}

/**
 * Implementation of "JsonObject" class: get attribute
 */
NXSL_Value *NXSL_JsonObjectClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;
   if (attr.value[0] == '?') // attribute scan
      return nullptr;
   return ValueFromJson(object->vm(), json_object_get(static_cast<json_t*>(object->getData()), attr.value));
}

/**
 * Implementation of "JsonObject" class: set attribute
 */
bool NXSL_JsonObjectClass::setAttr(NXSL_Object *object, const NXSL_Identifier& attr, NXSL_Value *value)
{
   SetAttribute(static_cast<json_t*>(object->getData()), attr.value, value);
   return true;
}

/**
 * Implementation of "JsonObject" class: toJson()
 */
json_t *NXSL_JsonObjectClass::toJson(NXSL_Object *object, int depth)
{
   return json_incref(static_cast<json_t*>(object->getData()));
}

/**
 * JsonArray method serialize()
 */
NXSL_METHOD_DEFINITION(JsonArray, serialize)
{
   char *s = json_dumps(static_cast<json_t*>(object->getData()), JSON_INDENT(3));
   *result = vm->createValue(s);
   MemFree(s);
   return 0;
}

/**
 * JsonArray method append(value)
 */
NXSL_METHOD_DEFINITION(JsonArray, append)
{
   if (argv[0]->isObject(_T("JsonObject")) || argv[0]->isObject(_T("JsonArray")))
   {
      json_array_append(static_cast<json_t*>(object->getData()),
               static_cast<json_t*>(argv[0]->getValueAsObject()->getData()));
   }
   else
   {
      json_array_append_new(static_cast<json_t*>(object->getData()), argv[0]->toJson());
   }

   *result = vm->createValue();
   return 0;
}

/**
 * JsonArray method get(index)
 */
NXSL_METHOD_DEFINITION(JsonArray, get)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   *result = ValueFromJson(vm, json_array_get(static_cast<json_t*>(object->getData()), argv[0]->getValueAsUInt32()));
   return 0;
}

/**
 * JsonArray method get(index)
 */
NXSL_METHOD_DEFINITION(JsonArray, insert)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   if (argv[1]->isObject(_T("JsonObject")) || argv[1]->isObject(_T("JsonArray")))
   {
      json_array_insert(static_cast<json_t*>(object->getData()), argv[0]->getValueAsUInt32(),
               static_cast<json_t*>(argv[0]->getValueAsObject()->getData()));
   }
   else
   {
      json_array_insert_new(static_cast<json_t*>(object->getData()), argv[0]->getValueAsUInt32(), argv[1]->toJson());
   }

   *result = vm->createValue();
   return 0;
}

/**
 * JsonArray method get(index)
 */
NXSL_METHOD_DEFINITION(JsonArray, set)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   if (argv[1]->isObject(_T("JsonObject")) || argv[1]->isObject(_T("JsonArray")))
   {
      json_array_set(static_cast<json_t*>(object->getData()), argv[0]->getValueAsUInt32(),
               static_cast<json_t*>(argv[0]->getValueAsObject()->getData()));
   }
   else
   {
      json_array_set_new(static_cast<json_t*>(object->getData()), argv[0]->getValueAsUInt32(), argv[1]->toJson());
   }

   *result = vm->createValue();
   return 0;
}

/**
 * Implementation of "JsonArray" class: constructor
 */
NXSL_JsonArrayClass::NXSL_JsonArrayClass() : NXSL_Class()
{
   setName(_T("JsonArray"));

   NXSL_REGISTER_METHOD(JsonArray, append, 1);
   NXSL_REGISTER_METHOD(JsonArray, get, 1);
   NXSL_REGISTER_METHOD(JsonArray, insert, 2);
   NXSL_REGISTER_METHOD(JsonArray, serialize, 0);
   NXSL_REGISTER_METHOD(JsonArray, set, 2);
}

/**
 * Object delete
 */
void NXSL_JsonArrayClass::onObjectDelete(NXSL_Object *object)
{
   json_decref(static_cast<json_t*>(object->getData()));
}

/**
 * Implementation of "JsonArray" class: get attribute
 */
NXSL_Value *NXSL_JsonArrayClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   if (NXSL_COMPARE_ATTRIBUTE_NAME("size"))
   {
      value = vm->createValue(static_cast<uint32_t>(json_array_size(static_cast<json_t*>(object->getData()))));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("values"))
   {
      NXSL_Array *values = new NXSL_Array(vm);
      json_t *jarray = static_cast<json_t*>(object->getData());
      size_t size = json_array_size(jarray);
      for(size_t i = 0; i < size; i++)
      {
         values->append(ValueFromJson(vm, json_array_get(jarray, i)));
      }
      value = vm->createValue(values);
   }
   return value;
}

/**
 * Implementation of "JsonArray" class: toJson()
 */
json_t *NXSL_JsonArrayClass::toJson(NXSL_Object *object, int depth)
{
   return json_incref(static_cast<json_t*>(object->getData()));
}

/**
 * NXSL constructor for "JsonObject" class
 */
int F_JsonObject(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (argc > 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if ((argc == 1) && !argv[0]->isHashMap() && !argv[0]->isObject() && !argv[0]->isNull())
      return NXSL_ERR_NOT_HASHMAP;

   *result = vm->createValue(vm->createObject(&g_nxslJsonObjectClass, ((argc == 0) || argv[0]->isNull()) ? json_object() : argv[0]->toJson()));
   return 0;
}

/**
 * NXSL constructor for "JsonArray" class
 */
int F_JsonArray(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (argc > 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if ((argc == 1) && !argv[0]->isArray() && !argv[0]->isNull())
      return NXSL_ERR_NOT_ARRAY;

   *result = vm->createValue(vm->createObject(&g_nxslJsonArrayClass, ((argc == 0) || argv[0]->isNull()) ? json_array() : argv[0]->toJson()));
   return 0;
}

/**
 * Parse JSON
 */
int F_JsonParse(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   size_t flags = ((argc > 1) && argv[1]->isTrue()) ? JSON_DECODE_INT_AS_REAL : 0;
#ifdef UNICODE
   char *utfString = UTF8StringFromWideString(argv[0]->getValueAsCString());
#else
   char *utfString = UTF8StringFromMBString(argv[0]->getValueAsCString());
#endif
   json_error_t error;
   json_t *json = json_loads(utfString, flags, &error);
   MemFree(utfString);

   if (json == nullptr)
   {
      nxlog_debug_tag(_T("nxsl"), 5, _T("JsonParse: JSON parsing error at line %d, column %d (%hs)"), error.line, error.column, error.text);
      vm->setGlobalVariable("$jsonErrorColumn", vm->createValue(error.column));
      vm->setGlobalVariable("$jsonErrorLine", vm->createValue(error.line));
      vm->setGlobalVariable("$jsonErrorMessage", vm->createValue(error.text));
      *result = vm->createValue();
   }
   else
   {
      *result = (json_typeof(json) == JSON_ARRAY) ? vm->createValue(vm->createObject(&g_nxslJsonArrayClass, json)) : vm->createValue(vm->createObject(&g_nxslJsonObjectClass, json));
   }
   return 0;
}
