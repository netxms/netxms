/* 
** NetXMS - Network Management System
** Database manager Library
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
** File: libnxdbmgr.h
**
**/

#ifndef _libnxdbmgr_h_
#define _libnxdbmgr_h_

#include <nms_common.h>
#include <nms_util.h>
#include <uuid.h>
#include <nms_threads.h>
#include <nxdbapi.h>
#include <nxsrvapi.h>
#include <nxdbmgr_tools.h>

// Non-standard SQL type codes
#define SQL_TYPE_TEXT      0
#define SQL_TYPE_TEXT4K    1
#define SQL_TYPE_INT64     2

extern bool g_queryTrace;
extern const TCHAR *g_sqlTypes[6][3];
extern const TCHAR *g_tableSuffix;

#endif   /* _libnxdbmgr_h_ */
