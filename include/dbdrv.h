/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: dbdrv.h
**
**/

#ifndef _dbdrv_h_
#define _dbdrv_h_

#include <nms_common.h>


//
// API version
//

#define DBDRV_API_VERSION           10


//
// Driver header
//

#ifdef _WIN32
#define __EXPORT __declspec(dllexport)
#else
#define __EXPORT
#endif

#define DECLARE_DRIVER_HEADER(name) \
extern "C" int EXPORT drvAPIVersion; \
extern "C" const char EXPORT *drvName; \
int EXPORT drvAPIVersion = DBDRV_API_VERSION; \
const char EXPORT *drvName = name;

#undef __EXPORT


//
// Constants
//

#define DBDRV_MAX_ERROR_TEXT        1024


//
// Datatypes
//

typedef void * DBDRV_CONNECTION;
typedef void * DBDRV_RESULT;
typedef void * DBDRV_ASYNC_RESULT;


//
// Error codes
//

#define DBERR_SUCCESS               0
#define DBERR_CONNECTION_LOST       1
#define DBERR_INVALID_HANDLE        2
#define DBERR_OTHER_ERROR           255

#endif   /* _dbdrv_h_ */
