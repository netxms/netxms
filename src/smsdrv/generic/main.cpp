/* 
** NetXMS - Network Management System
** Generic SMS driver
** Copyright (C) 2003-2016 Raden Solutions
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

#include "generic_smsdrv.h"

bool SMSCreatePDUString(const char* phoneNumber, const char* message, char* pduBuffer);

/**
 * Static data
 */
static Serial s_serial;
static const char *s_eosMarks[] = { "OK", "ERROR", NULL };
static const char *s_eosMarksSend[] = { ">", "ERROR", NULL };
static enum { OM_TEXT, OM_PDU } s_operationMode = OM_TEXT;
static bool s_cmgsUseQuotes = true;

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
static bool ReadToOK(Serial *serial, char *data = NULL)
{
   char buffer[1024];
   memset(buffer, 0, 1024);
   while(true)
   {
      char *mark;
      int rc = serial->readToMark(buffer, 1024, s_eosMarks, &mark);
      if (rc <= 0)
      {
         nxlog_debug(5, _T("SMS: ReadToOK: readToMark returned %d"), rc);
         return false;
      }
      NormalizeText(buffer);
      nxlog_debug(5, _T("SMS: ReadToOK buffer content: '%hs'"), buffer);
      if (mark != NULL) 
      {
         if (data != NULL)
         {
            int len = (int)(mark - buffer);
            memcpy(data, buffer, len);
            data[len] = 0;
         }

         if (!strncmp(mark, "OK", 2))
            return true;
 
#ifdef UNICODE
      	nxlog_debug(5, _T("SMS: non-OK response (%hs)"), mark);
#else
      	nxlog_debug(5, _T("SMS: non-OK response (%s)"), mark);
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
	nxlog_debug(5, _T("SMS: ATZ sent, got OK"));

   serial->write("ATE0\r\n", 6); // disable echo
   if (!ReadToOK(serial))
      return false;
	nxlog_debug(5, _T("SMS: ATE0 sent, got OK"));
   
   return true;
}

/**
 * Initialize driver
 *
 * pszInitArgs format: portname,speed,databits,parity,stopbits,mode,blocksize,writedelay
 */
extern "C" bool EXPORT SMSDriverInit(const TCHAR *pszInitArgs, Config *config)
{
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
	
	nxlog_debug(1, _T("Loading Generic SMS Driver (configuration: %s)"), pszInitArgs);
	
	TCHAR *p;
	const TCHAR *parityAsText;
	int portSpeed = 9600;
	int dataBits = 8;
	int parity = NOPARITY;
	int stopBits = ONESTOPBIT;
   int blockSize = 8;
   int writeDelay = 100;
	
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
                        {
									s_operationMode = OM_TEXT;
                        }
								else if (*p == _T('P'))
                        {
									s_operationMode = OM_PDU;
                        }

                        // Use quotes
							   if ((p = _tcschr(p, _T(','))) != NULL)
							   {
								   *p = 0; p++;
								   if (*p == _T('N'))
                           {
                              s_cmgsUseQuotes = false;
                           }
                           else
                           {
                              s_cmgsUseQuotes = true;
                           }

                           // block size
							      if ((p = _tcschr(p, _T(','))) != NULL)
							      {
   								   *p = 0; p++;
                              blockSize = _tcstol(p, NULL, 10);
                              if ((blockSize < 1) || (blockSize > 256))
                                 blockSize = 8;

                              // write delay
							         if ((p = _tcschr(p, _T(','))) != NULL)
							         {
   								      *p = 0; p++;
                                 writeDelay = _tcstol(p, NULL, 10);
                                 if ((writeDelay < 1) || (writeDelay > 10000))
                                    writeDelay = 100;
                              }
                           }
                        }
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
			parityAsText = _T("ODD");
			break;
		case EVENPARITY:
			parityAsText = _T("EVEN");
			break;
		default:
			parityAsText = _T("NONE");
			break;
	}
	nxlog_debug(2, _T("SMS init: port=\"%s\", speed=%d, data=%d, parity=%s, stop=%d, pduMode=%s, numberInQuotes=%s blockSize=%d writeDelay=%d"),
	          portName, portSpeed, dataBits, parityAsText, stopBits == TWOSTOPBITS ? 2 : 1, 
             (s_operationMode == OM_PDU) ? _T("true") : _T("false"),
             s_cmgsUseQuotes ? _T("true") : _T("false"), blockSize, writeDelay);
	
   s_serial.setWriteBlockSize(blockSize);
   s_serial.setWriteDelay(writeDelay);

	if (s_serial.open(portName))
	{
		nxlog_debug(5, _T("SMS: port opened"));
		s_serial.setTimeout(2000);
		
      if (!s_serial.set(portSpeed, dataBits, parity, stopBits))
      {
         nxlog_debug(0, _T("SMS: cannot configure serial port %s"), pszInitArgs);
         goto cleanup;
      }
		
      if (!InitModem(&s_serial))
         goto cleanup;

		// enter PIN: AT+CPIN="xxxx"
		// register network: AT+CREG1
		
		s_serial.write("ATI3\r\n", 6); // read vendor id
		char vendorId[1024];
      if (!ReadToOK(&s_serial, vendorId))
         goto cleanup;
		nxlog_debug(5, _T("SMS init: ATI3 sent, got OK"));
		
		char *sptr, *eptr;	
		for(sptr = vendorId; (*sptr != 0) && ((*sptr == '\r') || (*sptr == '\n') || (*sptr == ' ') || (*sptr == '\t')); sptr++);
		for(eptr = sptr; (*eptr != 0) && (*eptr != '\r') && (*eptr != '\n'); eptr++);
		*eptr = 0;
      nxlog_debug(0, _T("SMS: GSM modem found on %s: %hs"), pszInitArgs, sptr);
	}
	else
	{
      nxlog_debug(0, _T("SMS: cannot open serial port %s"), pszInitArgs);
	}

cleanup:
	safe_free(portName);
   s_serial.close();
	return true;   // return TRUE always to keep driver in memory
}

/**
 * Send SMS
 */
extern "C" bool EXPORT SMSDriverSend(const TCHAR *pszPhoneNumber, const TCHAR *pszText)
{
	if ((pszPhoneNumber == NULL) || (pszText == NULL))
      return false;

   nxlog_debug(3, _T("SMS: send to {%s}: {%s}"), pszPhoneNumber, pszText);
   if (!s_serial.restart())
   {
   	nxlog_debug(5, _T("SMS: failed to open port"));
      return false;
   }

   bool success = false;
   if (!InitModem(&s_serial))
      goto cleanup;
	
   if (s_operationMode == OM_PDU)
   {
	   s_serial.write("AT+CMGF=0\r\n", 11); // =0 - PDU message
      if (!ReadToOK(&s_serial))
         goto cleanup;
	   nxlog_debug(5, _T("SMS: AT+CMGF=0 sent, got OK"));

		char pduBuffer[PDU_BUFFER_SIZE];
#ifdef UNICODE
	   char mbPhoneNumber[128], mbText[161];

	   WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, pszPhoneNumber, -1, mbPhoneNumber, 128, NULL, NULL);
	   mbPhoneNumber[127] = 0;

	   WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, pszText, -1, mbText, 161, NULL, NULL);
	   mbText[160] = 0;

		SMSCreatePDUString(mbPhoneNumber, mbText, pduBuffer);
#else
		SMSCreatePDUString(pszPhoneNumber, pszText, pduBuffer);
#endif

      char buffer[256];
		snprintf(buffer, sizeof(buffer), "AT+CMGS=%d\r\n", (int)strlen(pduBuffer) / 2 - 1);
	   s_serial.write(buffer, (int)strlen(buffer));

      char *mark;
      if (s_serial.readToMark(buffer, sizeof(buffer), s_eosMarksSend, &mark) <= 0)
         goto cleanup;
      if ((mark == NULL) || (*mark != '>'))
      {
         NormalizeText(buffer);
   	   nxlog_debug(5, _T("SMS: wrong response to AT+CMGS=%d (%hs)"), (int)strlen(pduBuffer) / 2 - 1, mark);
         goto cleanup;
      }

      s_serial.write(pduBuffer, (int)strlen(pduBuffer)); // send PDU
      s_serial.write("\x1A", 1); // send ^Z
   }
   else
   {
	   s_serial.write("AT+CMGF=1\r\n", 11); // =1 - text message
      if (!ReadToOK(&s_serial))
         goto cleanup;
	   nxlog_debug(5, _T("SMS: AT+CMGF=1 sent, got OK"));

      char buffer[256];
#ifdef UNICODE
	   char mbPhoneNumber[128];
	   WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, pszPhoneNumber, -1, mbPhoneNumber, 128, NULL, NULL);
	   mbPhoneNumber[127] = 0;
      if (s_cmgsUseQuotes)
         snprintf(buffer, sizeof(buffer), "AT+CMGS=\"%s\"\r\n", mbPhoneNumber);
      else
         snprintf(buffer, sizeof(buffer), "AT+CMGS=%s\r\n", mbPhoneNumber);
