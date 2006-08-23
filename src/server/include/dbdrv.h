/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: dbdrv.h
**
**/

#ifndef _dbdrv_h_
#define _dbdrv_h_

#include <nms_common.h>


//
// API version
//

#define DBDRV_API_VERSION           1


//
// Datatypes
//

typedef void * DB_CONNECTION;
typedef void * DB_RESULT;
typedef void * DB_ASYNC_RESULT;


//
// Error codes
//

#define DBERR_SUCCESS               0
#define DBERR_CONNECTION_LOST       1
#define DBERR_INVALID_HANDLE        2
#define DBERR_OTHER_ERROR           255

#endif   /* _dbdrv_h_ */
