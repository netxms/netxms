/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: geolocation.cpp
**
**/

#include "libnxsl.h"

/**
 * Instance of NXSL_Table class
 */
NXSL_GeoLocationClass LIBNXSL_EXPORTABLE g_nxslGeoLocationClass;

/**
 * Implementation of "Table" class: constructor
 */
NXSL_GeoLocationClass::NXSL_GeoLocationClass() : NXSL_Class()
{
   setName(_T("GeoLocation"));
}

/**
 * Object delete
 */
void NXSL_GeoLocationClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<GeoLocation*>(object->getData());
}

/**
 * Get string representation
 */
void NXSL_GeoLocationClass::toString(StringBuffer *sb, NXSL_Object *object)
{
   GeoLocation *gl = static_cast<GeoLocation*>(object->getData());
   if (gl->getType() != GL_UNSET)
   {
      sb->append(gl->getLatitudeAsString());
      sb->append(_T(' '));
      sb->append(gl->getLongitudeAsString());
      switch(gl->getType())
      {
         case GL_GPS:
            sb->append(_T(" [gps]"));
            break;
         case GL_MANUAL:
            sb->append(_T(" [manual]"));
            break;
         case GL_NETWORK:
            sb->append(_T(" [network]"));
            break;
         default:
            sb->append(_T(" [other]"));
            break;
      }
   }
   else
   {
      sb->append(_T("[undefined]"));
   }
}

/**
 * Implementation of "GeoLocation" class: get attribute
 */
NXSL_Value *NXSL_GeoLocationClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   GeoLocation *gl = static_cast<GeoLocation*>(object->getData());
   if (NXSL_COMPARE_ATTRIBUTE_NAME("isManual"))
   {
      value = vm->createValue(gl->isManual());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isValid"))
   {
      value = vm->createValue(gl->isValid());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("latitude"))
   {
      value = vm->createValue(gl->getLatitude());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("latitudeText"))
   {
      value = vm->createValue(gl->getLatitudeAsString());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("longitude"))
   {
      value = vm->createValue(gl->getLongitude());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("longitudeText"))
   {
      value = vm->createValue(gl->getLongitudeAsString());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("timestamp"))
   {
      value = vm->createValue(static_cast<int64_t>(gl->getTimestamp()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("type"))
   {
      value = vm->createValue(gl->getType());
   }
   return value;
}

/**
 * Create NXSL object from GeoLocation object
 */
NXSL_Value *NXSL_GeoLocationClass::createObject(NXSL_VM *vm, const GeoLocation& gl)
{
   return vm->createValue(vm->createObject(&g_nxslGeoLocationClass, new GeoLocation(gl)));
}

/**
 * Create geolocation
 */
int F_GeoLocation(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 2) || (argc > 5))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isNumeric() || !argv[1]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   time_t timestamp = time(nullptr);
   int type = GL_MANUAL;
   int accuracy = 0;
   if (argc >= 3)
   {
      if (!argv[2]->isInteger())
         return NXSL_ERR_NOT_INTEGER;
      type = argv[2]->getValueAsInt32();
      if ((type < 0) || (type > 3))
      {
         type = GL_MANUAL;
      }

      if (argc >= 4)
      {
         if (!argv[3]->isInteger())
            return NXSL_ERR_NOT_INTEGER;
         timestamp = static_cast<time_t>(argv[3]->getValueAsInt64());

         if (argc >= 5)
         {
            if (!argv[4]->isInteger())
               return NXSL_ERR_NOT_INTEGER;
            accuracy = argv[4]->getValueAsInt32();
         }
      }
   }

   GeoLocation *gl = new GeoLocation(type, argv[0]->getValueAsReal(), argv[1]->getValueAsReal(), accuracy, timestamp);
   *result = vm->createValue(vm->createObject(&g_nxslGeoLocationClass, gl));
   return 0;
}
