/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: libnxjava.h
**
**/

#ifndef _libnxjava_h_
#define _libnxjava_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxjava.h>

bool RegisterConfigHelperNatives(JNIEnv *env);
bool RegisterPlatformNatives(JNIEnv *env);

#endif   /* _libnxjava_h_ */
