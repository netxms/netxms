/*
** NetXMS UPS management subagent
** Copyright (C) 2006-2014 Victor Kirhenshtein
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
** File: serial.cpp
**
**/

#include "ups.h"

/**
 * Create serial interface
 */
SerialInterface::SerialInterface(TCHAR *device) : UPSInterface(device)
{
	m_portSpeed = 0;
	m_dataBits = 8;
	m_parity = NOPARITY;
	m_stopBits = ONESTOPBIT;
	
	TCHAR *p;
	if ((p = _tcschr(m_pszDevice, _T(','))) != NULL)
	{
		*p = 0; p++;
		int tmp = _tcstol(p, NULL, 10);
		if (tmp != 0)
		{
			m_portSpeed = tmp;
			
			if ((p = _tcschr(p, _T(','))) != NULL)
			{
				*p = 0; p++;
				tmp = _tcstol(p, NULL, 10);
				if (tmp >= 5 && tmp <= 8)
				{
					m_dataBits = tmp;
					
					// parity
					if ((p = _tcschr(p, _T(','))) != NULL)
					{
						*p = 0; p++;
						switch (tolower(*p))
						{
							case _T('n'): // none
								m_parity = NOPARITY;
								break;
							case _T('o'): // odd
								m_parity = ODDPARITY;
								break;
							case _T('e'): // even
								m_parity = EVENPARITY;
								break;
						}
						
						// stop bits
						if ((p = _tcschr(p, _T(','))) != NULL)
						{
							*p = 0; p++;
							
							if (*p == _T('2'))
							{
								m_stopBits = TWOSTOPBITS;
							}
						}
					}
				}
			}
		}
	}
}

/**
 * Read line from serial port
 */
BOOL SerialInterface::readLineFromSerial(char *pszBuffer, int nBufLen)
{
   int nPos = 0, nRet;

   memset(pszBuffer, 0, nBufLen);
   do
   {
      nRet = m_serial.read(&pszBuffer[nPos], 1);
      if (nRet > 0)
         nPos += nRet;
   } while((nRet > 0) && (pszBuffer[nPos - 1] != '\n') && (nPos < nBufLen));
   if (nRet != -1)
   {
      if (pszBuffer[nPos - 2] == '\r')
         pszBuffer[nPos - 2] = 0;
      else
         pszBuffer[nPos - 1] = 0;
   }
   return nRet > 0;
}

/**
 * Open communication to UPS
 */
BOOL SerialInterface::open()
{
   return m_serial.open(m_pszDevice);
}

/**
 * Close communications with UPS
 */
void SerialInterface::close()
{
   m_serial.close();
   UPSInterface::close();
}
