/* 
** SMS Driver for sending SMS via NetXMS agent
** Copyright (C) 2007-2010 Victor Kirhenshtein
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
**
**/

#include <nms_common.h>
#include <nxsrvapi.h>

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif


//
// Static data
//

static TCHAR m_szHostName[256] = _T("localhost");
static WORD m_wPort = 4700;
static TCHAR m_szSecret[256] = _T("");
static DWORD m_dwTimeout = 30000;	// Default timeout is 30 seconds


//
// Initialize driver
// pszInitArgs format: hostname,port,timeout,secret
//

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
				nx_strncpy(m_szHostName, ptr, 256);
            break;
         case 1:  // Port
            m_wPort = (WORD)_tcstoul(ptr, NULL, 0);
            break;
         case 2:  // Timeout
            m_dwTimeout = _tcstoul(ptr, NULL, 0) * 1000;
            break;
         case 3:  // Secret
				nx_strncpy(m_szSecret, ptr, 256);
            break;
         default:
            break;
      }
   }
	free(temp);
	return TRUE;
}

extern "C" BOOL EXPORT SMSDriverSend(const TCHAR *pszPhoneNumber, const TCHAR *pszText)
{
	DWORD dwAddr;
	BOOL bSuccess = FALSE;

   dwAddr = ResolveHostName(m_szHostName);
   if ((dwAddr != INADDR_ANY) && (dwAddr != INADDR_NONE))
	{
      AgentConnection conn(dwAddr, m_wPort, (m_szSecret[0] != 0) ? AUTH_SHA1_HASH : AUTH_NONE, m_szSecret);

		conn.setCommandTimeout(m_dwTimeout);
		//conn.SetEncryptionPolicy(iEncryptionPolicy);

      if (conn.connect())
      {
			const TCHAR *argv[2];

			argv[0] = pszPhoneNumber;
			argv[1] = pszText;
         if (conn.ExecAction(_T("SMS.Send"), 2, argv) == ERR_SUCCESS)
				bSuccess = TRUE;
			conn.disconnect();
		}
	}
	return bSuccess;
}

extern "C" void EXPORT SMSDriverUnload()
{
}


//
// DLL Entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
