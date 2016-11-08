/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: npe.h
**
**/

#ifndef _npe_h_
#define _npe_h_

/**
 * Max. length for engine error message
 */
#define MAX_NPE_ERROR_MESSAGE_LEN   1024

/**
 * Max. length for engine name
 */
#define MAX_NPE_NAME_LEN            16

/**
 * Prediction engine interface
 */
class NXCORE_EXPORTABLE PredictionEngine
{
public:
   PredictionEngine();
   virtual ~PredictionEngine();

   virtual const TCHAR *getName() const = 0;
   virtual const TCHAR *getDescription() const = 0;
   virtual const TCHAR *getVersion() const = 0;
   virtual const TCHAR *getVendor() const = 0;

   virtual bool initialize(TCHAR *errorMessage);

   virtual void update(UINT32 dciId, time_t timestamp, double value) = 0;
   virtual double getPredictedValue(UINT32 dciId, time_t timestamp) = 0;
};

/**
 * API
 */
PredictionEngine NXCORE_EXPORTABLE *FindPredictionEngine(const TCHAR *name);

#endif
