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
static unique_ptr<StructArray<ScoredDciValue>> LoadDciValues(uint32_t nodeId, uint32_t dciId, DCObjectStorageClass storageClass, time_t timeFrom, time_t timeTo)
{
   StringBuffer query(_T("SELECT idata_timestamp,idata_value"));
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         query.append(_T(" FROM idata_sc_"));
         query.append(DCObject::getStorageClassName(storageClass));
         query.append(_T(" WHERE item_id=? AND idata_timestamp BETWEEN to_timestamp(?) AND to_timestamp(?)"));
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

   StructArray<ScoredDciValue> *values = nullptr;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, query);
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dciId);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<int32_t>(timeFrom));
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<int32_t>(timeTo));
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         values = new StructArray<ScoredDciValue>(count);
         for(int i = 0; i < count; i++)
         {
            ScoredDciValue *v = values->addPlaceholder();
            v->timestamp = DBGetFieldULong(hResult, i, 0);
            v->value = DBGetFieldDouble(hResult, i, 1);
            v->score = 0;
         }
         DBFreeResult(hResult);
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

   auto series = LoadDciValues(dcTarget.getId(), dciId, dci->getStorageClass(), timeFrom, timeTo);
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

#else /* WITH_LIBISOTREE */

/**
 * Detect anomalies within given time range (dummy implementation)
 */
unique_ptr<StructArray<ScoredDciValue>> DetectAnomalies(const DataCollectionTarget& dcTarget, uint32_t dciId, time_t timeFrom, time_t timeTo, double threshold)
{
   return unique_ptr<StructArray<ScoredDciValue>>();
}

#endif   /* WITH_LIBISOTREE */
