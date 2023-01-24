/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2023 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: welford.h
**/

#ifndef _welford_h_
#define _welford_h_

#include <math.h>

/**
 * Calculating variance and standard deviation from the streaming data using Welford's online algorithm.
 */
class WelfordVariance
{
private:
   double m_mean;
   double m_ss;         // sum of squares
   int64_t m_samples;   // number of samples

public:
   WelfordVariance()
   {
      m_mean = 0;
      m_ss = 0;
      m_samples = 0;
   }

   /**
    * Update with new sample
    */
   void update(double v)
   {
      m_samples++;
      double delta = v - m_mean;
      m_mean += delta / m_samples;
      double delta2 = v - m_mean;
      m_ss += delta * delta2;
   }

   /**
    * Update with new sample - convenience wrapper
    */
   void update(int64_t v)
   {
      update(static_cast<double>(v));
   }

   /**
    * Update with new sample - convenience wrapper
    */
   void update(uint64_t v)
   {
      update(static_cast<double>(v));
   }

   /**
    * Reset
    */
   void reset()
   {
      m_mean = 0;
      m_ss = 0;
      m_samples = 0;
   }

   /**
    * Get variance
    */
   double variance() const
   {
      return (m_samples <= 1) ? 0 : m_ss / (m_samples - 1);
   }

   /**
    * Get mean value
    */
   double mean() const
   {
      return m_mean;
   }

   /**
    * Get standard deviation
    */
   double sd() const
   {
      return sqrt(variance());
   }

   /**
    * Get number of samples
    */
   int64_t samples() const
   {
      return m_samples;
   }
};

#endif
