/* 
** NetXMS - Network Management System
** SMS driver for Portech MV-37x VoIP GSM gateways
** Copyright (C) 2011 Victor Kirhenshtein
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

#include <nxsrvapi.h>


//
// Functions
//

SOCKET NetConnectTCP(const TCHAR *hostName, unsigned short nPort, DWORD dwTimeout);
void NetClose(SOCKET nSocket);
bool NetWaitForText(SOCKET nSocket, const char *text, int timeout);
int NetWrite(SOCKET nSocket, const char *pBuff, int nSize);
bool NetWriteLine(SOCKET nSocket, const char *line);

bool SMSCreatePDUString(const char* phoneNumber, const char* message, char* pduBuffer, const int pduBufferSize);

#endif
