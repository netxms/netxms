/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Raden Solutions
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
** File: tsre.cpp
**
**/

#include "spe.h"
#include <netxms-version.h>

#define DEBUG_TAG _T("npe.tsre")

#define INPUT_LAYER_SIZE   5

/**
 * Constructor
 */
TimeSeriesRegressionEngine::TimeSeriesRegressionEngine() : PredictionEngine(), m_networks(Ownership::True), m_networkLock(MutexType::FAST)
{
}

/**
 * Destructor
 */
TimeSeriesRegressionEngine::~TimeSeriesRegressionEngine()
{
}

/**
 * Get engine name
 */
const TCHAR *TimeSeriesRegressionEngine::getName() const
{
   return _T("TSR");
}

/**
 * Get engine description
 */
const TCHAR *TimeSeriesRegressionEngine::getDescription() const
{
   return _T("Time series regression");
}

/**
 * Get engine version
 */
const TCHAR *TimeSeriesRegressionEngine::getVersion() const
{
   return NETXMS_BUILD_TAG;
}

/**
 * Get engine vendor
 */
const TCHAR *TimeSeriesRegressionEngine::getVendor() const
{
   return _T("Raden Solutions");
}

/**
 * Initialize engine
 *
 * @param errorMessage buffer for error message in case of failure
 * @return true on success
 */
bool TimeSeriesRegressionEngine::initialize(TCHAR *errorMessage)
{
   return true;
}

/**
 * Check if engine requires training
 *
 * @return true if engine requires training
 */
bool TimeSeriesRegressionEngine::requiresTraining()
{
   return true;
}

/**
 * Train engine using existing data for given DCI
 *
 * @param nodeId Node object ID
 * @param dciId DCI ID
 * @param storageClass DCI storage class
 */
void TimeSeriesRegressionEngine::train(UINT32 nodeId, UINT32 dciId, DCObjectStorageClass storageClass)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Starting training for DCI %u/%u"), nodeId, dciId);
   StructArray<DciValue> *values = getDciValues(nodeId, dciId, storageClass, 10000);
   if ((values != NULL) && (values->size() > INPUT_LAYER_SIZE))
   {
      double *series = new double[values->size()];
      for(int i = 0, j = values->size(); i < values->size(); i++)
         series[--j] = values->get(i)->value;
      NeuralNetwork *nn = acquireNetwork(nodeId, dciId);
      nn->train(series, values->size(), 10000, 0.01);
      nn->unlock();
      delete[] series;
   }
   delete values;
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Training completed for DCI %u/%u"), nodeId, dciId);
}

/**
 * Update internal model for given DCI
 *
 * @param nodeId Node object ID
 * @param dciId DCI ID
 * @param storageClass DCI storage class
 * @param timestamp timestamp of new value
 * @param value new value
 */
void TimeSeriesRegressionEngine::update(UINT32 nodeId, UINT32 dciId, DCObjectStorageClass storageClass, Timestamp timestamp, double value)
{
}

/**
 * Reset internal model for given DCI
 *
 * @param nodeId Node object ID
 * @param dciId DCI ID
 * @param storageClass DCI storage class
 */
void TimeSeriesRegressionEngine::reset(UINT32 nodeId, UINT32 dciId, DCObjectStorageClass storageClass)
{
   TCHAR nid[64];
   _sntprintf(nid, 64, _T("%u/%u"), nodeId, dciId);

   m_networkLock.lock();
   m_networks.remove(nid);
   m_networkLock.unlock();
}

/**
 * Get predicted value for given DCI and time
 *
 * @param nodeId Node object ID
 * @param dciId DCI ID
 * @param storageClass DCI storage class
 * @param timestamp timestamp of interest
 * @return predicted value
 */
double TimeSeriesRegressionEngine::getPredictedValue(UINT32 nodeId, UINT32 dciId, DCObjectStorageClass storageClass, time_t timestamp)
{
   StructArray<DciValue> *values = getDciValues(nodeId, dciId, storageClass, INPUT_LAYER_SIZE);
   if (values == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("TimeSeriesRegressionEngine::getPredictedValue: cannot read data for DCI %u/%u"), nodeId, dciId);
      return 0;
   }

   double *series = new double[INPUT_LAYER_SIZE];
   int j = INPUT_LAYER_SIZE;
   for(int i = 0; i < values->size(); i++)
      series[--j] = values->get(i)->value;
   while(j > 0)
      series[--j] = 0.0;
   delete values;

   NeuralNetwork *nn = acquireNetwork(nodeId, dciId);
   double result = nn->computeOutput(series);
   nn->unlock();

   delete[] series;
   return result;
}

/**
 * Get series of predicted values starting with current time. Default implementation
 * calls getPredictedValue with incrementing timestamp
 *
 * @param nodeId Node object ID
 * @param dciId DCI ID
 * @param storageClass DCI storage class
 * @param count number of values to retrieve
 * @param series buffer for values
 * @return true on success
 */
bool TimeSeriesRegressionEngine::getPredictedSeries(UINT32 nodeId, UINT32 dciId, DCObjectStorageClass storageClass, int count, double *series)
{
   StructArray<DciValue> *values = getDciValues(nodeId, dciId, storageClass, INPUT_LAYER_SIZE);
   if (values == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("TimeSeriesRegressionEngine::getPredictedValue: cannot read data for DCI %u/%u"), nodeId, dciId);
      return false;
   }

   double *input = new double[INPUT_LAYER_SIZE];
   int j = INPUT_LAYER_SIZE;
   for(int i = 0; i < values->size(); i++)
      input[--j] = values->get(i)->value;
   while(j > 0)
      input[--j] = 0.0;
   delete values;

   NeuralNetwork *nn = acquireNetwork(nodeId, dciId);
   for(int n = 0; n < count-1; n++)
   {
      series[n] = nn->computeOutput(input);

      for(int i = INPUT_LAYER_SIZE - 1; i > 0; i--)
         input[i-1] = input[i];
      input[INPUT_LAYER_SIZE-1] = series[n];
   }
   series[count-1] = nn->computeOutput(input); //no value bid
   nn->unlock();

   delete[] input;
   return true;
}

/**
 * Get DCI cache size required by this engine
 */
int TimeSeriesRegressionEngine::getRequiredCacheSize() const
{
   return 1;
}

/**
 * Acquire neural network object. Caller must unlock acquired object when done.
 */
NeuralNetwork *TimeSeriesRegressionEngine::acquireNetwork(UINT32 nodeId, UINT32 dciId)
{
   TCHAR nid[64];
   _sntprintf(nid, 64, _T("%u/%u"), nodeId, dciId);

   m_networkLock.lock();
   NeuralNetwork *nn = m_networks.get(nid);
   if (nn == nullptr)
   {
      nn = new NeuralNetwork(INPUT_LAYER_SIZE, 10);
      m_networks.set(nid, nn);
   }
   nn->lock();
   m_networkLock.unlock();

   return nn;
}
