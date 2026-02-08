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
** File: class.cpp
**
**/

#include "libnxsl.h"

/**
 * Class registry. ObjectArray cannot be used here to ensure that registry initialization
 * is done as part of static initialization before any global variable defining actual NXSL class is initialized.
 */
NXSL_ClassRegistry g_nxslClassRegistry = { 0, 0, nullptr };

/**
 * Instance of NXSL_Class
 */
NXSL_Class LIBNXSL_EXPORTABLE g_nxslBaseClass;

/**
 * Instance of NXSL_MetaClass
 */
NXSL_MetaClass LIBNXSL_EXPORTABLE g_nxslMetaClass;

/**
 * Method __get for NXSL base class
 */
NXSL_METHOD_DEFINITION(Object, __get)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   NXSL_Value *v = object->getClass()->getAttr(object, argv[0]->getValueAsMBString());
   *result = (v != nullptr) ? v : vm->createValue();
   return 0;
}

/**
 * Method __invoke for NXSL base class
 */
NXSL_METHOD_DEFINITION(Object, __invoke)
{
   if (argc < 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   return object->getClass()->callMethod(argv[0]->getValueAsMBString(), object, argc - 1, &argv[1], result, vm);
}

/**
 * Class constructor
 */
NXSL_Class::NXSL_Class() : m_metadataLock(MutexType::FAST)
{
   setName(_T("Object"));
   m_methods = new HashMap<NXSL_Identifier, NXSL_ExtMethod>(Ownership::True);

   NXSL_REGISTER_METHOD(Object, __get, 1);
   NXSL_REGISTER_METHOD(Object, __invoke, -1);

   if (g_nxslClassRegistry.size == g_nxslClassRegistry.allocated)
   {
      g_nxslClassRegistry.allocated += 64;
      g_nxslClassRegistry.classes = MemReallocArray(g_nxslClassRegistry.classes, g_nxslClassRegistry.allocated);
   }
   g_nxslClassRegistry.classes[g_nxslClassRegistry.size++] = this;
}

/**
 * Class destructor
 */
NXSL_Class::~NXSL_Class()
{
   delete m_methods;
}

/**
 * Set class name. Should be called only from constructor.
 */
void NXSL_Class::setName(const TCHAR *name)
{
   _tcslcpy(m_name, name, MAX_CLASS_NAME);
   m_classHierarchy.add(name);
}

/**
 * Get attribute
 * Default implementation always returns error
 */
NXSL_Value *NXSL_Class::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   if (NXSL_COMPARE_ATTRIBUTE_NAME("__class"))
      return object->vm()->createValue(object->vm()->createObject(&g_nxslMetaClass, object->getClass()));
   return nullptr;
}

/**
 * Set attribute
 * Default implementation always returns error
 */
bool NXSL_Class::setAttr(NXSL_Object *object, const NXSL_Identifier& attr, NXSL_Value *value)
{
   return false;
}

/**
 * Scan class attributes
 */
void NXSL_Class::scanAttributes()
{
   m_metadataLock.lock();
   if (m_attributes.isEmpty())
   {
      NXSL_VM vm(new NXSL_Environment());
      NXSL_Object *object = vm.createObject(&g_nxslBaseClass, nullptr);
      NXSL_Value *v = getAttr(object, "?"); // will populate m_attributes
      if (v != nullptr)
         vm.destroyValue(v);
      vm.destroyObject(object);
   }
   m_metadataLock.unlock();
}

/**
 * Get next value for iteration
 * Default implementation always returns null
 */
NXSL_Value *NXSL_Class::getNext(NXSL_Object *object, NXSL_Value *currValue)
{
   return nullptr;
}

/**
 * Call method
 * Default implementation calls methods registered with NXSL_REGISTER_METHOD macro.
 */
