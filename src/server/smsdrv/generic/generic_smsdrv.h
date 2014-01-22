/* 
** NetXMS - Network Management System
** Generic SMS driver
** Copyright (C) 2003-2014 Raden Solutions
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
** File: generic_smsdrv.h
**
**/

#ifndef __GENERIC_SMSDRV_H__
#define __GENERIC_SMSDRV_H__

#define INCLUDE_LIBNXSRV_MESSAGES

#include <nms_util.h>
#include <nxsrvapi.h>

#ifdef _WIN32

#define EXPORT __declspec(dllexport)

#else

#define EXPORT

#endif // _WIN32

#define PDU_BUFFER_SIZE    512

#endif // __GENERIC_SMSDRV_H__