#else
      if (s_cmgsUseQuotes)
         snprintf(buffer, sizeof(buffer), "AT+CMGS=\"%s\"\r\n", pszPhoneNumber);
      else
         snprintf(buffer, sizeof(buffer), "AT+CMGS=%s\r\n", pszPhoneNumber);
#endif
	   s_serial.write(buffer, (int)strlen(buffer)); // set number

      char *mark;
      if (s_serial.readToMark(buffer, sizeof(buffer), s_eosMarksSend, &mark) <= 0)
         goto cleanup;
      if ((mark == NULL) || (*mark != '>'))
      {
         NormalizeText(buffer);
   	   nxlog_debug(5, _T("SMS: wrong response to AT+CMGS=\"%hs\" (%hs)"), pszPhoneNumber, mark);
         goto cleanup;
      }
   	
#ifdef UNICODE
	   char mbText[161];
	   WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, pszText, -1, mbText, 161, NULL, NULL);
	   mbText[160] = 0;
      snprintf(buffer, sizeof(buffer), "%s\x1A", mbText);
#else
      if (strlen(pszText) <= 160)
      {
         snprintf(buffer, sizeof(buffer), "%s\x1A", pszText);
      }
      else
      {
         strncpy(buffer, pszText, 160);
         strcpy(&buffer[160], "\x1A");
      }
#endif
	   s_serial.write(buffer, (int)strlen(buffer)); // send text, end with ^Z
   }

   s_serial.setTimeout(30000);
   if (!ReadToOK(&s_serial))
      goto cleanup;

   nxlog_debug(5, _T("SMS: AT+CMGS + message body sent, got OK"));
   success = true;

cleanup:
   s_serial.setTimeout(2000);
   s_serial.close();
   nxlog_debug(5, _T("SMS: serial port closed"));
	return success;
}

/**
 * Driver unload handler
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
