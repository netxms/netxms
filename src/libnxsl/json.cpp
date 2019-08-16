/*
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
   NXSL_Value *value = NULL;
   if (json != NULL)
   {
      switch(json_typeof(json))
      {
         case JSON_OBJECT:
            json_incref(json);
            value = vm->createValue(new NXSL_Object(vm, &g_nxslJsonObjectClass, json));
            break;
         case JSON_ARRAY:
            json_incref(json);
            value = vm->createValue(new NXSL_Object(vm, &g_nxslJsonArrayClass, json));
            break;
         case JSON_STRING:
            value = vm->createValue(json_string_value(json));
            break;
         case JSON_INTEGER:
            value = vm->createValue(static_cast<INT64>(json_integer_value(json)));
            break;
         case JSON_REAL:
            value = vm->createValue(json_real_value(json));
            break;
         case JSON_TRUE:
            value = vm->createValue(1);
            break;
         case JSON_FALSE:
            value = vm->createValue(0);
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
static json_t *JsonFromValue(NXSL_Value *value)
{
   if (value->isInteger())
      return json_integer(value->getValueAsInt64());
   if (value->isReal())
      return json_real(value->getValueAsReal());
   if (value->isString())
      return json_string_t(value->getValueAsCString());
   if (value->isArray())
   {
      json_t *jarray = json_array();
      NXSL_Array *a = value->getValueAsArray();
      for(int i = 0; i < a->size(); i++)
      {
         NXSL_Value *v = a->getByPosition(i);
         if (v->isObject(_T("JsonObject")) || v->isObject(_T("JsonArray")))
         {
            json_array_append(jarray, static_cast<json_t*>(v->getValueAsObject()->getData()));
         }
         else
         {
            json_array_append_new(jarray, JsonFromValue(v));
         }
      }
      return jarray;
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
      json_object_set_new(json, attr, JsonFromValue(value));
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
   while(it != NULL)
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
   free(s);
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
   WideCharToMultiByte(CP_UTF8, 0, argv[0]->getValueAsCString(), -1, attr, 256, NULL, NULL);
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
   WideCharToMultiByte(CP_UTF8, 0, argv[0]->getValueAsCString(), -1, attr, 256, NULL, NULL);
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
 * Implementation of "JsonObject" class: destructor
 */
NXSL_JsonObjectClass::~NXSL_JsonObjectClass()
{
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
NXSL_Value *NXSL_JsonObjectClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_VM *vm = object->vm();
   return ValueFromJson(object->vm(), json_object_get(static_cast<json_t*>(object->getData()), attr));
}

/**
 * Implementation of "JsonObject" class: set attribute
 */
bool NXSL_JsonObjectClass::setAttr(NXSL_Object *object, const char *attr, NXSL_Value *value)
{
   SetAttribute(static_cast<json_t*>(object->getData()), attr, value);
   return true;
}

/**
 * JsonArray method serialize()
 */
NXSL_METHOD_DEFINITION(JsonArray, serialize)
{
   char *s = json_dumps(static_cast<json_t*>(object->getData()), JSON_INDENT(3));
   *result = vm->createValue(s);
   free(s);
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
      json_array_append_new(static_cast<json_t*>(object->getData()), JsonFromValue(argv[0]));
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
      json_array_insert_new(static_cast<json_t*>(object->getData()), argv[0]->getValueAsUInt32(), JsonFromValue(argv[1]));
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
      json_array_set_new(static_cast<json_t*>(object->getData()), argv[0]->getValueAsUInt32(), JsonFromValue(argv[1]));
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
 * Implementation of "JsonArray" class: destructor
 */
NXSL_JsonArrayClass::~NXSL_JsonArrayClass()
{
}

/**
 * Object delete
 */
void NXSL_JsonArrayClass::onObjectDelete(NXSL_Object *object)
{
   json_decref(static_cast<json_t*>(object->getData()));
}

/**
 * Implementation of "JsonObject" class: get attribute
 */
NXSL_Value *NXSL_JsonArrayClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_VM *vm = object->vm();
   NXSL_Value *value = NULL;
   if (!strcmp(attr, "size"))
   {
      value = vm->createValue(static_cast<UINT32>(json_array_size(static_cast<json_t*>(object->getData()))));
   }
   else if (!strcmp(attr, "values"))
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
 * NXSL constructor for "JsonObject" class
 */
int F_JsonObject(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   *result = vm->createValue(new NXSL_Object(vm, &g_nxslJsonObjectClass, json_object()));
   return 0;
}

/**
 * NXSL constructor for "JsonArray" class
 */
int F_JsonArray(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   *result = vm->createValue(new NXSL_Object(vm, &g_nxslJsonArrayClass, json_array()));
   return 0;
}

/**
 * Parse JSON
 */
int F_JsonParse(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

#ifdef UNICODE
   char *utfString = UTF8StringFromWideString(argv[0]->getValueAsCString());
#else
   char *utfString = UTF8StringFromMBString(argv[0]->getValueAsCString());
#endif
   json_error_t error;
   json_t *json = json_loads(utfString, 0, &error);
   MemFree(utfString);
   *result = (json != NULL) ? vm->createValue(new NXSL_Object(vm, &g_nxslJsonObjectClass, json)) : vm->createValue();
   return 0;
}
