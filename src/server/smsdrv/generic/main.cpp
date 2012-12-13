/* 
** NetXMS - Network Management System
** Generic SMS driver
** Copyright (C) 2003-2010 Alex Kirhenshtein
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

#include "main.h"

bool SMSCreatePDUString(const char* phoneNumber, const char* message, char* pduBuffer, const int pduBufferSize);

static BOOL SMSDriverSendPDU(const TCHAR *pszPhoneNumber, const TCHAR *pszText);

static Serial m_serial;
static enum { OM_TEXT, OM_PDU } omode = OM_PDU;

// pszInitArgs format: portname,speed,databits,parity,stopbits
extern "C" BOOL EXPORT SMSDriverInit(const TCHAR *pszInitArgs)
{
	bool bRet = false;
	TCHAR *portName;
	
	if (pszInitArgs == NULL || *pszInitArgs == 0)
	{
#ifdef _WIN32
		portName = _tcsdup(_T("COM1:"));
#else
		portName = _tcsdup(_T("/dev/ttyS0"));
#endif
	}
	else
	{
		portName = _tcsdup(pszInitArgs);
	}
	
	DbgPrintf(1, _T("Loading Generic SMS Driver (configuration: %s)"), pszInitArgs);
	
	TCHAR *p;
	int portSpeed = 9600;
	int dataBits = 8;
	int parity = NOPARITY;
	int stopBits = ONESTOPBIT;
	
	if ((p = _tcschr(portName, _T(','))) != NULL)
	{
		*p = 0; p++;
		int tmp = _tcstol(p, NULL, 10);
		if (tmp != 0)
		{
			portSpeed = tmp;
			
			if ((p = _tcschr(p, _T(','))) != NULL)
			{
				*p = 0; p++;
				tmp = _tcstol(p, NULL, 10);
				if (tmp >= 5 && tmp <= 8)
				{
					dataBits = tmp;
					
					// parity
					if ((p = _tcschr(p, _T(','))) != NULL)
					{
						*p = 0; p++;
						switch (tolower(*p))
						{
						case _T('n'): // none
							parity = NOPARITY;
							break;
						case _T('o'): // odd
							parity = ODDPARITY;
							break;
						case _T('e'): // even
							parity = EVENPARITY;
							break;
						}
						
						// stop bits
						if ((p = _tcschr(p, _T(','))) != NULL)
						{
							*p = 0; p++;
							
							if (*p == _T('2'))
							{
								stopBits = TWOSTOPBITS;
							}

							// Text or PDU mode
							if ((p = _tcschr(p, _T(','))) != NULL)
							{
								*p = 0; p++;
								if (*p == _T('T'))
									omode = OM_TEXT;
								else if (*p == _T('P'))
									omode = OM_PDU;
							}
						}
					}
				}
			}
		}
	}
	
	switch (parity)
	{
		case ODDPARITY:
			p = (TCHAR *)_T("ODD");
			break;
		case EVENPARITY:
			p = (TCHAR *)_T("EVEN");
			break;
		default:
			p = (TCHAR *)_T("NONE");
			break;
	}
	DbgPrintf(2, _T("SMS init: port={%s}, speed=%d, data=%d, parity=%s, stop=%d"),
	          portName, portSpeed, dataBits, p, stopBits == TWOSTOPBITS ? 2 : 1);
	
	bRet = m_serial.open(portName);
	if (bRet)
	{
		DbgPrintf(4, _T("SMS: port opened"));
		m_serial.setTimeout(1000);
		m_serial.set(portSpeed, dataBits, parity, stopBits);
		
		// enter PIN: AT+CPIN="xxxx"
		// register network: AT+CREG1
		
		char szTmp[128];
		m_serial.write("ATZ\r\n", 5); // init modem && read user prefs
		m_serial.read(szTmp, 128); // read OK
		DbgPrintf(4, _T("SMS init: ATZ sent, got {%hs}"), szTmp);
		m_serial.write("ATE0\r\n", 6); // disable echo
		m_serial.read(szTmp, 128); // read OK
		DbgPrintf(4, _T("SMS init: ATE0 sent, got {%hs}"), szTmp);
		m_serial.write("ATI3\r\n", 6); // read vendor id
		m_serial.read(szTmp, 128); // read version
		DbgPrintf(4, _T("SMS init: ATI3 sent, got {%hs}"), szTmp);
		
		if (stricmp(szTmp, "ERROR") != 0)
		{
			char *sptr, *eptr;
			
			for(sptr = szTmp; (*sptr != 0) && ((*sptr == '\r') || (*sptr == '\n') || (*sptr == ' ') || (*sptr == '\t')); sptr++);
			for(eptr = sptr; (*eptr != 0) && (*eptr != '\r') && (*eptr != '\n'); eptr++);
			*eptr = 0;
#ifdef UNICODE
			WCHAR *wdata = WideStringFromMBString(sptr);
			nxlog_write(MSG_GSM_MODEM_INFO, EVENTLOG_INFORMATION_TYPE, "ss", pszInitArgs, wdata);
			free(wdata);
#else
			nxlog_write(MSG_GSM_MODEM_INFO, EVENTLOG_INFORMATION_TYPE, "ss", pszInitArgs, sptr);
#endif
		}
	}
	else
	{
		DbgPrintf(1, _T("SMS: Unable to open serial port (%s)"), portName);
	}
	
	if (portName != NULL)
	{
		free(portName);
	}
	
	return bRet;
}

extern "C" BOOL EXPORT SMSDriverSend(const TCHAR *pszPhoneNumber, const TCHAR *pszText)
{
	if (omode == OM_PDU)
		return SMSDriverSendPDU(pszPhoneNumber, pszText);

	if (pszPhoneNumber != NULL && pszText != NULL)
	{
		char szTmp[128];
		
		DbgPrintf(3, _T("SMS send: to {%s}: {%s}"), pszPhoneNumber, pszText);
		
		m_serial.write("ATZ\r\n", 5); // init modem && read user prefs
		m_serial.read(szTmp, 128); // read OK
		DbgPrintf(4, _T("SMS send: ATZ sent, got {%hs}"), szTmp);
		m_serial.write("ATE0\r\n", 5); // disable echo
		m_serial.read(szTmp, 128); // read OK
		DbgPrintf(4, _T("SMS send: ATE0 sent, got {%hs}"), szTmp);
		m_serial.write("AT+CMGF=1\r\n", 11); // =1 - text message
		m_serial.read(szTmp, 128); // read OK
		DbgPrintf(4, _T("SMS send: AT+CMGF=1 sent, got {%hs}"), szTmp);
#ifdef UNICODE
		char mbPhoneNumber[256];
		WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, pszPhoneNumber, -1, mbPhoneNumber, 256, NULL, NULL);
		mbPhoneNumber[255] = 0;
		snprintf(szTmp, sizeof(szTmp), "AT+CMGS=\"%s\"\r\n", mbPhoneNumber);
#else
		snprintf(szTmp, sizeof(szTmp), "AT+CMGS=\"%s\"\r\n", pszPhoneNumber);
#endif
		m_serial.write(szTmp, (int)strlen(szTmp)); // set number
#ifdef UNICODE
		char mbText[161];
		WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, pszText, -1, mbText, 161, NULL, NULL);
		mbText[160] = 0;
		snprintf(szTmp, sizeof(szTmp), "%s%c\r\n", mbText, 0x1A);
#else
		snprintf(szTmp, sizeof(szTmp), "%s%c\r\n", pszText, 0x1A);
#endif
		m_serial.write(szTmp, (int)strlen(szTmp)); // send text, end with ^Z
		m_serial.read(szTmp, 128); // read +CMGS:ref_num
		DbgPrintf(4, _T("SMS send: AT+CMGS + message body sent, got {%hs}"), szTmp);
	}
	
	return true;
}

static BOOL SMSDriverSendPDU(const TCHAR *pszPhoneNumber, const TCHAR *pszText)
{
	if (pszPhoneNumber != NULL && pszText != NULL)
	{
		const int bufferSize = 512;
		char szTmp[bufferSize];
		char phoneNumber[bufferSize], text[bufferSize];

		DbgPrintf(3, _T("SMS send: to {%s}: {%s}"), pszPhoneNumber, pszText);

#ifdef UNICODE
		if (WideCharToMultiByte(CP_ACP, 0, pszPhoneNumber, -1, phoneNumber, bufferSize, NULL, NULL) == 0 ||
			WideCharToMultiByte(CP_ACP, 0, pszText, -1, text, bufferSize, NULL, NULL) == 0 )
		{
			DbgPrintf(2, _T("Failed to convert phone number or text to multibyte string"));
			return FALSE;
		}
#else
		nx_strncpy(phoneNumber, pszPhoneNumber, bufferSize);
		nx_strncpy(text, pszText, bufferSize);
#endif

		m_serial.write("ATZ\r\n", 5); // init modem && read user prefs
		m_serial.read(szTmp, 128); // read OK
		DbgPrintf(4, _T("SMS send: ATZ sent, got {%hs}"), szTmp);
		m_serial.write("ATE0\r\n", 6); // disable echo
		m_serial.read(szTmp, 128); // read OK
		DbgPrintf(4, _T("SMS send: ATE0 sent, got {%hs}"), szTmp);
		m_serial.write("AT+CMGF=0\r\n", 11); // =0 - send text in PDU mode
		m_serial.read(szTmp, 128); // read OK
		DbgPrintf(4, _T("SMS send: AT+CMGF=0 sent, got {%hs}"), szTmp);

		m_serial.write("AT+CSCA?\r\n", 10); // send SMSC query
		m_serial.read(szTmp, 128); // read OK
		DbgPrintf(4, _T("SMS send: AT+CSCA? sent, got {%hs}"), szTmp);
		if (strlen(szTmp) > 10 && !strncmp(szTmp, "\r\n+CSCA: ", 9))
		{
			char szTmp2[128];
			snprintf(szTmp2, 128, "AT+CSCA=%s\r\n", szTmp + 9);
			szTmp2[127] = '\0';
			m_serial.write(szTmp2, (int)strlen(szTmp2)); // set SMSC
			m_serial.read(szTmp, 128); // read OK
			DbgPrintf(4, _T("SMS send: %hs sent, got {%hs}"), szTmp2, szTmp);
		}

		char pduBuffer[bufferSize];
		SMSCreatePDUString(phoneNumber, text, pduBuffer, bufferSize);

		snprintf(szTmp, sizeof(szTmp), "AT+CMGS=%d\r\n", strlen(pduBuffer)/2-1);
		m_serial.write(szTmp, (int)strlen(szTmp)); 
		DbgPrintf(4, _T("SMS send: %hs sent"), szTmp);
		snprintf(szTmp, sizeof(szTmp), "%s%c\r\n", pduBuffer, 0x1A);
		DbgPrintf(4, _T("SMS about to send: %hs sent"), szTmp);
		m_serial.write(szTmp, (int)strlen(szTmp)); // send text, end with ^Z
		m_serial.read(szTmp, 128); // read +CMGS:ref_num
		DbgPrintf(4, _T("SMS send: AT+CMGS + message body sent, got {%hs}"), szTmp);
	}

	return true;
}

/**
 * Driver unload handler
 */
extern "C" void EXPORT SMSDriverUnload()
{
	m_serial.close();
}

/**
 * DLL Entry point
 */
#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
