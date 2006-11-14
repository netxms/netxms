/* $Id: main.cpp,v 1.6 2006-11-14 14:51:40 alk Exp $ */

#include "main.h"

#ifdef _WIN32
# define snprintf _snprintf
# define strcasecmp stricmp
# define sleep(x) Sleep(x * 1000)
#endif

static Serial m_serial;

// pszInitArgs format: portname,speed,databits,parity,stopbits
extern "C" BOOL EXPORT SMSDriverInit(TCHAR *pszInitArgs)
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

	DbgPrintf(AF_DEBUG_MISC, "Loading Generic SMS Driver (configuration: %s)", pszInitArgs);
	
	char *p;
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
			portSpeed = t;
			
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
							
							if (*p == 2)
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
		p = "ODD";
		break;
	case EVENPARITY:
		p = "EVEN";
		break;
	default:
		p = "NONE";
		break;
	}
	DbgPrintf(AF_DEBUG_ALL, "SMS init: port={%s}, speed=%d, data=%d, parity=%s, stop=%d",
		portName, portSpeed, dataBits, p, stopBits == TWOSTOPBITS ? "2" : "1");
	
	bRet = m_serial.Open(pszInitArgs);
	if (bRet)
	{
		DbgPrintf(AF_DEBUG_ALL, "SMS: port opened");
	   	m_serial.SetTimeout(1000);
		m_serial.Set(portSpeed, dataBits, parity, stopBits);

		// enter PIN: AT+CPIN="xxxx"
		// register network: AT+CREG1

		char szTmp[128];
		m_serial.Write("ATZ\r\n", 5); // init modem && read user prefs
		m_serial.Read(szTmp, 128); // read OK
		DbgPrintf(AF_DEBUG_ALL, "SMS init: ATZ sent, got {%s}", szTmp);
		m_serial.Write("ATE0\r\n", 6); // disable echo
		m_serial.Read(szTmp, 128); // read OK
		DbgPrintf(AF_DEBUG_ALL, "SMS init: ATE0 sent, got {%s}", szTmp);
		m_serial.Write("ATI3\r\n", 6); // read vendor id
		m_serial.Read(szTmp, 128); // read version
		DbgPrintf(AF_DEBUG_ALL, "SMS init: ATI3 sent, got {%s}", szTmp);

		if (strcasecmp(szTmp, "ERROR") != 0)
		{
			char *sptr, *eptr;

			for(sptr = szTmp; (*sptr != 0) && ((*sptr == '\r') || (*sptr == '\n') || (*sptr == ' ') || (*sptr == '\t')); sptr++);
			for(eptr = sptr; (*eptr != 0) && (*eptr != '\r') && (*eptr != '\n'); eptr++);
			*eptr = 0;
			WriteLog(MSG_GSM_MODEM_INFO, EVENTLOG_INFORMATION_TYPE, "ss", pszInitArgs, sptr);
		}
	}
	else
	{
		DbgPrintf(AF_DEBUG_MISC, "Unable to open serial port (%s)", pszInitArgs);
	}
	
	if (portName != NULL)
	{
		free(portName);
	}
	
	return bRet;
}

extern "C" BOOL EXPORT SMSDriverSend(TCHAR *pszPhoneNumber, TCHAR *pszText)
{
	if (pszPhoneNumber != NULL && pszText != NULL)
	{
		char szTmp[128];

		DbgPrintf(AF_DEBUG_MISC, "SMS send: to {%s}: {%s}", pszPhoneNumber, pszText);

		m_serial.Write("ATZ\r\n", 5); // init modem && read user prefs
		m_serial.Read(szTmp, 128); // read OK
		DbgPrintf(AF_DEBUG_ALL, "SMS send: ATZ sent, got {%s}", szTmp);
		m_serial.Write("ATE0\r\n", 5); // disable echo
		m_serial.Read(szTmp, 128); // read OK
		DbgPrintf(AF_DEBUG_ALL, "SMS send: ATE0 sent, got {%s}", szTmp);
		m_serial.Write("AT+CMGF=1\r\n", 11); // =1 - text message
		m_serial.Read(szTmp, 128); // read OK
		DbgPrintf(AF_DEBUG_ALL, "SMS send: AT+CMGF=1 sent, got {%s}", szTmp);
		snprintf(szTmp, sizeof(szTmp), "AT+CMGS=\"%s\"\r\n", pszPhoneNumber);
		m_serial.Write(szTmp, strlen(szTmp)); // set number
		snprintf(szTmp, sizeof(szTmp), "%s%c\r\n", pszText, 0x1A);
		m_serial.Write(szTmp, strlen(szTmp)); // send text, end with ^Z
		m_serial.Read(szTmp, 128); // read +CMGS:ref_num
		DbgPrintf(AF_DEBUG_ALL, "SMS send: AT+CMGS + message body sent, got {%s}", szTmp);
	}

	return true;
}

extern "C" void EXPORT SMSDriverUnload(void)
{
	m_serial.Close();
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


///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.5  2006/03/23 07:53:16  victor
Added DLL entry point

Revision 1.4  2006/01/22 16:08:14  victor
Minor changes

Revision 1.3  2005/06/16 20:54:26  victor
Modem hardware ID written to server log

Revision 1.2  2005/06/16 13:34:21  alk
project files addded

Revision 1.1  2005/06/16 13:19:38  alk
added sms-driver for generic gsm modem

*/
