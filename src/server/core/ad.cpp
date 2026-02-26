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
** File: ad.cpp
**/

#include "nxcore.h"

#include <vector>
#include <random>
#include <algorithm>
#include <cmath>

#define DEBUG_TAG _T("ad")

/**
 * Embedded Isolation Forest implementation (Liu et al., 2008)
 * Replaces external libisotree dependency. Only supports univariate numeric data
 * with default parameters matching isotree behavior (sample_size=nrows, limit_depth=true, ntrees=500).
 */

/**
 * Isolation tree node
 */
struct IsoTreeNode
{
   double splitValue;    // split threshold (for internal nodes)
   size_t left;          // left child index (0 = leaf)
   size_t right;         // right child index (0 = leaf)
   double score;         // leaf score = depth + c(remaining_count)
};

/**
 * Harmonic number H(n) via Euler-Maclaurin expansion (matches isotree's harmonic() in utils.hpp)
 */
static inline double HarmonicNumber(size_t n)
{
   if (n <= 1)
      return static_cast<double>(n);
   double dn = static_cast<double>(n);
   return log(dn) + 0.5772156649015329 + 1.0 / (2.0 * dn) - 1.0 / (12.0 * dn * dn);
}

/**
 * Expected average depth c(n) = 2 * (H(n) - 1)
 */
static inline double ExpectedAvgDepth(size_t n)
{
   if (n <= 1)
      return 0.0;
   return 2.0 * (HarmonicNumber(n) - 1.0);
}

/**
 * Recursively build one isolation tree
 *
 * @param nodes vector of tree nodes (appended to)
 * @param data pointer to data values
 * @param indices index array for current node's subset
 * @param count number of elements at this node
 * @param depth current depth
 * @param maxDepth maximum allowed depth
 * @param rng random number generator
 * @return index of the created node in the nodes vector
 */
static size_t BuildTree(std::vector<IsoTreeNode> &nodes, const double *data, size_t *indices, size_t count, size_t depth, size_t maxDepth, std::mt19937_64 &rng)
{
   size_t nodeIdx = nodes.size();
   nodes.push_back(IsoTreeNode());

   // Terminal conditions: single element, max depth reached, or constant data
   if (count <= 1 || depth >= maxDepth)
   {
      nodes[nodeIdx].left = 0;
      nodes[nodeIdx].right = 0;
      nodes[nodeIdx].score = static_cast<double>(depth) + ExpectedAvgDepth(count);
      return nodeIdx;
   }

   // Find min/max of data at this node
   double minVal = data[indices[0]];
   double maxVal = data[indices[0]];
   for (size_t i = 1; i < count; i++)
   {
      double v = data[indices[i]];
      if (v < minVal) minVal = v;
      if (v > maxVal) maxVal = v;
   }

   // Constant data - make leaf
   if (minVal == maxVal)
   {
      nodes[nodeIdx].left = 0;
      nodes[nodeIdx].right = 0;
      nodes[nodeIdx].score = static_cast<double>(depth) + ExpectedAvgDepth(count);
      return nodeIdx;
   }

   // Pick random split value uniformly between min and max
   std::uniform_real_distribution<double> dist(minVal, maxVal);
   double splitValue = dist(rng);
   nodes[nodeIdx].splitValue = splitValue;

   // Partition indices: left (value <= split), right (value > split)
   size_t leftCount = 0;
   for (size_t i = 0; i < count; i++)
   {
      if (data[indices[i]] <= splitValue)
      {
         if (i != leftCount)
            std::swap(indices[i], indices[leftCount]);
         leftCount++;
      }
   }

   // Edge case: all went one way (can happen with uniform split at boundary)
   if (leftCount == 0 || leftCount == count)
   {
      nodes[nodeIdx].left = 0;
      nodes[nodeIdx].right = 0;
      nodes[nodeIdx].score = static_cast<double>(depth) + ExpectedAvgDepth(count);
      return nodeIdx;
   }

   // Recursively build children
   size_t leftIdx = BuildTree(nodes, data, indices, leftCount, depth + 1, maxDepth, rng);
   size_t rightIdx = BuildTree(nodes, data, indices + leftCount, count - leftCount, depth + 1, maxDepth, rng);
   nodes[nodeIdx].left = leftIdx;
   nodes[nodeIdx].right = rightIdx;
   return nodeIdx;
}

/**
 * Fit an Isolation Forest model
 *
 * @param data array of data values
 * @param nrows number of data points
 * @param ntrees number of trees to build
 * @param trees output: vector of trees (each tree is a vector of IsoTreeNode)
 * @param treeRoots output: root index for each tree
 */
static void FitIsolationForest(const double *data, size_t nrows, size_t ntrees, std::vector<std::vector<IsoTreeNode>> &trees, std::vector<size_t> &treeRoots)
{
   // max_depth = ceil(log2(nrows)) when limit_depth=true (isotree default)
   size_t maxDepth = (nrows <= 1) ? 1 : static_cast<size_t>(ceil(log2(static_cast<double>(nrows))));

   trees.resize(ntrees);
   treeRoots.resize(ntrees);

   // Fixed seed for reproducibility (isotree defaults to seed=1)
   std::mt19937_64 rng(1);

   // Index array reused across trees (no subsampling: sample_size = nrows)
   std::vector<size_t> indices(nrows);

   for (size_t t = 0; t < ntrees; t++)
   {
      // Initialize index array
      for (size_t i = 0; i < nrows; i++)
         indices[i] = i;

      // Shuffle indices (Fisher-Yates) to randomize tree structure
      for (size_t i = nrows - 1; i > 0; i--)
      {
         std::uniform_int_distribution<size_t> idist(0, i);
         size_t j = idist(rng);
         std::swap(indices[i], indices[j]);
      }

      trees[t].clear();
      trees[t].reserve(nrows);
      treeRoots[t] = BuildTree(trees[t], data, indices.data(), nrows, 0, maxDepth, rng);
   }
}

