/* 
** NetXMS - Network Management System
** Copyright (C) 2005, 2006 Alex Kirhenshtein
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

#include "libnetxms.h"

#if HAVE_TERMIO_H
#include <termio.h>
#elif HAVE_TERMIOS_H
#include <termios.h>
#endif


//
// Constructor
//

Serial::Serial(void)
{
	m_nTimeout = 0;
	m_hPort = INVALID_HANDLE_VALUE;
   m_pszPort = NULL;
}


//
// Destructor
//

Serial::~Serial(void)
{
   Close();
   safe_free(m_pszPort);
}


//
// Open serial device
//

bool Serial::Open(TCHAR *pszPort)
{
	bool bRet = false;

   safe_free(m_pszPort);
	m_pszPort = _tcsdup(pszPort);

#ifdef _WIN32
	m_hPort = CreateFile(pszPort, GENERIC_READ | GENERIC_WRITE, 0, NULL,
			               OPEN_EXISTING, 0, NULL);
	if (m_hPort != INVALID_HANDLE_VALUE)
#else // UNIX
	m_hPort = open(pszPort, O_RDWR | O_NOCTTY | O_NDELAY); 
	if (m_hPort != -1)
#endif
   {
      Set(38400, 8, NOPARITY, ONESTOPBIT);
	   bRet = true;
   }
	return bRet;
}


//
// Set communication parameters
//

bool Serial::Set(int nSpeed, int nDataBits, int nParity, int nStopBits)
{
	bool bRet = false;

	m_nDataBits = nDataBits;
	m_nSpeed = nSpeed;
	m_nParity = nParity;
	m_nStopBits = nStopBits;

#ifdef _WIN32
	DCB dcb;

   dcb.DCBlength = sizeof(DCB);
	if (GetCommState(m_hPort, &dcb))
	{
		dcb.BaudRate = nSpeed;
		dcb.ByteSize = nDataBits;
		dcb.Parity = nParity;
		dcb.StopBits = nStopBits;
		dcb.fDtrControl = DTR_CONTROL_DISABLE;
      dcb.fBinary = TRUE;
		dcb.fParity = FALSE;
		dcb.fOutxCtsFlow = FALSE;
		dcb.fOutxDsrFlow = FALSE;
		dcb.fDsrSensitivity = FALSE;
		dcb.fOutX = FALSE;
		dcb.fInX = FALSE;
		dcb.fRtsControl = RTS_CONTROL_DISABLE;

		if (SetCommState(m_hPort, &dcb))
		{
	      COMMTIMEOUTS ct;

	      memset(&ct, 0, sizeof(COMMTIMEOUTS));
	      ct.ReadTotalTimeoutConstant = m_nTimeout;
	      ct.WriteTotalTimeoutConstant = m_nTimeout;
	      SetCommTimeouts(m_hPort, &ct);
			bRet = true;
		}
	}

#else // UNIX
	struct termios newTio;

	tcgetattr(m_hPort, &newTio);

	newTio.c_cc[VMIN] = 1;
	newTio.c_cc[VTIME] = m_nTimeout;

	newTio.c_cflag |= CLOCAL | CREAD;

	newTio.c_cflag &= ~(CBAUD | CBAUDEX);
	switch(nSpeed)
	{
		case 50:     newTio.c_cflag |= B50;     break;
		case 75:     newTio.c_cflag |= B75;     break;
		case 110:    newTio.c_cflag |= B110;    break;
		case 134:    newTio.c_cflag |= B134;    break;
		case 150:    newTio.c_cflag |= B150;    break;
		case 200:    newTio.c_cflag |= B200;    break;
		case 300:    newTio.c_cflag |= B300;    break;
		case 600:    newTio.c_cflag |= B600;    break;
		case 1200:   newTio.c_cflag |= B1200;   break;
		case 1800:   newTio.c_cflag |= B1800;   break;
		case 2400:   newTio.c_cflag |= B2400;   break;
		case 4800:   newTio.c_cflag |= B4800;   break;
		case 9600:   newTio.c_cflag |= B9600;   break;
		case 19200:  newTio.c_cflag |= B19200;  break;
		case 38400:  newTio.c_cflag |= B38400;  break;
		default:     newTio.c_cflag |= B38400;  break;
	}

	newTio.c_cflag &= ~(CSIZE);
	switch(nDataBits)
	{
		case 5:
			newTio.c_cflag |= CS5;
			break;
		case 6:
			newTio.c_cflag |= CS6;
			break;
		case 7:
			newTio.c_cflag |= CS7;
			break;
		case 8:
		default:
			newTio.c_cflag |= CS8;
			break;
	}

	newTio.c_cflag &= ~(PARODD | PARENB);
	switch(nParity)
	{
		case ODDPARITY:
			newTio.c_cflag |= PARODD | PARENB;
			break;
		case EVENPARITY:
			newTio.c_cflag |= PARENB;
			break;
		default:
			break;
	}

	newTio.c_cflag &= ~(CSTOPB);
	if (nStopBits != ONESTOPBIT)
	{
		newTio.c_cflag |= CSTOPB;
	}

	newTio.c_cflag &= ~(CRTSCTS | IXON | IXOFF);
	newTio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
   newTio.c_iflag &= ~(IXON | IXOFF | IXANY);
   newTio.c_oflag &= ~(OPOST);

	tcsetattr(m_hPort, TCSANOW, &newTio);

#endif // _WIN32

	return bRet;
}


//
// Close serial port
//

void Serial::Close(void)
{
   if (m_hPort != INVALID_HANDLE_VALUE)
   {
#ifdef _WIN32
   	CloseHandle(m_hPort);
#else // UNIX
	   close(m_hPort);
#endif // _WIN32s
      m_hPort = INVALID_HANDLE_VALUE;
   }
}


//
// Set receive timeout (in milliseconds)
//

void Serial::SetTimeout(int nTimeout)
{
#ifdef _WIN32
	COMMTIMEOUTS ct;

   m_nTimeout = nTimeout;
	memset(&ct, 0, sizeof(COMMTIMEOUTS));
	ct.ReadTotalTimeoutConstant = m_nTimeout;
	ct.WriteTotalTimeoutConstant = m_nTimeout;

	SetCommTimeouts(m_hPort, &ct);
#else
	struct termios tio;

	tcgetattr(m_hPort, &tio);

	m_nTimeout = nTimeout / 100; // convert to deciseconds (VTIME in 1/10sec)
	tio.c_cc[VTIME] = m_nTimeout;

   tcsetattr(m_hPort, TCSANOW, &tio);
#endif // WIN32
}


//
// Restart port
//

bool Serial::Restart(void)
{
	Close();
	if (Open(m_pszPort))
	   if (Set(m_nSpeed, m_nDataBits, m_nParity, m_nStopBits))
      	return true;
   return false;
}


//
// Read character(s) from port
//

int Serial::Read(char *pBuff, int nSize)
{
	int nRet;

	memset(pBuff, 0, nSize);

#ifdef _WIN32
	DWORD nDone;

	if (ReadFile(m_hPort, pBuff, nSize, &nDone, NULL) != 0)
	{
      nRet = (int)nDone;
	}
	else
	{
      nRet = -1;
	}

#else // UNIX
	fd_set rdfs;
	struct timeval tv;

	FD_ZERO(&rdfs);
	FD_SET(m_hPort, &rdfs);
	tv.tv_sec = m_nTimeout / 10;
	tv.tv_usec = 0;
	nRet = select(m_hPort + 1, &rdfs, NULL, NULL, &tv);
	if (nRet > 0)
		nRet = read(m_hPort, pBuff, nSize);
   else
      nRet = -1;  // Timeout is an error

#endif // _WIN32

   if (nRet == -1)
      Restart();

	return nRet;
}


//
// Write character(s) to port
//

bool Serial::Write(char *pBuff, int nSize)
{
	bool bRet = false;

#ifdef _WIN32

	Sleep(100);

	DWORD nDone;
	if (WriteFile(m_hPort, pBuff, nSize, &nDone, NULL) != 0)
	{
		if (nDone == (DWORD)nSize)
		{
			bRet = true;
		}
	}
	else
	{
		Restart();
	}

#else // UNIX
	usleep(100);

	if (write(m_hPort, pBuff, nSize) == nSize)
	{
		bRet = true;
	}
	else
	{
		Restart();
	}

#endif // _WIN32

	return bRet;
}


//
// Flush buffer
//

void Serial::Flush(void)
{
#ifdef _WIN32

	FlushFileBuffers(m_hPort);

#else // UNIX

#ifndef _NETWARE
	tcflush(m_hPort, TCIOFLUSH);
#endif

#endif // _WIN32
}
