/*
** NetXMS subagent for Novell NetWare
** Copyright (C) 2004, 2005 Victor Kirhenshtein
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
** $module: nwagent.h
**
**/

#ifndef _nwagent_h_
#define _nwagent_h_

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_threads.h>
#include <netdb.h>
#include <monitor.h>
#include <nks/plat.h>
#include <netware.h>


//
// Constants
//

#define CPU_HISTORY_SIZE         1200
#define MAX_CPU                  32

#define MEMINFO_PHYSICAL_TOTAL	0
#define MEMINFO_PHYSICAL_FREE    1
#define MEMINFO_PHYSICAL_USED    2


#endif
