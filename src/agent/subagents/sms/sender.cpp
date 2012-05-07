/* $Id$ */
/*
** NetXMS SMS sender subagent
** Copyright (C) 2007 Victor Kirhenshtein
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
** File: sender.cpp
**
**/

#include "sms.h"


//
// Static data
//

static Serial m_serial;


//
// Initialize sender
// pszInitArgs format: portname,speed,databits,parity,stopbits
//

BOOL InitSender(const TCHAR *pszInitArgs)
{
	bool bRet = false;
	char *portName;
	
	if (pszInitArgs == NULL || *pszInitArgs == 0)
	{
#ifdef _WIN32
		portName = strdup(_T("COM1:"));
#else
		portName = strdup(_T("/dev/ttyS0"));
#endif
	}
	else
	{
		portName = strdup(pszInitArgs);
	}
	
	AgentWriteDebugLog(1, "SMS Sender: initializing GSM modem at %s", pszInitArgs);
	
	char *p;
	const char *parityAsText;
	int portSpeed = 9600;
	int dataBits = 8;
	int parity = NOPARITY;
	int stopBits = ONESTOPBIT;
	
	if ((p = strchr(portName, ',')) != NULL)
	{
		*p = 0; p++;
		int tmp = atoi(p);
		if (tmp != 0)
		{
			portSpeed = tmp;
			
			if ((p = strchr(p, ',')) != NULL)
			{
				*p = 0; p++;
				tmp = atoi(p);
				if (tmp >= 5 && tmp <= 8)
				{
					dataBits = tmp;
					
					// parity
					if ((p = strchr(p, ',')) != NULL)
					{
						*p = 0; p++;
						switch (tolower(*p))
						{
						case 'n': // none
							parity = NOPARITY;
							break;
						case 'o': // odd
							parity = ODDPARITY;
							break;
						case 'e': // even
							parity = EVENPARITY;
							break;
						}
						
						// stop bits
						if ((p = strchr(p, ',')) != NULL)
						{
							*p = 0; p++;
							
							if (*p == '2')
							{
								stopBits = TWOSTOPBITS;
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
		parityAsText = "ODD";
		break;
	case EVENPARITY:
		parityAsText = "EVEN";
		break;
	default:
		parityAsText = "NONE";
		break;
	}
	AgentWriteDebugLog(1, "SMS init: port={%s}, speed=%d, data=%d, parity=%s, stop=%d",
	                portName, portSpeed, dataBits, parityAsText, stopBits == TWOSTOPBITS ? 2 : 1);
	
	bRet = m_serial.Open(portName);
	if (bRet)
	{
		AgentWriteDebugLog(5, "SMS Sender: port opened");
		m_serial.SetTimeout(1000);
		m_serial.Set(portSpeed, dataBits, parity, stopBits);
		
		// enter PIN: AT+CPIN="xxxx"
		// register network: AT+CREG1
		
		char szTmp[128];
		m_serial.Write("ATZ\r\n", 5); // init modem && read user prefs
		m_serial.Read(szTmp, 128); // read OK
		AgentWriteDebugLog(5, "SMS init: ATZ sent, got {%s}", szTmp);
		m_serial.Write("ATE0\r\n", 6); // disable echo
		m_serial.Read(szTmp, 128); // read OK
		AgentWriteDebugLog(5, "SMS init: ATE0 sent, got {%s}", szTmp);
		m_serial.Write("ATI3\r\n", 6); // read vendor id
		m_serial.Read(szTmp, 128); // read version
		AgentWriteDebugLog(5, "SMS init: ATI3 sent, got {%s}", szTmp);
		
		if (stricmp(szTmp, "ERROR") != 0)
		{
			char *sptr, *eptr;
			
			for(sptr = szTmp; (*sptr != 0) && ((*sptr == '\r') || (*sptr == '\n') || (*sptr == ' ') || (*sptr == '\t')); sptr++);
			for(eptr = sptr; (*eptr != 0) && (*eptr != '\r') && (*eptr != '\n'); eptr++);
			*eptr = 0;
			nx_strncpy(g_szDeviceModel, sptr, 256);
			AgentWriteLog(EVENTLOG_INFORMATION_TYPE, _T("SMS Sender: GSM modem initialized (Device=\"%s\" Model=\"%s\")"), pszInitArgs, sptr);
		}
	}
	else
	{
		AgentWriteLog(EVENTLOG_WARNING_TYPE, "SMS Sender: Unable to open serial port (%s)", pszInitArgs);
	}
	
	if (portName != NULL)
	{
		free(portName);
	}
	
	return bRet;
}


//
// Send SMS
//

BOOL SendSMS(const TCHAR *pszPhoneNumber, const TCHAR *pszText)
{
	if (pszPhoneNumber != NULL && pszText != NULL)
	{
		char szTmp[128];
		
		AgentWriteDebugLog(3, "SMS send: to {%s}: {%s}", pszPhoneNumber, pszText);
		
		m_serial.Write("ATZ\r\n", 5); // init modem && read user prefs
		m_serial.Read(szTmp, 128); // read OK
		AgentWriteDebugLog(5, "SMS send: ATZ sent, got {%s}", szTmp);
		m_serial.Write("ATE0\r\n", 5); // disable echo
		m_serial.Read(szTmp, 128); // read OK
		AgentWriteDebugLog(5, "SMS send: ATE0 sent, got {%s}", szTmp);
		m_serial.Write("AT+CMGF=1\r\n", 11); // =1 - text message
		m_serial.Read(szTmp, 128); // read OK
		AgentWriteDebugLog(5, "SMS send: AT+CMGF=1 sent, got {%s}", szTmp);
		snprintf(szTmp, sizeof(szTmp), "AT+CMGS=\"%s\"\r\n", pszPhoneNumber);
		m_serial.Write(szTmp, (int)strlen(szTmp)); // set number
		snprintf(szTmp, sizeof(szTmp), "%s%c\r\n", pszText, 0x1A);
		m_serial.Write(szTmp, (int)strlen(szTmp)); // send text, end with ^Z
		m_serial.Read(szTmp, 128); // read +CMGS:ref_num
		AgentWriteDebugLog(5, "SMS send: AT+CMGS + message body sent, got {%s}", szTmp);
	}
	
	return true;
}


//
// Shutdown sender
//

void ShutdownSender(void)
{
	m_serial.Close();
}
