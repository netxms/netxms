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
** File: inetaddr.cpp
**
**/

#include "libnxsl.h"

/**
 * Instance of InetAddress class
 */
NXSL_InetAddressClass LIBNXSL_EXPORTABLE g_nxslInetAddressClass;

/**
 * InetAddress method contains(a)
 */
NXSL_METHOD_DEFINITION(InetAddress, contains)
{
   if (!argv[0]->isString() && !argv[0]->isObject(_T("InetAddress")))
      return NXSL_ERR_NOT_STRING;

   InetAddress addr = argv[0]->isObject() ? *static_cast<InetAddress*>(argv[0]->getValueAsObject()->getData()) : InetAddress::parse(argv[0]->getValueAsCString());
   *result = vm->createValue(static_cast<InetAddress*>(object->getData())->contains(addr));
   return NXSL_ERR_SUCCESS;
}

/**
 * InetAddress method equals(a)
 */
NXSL_METHOD_DEFINITION(InetAddress, equals)
{
   if (!argv[0]->isString() && !argv[0]->isObject(_T("InetAddress")))
      return NXSL_ERR_NOT_STRING;

   InetAddress addr = argv[0]->isObject() ? *static_cast<InetAddress*>(argv[0]->getValueAsObject()->getData()) : InetAddress::parse(argv[0]->getValueAsCString());
   *result = vm->createValue(static_cast<InetAddress*>(object->getData())->equals(addr));
   return NXSL_ERR_SUCCESS;
}

/**
 * InetAddress method inRange(a1, a2)
 */
NXSL_METHOD_DEFINITION(InetAddress, inRange)
{
   if ((!argv[0]->isString() && !argv[0]->isObject(_T("InetAddress"))) || (!argv[1]->isString() && !argv[1]->isObject(_T("InetAddress"))))
      return NXSL_ERR_NOT_STRING;

   InetAddress start = argv[0]->isObject() ? *static_cast<InetAddress*>(argv[0]->getValueAsObject()->getData()) : InetAddress::parse(argv[0]->getValueAsCString());
   InetAddress end = argv[1]->isObject() ? *static_cast<InetAddress*>(argv[1]->getValueAsObject()->getData()) : InetAddress::parse(argv[1]->getValueAsCString());

   InetAddress *addr = static_cast<InetAddress*>(object->getData());
   if ((addr->getFamily() == start.getFamily()) && (addr->getFamily() == end.getFamily()))
   {
      *result = vm->createValue((addr->compareTo(start) >= 0) && (addr->compareTo(end) <= 0));
   }
   else
   {
      *result = vm->createValue(false);
   }
   return NXSL_ERR_SUCCESS;
}

/**
 * InetAddress method sameSubnet(a)
 */
NXSL_METHOD_DEFINITION(InetAddress, sameSubnet)
{
   if (!argv[0]->isObject(_T("InetAddress")))
      return NXSL_ERR_NOT_OBJECT;

   InetAddress addr = argv[0]->isObject() ? *static_cast<InetAddress*>(argv[0]->getValueAsObject()->getData()) : InetAddress::parse(argv[0]->getValueAsCString());
   *result = vm->createValue(static_cast<InetAddress*>(object->getData())->sameSubnet(addr));
   return NXSL_ERR_SUCCESS;
}

/**
 * Implementation of "InetAddress" class: constructor
 */
NXSL_InetAddressClass::NXSL_InetAddressClass() : NXSL_Class()
{
   setName(_T("InetAddress"));

   NXSL_REGISTER_METHOD(InetAddress, contains, 1);
   NXSL_REGISTER_METHOD(InetAddress, equals, 1);
   NXSL_REGISTER_METHOD(InetAddress, inRange, 2);
   NXSL_REGISTER_METHOD(InetAddress, sameSubnet, 1);
}

/**
 * Object delete
 */
void NXSL_InetAddressClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<InetAddress*>(object->getData());
}

/**
 * Implementation of "InetAddress" class: get attribute
 */
NXSL_Value *NXSL_InetAddressClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   InetAddress *a = static_cast<InetAddress*>(object->getData());
   if (NXSL_COMPARE_ATTRIBUTE_NAME("address"))
   {
      TCHAR buffer[64];
      value = vm->createValue(a->toString(buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("family"))
   {
      value = vm->createValue((a->getFamily() == AF_INET) ? _T("inet") : (a->getFamily() == AF_INET6 ? _T("inet6") : _T("unspec")));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isAnyLocal"))
   {
      value = vm->createValue(a->isAnyLocal());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isBroadcast"))
   {
      value = vm->createValue(a->isBroadcast());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isLinkLocal"))
   {
      value = vm->createValue(a->isLinkLocal());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isLoopback"))
   {
      value = vm->createValue(a->isLoopback());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isMulticast"))
   {
      value = vm->createValue(a->isMulticast());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isSubnetBase"))
   {
      value = vm->createValue(a->getSubnetAddress().equals(*a));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isSubnetBroadcast"))
   {
      value = vm->createValue(a->isSubnetBroadcast());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isValid"))
   {
      value = vm->createValue(a->isValid());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isValidUnicast"))
   {
      value = vm->createValue(a->isValidUnicast());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("mask"))
   {
      value = vm->createValue(a->getMaskBits());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("subnet"))
   {
      value = NXSL_InetAddressClass::createObject(vm, a->getSubnetAddress());
   }
   return value;
}

/**
 * Convert to string representation
 */
void NXSL_InetAddressClass::toString(StringBuffer *sb, NXSL_Object *object)
{
   InetAddress *a = static_cast<InetAddress*>(object->getData());
   TCHAR buffer[64];
   sb->append(a->toString(buffer));
}

/**
 * Create NXSL object from InetAddress object
 */
NXSL_Value *NXSL_InetAddressClass::createObject(NXSL_VM *vm, const InetAddress& addr)
{
   return vm->createValue(vm->createObject(&g_nxslInetAddressClass, new InetAddress(addr)));
}

/**
 * Constructor for InetAddress class
 */
int F_InetAddress(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (argc > 2)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if ((argc >= 1) && !argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   if ((argc >= 2) && !argv[1]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   InetAddress addr = (argc == 0) ? InetAddress() : InetAddress::parse(argv[0]->getValueAsCString());
   if (argc >= 2)
   {
      int bits = argv[1]->getValueAsInt32();
      if (bits < 0)
         bits = 0;
      else if ((bits > 128) && (addr.getFamily() == AF_INET6))
         bits = 128;
      else if ((bits > 32) && (addr.getFamily() == AF_INET))
         bits = 32;
      addr.setMaskBits(bits);
   }
   *result = NXSL_InetAddressClass::createObject(vm, addr);
   return 0;
}
