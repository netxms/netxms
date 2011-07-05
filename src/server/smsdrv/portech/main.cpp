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
** File: main.cpp
**
**/

#include "portech.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif


//
// Static data
//

static TCHAR s_hostName[MAX_PATH] = _T("10.0.0.1");
static char s_login[MAX_PATH] = "admin";
static char s_password[MAX_PATH] = "admin";
static enum { OM_TEXT, OM_PDU } s_mode = OM_PDU;


//
// Initialize driver
// option string format: option1=value1;option2=value2;...
// Valid options are: host, login, password, mode
//

extern "C" BOOL EXPORT SMSDriverInit(const TCHAR *options)
{
	DbgPrintf(1, _T("Loading Portech MV-72x SMS Driver (configuration: %s)"), options);

	// Parse arguments
	ExtractNamedOptionValue(options, _T("host"), s_hostName, MAX_PATH);

#ifdef UNICODE
	WCHAR temp[MAX_PATH];
	ExtractNamedOptionValue(options, _T("login"), temp, MAX_PATH);
	WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, temp, -1, s_login, MAX_PATH, NULL, NULL);
#else
	ExtractNamedOptionValue(options, _T("login"), s_login, MAX_PATH);
#endif

#ifdef UNICODE
	ExtractNamedOptionValue(options, _T("password"), temp, MAX_PATH);
	WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, temp, -1, s_password, MAX_PATH, NULL, NULL);
#else
	ExtractNamedOptionValue(options, _T("password"), s_password, MAX_PATH);
#endif

	TCHAR mode[256] = _T("");
	if (ExtractNamedOptionValue(options, _T("mode"), mode, 256))
	{
		if (!_tcsicmp(mode, _T("PDU")))
			s_mode = OM_PDU;
		else if (!_tcsicmp(mode, _T("TEXT")))
			s_mode = OM_TEXT;
		else
		{
			nxlog_write(MSG_SMSDRV_INVALID_OPTION, NXLOG_ERROR, "s", _T("mode"));
			return FALSE;
		}
	}
	
	return TRUE;
}


//
// Login to unit
//

static bool DoLogin(SOCKET nSd)
{
	if (!NetWaitForText(nSd, "username: ", 2000))
		return false;

	if (!NetWriteLine(nSd, s_login))
		return false;

	if (!NetWaitForText(nSd, "password: ", 2000))
		return false;

	if (!NetWriteLine(nSd, s_password))
		return false;

	if (!NetWaitForText(nSd, "]", 2000))
		return false;

	if (!NetWriteLine(nSd, "module"))
		return false;

	if (!NetWaitForText(nSd, "got!! press 'ctrl-x' to release module", 2000))
		return false;

	return true;
}


//
// Logout from unit
//

static void DoLogout(SOCKET nSd)
{
	if (NetWrite(nSd, "\x18", 1) > 0)
	{
		if (NetWaitForText(nSd, "]", 5000))
			NetWriteLine(nSd, "logout");
	}
}


//
// Send SMS in text mode
//

#define __chk(x) if (!(x)) { return FALSE; }

static BOOL SendText(SOCKET nSd, const TCHAR *pszPhoneNumber, const TCHAR *pszText)
{
	char szTmp[128];
	
	DbgPrintf(3, _T("Sending SMS (text mode): rcpt=\"%s\" text=\"%s\""), pszPhoneNumber, pszText);
	
	__chk(NetWriteLine(nSd, "ATZ"));	// init modem
	__chk(NetWaitForText(nSd, "OK", 2000));
	DbgPrintf(4, _T("SMS: ATZ sent"));

	ThreadSleep(1);
	__chk(NetWrite(nSd, "\r\n", 2));		// empty string, otherwise ATE0 may fail
	__chk(NetWriteLine(nSd, "ATE0"));	// disable echo
	__chk(NetWaitForText(nSd, "OK", 2000));
	DbgPrintf(4, _T("SMS: ATE0 sent"));

	ThreadSleep(1);
	__chk(NetWriteLine(nSd, "AT+CMGF=1"));	// text mode
	__chk(NetWaitForText(nSd, "OK", 2000));
	DbgPrintf(4, _T("SMS: AT+CMGF=1 sent"));

#ifdef UNICODE
	char mbPhoneNumber[256];
	WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, pszPhoneNumber, -1, mbPhoneNumber, 256, NULL, NULL);
	mbPhoneNumber[255] = 0;
	snprintf(szTmp, sizeof(szTmp), "AT+CMGS=\"%s\"\r\n", mbPhoneNumber);
