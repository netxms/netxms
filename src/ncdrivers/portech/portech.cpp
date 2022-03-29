/* 
** NetXMS - Network Management System
** SMS driver for Portech MV-37x VoIP GSM gateways
** Copyright (C) 2011-2021 Raden Solutions
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
** File: portech.cpp
**
**/

#include "portech.h"

/**
 * Static data
 */
static const TCHAR *s_hostName = _T("10.0.0.1");

enum Mode
{ 
	OM_TEXT, 
	OM_PDU 
};

/**
 * Portech driver class
 */
class PortechDriver : public NCDriver
{
private:
	TCHAR m_primaryHostName[MAX_PATH];
	TCHAR m_secondaryHostName[MAX_PATH];
	char m_login[MAX_PATH];
	char m_password[MAX_PATH];
	Mode s_mode;

   bool DoLogin(SocketConnection *conn);

public:
   PortechDriver(Config *config);
   virtual int send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body) override;
};


/**
 * Initialize driver
 */
PortechDriver::PortechDriver(Config *config)
{
	_tcscpy(m_primaryHostName, _T("10.0.0.1"));
	_tcscpy(m_secondaryHostName, _T(""));
	strcpy(m_login, "admin");
	strcpy(m_password, "admin");
	s_mode = OM_PDU;

	nxlog_debug_tag(DEBUG_TAG, 8, _T("Loading Portech MV-72x SMS Driver (configuration: %s)"), (const TCHAR *)config->createXml());

	TCHAR mode[8] = _T("");
    NX_CFG_TEMPLATE configTemplate[] = 
      {
         { _T("host"), CT_STRING, 0, 0, MAX_PATH, 0, m_primaryHostName },	
         { _T("secondaryHost"), CT_STRING, 0, 0, MAX_PATH, 0, m_secondaryHostName },	
         { _T("login"), CT_MB_STRING, 0, 0, MAX_PATH, 0, m_login  },	
         { _T("password"), CT_MB_STRING, 0, 0, MAX_PATH, 0, m_password  },	
         { _T("mode"), CT_STRING, 0, 0, sizeof(mode)/sizeof(TCHAR), 0, mode },
         { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
      };

	config->parseTemplate(_T("Portech"), configTemplate);

   if (!_tcsicmp(mode, _T("PDU")))
      s_mode = OM_PDU;
   else if (!_tcsicmp(mode, _T("TEXT")))
      s_mode = OM_TEXT;
   else
      nxlog_debug_tag(DEBUG_TAG, 0, _T("Invalid sending mode \"%s\""), mode);
}

/**
 * Login to unit
 */
bool PortechDriver::DoLogin(SocketConnection *conn)
{
	if (!conn->waitForText("username: ", 5000))
		return false;

	if (!conn->writeLine(m_login))
		return false;

	if (!conn->waitForText("password: ", 5000))
		return false;

	if (!conn->writeLine(m_password))
		return false;

	if (!conn->waitForText("]", 5000))
		return false;

	if (!conn->writeLine("module"))
		return false;

	if (!conn->waitForText("got!! press 'ctrl-x' to release module", 2000))
		return false;

	return true;
}

/**
 * Logout from unit
 */
static void DoLogout(SocketConnection *conn)
{
	if (conn->write("\x18", 1) > 0)
	{
		if (conn->waitForText("]", 5000))
			conn->writeLine("logout");
	}
}

/**
 * Return immediately if given operation failed
 */
#define __chk(x) if (!(x)) { return false; }

/**
 * Send SMS in text mode
 */
static bool SendText(SocketConnection *conn, const TCHAR *recipient, const TCHAR *body)
{
	char szTmp[128];
	
	nxlog_debug_tag(DEBUG_TAG, 3, _T("Text mode:rcpt=\"%s\" text=\"%s\""), recipient, body);

   // Older versions responded with OK, newer with 0
   const char *okText;
	__chk(conn->writeLine("ATZ"));	// init modem
   if (conn->waitForText("0\r\n", 10000))
   {
      okText = "0\r\n";
   }
   else
   {
      __chk(conn->writeLine("ATZ"));	// init modem
      __chk(conn->waitForText("OK\r\n", 10000));
      okText = "OK\r\n";
   }
	nxlog_debug_tag(DEBUG_TAG, 4, _T("ATZ sent"));

	ThreadSleep(1);
	__chk(conn->writeLine("ATE0"));	// disable echo
	__chk(conn->waitForText(okText, 10000));
	nxlog_debug_tag(DEBUG_TAG, 4, _T("ATE0 sent"));

	ThreadSleep(1);
	__chk(conn->writeLine("AT+CMGF=1"));	// text mode
	__chk(conn->waitForText(okText, 5000));
	nxlog_debug_tag(DEBUG_TAG, 4, _T("AT+CMGF=1 sent"));

   ThreadSleep(1);
#ifdef UNICODE
	char mbPhoneNumber[256];
	wchar_to_ASCII(recipient, -1, mbPhoneNumber, 256);
	mbPhoneNumber[255] = 0;
	snprintf(szTmp, sizeof(szTmp), "AT+CMGS=\"%s\"\r\n", mbPhoneNumber);
#else
	snprintf(szTmp, sizeof(szTmp), "AT+CMGS=\"%s\"\r\n", recipient);
#endif
	__chk(conn->writeLine(szTmp));	// set number
	__chk(conn->waitForText(">", 10000));

#ifdef UNICODE
	char mbText[161];
	wchar_to_utf8(body, -1, mbText, 161);
	mbText[160] = 0;
   __chk(conn->writeLine(mbText));
#else
   __chk(conn->writeLine(body));
#endif
   snprintf(szTmp, sizeof(szTmp), "%c\r\n", 0x1A);
   __chk(conn->write(szTmp, (int)strlen(szTmp)) > 0); // send text, end with ^Z
	__chk(conn->waitForText("+CMGS", 45000));
	__chk(conn->waitForText(okText, 5000));
	nxlog_debug_tag(DEBUG_TAG, 4, _T("SMS sent successfully"));

	return true;
}

/**
 * Send SMS in PDU mode
 */
static bool SendPDU(SocketConnection *conn, const TCHAR *recipient, const TCHAR *body)
{
	const int bufferSize = 512;
	char szTmp[bufferSize];
	char phoneNumber[bufferSize], text[bufferSize];

	nxlog_debug_tag(DEBUG_TAG, 3, _T("PDU mode: rcpt=\"%s\" text=\"%s\""), recipient, body);

#ifdef UNICODE
	if (wchar_to_ASCII(recipient, -1, phoneNumber, bufferSize) == 0 ||
	    wchar_to_utf8(body, -1, text, bufferSize) == 0)
	{
		nxlog_debug_tag(DEBUG_TAG, 2, _T("Failed to convert phone number or text to multibyte string"));
		return false;
	}
#else
	strlcpy(phoneNumber, recipient, bufferSize);
	strlcpy(text, body, bufferSize);
#endif

   // Older versions responded with OK, newer with 0
   const char *okText;
   __chk(conn->writeLine("ATZ"));	// init modem
   if (conn->waitForText("0\r\n", 10000))
   {
      okText = "0\r\n";
   }
   else
   {
      __chk(conn->writeLine("ATZ"));	// init modem
      __chk(conn->waitForText("OK\r\n", 10000));
      okText = "OK\r\n";
   }
   nxlog_debug_tag(DEBUG_TAG, 4, _T("ATZ sent"));

	ThreadSleep(1);
	__chk(conn->writeLine("ATE0"));	// disable echo
	__chk(conn->waitForText(okText, 10000));
	nxlog_debug_tag(DEBUG_TAG, 4, _T("ATE0 sent"));

	ThreadSleep(1);
	__chk(conn->writeLine("AT+CMGF=0"));	// PDU mode
	__chk(conn->waitForText(okText, 10000));
	nxlog_debug_tag(DEBUG_TAG, 4, _T("AT+CMGF=0 sent"));

	char pduBuffer[bufferSize];
	SMSCreatePDUString(phoneNumber, text, pduBuffer, bufferSize);

   ThreadSleep(1);
   snprintf(szTmp, sizeof(szTmp), "AT+CMGS=%d\r\n", (int)strlen(pduBuffer) / 2 - 1);
	__chk(conn->write(szTmp, (int)strlen(szTmp)) > 0);
	__chk(conn->waitForText(">", 10000));
	nxlog_debug_tag(DEBUG_TAG, 4, _T("%hs sent"), szTmp);

	snprintf(szTmp, sizeof(szTmp), "%s%c\r\n", pduBuffer, 0x1A);
	__chk(conn->write(szTmp, (int)strlen(szTmp)) > 0);
	__chk(conn->waitForText("+CMGS", 45000));
	__chk(conn->waitForText(okText, 2000));
	nxlog_debug_tag(DEBUG_TAG, 4, _T("SMS sent successfully"));

	return true;
}

/**
 * Send SMS
 */
int PortechDriver::send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body)
{
	SocketConnection *conn;
   int result = -1;

   if ((recipient == NULL) || (body == NULL))
      return -1;

   bool canRetry = true;

retry:
   conn = SocketConnection::createTCPConnection(s_hostName, 23, 10000);
   if (conn != NULL)
   {
      conn->write("\xFF\xFE\x01\xFF\xFC\x01", 6);
      if (DoLogin(conn))
      {
         bool success = (s_mode == OM_PDU) ? SendPDU(conn, recipient, body) : SendText(conn, recipient, body);
         result = success ? 0 : -1;
      }
      DoLogout(conn);
      conn->disconnect();
      delete conn;
   }

   if (result != 0 && canRetry)
   {
      const TCHAR *newName = (s_hostName == m_primaryHostName) ? m_secondaryHostName : m_primaryHostName;
      if (newName[0] != 0)
      {
         s_hostName = newName;
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Switched to host %s"), s_hostName);
         canRetry = false;
         goto retry;
      }
   }

   return result;
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(Portech, NULL)
{
   return new PortechDriver(config);
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
