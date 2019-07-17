/* 
** SMS Driver for sending SMS via NetXMS agent
** Copyright (C) 2007-2019 Raden Solutions
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
** File: nxagent.cpp
**/

#include <ncdrv.h>
#include <nxsrvapi.h>

#define DEBUG_TAG _T("ncd.nxagent")

/**
 * NXAgent driver class
 */
class NXAgentDriver : public NCDriver
{
private:
   TCHAR m_hostName[256];
   UINT16 m_port;
   TCHAR m_secret[256];
   UINT32 m_timeout;	// Default timeout is 30 seconds

public:
   NXAgentDriver(Config *config);
   virtual bool send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body) override;
};

/**
 * Initialize driver
 */
NXAgentDriver::NXAgentDriver(Config *config)
{
   _tcscpy(m_hostName, _T("localhost"));
   _tcscpy(m_secret, _T(""));
   m_port = 4700;
   m_timeout = 30;	

   static NX_CFG_TEMPLATE configTemplate[] = 
	{
		{ _T("hostname"), CT_STRING, 0, 0, sizeof(m_hostName), 0, m_hostName },	
		{ _T("port"), CT_LONG, 0, 0, 0, 0, &m_port },	
		{ _T("timeout"), CT_LONG, 0, 0, 0, 0,	&m_timeout },	
		{ _T("secret"), CT_STRING, 0, 0, sizeof(m_secret), 0,	m_secret },
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
	};

	config->parseTemplate(_T("NXAgent"), configTemplate);
   m_timeout *= 1000; 
}

/**
 * Send SMS
 */
bool NXAgentDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
	bool bSuccess = false;

   InetAddress addr = InetAddress::resolveHostName(m_hostName);
   if (addr.isValid())
	{
      AgentConnection *conn = new AgentConnection(addr, m_port, (m_secret[0] != 0) ? AUTH_SHA1_HASH : AUTH_NONE, m_secret);
		conn->setCommandTimeout(m_timeout);
      if (conn->connect())
      {
			StringList list;
			list.add(recipient);
         list.add(body);
			UINT32 rcc = conn->execAction(_T("SMS.Send"), list);
			nxlog_debug_tag(DEBUG_TAG, 4, _T("Agent action execution result: %d (%s)"), rcc, AgentErrorCodeToText(rcc));
         if (rcc == ERR_SUCCESS)
				bSuccess = true;
		}
      conn->decRefCount();
	}
	return bSuccess;
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(NXAgent, NULL)
{
   return new NXAgentDriver(config);
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
