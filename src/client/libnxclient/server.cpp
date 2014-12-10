/*
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: server.cpp
**
**/

#include "libnxclient.h"

/**
 * Send SMS from server
 */
UINT32 ServerController::sendSMS(const TCHAR *recipient, const TCHAR *text)
{
   NXCPMessage msg;

   msg.setCode(CMD_SEND_SMS);
   msg.setId(m_session->createMessageId());
   msg.setField(VID_RCPT_ADDR, recipient);
   msg.setField(VID_MESSAGE, text);

   if (!m_session->sendMessage(&msg))
      return RCC_COMM_FAILURE;
   return m_session->waitForRCC(msg.getId());
}
