/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: local_admin.h
**
**/

#ifndef _local_admin_h_
#define _local_admin_h_

#define LOCAL_ADMIN_PORT      21784

#define LA_RESP_SUCCESS       0x7777
#define LA_RESP_ERROR         0xFFFF

#define LA_CMD_LIST_CONFIG    1
#define LA_CMD_GET_CONFIG     2
#define LA_CMD_SET_CONFIG     3
#define LA_CMD_STOP           4
#define LA_CMD_GET_FLAGS      5
#define LA_CMD_SET_FLAGS      6


#endif   /* _local_admin_h_ */
