/* 
** NetXMS - Network Management System
** Notification channel driver - GSM modem
** Copyright (C) 2003-2021 Raden Solutions
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
** File: gsm.cpp
**
**/

#include "gsm.h"

bool SMSCreatePDUString(const char* phoneNumber, const char* message, char* pduBuffer);

static const char *s_eosMarks[] = { "OK", "ERROR", NULL };
static const char *s_eosMarksSend[] = { ">", "ERROR", NULL };

enum OperationMode
{ 
   OM_TEXT, 
   OM_PDU 
};

/**
 * GSM driver class
 */
class GSMDriver : public NCDriver
{
private:
	Serial m_serial;
	OperationMode m_operationMode;
	bool m_cmgsUseQuotes;

public:
	GSMDriver(Config *config);

   virtual int send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body) override;
};

/**
 * Normalize text buffer (remove non-printable characters)
 */
static void NormalizeText(char *text)
{
   for(int i = 0; text[i] != 0; i++)
      if (text[i] < ' ')
         text[i] = ' ';
}

/**
 * Read input to OK
 */
static bool ReadToOK(Serial *serial, char *data = nullptr)
{
   char buffer[1024];
   memset(buffer, 0, 1024);
   while(true)
   {
      char *mark;
      ssize_t rc = serial->readToMark(buffer, 1024, s_eosMarks, &mark);
      if (rc <= 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("ReadToOK: readToMark returned %d"), static_cast<int>(rc));
         return false;
      }
      NormalizeText(buffer);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ReadToOK buffer content: '%hs'"), buffer);
      if (mark != NULL) 
      {
         if (data != nullptr)
         {
            size_t len = static_cast<size_t>(mark - buffer);
            memcpy(data, buffer, len);
            data[len] = 0;
         }

         if (!strncmp(mark, "OK", 2))
            return true;
 
#ifdef UNICODE
      	nxlog_debug_tag(DEBUG_TAG, 5, _T("Non-OK response (%hs)"), mark);
#else
      	nxlog_debug_tag(DEBUG_TAG, 5, _T("Non-OK response (%s)"), mark);
#endif
         return false;
      }
   }
}

/**
 * Initialize modem
 */
static bool InitModem(Serial *serial)
{
	serial->write("\x1A\r\n", 3); // in case of pending send operation
   ReadToOK(serial);

	serial->write("ATZ\r\n", 5); // init modem
   if (!ReadToOK(serial))
      return false;
	nxlog_debug_tag(DEBUG_TAG, 5, _T("ATZ sent, got OK"));

   serial->write("ATE0\r\n", 6); // disable echo
   if (!ReadToOK(serial))
      return false;
	nxlog_debug_tag(DEBUG_TAG, 5, _T("ATE0 sent, got OK"));
   
   return true;
}

/**
 * Initialize driver
 */
