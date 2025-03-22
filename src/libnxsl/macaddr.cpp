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
** File: macaddr.cpp
**
**/

#include "libnxsl.h"

/**
 * Instance of MacAddress class
 */
NXSL_MacAddressClass LIBNXSL_EXPORTABLE g_nxslMacAddressClass;

/**
 * MacAddress method equals(a)
 */
NXSL_METHOD_DEFINITION(MacAddress, equals)
{
   if (!argv[0]->isString() && !argv[0]->isObject(_T("MacAddress")))
      return NXSL_ERR_NOT_STRING;

   MacAddress addr = argv[0]->isObject() ? *static_cast<MacAddress*>(argv[0]->getValueAsObject()->getData()) : MacAddress::parse(argv[0]->getValueAsCString());
   *result = vm->createValue(static_cast<MacAddress*>(object->getData())->equals(addr));
   return NXSL_ERR_SUCCESS;
}

/**
 * MacAddress method toString([notation])
 */
NXSL_METHOD_DEFINITION(MacAddress, toString)
{
   if (argc > 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if ((argc > 0) && !argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   MacAddressNotation notation = (argc > 0) ? (MacAddressNotation)argv[0]->getValueAsInt32() : MacAddressNotation::COLON_SEPARATED;
   *result = vm->createValue(static_cast<MacAddress*>(object->getData())->toString(notation));
   return NXSL_ERR_SUCCESS;
}

/**
 * Implementation of "MacAddress" class: constructor
 */
NXSL_MacAddressClass::NXSL_MacAddressClass() : NXSL_Class()
{
   setName(_T("MacAddress"));

   NXSL_REGISTER_METHOD(MacAddress, equals, 1);
   NXSL_REGISTER_METHOD(MacAddress, toString, -1);
}

/**
 * Object delete
 */
void NXSL_MacAddressClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<MacAddress*>(object->getData());
}

/**
 * Implementation of "MacAddress" class: get attribute
 */
NXSL_Value *NXSL_MacAddressClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   MacAddress *a = static_cast<MacAddress*>(object->getData());
   if (NXSL_COMPARE_ATTRIBUTE_NAME("isBroadcast"))
   {
      value = vm->createValue(a->isBroadcast());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isMulticast"))
   {
      value = vm->createValue(a->isMulticast());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isValid"))
   {
      value = vm->createValue(a->isValid());
   }
   return value;
}

/**
 * Convert to string representation
 */
void NXSL_MacAddressClass::toString(StringBuffer *sb, NXSL_Object *object)
{
   MacAddress *a = static_cast<MacAddress*>(object->getData());
   TCHAR buffer[64];
   sb->append(a->toString(buffer));
}

/**
 * Create NXSL object from MacAddress object
 */
NXSL_Value *NXSL_MacAddressClass::createObject(NXSL_VM *vm, const MacAddress& addr)
{
   return vm->createValue(vm->createObject(&g_nxslMacAddressClass, new MacAddress(addr)));
}

/**
 * Constructor for MacAddress class
 */
int F_MacAddress(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (argc > 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if ((argc > 0) && !argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   MacAddress addr = (argc == 0) ? MacAddress() : MacAddress::parse(argv[0]->getValueAsCString());
   *result = NXSL_MacAddressClass::createObject(vm, addr);
   return NXSL_ERR_SUCCESS;
}
