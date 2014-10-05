/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: pdsdrv.h
**
**/

#ifndef _pdsdrv_h_
#define _pdsdrv_h_

#include <nms_common.h>
#include <nxsrvapi.h>

/**
 *API version
 */
#define PDSDRV_API_VERSION          1

/**
 * Driver header
 */
#ifdef _WIN32
#define __PDSDRV_EXPORT __declspec(dllexport)
#else
#define __PDSDRV_EXPORT
#endif

#define DECLARE_PDSDRV_ENTRY_POINT(name, implClass) \
extern "C" int __PDSDRV_EXPORT pdsdrvAPIVersion; \
extern "C" const TCHAR __PDSDRV_EXPORT *pdsdrvName; \
int __PDSDRV_EXPORT pdsdrvAPIVersion = PDSDRV_API_VERSION; \
const TCHAR __PDSDRV_EXPORT *pdsdrvName = name; \
extern "C" PerfDataStorageDriver __PDSDRV_EXPORT *pdsdrvCreateInstance() { return new implClass; }

/**
 * Base class for performance data storage drivers
 */
class NXCORE_EXPORTABLE PerfDataStorageDriver
{
public:
   PerfDataStorageDriver();
   virtual ~PerfDataStorageDriver();

   virtual const TCHAR *getName();
   virtual const TCHAR *getVersion();

   virtual bool init();
   virtual void shutdown();

   virtual bool saveDCItemValue(DCItem *dcObject, time_t timestamp, const TCHAR *value);
   virtual bool saveDCTableValue(DCTable *dcObject, time_t timestamp, Table *value);
};

#endif   /* _pdsdrv_h_ */
