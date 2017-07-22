/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
 * Implementation of "InetAddress" class: constructor
 */
NXSL_InetAddressClass::NXSL_InetAddressClass() : NXSL_Class()
{
   setName(_T("InetAddress"));
}

/**
 * Implementation of "InetAddress" class: destructor
 */
NXSL_InetAddressClass::~NXSL_InetAddressClass()
{
}

/**
 * Object delete
 */
void NXSL_InetAddressClass::onObjectDelete(NXSL_Object *object)
{
   delete (InetAddress *)object->getData();
}

/**
 * Implementation of "InetAddress" class: get attribute
 */
NXSL_Value *NXSL_InetAddressClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
   NXSL_Value *value = NULL;
   InetAddress *a = (InetAddress *)object->getData();
   if (!_tcscmp(attr, _T("address")))
   {
      TCHAR buffer[64];
      value = new NXSL_Value(a->toString(buffer));
   }
   else if (!_tcscmp(attr, _T("family")))
   {
      value = new NXSL_Value((a->getFamily() == AF_INET) ? _T("inet") : (a->getFamily() == AF_INET6 ? _T("inet6") : _T("unspec")));
   }
   else if (!_tcscmp(attr, _T("isAnyLocal")))
   {
      value = new NXSL_Value(a->isAnyLocal());
   }
   else if (!_tcscmp(attr, _T("isBroadcast")))
   {
      value = new NXSL_Value(a->isBroadcast());
   }
   else if (!_tcscmp(attr, _T("isLinkLocal")))
   {
      value = new NXSL_Value(a->isLinkLocal());
   }
   else if (!_tcscmp(attr, _T("isLoopback")))
   {
      value = new NXSL_Value(a->isLoopback());
   }
   else if (!_tcscmp(attr, _T("isMulticast")))
   {
      value = new NXSL_Value(a->isMulticast());
   }
   else if (!_tcscmp(attr, _T("isValid")))
   {
      value = new NXSL_Value(a->isValid());
   }
   else if (!_tcscmp(attr, _T("isValidUnicast")))
   {
      value = new NXSL_Value(a->isValidUnicast());
   }
   else if (!_tcscmp(attr, _T("mask")))
   {
      value = new NXSL_Value(a->getMaskBits());
   }
   return value;
}

/**
 * Create NXSL object from InetAddress object
 */
NXSL_Value *NXSL_InetAddressClass::createObject(const InetAddress& addr)
{
   return new NXSL_Value(new NXSL_Object(&g_nxslInetAddressClass, new InetAddress(addr)));
}
