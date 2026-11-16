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

/**
 * Embedded Isolation Forest implementation (Liu et al., 2008). Only supports univariate numeric data,
 * without subsampling (sample_size=nrows) and with depth limit. Not used for checking of collected DCI values
 * (see anomaly_engine.cpp for that).
 */

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
void FitIsolationForest(const double *data, size_t nrows, size_t ntrees, std::vector<std::vector<IsoTreeNode>> &trees, std::vector<size_t> &treeRoots)
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
std::vector<double> PredictIsolationForest(const double *data, size_t nrows, const std::vector<std::vector<IsoTreeNode>> &trees, const std::vector<size_t> &treeRoots)
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
