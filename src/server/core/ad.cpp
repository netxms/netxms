/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: ad.cpp
**/

#include "nxcore.h"

#if WITH_LIBISOTREE

#include <isotree_oop.hpp>

#define DEBUG_TAG _T("ad")

/**
 * Helper function to read last N values of given DCI
 */
static unique_ptr<StructArray<ScoredDciValue>> LoadDciValues(uint32_t nodeId, uint32_t dciId, DCObjectStorageClass storageClass, const std::pair<int64_t, int64_t> *timeRanges, int numTimeRanges)
{
   StringBuffer query(_T("SELECT idata_timestamp,idata_value"));
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         query.append(_T(" FROM idata_sc_"));
         query.append(DCObject::getStorageClassName(storageClass));
         query.append(_T(" WHERE item_id=? AND idata_timestamp BETWEEN ms_to_timestamptz(?) AND ms_to_timestamptz(?)"));
      }
      else
      {
         query.append(_T(" FROM idata WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"));
      }
   }
   else
   {
      query.append(_T(" FROM idata_"));
      query.append(nodeId);
      query.append(_T(" WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"));
   }
   query.append(_T(" ORDER BY idata_timestamp DESC"));

   StructArray<ScoredDciValue> *values = new StructArray<ScoredDciValue>(0, 1024);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, query, numTimeRanges > 1);
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dciId);
      for(int n = 0; n < numTimeRanges; n++)
      {
         DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, timeRanges[n].first);
         DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, timeRanges[n].second);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int count = DBGetNumRows(hResult);
            for(int i = 0; i < count; i++)
            {
               ScoredDciValue *v = values->addPlaceholder();
               v->timestamp = DBGetFieldInt64(hResult, i, 0);
               v->value = DBGetFieldDouble(hResult, i, 1);
               v->score = 0;
            }
            DBFreeResult(hResult);
         }
      }
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return unique_ptr<StructArray<ScoredDciValue>>(values);
}

/**
 * Detect anomalies within given time range
 */
unique_ptr<StructArray<ScoredDciValue>> DetectAnomalies(const DataCollectionTarget& dcTarget, uint32_t dciId, time_t timeFrom, time_t timeTo, double threshold)
{
   shared_ptr<DCObject> dci = dcTarget.getDCObjectById(dciId, 0);
   if ((dci == nullptr) || (dci->getType() != DCO_TYPE_ITEM))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("DetectAnomalies: invalid DCI ID [%u] on object %s [%u]"), dciId, dcTarget.getName(), dcTarget.getId());
   }

   std::pair<int64_t, int64_t> timeRange(TimeToMs(timeFrom), TimeToMs(timeTo));
   auto series = LoadDciValues(dcTarget.getId(), dciId, dci->getStorageClass(), &timeRange, 1);
   if (series == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("DetectAnomalies: cannot load data for DCI \"%s\" [%u] on object %s [%u]"), dci->getName().cstr(), dciId, dcTarget.getName(), dcTarget.getId());
      return unique_ptr<StructArray<ScoredDciValue>>();
   }

   if (series->isEmpty())
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("DetectAnomalies: no data for DCI \"%s\" [%u] on object %s [%u]"), dci->getName().cstr(), dciId, dcTarget.getName(), dcTarget.getId());
      return unique_ptr<StructArray<ScoredDciValue>>();
   }

   int count = series->size();
   nxlog_debug_tag(DEBUG_TAG, 6, _T("DetectAnomalies: %d data points loaded for DCI \"%s\" [%u] on object %s [%u]"), count, dci->getName().cstr(), dciId, dcTarget.getName(), dcTarget.getId());

   isotree::IsolationForest model;
   double *points = MemAllocArrayNoInit<double>(count);
   for(int i = 0; i < count; i++)
      points[i] = series->get(i)->value;

   model.fit(points, count, 1);
   std::vector<double> scores = model.predict(points, count, true);
   MemFree(points);
   for(int i = 0; i < count; i++)
      series->get(i)->score = scores[i];

   series->sort(
      [] (const ScoredDciValue *v1, const ScoredDciValue *v2) -> int
      {
         return v1->score < v2->score ? 1 : (v1->score > v2->score ? -1 : 0);
      });
   for(int i = 0; i < count; i++)
      if (series->get(i)->score < threshold)
      {
         return make_unique<StructArray<ScoredDciValue>>(series->getBuffer(), i);
      }

   return series;
}

