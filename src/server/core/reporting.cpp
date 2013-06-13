/* 
** NetXMS - Network Management System
** Copyright (C) 2013 Alex Kirhenshtein
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
** File: reporting.cpp
**
**/

#include "nxcore.h"

class RSConnector : public ISC
{
public:
   RSConnector(UINT32 addr, WORD port) : ISC(addr, port)
   {
   }

   virtual void PrintMsg(const TCHAR *format, ...)
   {
      va_list args;
      va_start(args, format);
      DbgPrintf2(7, format, args);
      va_end(args);
   }
};

static RSConnector *m_connector = NULL;

THREAD_RESULT THREAD_CALL ReportingServerConnector(void *arg)
{
	TCHAR hostname[256];
	ConfigReadStr(_T("ReportingServerHostname"), hostname, 256, _T("localhost"));
   WORD port = (WORD)ConfigReadInt(_T("ReportingServerPort"), 4710);

	DbgPrintf(1, _T("Reporting Server connector started (%s:%d)"), hostname, port);

   // Keep connection open
   m_connector = new RSConnector(ResolveHostName(hostname), port);
   while(!IsShutdownInProgress())
   {
      if (m_connector->Nop() == ISC_ERR_SUCCESS)
      {
         ThreadSleep(1);
      }
      else
      {
         if (m_connector->Connect(0) == ISC_ERR_SUCCESS)
         {
            DbgPrintf(6, _T("Connection to Reporting Server restored"));
         }
         else
         {
            ThreadSleep(1);
         }
      }
	}
   m_connector->Disconnect();
	delete m_connector;
   m_connector = NULL;

	DbgPrintf(1, _T("Reporting Server connector stopped"));
   return THREAD_OK;
}

CSCPMessage *ForwardMessageToReportingServer(CSCPMessage *request)
{
   CSCPMessage *reply = NULL;

   UINT32 originalId = request->GetId();

   if (m_connector != NULL)
   {
      request->SetId(0); // force ISC to generate unique ID
      if (m_connector->SendMessage(request))
      {
         reply = m_connector->WaitForMessage(CMD_REQUEST_COMPLETED, request->GetId(), 10000);
      }
   }

   if (reply != NULL)
   {
      reply->SetId(originalId);
   }

   return reply;
}
