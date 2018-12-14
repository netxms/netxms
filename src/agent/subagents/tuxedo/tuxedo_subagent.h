/*
** NetXMS Tuxedo subagent
** Copyright (C) 2014-2018 Raden Solutions
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
** File: tuxedo_subagent.h
**
**/

#ifndef _tuxedo_subagent_h_
#define _tuxedo_subagent_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <nxtux.h>
#include <tpadm.h>

#define TUXEDO_DEBUG_TAG   _T("sa.tuxedo")

bool TuxedoGetMachinePhysicalID(const TCHAR *lmid, char *pmid);

#define LOCAL_DATA_MACHINES   0x01
#define LOCAL_DATA_QUEUES     0x02
#define LOCAL_DATA_SERVERS    0x04

extern UINT32 g_tuxedoQueryLocalData;

#endif
