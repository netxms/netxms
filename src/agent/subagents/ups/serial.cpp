/*
** NetXMS UPS management subagent
** Copyright (C) 2006 Victor Kirhenshtein
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


//
// Read line from serial port
//

BOOL SerialInterface::ReadLineFromSerial(char *pszBuffer, int nBufLen)
{
   int nPos = 0, nRet;

   memset(pszBuffer, 0, nBufLen);
   do
   {
      nRet = m_serial.Read(&pszBuffer[nPos], 1);
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
   return nRet != -1;
}


//
// Open communication to UPS
//

BOOL SerialInterface::Open(void)
{
   return m_serial.Open(m_pszDevice);
}


//
// Close communications with UPS
//

void SerialInterface::Close(void)
{
   m_serial.Close();
   UPSInterface::Close();
}
