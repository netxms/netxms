/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: appagent-internal.h
**
**/

#ifndef _appagent_internal_h_
#define _appagent_internal_h_

#include <appagent.h>

/**
 * Message buffer
 */
class AppAgentMessageBuffer
{
public:
	static const int DATA_SIZE = 65536;

	char m_data[DATA_SIZE];
	int m_pos;

	AppAgentMessageBuffer() { m_pos = 0; }

	int seek();
	void shrink(int pos);
};

/**
 * Internal functions
 */
APPAGENT_MSG *ReadMessageFromPipe(HPIPE hPipe, HANDLE hEvent, AppAgentMessageBuffer *mb);
bool SendMessageToPipe(HPIPE hPipe, APPAGENT_MSG *msg);
APPAGENT_MSG *NewMessage(int command, int rcc, int length);

#endif
