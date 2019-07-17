/* 
** NetXMS - Network Management System
** SMS driver for Portech MV-37x VoIP GSM gateways
** Copyright (C) 2011-2019 Raden Solutions
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
** File: portech.h
**
**/

#ifndef _portech_h_
#define _portech_h_

#define INCLUDE_LIBNXSRV_MESSAGES

#include <ncdrv.h>
#include <nms_util.h>

#define DEBUG_TAG _T("ncd.portech")

//
// Functions
//

bool SMSCreatePDUString(const char* phoneNumber, const char* message, char* pduBuffer, const int pduBufferSize);

#endif
