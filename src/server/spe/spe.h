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
** File: spe.h
**
**/

#ifndef _spe_h_
#define _spe_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_core.h>
#include <nxmodule.h>
#include <nxconfig.h>
#include <npe.h>

/**
 * Neural network node
 */
struct NeuralNetworkNode
{
   int numWeights;
   double *weights;
   double *weightGradients;
   double bias;
   double biasGradient;
   double value;

   NeuralNetworkNode(int nextLevelSize);
   ~NeuralNetworkNode();

   void reset();
};

/**
 * Neural network class
 */
class NeuralNetwork
{
private:
   ObjectArray<NeuralNetworkNode> m_input;
   ObjectArray<NeuralNetworkNode> m_hidden;
   NeuralNetworkNode m_output;
   Mutex m_mutex;

public:
   NeuralNetwork(int inputCount, int hiddenCount);
   ~NeuralNetwork();

   double computeOutput(double *inputs);
   void train(double *series, size_t length, int rounds, double learnRate);

   void lock() { m_mutex.lock(); }
   void unlock() { m_mutex.unlock(); }
};

/**
 * Prediction engine based on time series regression
 */
class TimeSeriesRegressionEngine : public PredictionEngine
{
private:
   StringObjectMap<NeuralNetwork> m_networks;
   Mutex m_networkLock;

   NeuralNetwork *acquireNetwork(UINT32 nodeId, UINT32 dciId);

public:
   TimeSeriesRegressionEngine();
   virtual ~TimeSeriesRegressionEngine();

   /**
    * Get engine name (must not be longer than 15 characters)
    */
   virtual const TCHAR *getName() const override;

   /**
    * Get engine description
    */
   virtual const TCHAR *getDescription() const override;

   /**
    * Get engine version
    */
   virtual const TCHAR *getVersion() const override;

   /**
    * Get engine vendor
    */
   virtual const TCHAR *getVendor() const override;

   /**
    * Initialize engine
    *
    * @param errorMessage buffer for error message in case of failure
    * @return true on success
    */
   virtual bool initialize(TCHAR *errorMessage) override;

   /**
    * Check if engine requires training
    *
    * @return true if engine requires training
    */
   virtual bool requiresTraining() override;

   /**
    * Train engine using existing data for given DCI
    *
    * @param nodeId Node object ID
    * @param dciId DCI ID
    * @param storageClass DCI storage class
    */
   virtual void train(UINT32 nodeId, UINT32 dciId, DCObjectStorageClass storageClass) override;

   /**
    * Update internal model for given DCI
    *
    * @param nodeId Node object ID
    * @param dciId DCI ID
    * @param storageClass DCI storage class
    * @param timestamp timestamp of new value
    * @param value new value
    */
   virtual void update(UINT32 nodeId, UINT32 dciId, DCObjectStorageClass storageClass, Timestamp timestamp, double value) override;

   /**
    * Reset internal model for given DCI
    *
    * @param nodeId Node object ID
    * @param dciId DCI ID
    * @param storageClass DCI storage class
    */
   virtual void reset(UINT32 nodeId, UINT32 dciId, DCObjectStorageClass storageClass) override;

   /**
    * Get predicted value for given DCI and time
    *
    * @param nodeId Node object ID
    * @param dciId DCI ID
    * @param storageClass DCI storage class
    * @param timestamp timestamp of interest
    * @return predicted value
    */
   virtual double getPredictedValue(UINT32 nodeId, UINT32 dciId, DCObjectStorageClass storageClass, time_t timestamp) override;

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
   virtual bool getPredictedSeries(UINT32 nodeId, UINT32 dciId, DCObjectStorageClass storageClass, int count, double *series) override;

   /**
    * Get DCi cache size required by this engine
    */
   virtual int getRequiredCacheSize() const override;
};

#endif
