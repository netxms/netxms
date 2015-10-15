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
** File: appagent.cpp
**/

#include "appagent-internal.h"

/**
 * Pipe read timeout
 */
#define PIPE_READ_TIMEOUT  2000

/**
 * Message buffer - seek to message start
 * 
 * @return message length or -1 if message start not found
 */
int AppAgentMessageBuffer::seek()
{
	if (m_pos < APPAGENT_MSG_START_INDICATOR_LEN + APPAGENT_MSG_SIZE_FIELD_LEN)
		return -1;

	for(int i = 0; i < m_pos - APPAGENT_MSG_START_INDICATOR_LEN - APPAGENT_MSG_SIZE_FIELD_LEN; i++)
		if (!memcmp(&m_data[i], APPAGENT_MSG_START_INDICATOR, APPAGENT_MSG_START_INDICATOR_LEN))
		{
			shrink(i);
			WORD len;
			memcpy(&len, &m_data[APPAGENT_MSG_START_INDICATOR_LEN], APPAGENT_MSG_SIZE_FIELD_LEN);
			return (int)len;
		}

	shrink(m_pos - APPAGENT_MSG_START_INDICATOR_LEN - APPAGENT_MSG_SIZE_FIELD_LEN + 1);
	return -1;
}

/**
 * Message buffer - shrink
 *
 * @param pos start position of remaining content
 */
void AppAgentMessageBuffer::shrink(int pos)
{
	if (pos > m_pos)
		pos = m_pos;
	m_pos -= pos;
	memmove(m_data, &m_data[pos], m_pos);
}

/**
 * Receive message from pipe
 */
APPAGENT_MSG *ReadMessageFromPipe(HPIPE hPipe, AppAgentMessageBuffer *mb)
{
	APPAGENT_MSG *msg = NULL;

	int msgLen = mb->seek();
	while((msgLen < 0) || (mb->m_pos < msgLen))
	{
#ifdef _WIN32
		DWORD bytes;
		OVERLAPPED ov;

		memset(&ov, 0, sizeof(OVERLAPPED));
      ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (!ReadFile(hPipe, &mb->m_data[mb->m_pos], AppAgentMessageBuffer::DATA_SIZE - mb->m_pos, NULL, &ov))
		{
			if (GetLastError() != ERROR_IO_PENDING)
         {
            CloseHandle(ov.hEvent);
				return NULL;
         }
		}
      if (WaitForSingleObject(ov.hEvent, PIPE_READ_TIMEOUT) != WAIT_OBJECT_0)
      {
         CancelIo(hPipe);
         CloseHandle(ov.hEvent);
         return NULL;
      }
		if (!GetOverlappedResult(hPipe, &ov, &bytes, TRUE))
		{
         CloseHandle(ov.hEvent);
			return NULL;
		}
      CloseHandle(ov.hEvent);
#else
		int bytes = RecvEx(hPipe, &mb->m_data[mb->m_pos], AppAgentMessageBuffer::DATA_SIZE - mb->m_pos, 0, PIPE_READ_TIMEOUT);
		if (bytes <= 0)
			return NULL;
#endif
		mb->m_pos += (int)bytes;
		if (mb->m_pos == AppAgentMessageBuffer::DATA_SIZE)
		{
			mb->m_pos = 0;
		}
		else
		{
			msgLen = mb->seek();
		}
	}

	if (msgLen > 0)
	{
		msg = (APPAGENT_MSG *)malloc(msgLen);
		memcpy(msg, mb->m_data, msgLen);
		mb->shrink(msgLen);
	}

	return msg;
}

/*
 * Send message to external subagent
 */
bool SendMessageToPipe(HPIPE hPipe, APPAGENT_MSG *msg)
{
	bool success = false;

#ifdef _WIN32
	DWORD bytes = 0;
	if (WriteFile(hPipe, msg, msg->length, &bytes, NULL))
	{
		success = (bytes == msg->length);
	}
#else
	int bytes = SendEx(hPipe, msg, msg->length, 0, NULL); 
	success = (bytes == msg->length);
#endif
	return success;
}

/**
 * Create new message
 *
 * @param command command code
 * @param length payload length
 */
APPAGENT_MSG *NewMessage(int command, int rcc, int length)
{
	APPAGENT_MSG *msg = (APPAGENT_MSG *)malloc(length + APPAGENT_MSG_HEADER_LEN);
	memcpy(msg->prefix, APPAGENT_MSG_START_INDICATOR, APPAGENT_MSG_START_INDICATOR_LEN);
	msg->length = length + APPAGENT_MSG_HEADER_LEN;
	msg->command = (WORD)command;
	msg->rcc = (WORD)rcc;
	return msg;
}
