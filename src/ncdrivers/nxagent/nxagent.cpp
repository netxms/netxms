/* 
** SMS Driver for sending SMS via NetXMS agent
** Copyright (C) 2007-2023 Raden Solutions
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
   uint16_t m_port;
   TCHAR m_secret[256];
   uint32_t m_timeout;	// Default timeout is 30 seconds
   TCHAR m_keyFile[MAX_PATH];

public:
   NXAgentDriver(Config *config);

   virtual int send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body) override;
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
   GetNetXMSDirectory(nxDirData, m_keyFile);
   _tcslcat(m_keyFile, DFILE_KEYS, MAX_PATH);

   NX_CFG_TEMPLATE configTemplate[] = 
	{
		{ _T("Hostname"), CT_STRING, 0, 0, sizeof(m_hostName) / sizeof(TCHAR), 0, m_hostName },
		{ _T("Port"), CT_WORD, 0, 0, 0, 0, &m_port },
		{ _T("Timeout"), CT_LONG, 0, 0, 0, 0, &m_timeout },
		{ _T("Secret"), CT_STRING, 0, 0, sizeof(m_secret) / sizeof(TCHAR), 0, m_secret },
		{ _T("KeyFile"), CT_STRING, 0, 0, sizeof(m_keyFile) / sizeof(TCHAR), 0, m_keyFile },
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
	};

	config->parseTemplate(_T("NXAgent"), configTemplate);
   m_timeout *= 1000; 
}

/**
 * Send SMS
 */
int NXAgentDriver::send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body)
{
   int result = -1;

   InetAddress addr = InetAddress::resolveHostName(m_hostName);
   if (addr.isValid())
	{
      RSA_KEY serverKey = nullptr;
      bool start = true;
      // Load server key if requested
#ifdef _WITH_ENCRYPTION
      serverKey = RSALoadKey(m_keyFile);
      if (serverKey == nullptr)
      {
         serverKey = RSAGenerateKey(2048);
         if (serverKey == nullptr)
         {
            nxlog_debug_tag(DEBUG_TAG, 2, _T("Cannot load server RSA key from \"%s\" or generate new key"), m_keyFile);
            _tprintf(_T("Cannot load server RSA key from \"%s\" or generate new key\n"), m_keyFile);
            start = false;
         }
      }
#endif

      if (start)
      {
         auto conn = make_shared<AgentConnection>(addr, m_port, m_secret);
         conn->setCommandTimeout(m_timeout);
         conn->setEncryptionPolicy(ENCRYPTION_REQUIRED);
         uint32_t rcc;
         if (conn->connect(serverKey, &rcc))
         {
            StringList list;
            list.add(recipient);
            list.add(body);
            rcc = conn->executeCommand(_T("SMS.Send"), list);
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Agent action execution result: %d (%s)"), rcc, AgentErrorCodeToText(rcc));
            switch (rcc)
            {
               case ERR_SUCCESS:
                  result = 0;
                  break;
               case ERR_REQUEST_TIMEOUT:
               case ERR_NOT_CONNECTED:
               case ERR_CONNECTION_BROKEN:
                  result = 30;
                  break;
            }
         }
         else
         {            
               nxlog_debug_tag(DEBUG_TAG, 2, _T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
         }
      }
	}
   return result;
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(NXAgent, nullptr)
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
