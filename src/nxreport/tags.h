/*
** NetXMS - Network Management System
** Command line event sender
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: tags.h
**
**/

#ifndef _nx_tags_h_
#define _nx_tags_h_

#include <nms_common.h>
#include <nxclapi.h>
#include <stdarg.h>

#define TAG_CONT_OPEN		1
#define TAG_CONT_CLOSE  	2
#define TAG_NODE_OPEN		3
#define TAG_NODE_CLOSE		4
#define TAG_IFACE_OPEN		5
#define TAG_IFACE_CLOSE		6
#define TAG_DCI_OPEN		7
#define TAG_DCI_CLOSE		8
#define TAG_SERVICE_OPEN	9
#define TAG_SERVICE_CLOSE	901
#define TAG_SUBNET_OPEN		10
#define TAG_SUBNET_CLOSE	11
#define TAG_NETW_OPEN		12
#define TAG_NETW_CLOSE		13
#define TAG_ZONE_OPEN		14
#define TAG_ZONE_CLOSE		15
#define	TAG_SERVROOT_OPEN	16
#define TAG_SERVROOT_CLOSE	17
#define TAG_TEMPL_OPEN		18
#define TAG_TEMPL_CLOSE		19
#define TAG_TEMPLGRP_OPEN	20
#define TAG_TEMPLGRP_CLOSE	21
#define TAG_VPNCONN_OPEN	24
#define TAG_VPNCONN_CLOSE	25
#define TAG_COND_OPEN		26
#define TAG_COND_CLOSE		27
#define TAG_GENERIC		28
#define TAG_DUMMY_BEGIN		9998
#define TAG_DUMMY_END		9999
	

void print_tag(int nType, ... );

#endif
