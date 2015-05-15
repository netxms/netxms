/* 
** SMS Driver for sending SMS via NetXMS agent
** Copyright (C) 2007-2015 Victor Kirhenshtein
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
** File: main.cpp
**/

#include <nms_common.h>
#include <nxsrvapi.h>

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

/**
 * Configuration
 */
static TCHAR m_hostName[256] = _T("localhost");
static UINT16 m_port = 4700;
static TCHAR m_secret[256] = _T("");
static UINT32 m_timeout = 30000;	// Default timeout is 30 seconds

/**
 * Initialize driver
 * pszInitArgs format: hostname,port,timeout,secret
 */
extern "C" BOOL EXPORT SMSDriverInit(const TCHAR *pszInitArgs)
{
	TCHAR *temp, *ptr, *eptr;
	int field;

	temp = _tcsdup(pszInitArgs);
   for(ptr = eptr = temp, field = 0; eptr != NULL; field++, ptr = eptr + 1)
   {
		eptr = _tcschr(ptr, ',');
		if (eptr != NULL)
			*eptr = 0;
      switch(field)
      {
         case 0:  // Host name
				nx_strncpy(m_hostName, ptr, 256);
            break;
         case 1:  // Port
            m_port = (WORD)_tcstoul(ptr, NULL, 0);
            break;
         case 2:  // Timeout
            m_timeout = _tcstoul(ptr, NULL, 0) * 1000;
            break;
         case 3:  // Secret
				nx_strncpy(m_secret, ptr, 256);
            break;
         default:
            break;
      }
   }
	free(temp);
	return TRUE;
}

/**
 * Send SMS
 */
extern "C" BOOL EXPORT SMSDriverSend(const TCHAR *pszPhoneNumber, const TCHAR *pszText)
{
	BOOL bSuccess = FALSE;

   InetAddress addr = InetAddress::resolveHostName(m_hostName);
   if (addr.isValid())
	{
      AgentConnection conn(addr, m_port, (m_secret[0] != 0) ? AUTH_SHA1_HASH : AUTH_NONE, m_secret);

		conn.setCommandTimeout(m_timeout);
      if (conn.connect())
      {
			TCHAR *argv[2];

			argv[0] = (TCHAR *)pszPhoneNumber;
			argv[1] = (TCHAR *)pszText;
         if (conn.execAction(_T("SMS.Send"), 2, argv) == ERR_SUCCESS)
				bSuccess = TRUE;
			conn.disconnect();
		}
	}
	return bSuccess;
}

/**
 * Unload driver
 */
extern "C" void EXPORT SMSDriverUnload()
{
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
