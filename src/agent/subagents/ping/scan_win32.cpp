/*
** NetXMS PING subagent
** Copyright (C) 2004-2019 Victor Kirhenshtein
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
** File: scan_win32.cpp
**
**/

#include "ping.h"

/**
 * Scan IP address range and return list of responding addresses
 */
StructArray<InetAddress> *ScanAddressRange(const InetAddress& start, const InetAddress& end, UINT32 timeout)
{
   // TODO: implement Windows version
   return NULL;
}
