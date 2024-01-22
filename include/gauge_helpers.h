/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
** File: gauge_helpers.h
**
**/

#ifndef _gauge_helpers_h_
#define _gauge_helpers_h_

#include <nms_util.h>
#include <nms_threads.h>
#include <nxqueue.h>
#include <math.h>

/**
 * Maximum name length for gauge
 */
#define MAX_GAUGE_NAME_LEN    64

/**
 * Generic gauge data
 */
template <typename T> class GaugeData
{
protected:
   T m_current;
   T m_min;
   T m_max;
   T m_average; // exponential moving average
   bool m_null;
   bool m_reset;
   int m_exp;        // Pre-calculated exponent value for moving average calculation
   
public:
   GaugeData(int interval, int period)
   {
      m_current = 0;
      m_min = 0;
      m_max = 0;
      m_average = 0;
      m_null = true;
      m_exp = EMA_EXP(interval, period);
   }

   T getCurrent() const { return m_current; }
   T getMin() const { return m_min; }
   T getMax() const { return m_max; }
   double getAverage() const { return GetExpMovingAverageValue(m_average); }
   bool isNull() const { return m_null; }
   
   /**
    * Update gauge with new value
    */
   void update(T curr)
   {
      m_current = curr;
      if (m_null)
      {
         m_null = false;
         m_min = curr;
         m_max = curr;
         m_average = curr * EMA_FP_1;
      }
      else
      {
         if (m_min > curr)
            m_min = curr;
         if (m_max < curr)
            m_max = curr;
         UpdateExpMovingAverage(m_average, m_exp, curr);
      }
   }

   void reset()
   {
      m_min = m_current;
      m_max = m_current;
   }
};

/**
 * Generic gauge
 */
template <typename T> class Gauge
{
protected:
   TCHAR m_name[MAX_GAUGE_NAME_LEN];
   GaugeData<T> m_data;
   int m_interval;   // Time interval (in seconds) for measuring average
   int m_period;     // Time period (in seconds) for measuring average
   std::function<int64_t ()> m_reader;  // Data reader
   
public:   
   Gauge(std::function<int64_t ()> reader, const TCHAR *name, int interval, int period) : m_data(interval, period)
   {
      _tcslcpy(m_name, name, MAX_GAUGE_NAME_LEN);
      m_interval = interval;
      m_period = period;
      m_reader = reader;
   }

   const TCHAR *getName() const { return m_name; }
   T getCurrent() const { return m_data.getCurrent(); }
   T getMin() const { return m_data.getMin(); }
   T getMax() const { return m_data.getMax(); }
   double getAverage() const { return m_data.getAverage(); }
   bool isNull() const { return m_data.isNull(); }
   int getInterval() const { return m_interval; }
   int getPeriod() const { return m_period; }
   void reset() { m_data.reset(); }
   
   void update() 
   {
      m_data.update(static_cast<T>(m_reader()));
   }

   void update(T value)
   {
      m_data.update(value);
   }
};

#ifdef _WIN32
template class LIBNETXMS_TEMPLATE_EXPORTABLE std::function<int64_t ()>;
template class LIBNETXMS_TEMPLATE_EXPORTABLE GaugeData<int32_t>;
template class LIBNETXMS_TEMPLATE_EXPORTABLE GaugeData<int64_t>;
template class LIBNETXMS_TEMPLATE_EXPORTABLE Gauge<int32_t>;
template class LIBNETXMS_TEMPLATE_EXPORTABLE Gauge<int64_t>;
#endif

/**
 * 64 bit integer gauge
 */
class LIBNETXMS_EXPORTABLE Gauge64 : public Gauge<int64_t>
{
public:
   Gauge64(std::function<int64_t()> reader, const TCHAR *name, int interval, int period) : Gauge<int64_t>(reader, name, interval, period) { }
};

/**
 * 32 bit integer gauge
 */
class LIBNETXMS_EXPORTABLE Gauge32 : public Gauge<int32_t>
{
public:
   Gauge32(std::function<int64_t()> reader, const TCHAR *name, int interval, int period) : Gauge<int32_t>(reader, name, interval, period) { }
};

/**
 * Queue length gauge
 */
class LIBNETXMS_EXPORTABLE GaugeQueueLength : public Gauge64
{
public:
   GaugeQueueLength(const TCHAR *name, Queue *queue, int interval = 5, int period = 900) :
      Gauge64([queue]() -> int64_t { return static_cast<int64_t>(queue->size()); }, name, interval, period) { }
};

/**
 * Thread pool request execution queue length gauge
 */
class LIBNETXMS_EXPORTABLE GaugeThreadPoolRequests : public Gauge64
{
public:
   GaugeThreadPoolRequests(const TCHAR *name, ThreadPool *pool, int interval = 5, int period = 900) :
      Gauge64([pool]() -> int64_t {
         ThreadPoolInfo poolInfo;
         ThreadPoolGetInfo(pool, &poolInfo);
         return (poolInfo.activeRequests > poolInfo.curThreads) ? poolInfo.activeRequests - poolInfo.curThreads : 0;
      }, name, interval, period) { }
};

/**
 * Gauge for data provided by external function
 */
class LIBNETXMS_EXPORTABLE GaugeFunction : public Gauge64
{
public:
   GaugeFunction(const TCHAR *name, int64_t (*function)(), int interval = 5, int period = 900) : Gauge64(function, name, interval, period) { }
};

/**
 * Manually updated 64 bit integer gauge
 */
class LIBNETXMS_EXPORTABLE ManualGauge64 : public Gauge64
{
public:
   ManualGauge64(const TCHAR *name, int interval, int period) : Gauge64([]() -> int64_t { return 0; }, name, interval, period) { }
};

#endif
