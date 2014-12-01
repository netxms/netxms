/* 
** NetXMS - Network Management System
** Performance Data Storage Driver for RRDTools
** Copyright (C) 2014 Raden Solutions
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
** File: rrd.cpp
**/

#include <nms_core.h>
#include <pdsdrv.h>

/**
 * Driver class definition
 */
class RRDToolStorageDriver : public PerfDataStorageDriver
{
private:

public:
   virtual const TCHAR *getName();

   virtual bool saveDCItemValue(DCItem *dcObject, time_t timestamp, const TCHAR *value);
   virtual bool saveDCTableValue(DCTable *dcObject, time_t timestamp, Table *value);
};

/**
 * Driver name
 */
static TCHAR *s_driverName = _T("RRDTOOL");

/**
 * Get name
 */
const TCHAR *RRDToolStorageDriver::getName()
{
   return s_driverName;
}

/**
 * Save DCI value
 */
bool RRDToolStorageDriver::saveDCItemValue(DCItem *dcObject, time_t timestamp, const TCHAR *value)
{
   _tprintf(_T("SAVE: %s %s\n\n"), dcObject->getName(), value);
   return true;
}

/**
 * Save table DCI value
 */
bool RRDToolStorageDriver::saveDCTableValue(DCTable *dcObject, time_t timestamp, Table *value)
{
   _tprintf(_T("SAVE TABLE: %s %d\n\n"), dcObject->getName(), value->getNumRows());
   return true;
}

/**
 * Driver entry point
 */
DECLARE_PDSDRV_ENTRY_POINT(s_driverName, RRDToolStorageDriver);
