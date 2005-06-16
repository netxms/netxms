/* $Id: main.cpp,v 1.3 2005-06-16 20:54:26 victor Exp $ */

#include "main.h"

#ifdef _WIN32
# define snprintf _snprintf
# define strcasecmp stricmp
# define sleep(x) Sleep(x * 1000)
#endif

static Serial m_serial;

extern "C" BOOL EXPORT SMSDriverInit(TCHAR *pszInitArgs)
{
	bool bRet = false;
	if (pszInitArgs == NULL || *pszInitArgs == 0)
	{
#ifdef _WIN32
		pszInitArgs = _T("COM1:");
#else
		pszInitArgs = _T("/dev/ttyS0");
#endif
	}

	m_serial.SetTimeout(1);
	bRet = m_serial.Open(pszInitArgs);
	if (bRet)
	{
		m_serial.Set(9600, 8, NOPARITY, ONESTOPBIT);

		// enter PIN: AT+CPIN="xxxx"
		// register network: AT+CREG1

		char szTmp[128];
		m_serial.Write("ATZ\r\n", 5); // init modem && read user prefs
		m_serial.Read(szTmp, 128); // read OK
		m_serial.Write("ATE0\r\n", 5); // disable echo
		m_serial.Read(szTmp, 128); // read OK
		m_serial.Write("ATI3\r\n", 6); // read vendor id
		m_serial.Read(szTmp, 128); // read version

		if (strcasecmp(szTmp, "ERROR") != 0)
		{
         char *sptr, *eptr;

         for(sptr = szTmp; (*sptr != 0) && ((*sptr == '\r') || (*sptr == '\n') || (*sptr == ' ') || (*sptr == '\t')); sptr++);
         for(eptr = sptr; (*eptr != 0) && (*eptr != '\r') && (*eptr != '\n'); eptr++);
         *eptr = 0;
         WriteLog(MSG_GSM_MODEM_INFO, EVENTLOG_INFORMATION_TYPE, "ss", pszInitArgs, sptr);
		}
	}

	return bRet;
}

extern "C" BOOL EXPORT SMSDriverSend(TCHAR *pszPhoneNumber, TCHAR *pszText)
{
	if (pszPhoneNumber != NULL && pszText != NULL)
	{
		char szTmp[128];

		m_serial.Write("ATZ\r\n", 5); // init modem && read user prefs
		m_serial.Read(szTmp, 128); // read OK
		m_serial.Write("ATE0\r\n", 5); // disable echo
		m_serial.Read(szTmp, 128); // read OK
		m_serial.Write("AT+CMGF=1\r\n", 11); // =1 - text message
		m_serial.Read(szTmp, 128); // read OK
		snprintf(szTmp, sizeof(szTmp), "AT+CMGS=\"%s\"\r\n", pszPhoneNumber);
		m_serial.Write(szTmp, strlen(szTmp)); // set number
		snprintf(szTmp, sizeof(szTmp), "%s%c\r\n", pszText, 0x1A);
		m_serial.Write(szTmp, strlen(szTmp)); // send text, end with ^Z
		m_serial.Read(szTmp, 128); // read +CMGS:ref_num
	}

	return true;
}

extern "C" void EXPORT SMSDriverUnload(void)
{
	m_serial.Close();
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.2  2005/06/16 13:34:21  alk
project files addded

Revision 1.1  2005/06/16 13:19:38  alk
added sms-driver for generic gsm modem

*/
