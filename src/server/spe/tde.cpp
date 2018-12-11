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

#define DEBUG_TAG _T("npe.tde")

#define INPUT_LAYER_SIZE   6
#define TRAIN_SIZE   7

/**
 * Constructor
 */
TimeDependentEngine::TimeDependentEngine() : PredictionEngine(), m_networks(false)
{
   m_networkLock = MutexCreate();
}

/**
 * Destructor
 */
TimeDependentEngine::~TimeDependentEngine()
{
   MutexDestroy(m_networkLock);
}

/**
 * Get engine name
 */
const TCHAR *TimeDependentEngine::getName() const
{
   return _T("TDE");
}

/**
 * Get engine description
 */
const TCHAR *TimeDependentEngine::getDescription() const
{
   return _T("Time dependent engine");
}

/**
 * Get engine version
 */
const TCHAR *TimeDependentEngine::getVersion() const
{
   return NETXMS_BUILD_TAG;
}

/**
 * Get engine vendor
 */
const TCHAR *TimeDependentEngine::getVendor() const
{
   return _T("Raden Solutions");
}

/**
 * Initialize engine
 *
 * @param errorMessage buffer for error message in case of failure
 * @return true on success
 */
bool TimeDependentEngine::initialize(TCHAR *errorMessage)
{
   return true;
}

/**
 * Check if engine requires training
 *
 * @return true if engine requires training
 */
bool TimeDependentEngine::requiresTraining()
{
   return true;
}

void formateValue(DciValue *value, double oldValue, double *array)
{
   tm *time = localtime(&(value->timestamp));
   array[0] = time->tm_wday;
   array[1] = time->tm_mday;
   array[2] = time->tm_mon;
   array[3] = time->tm_hour;
   array[4] = time->tm_min;
   array[5] = oldValue;

}

/**
 * Get engine accuracy using existing training data for given DCI
 *
 * @param nodeId Node object ID
 * @param dciId DCI ID
 */