int NXSL_Class::callMethod(const NXSL_Identifier& name, NXSL_Object *object, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   NXSL_ExtMethod *m = m_methods->get(name);
   if (m != nullptr)
   {
      if ((argc == m->numArgs) || (m->numArgs == -1))
         return m->handler(object, argc, argv, result, vm);
      else
         return NXSL_ERR_INVALID_ARGUMENT_COUNT;
   }
   return NXSL_ERR_NO_SUCH_METHOD;
}

/**
 * Object creation handler. Called by interpreter immediately after new object was created.
 */
void NXSL_Class::onObjectCreate(NXSL_Object *object)
{
}

/**
 * Object deletion handler. Called by interpreter when last reference to object being deleted.
 */
void NXSL_Class::onObjectDelete(NXSL_Object *object)
{
}

/**
 * Convert object to string
 */
void NXSL_Class::toString(StringBuffer *sb, NXSL_Object *object)
{
   TCHAR buffer[128];
   _sntprintf(buffer, 128, _T("%s@%p"), m_name, object);
   sb->append(buffer);
}

/**
 * Get JSON representation of NXSL object
 */
json_t *NXSL_Class::toJson(NXSL_Object *object, int depth)
{
   json_t *jobject = json_object();
   if (depth <= 0)
      return jobject;

   scanAttributes();
   for(const TCHAR *a : getAttributes())
   {
      if (!_tcscmp(a, _T("__class")))
         continue;

      NXSL_Value *v = getAttr(object, a);
      if (v != nullptr)
      {
         char jkey[1024];
         tchar_to_utf8(a, -1, jkey, 1024);
         jkey[1023] = 0;
         json_object_set_new(jobject, jkey, v->toJson(depth - 1));
      }
   }
   return jobject;
}

/**
 * Class "Class" constructor
 */
NXSL_MetaClass::NXSL_MetaClass() : NXSL_Class()
{
   setName(_T("Class"));
}

/**
 * Callback to fill method list
 */
static EnumerationCallbackResult FillMethodList(const NXSL_Identifier& name, NXSL_ExtMethod *method, NXSL_Array *array)
{
   array->append(array->vm()->createValue(name.value));
   return _CONTINUE;
}

/**
 * Class "Class" attribute handler
 */
NXSL_Value *NXSL_MetaClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   NXSL_Class *c = static_cast<NXSL_Class*>(object->getData());
   if (NXSL_COMPARE_ATTRIBUTE_NAME("attributes"))
   {
      c->scanAttributes();
      value = vm->createValue(new NXSL_Array(vm, c->getAttributes()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("hierarchy"))
   {
      value = vm->createValue(new NXSL_Array(vm, c->getClassHierarchy()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("methods"))
   {
      NXSL_Array *values = new NXSL_Array(vm);
      c->m_methods->forEach(FillMethodList, values);
      value = vm->createValue(values);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("name"))
   {
      value = vm->createValue(c->getName());
   }
   return value;
}

/**
 * Create new NXSL object of given class
 */
NXSL_Object::NXSL_Object(NXSL_VM *vm, NXSL_Class *nxslClass, const void *data, bool constant) : NXSL_RuntimeObject(vm)
{
   m_class = nxslClass;
   m_data = vm->allocateObjectClassData();
   m_data->refCount = 1;
   m_data->data = (void *)data;
   m_data->constant = constant;
   m_class->onObjectCreate(this);
}

/**
 * Create new reference to existing object
 */
NXSL_Object::NXSL_Object(NXSL_Object *object) : NXSL_RuntimeObject(object->m_vm)
{
   m_class = object->m_class;
   m_data = object->m_data;
	m_data->refCount++;
}

/**
 * Default constructor (needed for correct template instantiation but should not be used)
 */
NXSL_Object::NXSL_Object() : NXSL_RuntimeObject(nullptr)
{
   m_class = nullptr;
   m_data = nullptr;
}

/**
 * Object destructor
 */
NXSL_Object::~NXSL_Object()
{
	m_data->refCount--;
	if (m_data->refCount == 0)
	{
	   if (!m_data->constant)
	      m_class->onObjectDelete(this);
	   m_vm->freeObjectClassData(m_data);
	}
}