/**
 * Check if given value is an anomaly
 * Period is number of days, depth is number of periods to look into, width is time interval around current time in minutes
 */
bool IsAnomalousValue(const DataCollectionTarget& dcTarget, const DCObject& dci, double value, double threshold, int period, int depth, int width)
{
   if (depth > 90)
      depth = 90;

   // Construct time ranges for daily periods
   time_t now = time(nullptr);
   time_t t = now - width * 30;  // Half-interval in minutes to seconds
   std::pair<int64_t, int_t> timeRanges[90]; // up to 90 days back
   for(int i = 0; i < depth; i++)
   {
      t -= 86400 * period;
      timeRanges[i].first = TimeToMs(t);
      timeRanges[i].second = TimeToMs(t + width * 60);
   }

   auto series = LoadDciValues(dcTarget.getId(), dci.getId(), dci.getStorageClass(), timeRanges, depth);
   if (series == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("IsAnomalousValue(%s [%u], \"%s\"): cannot load DCI data"), dcTarget.getName(), dcTarget.getId(), dci.getName().cstr());
      return false;
   }

   if (series->isEmpty())
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("IsAnomalousValue(%s [%u], \"%s\"): no data points for period [p=%d d=%d w=%d]"),
            dcTarget.getName(), dcTarget.getId(), dci.getName().cstr(), period, depth, width);
      return false;
   }

   ScoredDciValue *v = series->addPlaceholder();
   v->timestamp = now;
   v->value = value;

   int count = series->size();
   nxlog_debug_tag(DEBUG_TAG, 6, _T("IsAnomalousValue(%s [%u], \"%s\"): %d data points loaded for period [p=%d d=%d w=%d]"),
         dcTarget.getName(), dcTarget.getId(), dci.getName().cstr(), count - 1, period, depth, width);

   isotree::IsolationForest model;
   double *points = MemAllocArrayNoInit<double>(count);
   for(int i = 0; i < count; i++)
      points[i] = series->get(i)->value;

   if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 7)
   {
      StringBuffer sb;
      for(int i = 0; i < count; i++)
      {
         sb.append(points[i]);
         sb.append(_T("  "));
      }
      nxlog_debug_tag(DEBUG_TAG, 7, _T("IsAnomalousValue(%s [%u], \"%s\"): %s"), dcTarget.getName(), dcTarget.getId(), dci.getName().cstr(), sb.cstr());
   }

   model.fit(points, count, 1);
   std::vector<double> scores = model.predict(points, count, true);
   MemFree(points);

   nxlog_debug_tag(DEBUG_TAG, 6, _T("IsAnomalousValue(%s [%u], \"%s\"): score for value %f and period [p=%d d=%d w=%d] is %f"),
         dcTarget.getName(), dcTarget.getId(), dci.getName().cstr(), value, period, depth, width, scores[count - 1]);
   return scores[count - 1] >= threshold;
}

#else /* WITH_LIBISOTREE */

/**
 * Detect anomalies within given time range (dummy implementation)
 */
unique_ptr<StructArray<ScoredDciValue>> DetectAnomalies(const DataCollectionTarget& dcTarget, uint32_t dciId, time_t timeFrom, time_t timeTo, double threshold)
{
   return unique_ptr<StructArray<ScoredDciValue>>();
}

/**
 * Dummy implementation of IsAnomalousValue
 */
bool IsAnomalousValue(const DataCollectionTarget& dcTarget, const DCObject& dci, double value, double threshold, int period, int depth, int width)
{
   return false;
}

#endif   /* WITH_LIBISOTREE */
