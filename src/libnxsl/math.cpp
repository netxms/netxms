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
** File: math.cpp
**
**/

#include "libnxsl.h"
#include <math.h>

/**
 * NXSL function: Absolute value
 */
int F_MathAbs(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   if (argv[0]->isReal())
   {
      *result = vm->createValue(fabs(argv[0]->getValueAsReal()));
   }
   else
   {
      *result = vm->createValue(argv[0]);
      if (!argv[0]->isUnsigned())
         if ((*result)->getValueAsInt64() < 0)
            (*result)->negate();
   }
   return NXSL_ERR_SUCCESS;
}

/**
 * Calculates x raised to the power of y
 */
int F_MathPow(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric() || !argv[1]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(pow(argv[0]->getValueAsReal(), argv[1]->getValueAsReal()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Calculates 10 raised to the power of x
 */
int F_MathPow10(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   *result = vm->createValue(pow(10.0, argv[0]->getValueAsInt32()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Calculates square root
 */
int F_MathSqrt(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(sqrt(argv[0]->getValueAsReal()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Calculates natural logarithm
 */
int F_MathLog(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(log(argv[0]->getValueAsReal()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Calculates common logarithm
 */
int F_MathLog10(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(log10(argv[0]->getValueAsReal()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Calculates x raised to the power of e
 */
int F_MathExp(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(exp(argv[0]->getValueAsReal()));
   return 0;
}

/**
 * Calculates sine x
 */
int F_MathSin(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(sin(argv[0]->getValueAsReal()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Calculates cosine x
 */
int F_MathCos(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(cos(argv[0]->getValueAsReal()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Calculates tangent x
 */
int F_MathTan(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(tan(argv[0]->getValueAsReal()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Calculates hyperbolic sine x
 */
int F_MathSinh(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(sinh(argv[0]->getValueAsReal()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Calculates hyperbolic cosine x
 */
int F_MathCosh(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(cosh(argv[0]->getValueAsReal()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Calculates hyperbolic tangent x
 */
int F_MathTanh(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(tanh(argv[0]->getValueAsReal()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Calculates arc sine x
 */
int F_MathAsin(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(asin(argv[0]->getValueAsReal()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Calculates arc cosine x
 */
int F_MathAcos(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(acos(argv[0]->getValueAsReal()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Calculates arc tangent x
 */
int F_MathAtan(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(atan(argv[0]->getValueAsReal()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Calculates hyperbolic arc tangent x
 */
int F_MathAtanh(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(atanh(argv[0]->getValueAsReal()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Calculates 2-argument arc tangent
 */
int F_MathAtan2(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric() || !argv[1]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(atan2(argv[0]->getValueAsReal(), argv[1]->getValueAsReal()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Common implementation for Math::Min and Math::Max
 */
static int MinMaxImpl(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm, bool (*Comparator)(NXSL_Value*, NXSL_Value*))
{
   if (argc == 0)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   NXSL_Value *selectedValue = nullptr;
   for(int i = 0; i < argc; i++)
   {
      if (argv[i]->isArray())
      {
         NXSL_Array *series = argv[i]->getValueAsArray();
         for(int j = 0; j < series->size(); j++)
         {
            NXSL_Value *v = series->get(j);
            if (!v->isNumeric())
               return NXSL_ERR_NOT_NUMBER;
            if ((selectedValue == nullptr) || Comparator(v, selectedValue))
               selectedValue = v;
         }
      }
      else if (argv[i]->isNumeric())
      {
         if ((selectedValue == nullptr) || Comparator(argv[i], selectedValue))
            selectedValue = argv[i];
      }
      else
      {
         return NXSL_ERR_NOT_NUMBER;
      }
   }
   *result = vm->createValue(selectedValue);
   return NXSL_ERR_SUCCESS;
}

/**
 * Minimal value from the list of values
 */
int F_MathMin(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   return MinMaxImpl(argc, argv, result, vm,
      [] (NXSL_Value *curr, NXSL_Value *selected) -> bool
      {
         return curr->getValueAsReal() < selected->getValueAsReal();
      });
}

/**
 * Maximal value from the list of values
 */
int F_MathMax(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   return MinMaxImpl(argc, argv, result, vm,
      [] (NXSL_Value *curr, NXSL_Value *selected) -> bool
      {
         return curr->getValueAsReal() > selected->getValueAsReal();
      });
}

/**
 * Find average value for given series
 */
int F_MathAverage(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (argc == 0)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   double total = 0;
   int count = 0;
   for(int i = 0; i < argc; i++)
   {
      if (argv[i]->isArray())
      {
         NXSL_Array *series = argv[i]->getValueAsArray();
         for(int j = 0; j < series->size(); j++)
         {
            if (!series->get(j)->isNumeric())
               return NXSL_ERR_NOT_NUMBER;
            total += series->get(j)->getValueAsReal();
         }
         count += series->size();
      }
      else if (argv[i]->isNumeric())
      {
         total += argv[i]->getValueAsReal();
         count++;
      }
      else
      {
         return NXSL_ERR_NOT_NUMBER;
      }
   }
   *result = vm->createValue(total / count);
   return NXSL_ERR_SUCCESS;
}

/**
 * Find sum of values in given series
 */
int F_MathSum(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   double value = 0;
   for(int i = 0; i < argc; i++)
   {
      if (argv[i]->isArray())
      {
         NXSL_Array *series = argv[i]->getValueAsArray();
         for(int j = 0; j < series->size(); j++)
         {
            if (!series->get(j)->isNumeric())
               return NXSL_ERR_NOT_NUMBER;
            value += series->get(j)->getValueAsReal();
         }
      }
      else if (argv[i]->isNumeric())
      {
         value += argv[i]->getValueAsReal();
      }
      else
      {
         return NXSL_ERR_NOT_NUMBER;
      }
   }
   *result = vm->createValue(value);
   return NXSL_ERR_SUCCESS;
}

/**
 * Build series from function arguments (can be mix of single values and arrays)
 */
static inline bool BuildSeries(int argc, NXSL_Value **argv, ObjectRefArray<NXSL_Value>& series)
{
   for(int i = 0; i < argc; i++)
   {
      if (argv[i]->isArray())
      {
         NXSL_Array *a = argv[i]->getValueAsArray();
         for(int j = 0; j < a->size(); j++)
         {
            if (!a->get(j)->isNumeric())
               return false;
            series.add(a->get(j));
         }
      }
      else if (argv[i]->isNumeric())
      {
         series.add(argv[i]);
      }
      else
      {
         return false;
      }
   }
   return true;
}

/**
 * Find mean absolute deviation for given series
 */
int F_MathMeanAbsoluteDeviation(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   ObjectRefArray<NXSL_Value> series(128, 128);
   if (!BuildSeries(argc, argv, series))
      return NXSL_ERR_NOT_NUMBER;

   if (!series.isEmpty())
   {
      double mean = 0;
      for(int i = 0; i < series.size(); i++)
      {
         mean += series.get(i)->getValueAsReal();
      }
      mean /= series.size();

      double deviation = 0;
      for(int i = 0; i < series.size(); i++)
      {
         deviation += fabs(series.get(i)->getValueAsReal() - mean);
      }
      deviation /= series.size();

      *result = vm->createValue(deviation);
   }
   else
   {
      *result = vm->createValue(0);
   }
   return NXSL_ERR_SUCCESS;
}

/**
 * Find standard deviation for given series
 */
int F_MathStandardDeviation(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   ObjectRefArray<NXSL_Value> series(128, 128);
   if (!BuildSeries(argc, argv, series))
      return NXSL_ERR_NOT_NUMBER;

   if (!series.isEmpty())
   {
      double mean = 0;
      for(int i = 0; i < series.size(); i++)
      {
         mean += series.get(i)->getValueAsReal();
      }
      mean /= series.size();

      double variance = 0;
      for(int i = 0; i < series.size(); i++)
      {
         double v = series.get(i)->getValueAsReal();
         variance += (v - mean) * (v - mean);
      }
      variance /= series.size();

      *result = vm->createValue(sqrt(variance));
   }
   else
   {
      *result = vm->createValue(0);
   }
   return NXSL_ERR_SUCCESS;
}

/**
 * NXSL function: Generate random number in given range
 */
int F_MathRandom(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isInteger() || !argv[1]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   *result = vm->createValue(GenerateRandomNumber(argv[0]->getValueAsInt32(), argv[1]->getValueAsInt32()));
   return NXSL_ERR_SUCCESS;
}

/**
 * NXSL function: Math::Floor
 */
int F_MathFloor(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(floor(argv[0]->getValueAsReal()));
   return NXSL_ERR_SUCCESS;
}

/**
 * NXSL function: Math::Ceil
 */
int F_MathCeil(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   *result = vm->createValue(ceil(argv[0]->getValueAsReal()));
   return NXSL_ERR_SUCCESS;
}

/**
 * NXSL function: Math::Round
 */
int F_MathRound(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc != 1) && (argc != 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   double d = argv[0]->getValueAsReal();
   if (argc == 1)
   {
      // round to whole number
      *result = vm->createValue((d > 0.0) ? floor(d + 0.5) : ceil(d - 0.5));
   }
   else
   {
      // round with given number of decimal places
      if (!argv[1]->isInteger())
         return NXSL_ERR_NOT_INTEGER;

      int p = argv[1]->getValueAsInt32();
      if (p < 0)
         p = 0;

      d *= pow(10.0, p);
      d = (d > 0.0) ? floor(d + 0.5) : ceil(d - 0.5);
      d *= pow(10.0, -p);
      *result = vm->createValue(d);
   }
   return NXSL_ERR_SUCCESS;
}

/**
 * Calculate Weierstrass function for given x, a, and b
 */
int F_MathWeierstrass(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric() || !argv[1]->isNumeric() || !argv[2]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   double a = argv[0]->getValueAsReal();
   double b = argv[1]->getValueAsReal();
   double x = argv[2]->getValueAsReal();

   if (b < 7)
      b = 7;

   // Because 0 < a < 1, that value becomes smaller as n grows larger. For example,
   // if a = 0.9, pow(a, 100) is around 0.000027, so that term adds little to the total.
   // Because terms with larger values of n don't contribute much to the total, only
   // first 100 terms are used.
   double y = 0;
   for(int n = 0; n < 100; n++)
   {
      double c = cos(pow(b, n) * 3.1415926535 * x);
      if ((c > 1) || (c < -1))
         c = 0;
      y += pow(a, n) * c;
   }

   *result = vm->createValue(y);
   return 0;
}
