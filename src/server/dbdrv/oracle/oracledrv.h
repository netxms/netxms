/* $Id: oracledrv.h,v 1.1 2007-02-15 23:00:37 victor Exp $ */
/** Oracle Database Driver
** Copyright (C) 2007 Victor Kirhenshtein
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
** File: oracledrv.h
**
**/

#ifndef _oracledrv_h_
#define _oracledrv_h_

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif   /* _WIN32 */

#include <nms_common.h>
#include <nms_util.h>
#include <dbdrv.h>
#include <oci.h>

typedef struct
{
	OCIEnv *handleEnv;
	OCIServer *handleServer;
	OCISvcCtx *handleService;
	OCISession *handleSession;
	OCIError *handleError;
	MUTEX mutexQueryLock;
	int nTransLevel;
} ORACLE_CONN;

typedef struct
{
	int nRows;
	int nCols;
	WCHAR **pData;
} ORACLE_RESULT;

#endif   /* _oracledrv_h_ */
