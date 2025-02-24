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
** File: range.cpp
**/

#include "libnxsl.h"
#include <math.h>

/**
 * Instance of NXSL_RangeClass
 */
NXSL_RangeClass LIBNXSL_EXPORTABLE g_nxslRangeClass;

/**
 * Range value
 */
union RangeValue
{
   int64_t i;
   double d;
};

/**
 * Internal range object
 */
struct Range
{
   RangeValue start;
   RangeValue end;
   RangeValue step;
   double margin;
   bool integer;
};

/**
 * Implementation of "Range" class: constructor
 */
NXSL_RangeClass::NXSL_RangeClass() : NXSL_Class()
{
   setName(_T("Range"));
   m_iterable = true;
}

/**
 * Object delete
 */
void NXSL_RangeClass::onObjectDelete(NXSL_Object *object)
{
   MemFree(object->getData());
}

/**
 * Implementation of "Range" class: get attribute
 */
NXSL_Value *NXSL_RangeClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   Range *range = static_cast<Range*>(object->getData());
   if (NXSL_COMPARE_ATTRIBUTE_NAME("end"))
   {
      value = range->integer ? vm->createValue(range->end.i) : vm->createValue(range->end.d);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("start"))
   {
      value = range->integer ? vm->createValue(range->start.i) : vm->createValue(range->start.d);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("step"))
   {
      value = range->integer ? vm->createValue(range->step.i) : vm->createValue(range->step.d);
   }
   return value;
}

/**
 * Convert to string representation
 */
void NXSL_RangeClass::toString(StringBuffer *sb, NXSL_Object *object)
{
   Range *r = static_cast<Range*>(object->getData());
   sb->append(_T('['));
   if (r->integer)
   {
      sb->append(r->start.i);
      sb->append(_T(" .. "));
      sb->append(r->end.i);
      sb->append(_T(", "));
      sb->append(r->step.i);
   }
   else
   {
      sb->append(r->start.d);
      sb->append(_T(" .. "));
      sb->append(r->end.d);
      sb->append(_T(", "));
      sb->append(r->step.d);
   }
   sb->append(_T(']'));
}

/**
 * Get next value for iteration
 */
NXSL_Value *NXSL_RangeClass::getNext(NXSL_Object *object, NXSL_Value *currValue)
{
   Range *range = static_cast<Range*>(object->getData());
   NXSL_VM *vm = object->vm();

   if (currValue == nullptr)
      return range->integer ? vm->createValue(range->start.i) : vm->createValue(range->start.d);

   if (range->integer)
   {
      int64_t v = currValue->getValueAsInt64();
      v += range->step.i;
      return (range->end.i > 0) ? (v < range->end.i ? vm->createValue(v) : nullptr) : (v > range->end.i ? vm->createValue(v) : nullptr);
   }

   double v = currValue->getValueAsReal();
   v += range->step.d;
   return (range->end.d > 0) ? (v < range->end.d - range->margin ? vm->createValue(v) : nullptr) : (v > range->end.d + range->margin ? vm->createValue(v) : nullptr);
}

/**
 * Create range object
 */
int F_range(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   Range range;
   if (argc == 1)
   {
      range.integer = argv[0]->isInteger();
      if (range.integer)
      {
         range.start.i = 0;
         range.end.i = argv[0]->getValueAsInt64();
         range.step.i = 1;
      }
      else
      {
         range.start.d = 0;
         range.end.d = argv[0]->getValueAsReal();
         range.step.d = 1.0;
         range.margin = 0.00001;
      }
   }
   else
   {
      if (!argv[1]->isNumeric())
         return NXSL_ERR_NOT_NUMBER;

      range.integer = argv[0]->isInteger() && ((argc < 2) || argv[1]->isInteger()) && ((argc < 3) || argv[2]->isInteger());
      if (range.integer)
      {
         range.start.i = argv[0]->getValueAsInt64();
         range.end.i = argv[1]->getValueAsInt64();
         range.step.i = (argc > 2) ? argv[2]->getValueAsInt64() : 1;
      }
      else
      {
         range.start.d = argv[0]->getValueAsReal();
         range.end.d = argv[1]->getValueAsReal();
         if (argc > 2)
         {
            if (!argv[2]->isNumeric())
               return NXSL_ERR_NOT_NUMBER;
            range.step.d = argv[2]->getValueAsReal();
            range.margin = fabs(range.step.d) * 0.00001;
         }
         else
         {
            range.step.d = 1.0;
            range.margin = 0.00001;
         }
      }
   }

   *result = vm->createValue(vm->createObject(&g_nxslRangeClass, MemCopyBlock(&range, sizeof(Range))));
   return NXSL_ERR_SUCCESS;
}
