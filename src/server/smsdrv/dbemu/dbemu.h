/* 
** NetXMS - Network Management System
** Copyright (C) 2011 NetXMS Team
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
** File: dbemu.h
**
**/

#ifndef __DBEMU_H__
#define __DBEMU_H__

#include <nms_common.h>
#include <nxsrvapi.h>
#include <nxdbapi.h>

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#define MAX_DBPARAM_LEN		(31)
#define MAX_STR				(256)
#define MAX_SQL				(1024)
#define MYNAMESTR			(_T("SMSDBEmu"))

#endif