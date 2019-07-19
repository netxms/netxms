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
 * 64 bit gauge
 */
template <typename T> class Gauge
{
protected:
   TCHAR m_name[MAX_GAUGE_NAME_LEN];
   GaugeData<T> m_data;
   int m_interval;   // Time interval (in seconds) for measuring average
   int m_period;     // Time period (in seconds) for measuring average
   
   virtual T readCurrentValue() = 0;
   
public:   
   Gauge(const TCHAR *name, int interval, int period) : m_data(interval, period)
   {
      _tcslcpy(m_name, name, MAX_GAUGE_NAME_LEN);
      m_interval = interval;
      m_period = period;
   }

   const TCHAR *getName() const { return m_name; }
   T getCurrent() const { return m_data.getCurrent(); }
   T getMin() const { return m_data.getMin(); }
   T getMax() const { return m_data.getMax(); }
   double getAverage() const { return m_data.getAverage(); }
   bool isNull() const { return m_data.isNull(); }
   int getInterval() const { return m_interval; }
   int getPeriod() const { return m_period; }
   void reset() {m_data.reset(); }
   
   void update() 
   {
      m_data.update(readCurrentValue());
   }

   void update(T value)
   {
      m_data.update(value);
   }
};

/**
 * 64 bit integer gauge
 */
class Gauge64 : public Gauge<INT64>
{
public:
   Gauge64(const TCHAR *name, int interval, int period) : Gauge<INT64>(name, interval, period) { }
};

/**
 * 32 bit integer gauge
 */
class Gauge32 : public Gauge<INT32>
{
public:
   Gauge32(const TCHAR *name, int interval, int period) : Gauge<INT32>(name, interval, period) { }
};

/**
 * Queue length gauge
 */
class GaugeQueueLength : public Gauge64
{
private:
   Queue *m_queue;

protected:
   virtual INT64 readCurrentValue() override
   {
      return m_queue->size();
   }

public:
   GaugeQueueLength(const TCHAR *name, Queue *queue, int interval = 5, int period = 900) : Gauge64(name, interval, period)
   {
      m_queue = queue;
   }
};

/**
 * Thread pool request execution queue length gauge
 */
class GaugeThreadPoolRequests : public Gauge64
{
private:
   ThreadPool *m_pool;

protected:
   virtual INT64 readCurrentValue() override
   {
      ThreadPoolInfo poolInfo;
      ThreadPoolGetInfo(m_pool, &poolInfo);
      return (poolInfo.activeRequests > poolInfo.curThreads) ? poolInfo.activeRequests - poolInfo.curThreads : 0;
   }

public:
   GaugeThreadPoolRequests(const TCHAR *name, ThreadPool *pool, int interval = 5, int period = 900) : Gauge64(name, interval, period)
   {
      m_pool = pool;
   }
};

/**
 * Gauge for data provided by external function
 */
class GaugeFunction : public Gauge64
{
private:
   INT64 (*m_function)();

protected:
   virtual INT64 readCurrentValue() override
   {
      return m_function();
   }

public:
   GaugeFunction(const TCHAR *name, INT64 (*function)(), int interval = 5, int period = 900) : Gauge64(name, interval, period)
   {
      m_function = function;
   }
};

/**
 * Manually updated 64 bit integer gauge
 */
class ManualGauge64 : public Gauge64
{
protected:
   virtual INT64 readCurrentValue() override { return 0; }

public:
   ManualGauge64(const TCHAR *name, int interval, int period) : Gauge64(name, interval, period) { }
};

#endif
