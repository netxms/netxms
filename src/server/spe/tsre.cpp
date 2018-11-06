/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Raden Solutions
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

#define DEBUG_TAG _T("npe.tsre")

#define INPUT_LAYER_SIZE   5

/**
 * Constructor
 */
TimeSeriesRegressionEngine::TimeSeriesRegressionEngine() : PredictionEngine(), m_networks(true)
{
   m_networkLock = MutexCreate();
}

/**
 * Destructor
 */
TimeSeriesRegressionEngine::~TimeSeriesRegressionEngine()
{
   MutexDestroy(m_networkLock);
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
 * Get engine accuracy using existing training data for given DCI
 *
 * @param nodeId Node object ID
 * @param dciId DCI ID
 */
void TimeSeriesRegressionEngine::getAccuracy(UINT32 nodeId, UINT32 dciId)
{
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Starting accuracy check for DCI %u/%u"), nodeId, dciId);
   StructArray<DciValue> *values = getDciValues(nodeId, dciId, 1000);
   double result = 0;

   if ((values != NULL) && (values->size() > INPUT_LAYER_SIZE))
   {
      double *series = new double[values->size()];
      for(int i = 0, j = values->size(); i < values->size(); i++)
         series[--j] = values->get(i)->value;
      NeuralNetwork *nn = acquireNetwork(nodeId, dciId);
      result = nn->accuracy(series, values->size(), 0.5);
      nn->unlock();
      delete[] series;
   }
   delete values;
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Prediction accuracy for DCI %u/%u is %f"), nodeId, dciId, result);
}

/**
 * Train engine using existing data for given DCI
 *
 * @param nodeId Node object ID
 * @param dciId DCI ID
 */
void TimeSeriesRegressionEngine::train(UINT32 nodeId, UINT32 dciId)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Starting training for DCI %u/%u"), nodeId, dciId);
   StructArray<DciValue> *values = getDciValues(nodeId, dciId, 5000);
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
 * @param timestamp timestamp of new value
 * @param value new value
 */
void TimeSeriesRegressionEngine::update(UINT32 nodeId, UINT32 dciId, time_t timestamp, double value)
{
}

/**
 * Reset internal model for given DCI
 *
 * @param nodeId Node object ID
 * @param dciId DCI ID
 */
void TimeSeriesRegressionEngine::reset(UINT32 nodeId, UINT32 dciId)
{
   TCHAR nid[64];
   _sntprintf(nid, 64, _T("%u/%u"), nodeId, dciId);

   MutexLock(m_networkLock);
   m_networks.remove(nid);
   MutexUnlock(m_networkLock);
}

/**
 * Get predicted value for given DCI and time
 *
 * @param nodeId Node object ID
 * @param dciId DCI ID
 * @param timestamp timestamp of interest
 * @return predicted value
 */
double TimeSeriesRegressionEngine::getPredictedValue(UINT32 nodeId, UINT32 dciId, time_t timestamp)
{
   StructArray<DciValue> *values = getDciValues(nodeId, dciId, INPUT_LAYER_SIZE);
   if (values == NULL)
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
    * @param count number of values to retrieve
    * @param series buffer for values
    * @return true on success
    */
bool TimeSeriesRegressionEngine::getPredictedSeries(UINT32 nodeId, UINT32 dciId, int count, double *series)
{
   StructArray<DciValue> *values = getDciValues(nodeId, dciId, INPUT_LAYER_SIZE);
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
      series[count-1-n] = nn->computeOutput(input);
      nxlog_debug_tag(DEBUG_TAG, 2, _T("TimeSeriesRegressionEngine::getPredictedValue: input: %lf %lf %lf %lf %lf output: %lf"), input[0], input[1], input[2], input[3], input[4], series[count-1-n]);

      for(int i = 0; i < INPUT_LAYER_SIZE-1; i++)
         input[i] = input[i+1];
      input[INPUT_LAYER_SIZE-1] = series[count-1-n];
   }
   series[0] = nn->computeOutput(input); //no value bid
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

   MutexLock(m_networkLock);
   NeuralNetwork *nn = m_networks.get(nid);
   if (nn == NULL)
   {
      nn = new NeuralNetwork(INPUT_LAYER_SIZE, 24);
      m_networks.set(nid, nn);
   }
   nn->lock();
   MutexUnlock(m_networkLock);

   return nn;
}