#else
	snprintf(szTmp, sizeof(szTmp), "AT+CMGS=\"%s\"\r\n", pszPhoneNumber);
#endif
	__chk(NetWriteLine(nSd, szTmp));	// set number
	__chk(NetWaitForText(nSd, ">", 2000));

#ifdef UNICODE
	char mbText[161];
	WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, pszText, -1, mbText, 161, NULL, NULL);
	mbText[160] = 0;
	snprintf(szTmp, sizeof(szTmp), "%s%c\r\n", mbText, 0x1A);
#else
	snprintf(szTmp, sizeof(szTmp), "%s%c\r\n", pszText, 0x1A);
#endif
	__chk(NetWrite(nSd, szTmp, (int)strlen(szTmp)) > 0); // send text, end with ^Z
	__chk(NetWaitForText(nSd, "+CMGS", 5000));
	__chk(NetWaitForText(nSd, "OK", 2000));
	DbgPrintf(4, _T("SMS sent successfully"));

	return TRUE;
}


//
// Send SMS in PDU mode
//

static BOOL SendPDU(SOCKET nSd, const TCHAR *pszPhoneNumber, const TCHAR *pszText)
{
	const int bufferSize = 512;
	char szTmp[bufferSize];
	char phoneNumber[bufferSize], text[bufferSize];

	DbgPrintf(3, _T("Sending SMS (PDU mode): rcpt=\"%s\" text=\"%s\""), pszPhoneNumber, pszText);

#ifdef UNICODE
	if (WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, pszPhoneNumber, -1, phoneNumber, bufferSize, NULL, NULL) == 0 ||
	    WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, pszText, -1, text, bufferSize, NULL, NULL) == 0)
	{
		DbgPrintf(2, _T("SMS: Failed to convert phone number or text to multibyte string"));
		return FALSE;
	}
#else
	nx_strncpy(phoneNumber, pszPhoneNumber, bufferSize);
	nx_strncpy(text, pszText, bufferSize);
#endif

	__chk(NetWriteLine(nSd, "ATZ"));	// init modem
	__chk(NetWaitForText(nSd, "OK", 2000));
	DbgPrintf(4, _T("SMS: ATZ sent"));

	ThreadSleep(1);
	__chk(NetWrite(nSd, "\r\n", 2));		// empty string, otherwise ATE0 may fail
	__chk(NetWriteLine(nSd, "ATE0"));	// disable echo
	__chk(NetWaitForText(nSd, "OK", 2000));
	DbgPrintf(4, _T("SMS: ATE0 sent"));

	ThreadSleep(1);
	__chk(NetWriteLine(nSd, "AT+CMGF=0"));	// text mode
	__chk(NetWaitForText(nSd, "OK", 2000));
	DbgPrintf(4, _T("SMS: AT+CMGF=0 sent"));

	char pduBuffer[bufferSize];
	SMSCreatePDUString(phoneNumber, text, pduBuffer, bufferSize);

	snprintf(szTmp, sizeof(szTmp), "AT+CMGS=%d\r\n", strlen(pduBuffer) / 2 - 1);
	__chk(NetWrite(nSd, szTmp, (int)strlen(szTmp)) > 0);
	DbgPrintf(4, _T("SMS: %hs sent"), szTmp);

	snprintf(szTmp, sizeof(szTmp), "%s%c\r\n", pduBuffer, 0x1A);
	__chk(NetWrite(nSd, szTmp, (int)strlen(szTmp)) > 0);
	__chk(NetWaitForText(nSd, "+CMGS", 10000));
	__chk(NetWaitForText(nSd, "OK", 2000));
	DbgPrintf(4, _T("SMS sent successfully"));

	return TRUE;
}


//
// Send SMS
//

extern "C" BOOL EXPORT SMSDriverSend(const TCHAR *pszPhoneNumber, const TCHAR *pszText)
{
	SOCKET nSd;
	BOOL success = FALSE;

	if ((pszPhoneNumber == NULL) || (pszText == NULL))
		return FALSE;

	nSd = NetConnectTCP(s_hostName, 23, 10000);
	if (nSd != INVALID_SOCKET)
	{
		if (DoLogin(nSd))
		{
			success = (s_mode == OM_PDU) ? SendPDU(nSd, pszPhoneNumber, pszText) : SendText(nSd, pszPhoneNumber, pszText);
		}
		DoLogout(nSd);
		NetClose(nSd);
	}
	return success;
}


//
// Unload SMS driver
//

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
