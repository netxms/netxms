/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Raden Solutions
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
** File: nxtux.h
**
**/

#ifndef _nxtux_h_
#define _nxtux_h_

#ifdef _WIN32
#ifdef LIBNXTUX_EXPORTS
#define LIBNXTUX_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXTUX_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXTUX_EXPORTABLE
#endif

#include <nms_common.h>
#include <nms_util.h>

#undef getopt
#include <atmi.h>
#include <fml32.h>

bool LIBNXTUX_EXPORTABLE TuxedoConnect();
void LIBNXTUX_EXPORTABLE TuxedoDisconnect();

bool LIBNXTUX_EXPORTABLE CFgetString(FBFR32 *fb, FLDID32 fieldid, FLDOCC32 oc, char *buf, size_t size);

#endif   /* _nms_agent_h_ */