/**
 * Score a single data point through one tree
 */
static inline double ScorePoint(const std::vector<IsoTreeNode> &tree, size_t root, double value)
{
   size_t idx = root;
   while (tree[idx].left != 0)  // not a leaf
   {
      idx = (value <= tree[idx].splitValue) ? tree[idx].left : tree[idx].right;
   }
   return tree[idx].score;
}

/**
 * Predict anomaly scores for all data points
 *
 * @param data array of data values
 * @param nrows number of data points
 * @param trees vector of trees
 * @param treeRoots root index for each tree
 * @return vector of standardized anomaly scores (higher = more anomalous)
 */
static std::vector<double> PredictIsolationForest(const double *data, size_t nrows, const std::vector<std::vector<IsoTreeNode>> &trees, const std::vector<size_t> &treeRoots)
{
   size_t ntrees = trees.size();
   double cn = ExpectedAvgDepth(nrows);
   std::vector<double> scores(nrows);

   for (size_t i = 0; i < nrows; i++)
   {
      double sumScore = 0.0;
      for (size_t t = 0; t < ntrees; t++)
         sumScore += ScorePoint(trees[t], treeRoots[t], data[i]);

      if (cn > 0.0)
         scores[i] = exp2(-sumScore / (static_cast<double>(ntrees) * cn));
      else
         scores[i] = 0.5;  // undefined for trivial data
   }

   return scores;
}

/**
 * Helper function to read last N values of given DCI
 */
static unique_ptr<StructArray<ScoredDciValue>> LoadDciValues(uint32_t nodeId, uint32_t dciId, DCObjectStorageClass storageClass, const std::pair<Timestamp, Timestamp> *timeRanges, int numTimeRanges, bool hasV5Table = false)
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
   else if (hasV5Table)
   {
      query.append(_T(" FROM (SELECT idata_timestamp,idata_value FROM idata_"));
      query.append(nodeId);
      query.append(_T(" WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"));
      query.append(_T(" UNION ALL SELECT "));
      query.append(V5TimestampToMs(false));
      query.append(_T(",idata_value FROM idata_v5_"));
      query.append(nodeId);
      query.append(_T(" WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?) d"));
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
         if (hasV5Table)
         {
            DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, dciId);
            DBBind(hStmt, 5, DB_SQLTYPE_BIGINT, timeRanges[n].first.asMilliseconds() / 1000);
            DBBind(hStmt, 6, DB_SQLTYPE_BIGINT, timeRanges[n].second.asMilliseconds() / 1000);
         }
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int count = DBGetNumRows(hResult);
            for(int i = 0; i < count; i++)
            {
               ScoredDciValue *v = values->addPlaceholder();
               v->timestamp = DBGetFieldTimestamp(hResult, i, 0);
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

   std::pair<Timestamp, Timestamp> timeRange(Timestamp::fromTime(timeFrom), Timestamp::fromTime(timeTo));
   auto series = LoadDciValues(dcTarget.getId(), dciId, dci->getStorageClass(), &timeRange, 1, dcTarget.hasV5IdataTable());
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

   double *points = MemAllocArrayNoInit<double>(count);
   for(int i = 0; i < count; i++)
      points[i] = series->get(i)->value;

   std::vector<std::vector<IsoTreeNode>> trees;
   std::vector<size_t> treeRoots;
   FitIsolationForest(points, static_cast<size_t>(count), 500, trees, treeRoots);
   std::vector<double> scores = PredictIsolationForest(points, static_cast<size_t>(count), trees, treeRoots);
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
   Timestamp now = Timestamp::now();
   Timestamp t = now - width * 30000;  // Half-interval in minutes to milliseconds
   std::pair<Timestamp, Timestamp> timeRanges[90]; // up to 90 days back
   int64_t w = static_cast<int64_t>(width) * 60000; // width in milliseconds
   for(int i = 0; i < depth; i++)
   {
      t -= 86400 * period;
      timeRanges[i].first = t;
      timeRanges[i].second = t + w;
   }

   auto series = LoadDciValues(dcTarget.getId(), dci.getId(), dci.getStorageClass(), timeRanges, depth, dcTarget.hasV5IdataTable());
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

   std::vector<std::vector<IsoTreeNode>> trees;
   std::vector<size_t> treeRoots;
   FitIsolationForest(points, static_cast<size_t>(count), 500, trees, treeRoots);
   std::vector<double> scores = PredictIsolationForest(points, static_cast<size_t>(count), trees, treeRoots);
   MemFree(points);

   nxlog_debug_tag(DEBUG_TAG, 6, _T("IsAnomalousValue(%s [%u], \"%s\"): score for value %f and period [p=%d d=%d w=%d] is %f"),
         dcTarget.getName(), dcTarget.getId(), dci.getName().cstr(), value, period, depth, width, scores[count - 1]);
   return scores[count - 1] >= threshold;
}
