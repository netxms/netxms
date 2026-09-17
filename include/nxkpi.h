/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: nxkpi.h
**
**/

#ifndef _nxkpi_h_
#define _nxkpi_h_

#include <nms_util.h>

/**
 * Quality class of a sample. Numeric order is the degradation order (worst-of = max).
 */
enum class SampleQuality : int16_t
{
   MEASURED = 0,
   PROXY = 1,
   ESTIMATED = 2,
   INTERPOLATED = 3,
   MISSING = 4
};

/**
 * Per-sample attributes (provenance of a single DCI history sample)
 */
struct SampleAttributes
{
   SampleQuality quality;
   bool bounded;          // false: [lower, upper] undefined
   double lower;
   double upper;
   double completeness;   // 0..1
   uint32_t methodId;     // kpi_methods.id; 0 = none

   /**
    * Default attributes: measured, unbounded, complete, no method. Samples with default attributes have no stored attribute record.
    */
   SampleAttributes()
   {
      quality = SampleQuality::MEASURED;
      bounded = false;
      lower = 0;
      upper = 0;
      completeness = 1;
      methodId = 0;
   }

   /**
    * Check if attributes are default ones
    */
   bool isDefault() const
   {
      return (quality == SampleQuality::MEASURED) && !bounded && (completeness == 1) && (methodId == 0);
   }
};

/**
 * DCI history sample with attributes. Value is NaN for samples with quality MISSING.
 */
struct AttributedSample
{
   Timestamp timestamp;
   double value;
   SampleAttributes attributes;
};

/**
 * Computation method tier
 */
enum class MethodTier : char
{
   OPEN = '0',
   COMMERCIAL = '1'
};

/**
 * Computation method descriptor. Method is identified by method ID and version.
 */
struct MethodDescriptor
{
   const wchar_t *methodId;     // e.g. "kpidirect"
   const wchar_t *version;
   const wchar_t *engine;
   const wchar_t *displayName;
   MethodTier tier;
   double coverageLevel;        // coverage level of bounds produced by this method (e.g. 0.95)
};

#endif   /* _nxkpi_h_ */
