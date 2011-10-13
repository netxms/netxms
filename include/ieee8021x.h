/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: ieee8021x.h
**
**/

#ifndef _ieee8021x_h
#define _ieee8021x_h


/**
 * States of PAE state machine
 */
#define PAE_STATE_INITIALIZE     1
#define PAE_STATE_DISCONNECTED   2
#define PAE_STATE_CONNECTING     3
#define PAE_STATE_AUTHENTICATING 4
#define PAE_STATE_AUTHENTICATED  5
#define PAE_STATE_ABORTING       6
#define PAE_STATE_HELD           7
#define PAE_STATE_FORCE_AUTH     8
#define PAE_STATE_FORCE_UNAUTH   9
#define PAE_STATE_RESTART        10

/**
 * States of backend authentication state machine
 */
#define BACKEND_STATE_REQUEST    1
#define BACKEND_STATE_RESPONSE   2
#define BACKEND_STATE_SUCCESS    3
#define BACKEND_STATE_FAIL       4
#define BACKEND_STATE_TIMEOUT    5
#define BACKEND_STATE_IDLE       6
#define BACKEND_STATE_INITIALIZE 7
#define BACKEND_STATE_IGNORE     8

#endif