void TimeDependentEngine::getAccuracy(UINT32 nodeId, UINT32 dciId)
{
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Starting accuracy check for DCI %u/%u"), nodeId, dciId);
   StructArray<DciValue> *values = getDciValues(nodeId, dciId, 10001);
   double result = 0;

   if ((values != NULL) && (values->size() > 2))
   {
      double *series = new double[(values->size() - 1) * TRAIN_SIZE];
      for(int i = 0; i < (values->size() - 1); i++)
      {
         formateValue(values->get(i+1), values->get(i)->value, series+i*TRAIN_SIZE);
         series[i*TRAIN_SIZE+6] = values->get(i+1)->value;
      }
      NeuralNetwork *nn = acquireNetwork(nodeId, dciId);
      result = nn->accuracy(series, values->size() - 1, 0.1);
      nn->decRefCount();
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
void TimeDependentEngine::train(UINT32 nodeId, UINT32 dciId)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Starting training for DCI %u/%u"), nodeId, dciId);
   StructArray<DciValue> *values = getDciValues(nodeId, dciId, 30001);
   if ((values != NULL) && (values->size() > 2))
   {
      double *series = new double[(values->size() - 1) * TRAIN_SIZE];
      for(int i = 0; i < (values->size() - 1); i++)
      {
         formateValue(values->get(i+1), values->get(i)->value, series+i*TRAIN_SIZE);
         series[i*TRAIN_SIZE+6] = values->get(i+1)->value;
         //nxlog_debug_tag(DEBUG_TAG, 2, _T("!!!train %f %f"), series[i*TRAIN_SIZE+5], series[i*TRAIN_SIZE+6]);
      }
      NeuralNetwork *nn = acquireNetwork(nodeId, dciId);
      NeuralNetwork *newNn = new NeuralNetwork(nn);
      nn->decRefCount();
      newNn->train(series, values->size()-1, 10000, 0.01);
      replaceNetwork(nodeId, dciId, newNn);
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Final neural network model weights and biases:"));
      delete[] series;
   }
   delete values;
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Training completed for DCI %u/%u"), nodeId, dciId);
   getAccuracy(nodeId, dciId);
}

/**
 * Update internal model for given DCI
 *
 * @param nodeId Node object ID
 * @param dciId DCI ID
 * @param timestamp timestamp of new value
 * @param value new value
 */
void TimeDependentEngine::update(UINT32 nodeId, UINT32 dciId, time_t timestamp, double value)
{
}

/**
 * Reset internal model for given DCI
 *
 * @param nodeId Node object ID
 * @param dciId DCI ID
 */
void TimeDependentEngine::reset(UINT32 nodeId, UINT32 dciId)
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
double TimeDependentEngine::getPredictedValue(UINT32 nodeId, UINT32 dciId, time_t timestamp)
{
   StructArray<DciValue> *values = getDciValues(nodeId, dciId, 1);
   if (values == NULL || values->size() < 1)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("TimeDependentEngine::getPredictedValue: cannot read data for DCI %u/%u"), nodeId, dciId);
      return 0;
   }

   double *series = new double[INPUT_LAYER_SIZE];
   tm *time = localtime(&timestamp);
   series[0] = time->tm_wday;
   series[1] = time->tm_mday;
   series[2] = time->tm_mon;
   series[3] = time->tm_hour;
   series[4] = time->tm_min;
   series[5] = values->get(0)->value;
   delete values;

   NeuralNetwork *nn = acquireNetwork(nodeId, dciId);
   double result = nn->computeOutput(series);
   nn->decRefCount();

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
bool TimeDependentEngine::getPredictedSeries(UINT32 nodeId, UINT32 dciId, int count, double *series)
{
   StructArray<DciValue> *values = getDciValues(nodeId, dciId, 1);
   if (values == NULL || values->size() < 1)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("TimeDependentEngine::getPredictedValue: cannot read data for DCI %u/%u"), nodeId, dciId);
      return false;
   }

   double *input = new double[INPUT_LAYER_SIZE];
   time_t startTime=time(NULL);
   tm *time = localtime(&startTime);
   input[0] = time->tm_wday;
   input[1] = time->tm_mday;
   input[2] = time->tm_mon;
   input[3] = time->tm_hour;
   input[4] = time->tm_min;
   input[5] = values->get(0)->value;
   delete values;

   NetObj *object = FindObjectById(nodeId);
   DCObject *dci = static_cast<DataCollectionTarget*>(object)->getDCObjectById(dciId, 0);
   time_t interval = dci->getEffectivePollingInterval();
   NeuralNetwork *nn = acquireNetwork(nodeId, dciId);
   //nxlog_debug_tag(DEBUG_TAG, 2, _T("!!!requested time %d %f %d"), startTime, input[5], interval);
   for(int n = 0; n < count-1; n++)
   {
      series[count-1-n] = nn->computeOutput(input);

      startTime+=interval;
      time = localtime(&startTime);
      input[0] = time->tm_wday;
      input[1] = time->tm_mday;
      input[2] = time->tm_mon;
      input[3] = time->tm_hour;
      input[4] = time->tm_min;
      input[5] = series[count-1-n];

      //nxlog_debug_tag(DEBUG_TAG, 2, _T("!!!requested time %d %f"), startTime, series[count-1-n]);
   }
   series[0] = nn->computeOutput(input); //no value bid
   nn->decRefCount();

   delete[] input;
   return true;
}

/**
 * Get DCI cache size required by this engine
 */
int TimeDependentEngine::getRequiredCacheSize() const
{
   return 1;
}

/**
 * Acquire neural network object. Caller must unlock acquired object when done.
 */
NeuralNetwork *TimeDependentEngine::acquireNetwork(UINT32 nodeId, UINT32 dciId)
{
   TCHAR nid[64];
   _sntprintf(nid, 64, _T("%u/%u"), nodeId, dciId);

   MutexLock(m_networkLock);
   NeuralNetwork *nn = m_networks.get(nid);
   if (nn == NULL)
   {
      nn = new NeuralNetwork(INPUT_LAYER_SIZE, 40);
      m_networks.set(nid, nn);
   }
   MutexUnlock(m_networkLock);

   return nn;
}

/**
 * Update neural network after training
 */
void TimeDependentEngine::replaceNetwork(UINT32 nodeId, UINT32 dciId, NeuralNetwork *newNn)
{
   TCHAR nid[64];
   _sntprintf(nid, 64, _T("%u/%u"), nodeId, dciId);

   MutexLock(m_networkLock);
   NeuralNetwork *nn = m_networks.get(nid);
   m_networks.set(nid, newNn);
   nn->decRefCount();
   MutexUnlock(m_networkLock);
}