GSMDriver::GSMDriver(Config *config)
{
	TCHAR portName[128] = _T("");
	const TCHAR *parityAsText;
	int portSpeed = 9600;
	int dataBits = 8;
   char patryCr[2] = "";
	int parity = NOPARITY;
	int stopBits = ONESTOPBIT;
   int mode = 3; // both true
   int blockSize = 8;
   int writeDelay = 100;

   NX_CFG_TEMPLATE configTemplate[] = 
   {
      { _T("BlockSize"), CT_LONG, 0, 0, 0, 0, &blockSize },
      { _T("DataBits"), CT_LONG, 0, 0, 0, 0, &dataBits },
      { _T("Parity"), CT_MB_STRING, 0, 0, 2, 0, patryCr },
      { _T("Port"), CT_STRING, 0, 0, sizeof(portName) / sizeof(TCHAR), 0, portName },
      { _T("PortName"), CT_STRING, 0, 0, sizeof(portName) / sizeof(TCHAR), 0, portName },
      { _T("Speed"), CT_LONG, 0, 0, 0, 0, &portSpeed },
      { _T("StopBits"), CT_LONG, 0, 0, 0, 0, &stopBits },
      { _T("TextMode"), CT_BOOLEAN_FLAG_32, 0, 0, 1, 0, &mode },
      { _T("UseQuotes"), CT_BOOLEAN_FLAG_32, 0, 0, 2, 0, &mode },
      { _T("WriteDelay"), CT_LONG, 0, 0, 0, 0, &writeDelay },
      { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
   };

	nxlog_debug_tag(DEBUG_TAG, 1, _T("Loading Generic SMS Driver"));

	if (config->parseTemplate(_T("GSM"), configTemplate))
   {
      switch (tolower(patryCr[0]))
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
      switch (stopBits)
      {
         case 2:
            stopBits = TWOSTOPBITS;
            break;
         case 1:
         default:
            stopBits = ONESTOPBIT;
            break;
      }
      m_operationMode = (mode & 1) == 0 ? OM_PDU : OM_TEXT;
      m_cmgsUseQuotes = (mode & 2) != 0;

      if ((blockSize < 1) || (blockSize > 256))
         blockSize = 8;

      if ((writeDelay < 1) || (writeDelay > 10000))
         writeDelay = 100;
   }
   else
	{
#ifdef _WIN32
		_tcscpy(portName, _T("COM1:"));
#else
		_tcscpy(portName, _T("/dev/ttyS0"));
#endif
	}
	
	switch (parity)
	{
		case ODDPARITY:
			parityAsText = _T("ODD");
			break;
		case EVENPARITY:
			parityAsText = _T("EVEN");
			break;
		default:
			parityAsText = _T("NONE");
			break;
	}
	nxlog_debug_tag(DEBUG_TAG, 2, _T("Init: port=\"%s\", speed=%d, data=%d, parity=%s, stop=%d, pduMode=%s, numberInQuotes=%s blockSize=%d writeDelay=%d"),
	          portName, portSpeed, dataBits, parityAsText, stopBits == TWOSTOPBITS ? 2 : 1, 
             (m_operationMode == OM_PDU) ? _T("true") : _T("false"),
             m_cmgsUseQuotes ? _T("true") : _T("false"), blockSize, writeDelay);
	
   m_serial.setWriteBlockSize(blockSize);
   m_serial.setWriteDelay(writeDelay);

	if (m_serial.open(portName))
	{
		nxlog_debug_tag(DEBUG_TAG, 5, _T("Port opened"));
		m_serial.setTimeout(2000);
		
      if (!m_serial.set(portSpeed, dataBits, parity, stopBits))
      {
         nxlog_debug_tag(DEBUG_TAG, 0, _T("Cannot configure serial port. Current configuration: %s"), (const TCHAR *)config->createXml());
         goto cleanup;
      }
		
      if (!InitModem(&m_serial))
         goto cleanup;

		// enter PIN: AT+CPIN="xxxx"
		// register network: AT+CREG1
		
		m_serial.write("ATI3\r\n", 6); // read vendor id
		char vendorId[1024];
      if (!ReadToOK(&m_serial, vendorId))
         goto cleanup;
		nxlog_debug_tag(DEBUG_TAG, 5, _T("Init: ATI3 sent, got OK"));
		
		char *sptr, *eptr;	
		for(sptr = vendorId; (*sptr != 0) && ((*sptr == '\r') || (*sptr == '\n') || (*sptr == ' ') || (*sptr == '\t')); sptr++);
		for(eptr = sptr; (*eptr != 0) && (*eptr != '\r') && (*eptr != '\n'); eptr++);
		*eptr = 0;
      nxlog_debug_tag(DEBUG_TAG, 1, _T("GSM modem found on %s: %hs"), (const TCHAR *)config->createXml(), sptr);
	}
	else
	{
      nxlog_debug_tag(DEBUG_TAG, 0, _T("Cannot open serial port. Current configuration: %s"), (const TCHAR *)config->createXml());
	}

cleanup:
   m_serial.close();
}

/**
 * Send SMS
 */
int GSMDriver::send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body)
{
	if ((recipient == NULL) || (body == NULL))
      return -1;

   nxlog_debug_tag(DEBUG_TAG, 3, _T("Send to {%s}: {%s}"), recipient, body);
   if (!m_serial.restart())
   {
   	nxlog_debug_tag(DEBUG_TAG, 5, _T("Failed to open port"));
      return -1;
   }

   int result = -1;
   if (!InitModem(&m_serial))
      goto cleanup;
	
   if (m_operationMode == OM_PDU)
   {
	   m_serial.write("AT+CMGF=0\r\n", 11); // =0 - PDU message
      if (!ReadToOK(&m_serial))
         goto cleanup;
	   nxlog_debug_tag(DEBUG_TAG, 5, _T("AT+CMGF=0 sent, got OK"));

		char pduBuffer[PDU_BUFFER_SIZE];
#ifdef UNICODE
	   char mbPhoneNumber[128], mbText[161];

	   wchar_to_ASCII(recipient, -1, mbPhoneNumber, 128);
	   mbPhoneNumber[127] = 0;

	   wchar_to_utf8(body, -1, mbText, 161);
	   mbText[160] = 0;

		SMSCreatePDUString(mbPhoneNumber, mbText, pduBuffer);
#else
		SMSCreatePDUString(recipient, body, pduBuffer);
#endif

      char buffer[256];
		snprintf(buffer, sizeof(buffer), "AT+CMGS=%d\r\n", (int)strlen(pduBuffer) / 2 - 1);
	   m_serial.write(buffer, (int)strlen(buffer));

      char *mark;
      if (m_serial.readToMark(buffer, sizeof(buffer), s_eosMarksSend, &mark) <= 0)
         goto cleanup;
      if ((mark == NULL) || (*mark != '>'))
      {
         NormalizeText(buffer);
   	   nxlog_debug_tag(DEBUG_TAG, 5, _T("Wrong response to AT+CMGS=%d (%hs)"), (int)strlen(pduBuffer) / 2 - 1, mark);
         goto cleanup;
      }

      m_serial.write(pduBuffer, (int)strlen(pduBuffer)); // send PDU
      m_serial.write("\x1A", 1); // send ^Z
   }
   else
   {
	   m_serial.write("AT+CMGF=1\r\n", 11); // =1 - text message
      if (!ReadToOK(&m_serial))
         goto cleanup;
	   nxlog_debug_tag(DEBUG_TAG, 5, _T("AT+CMGF=1 sent, got OK"));

      char buffer[256];
#ifdef UNICODE
	   char mbPhoneNumber[128];
	   wchar_to_ASCII(recipient, -1, mbPhoneNumber, 128);
	   mbPhoneNumber[127] = 0;
      if (m_cmgsUseQuotes)
         snprintf(buffer, sizeof(buffer), "AT+CMGS=\"%s\"\r\n", mbPhoneNumber);
      else
         snprintf(buffer, sizeof(buffer), "AT+CMGS=%s\r\n", mbPhoneNumber);
#else
      if (m_cmgsUseQuotes)
         snprintf(buffer, sizeof(buffer), "AT+CMGS=\"%s\"\r\n", recipient);
      else
         snprintf(buffer, sizeof(buffer), "AT+CMGS=%s\r\n", recipient);
#endif
	   m_serial.write(buffer, (int)strlen(buffer)); // set number

      char *mark;
      if (m_serial.readToMark(buffer, sizeof(buffer), s_eosMarksSend, &mark) <= 0)
         goto cleanup;
      if ((mark == NULL) || (*mark != '>'))
      {
         NormalizeText(buffer);
   	   nxlog_debug_tag(DEBUG_TAG, 5, _T("Wrong response to AT+CMGS=\"%hs\" (%hs)"), recipient, mark);
         goto cleanup;
      }
   	
#ifdef UNICODE
	   char mbText[161];
      wchar_to_utf8(body, -1, mbText, 161);
	   mbText[160] = 0;
      snprintf(buffer, sizeof(buffer), "%s\x1A", mbText);
#else
      if (strlen(body) <= 160)
      {
         snprintf(buffer, sizeof(buffer), "%s\x1A", body);
      }
      else
      {
         strncpy(buffer, body, 160);
         strcpy(&buffer[160], "\x1A");
      }
#endif
	   m_serial.write(buffer, (int)strlen(buffer)); // send text, end with ^Z
   }

   m_serial.setTimeout(30000);
   if (!ReadToOK(&m_serial))
      goto cleanup;

   nxlog_debug_tag(DEBUG_TAG, 5, _T("AT+CMGS + message body sent, got OK"));
   result = 0;

cleanup:
   m_serial.setTimeout(2000);
   m_serial.close();
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Serial port closed"));
   return result;
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(GSM, NULL)
{
   return new GSMDriver(config);
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
