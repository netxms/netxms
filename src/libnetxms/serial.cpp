/* 
** NetXMS - Network Management System
** Copyright (C) 2005, 2006 Alex Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: serial.cpp
**
**/

#include "libnetxms.h"

#ifndef CRTSCTS
# ifdef CNEW_RTSCTS
#  define CRTSCTS CNEW_RTSCTS
# else
#  define CRTSCTS 0
# endif
#endif

#ifdef _NETWARE
#ifndef ECHOKE
#define ECHOKE   0
#endif
#ifndef ECHOCTL
#define ECHOCTL  0
#endif
#ifndef OPOST
#define OPOST    0
#endif
#ifndef ONLCR
#define ONLCR    0
#endif
#endif

/**
 * Constructor
 */
Serial::Serial()
{
	m_nTimeout = 0;
	m_hPort = INVALID_HANDLE_VALUE;
	m_pszPort = NULL;
#ifndef _WIN32
	memset(&m_originalSettings, 0, sizeof(m_originalSettings));
#endif
}

/**
 * Destructor
 */
Serial::~Serial()
{
	close();
	safe_free(m_pszPort);
}

/**
 * Open serial device
 */
bool Serial::open(const TCHAR *pszPort)
{
	bool bRet = false;
	
	close();    // In case if port already open
	safe_free(m_pszPort);
	m_pszPort = _tcsdup(pszPort);
	
#ifdef _WIN32
	m_hPort = CreateFile(pszPort, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, 0, NULL);
	if (m_hPort != INVALID_HANDLE_VALUE)
	{
#else // UNIX
	m_hPort = ::_topen(pszPort, O_RDWR | O_NOCTTY | O_NDELAY); 
	if (m_hPort != -1)
	{
		tcgetattr(m_hPort, &m_originalSettings);
#endif
		set(38400, 8, NOPARITY, ONESTOPBIT, FLOW_NONE);
		bRet = true;
	}
	return bRet;
}

/**
 * Set communication parameters
 */
bool Serial::set(int nSpeed, int nDataBits, int nParity, int nStopBits, int nFlowControl)
{
	bool bRet = false;
		
	m_nDataBits = nDataBits;
	m_nSpeed = nSpeed;
	m_nParity = nParity;
	m_nStopBits = nStopBits;
	m_nFlowControl = nFlowControl;
	
#ifdef _WIN32
	DCB dcb;
		
	dcb.DCBlength = sizeof(DCB);
	if (GetCommState(m_hPort, &dcb))
	{
		dcb.BaudRate = nSpeed;
		dcb.ByteSize = nDataBits;
		dcb.Parity = nParity;
		dcb.StopBits = nStopBits;
		dcb.fDtrControl = DTR_CONTROL_ENABLE;
		dcb.fBinary = TRUE;
		dcb.fParity = FALSE;
		dcb.fOutxCtsFlow = FALSE;
		dcb.fOutxDsrFlow = FALSE;
		dcb.fDsrSensitivity = FALSE;
		dcb.fOutX = FALSE;
		dcb.fInX = FALSE;
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
		dcb.fAbortOnError = FALSE;
			
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
		
#else /* UNIX */
	struct termios newTio;
		
	tcgetattr(m_hPort, &newTio);
		
	newTio.c_cc[VMIN] = 1;
	newTio.c_cc[VTIME] = m_nTimeout;
		
	newTio.c_cflag |= CLOCAL | CREAD;
		
	int baud;
	switch(nSpeed)
	{
		case 50:     baud = B50;     break;
		case 75:     baud = B75;     break;
		case 110:    baud = B110;    break;
		case 134:    baud = B134;    break;
		case 150:    baud = B150;    break;
		case 200:    baud = B200;    break;
		case 300:    baud = B300;    break;
		case 600:    baud = B600;    break;
		case 1200:   baud = B1200;   break;
		case 1800:   baud = B1800;   break;
		case 2400:   baud = B2400;   break;
		case 4800:   baud = B4800;   break;
		case 9600:   baud = B9600;   break;
		case 19200:  baud = B19200;  break;
		case 38400:  baud = B38400;  break;
#ifdef B57600
		case 57600:  baud = B57600;  break;
#endif
#ifdef B115200
		case 115200: baud = B115200;  break;
#endif
#ifdef B230400
		case 230400: baud = B230400;  break;
#endif
#ifdef B460800
		case 460800: baud = B460800;  break;
#endif
#ifdef B500000
		case 500000: baud = B500000;  break;
#endif
#ifdef B576000
		case 576000: baud = B576000;  break;
#endif
#ifdef B921600
		case 921600: baud = B921600;  break;
#endif
		default:     
         return false;  // wrong/unsupported speed
	}
#ifdef _NETWARE
	cfsetispeed(&newTio, (speed_t)baud);
	cfsetospeed(&newTio, (speed_t)baud);
#else
	cfsetispeed(&newTio, baud);
	cfsetospeed(&newTio, baud);
#endif
		
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
		
	//newTio.c_cflag &= ~(CRTSCTS | IXON | IXOFF);
	newTio.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHOKE | ECHOCTL | ISIG | IEXTEN);
	newTio.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
	newTio.c_iflag |= IGNBRK;
	newTio.c_oflag &= ~(OPOST | ONLCR);
		
	switch(nFlowControl)
	{
		case FLOW_HARDWARE:
			newTio.c_cflag |= CRTSCTS;
			break;
		case FLOW_SOFTWARE:
			newTio.c_iflag |= IXON | IXOFF;
			break;
		default:
			break;
	}
		
	tcsetattr(m_hPort, TCSANOW, &newTio);
		
#endif // _WIN32
	return bRet;
}

/**
 * Close serial port
 */
void Serial::close()
{
	if (m_hPort != INVALID_HANDLE_VALUE)
	{
#ifdef _WIN32
		//PurgeComm(m_hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
		CloseHandle(m_hPort);
#else // UNIX
		tcsetattr(m_hPort, TCSANOW, &m_originalSettings);
		::close(m_hPort);
#endif // _WIN32s
		m_hPort = INVALID_HANDLE_VALUE;
	}
}

/**
 * Set receive timeout (in milliseconds)
 */
void Serial::setTimeout(int nTimeout)
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

/**
 * Restart port
 */
bool Serial::restart()
{
	if (m_pszPort == NULL)
		return false;

	close();
	ThreadSleepMs(500);
	
	TCHAR *temp = m_pszPort;
	m_pszPort = NULL;	// to prevent desctruction by open()
	if (open(temp))
		if (set(m_nSpeed, m_nDataBits, m_nParity, m_nStopBits, m_nFlowControl))
		{
			free(temp);
			return true;
		}
	free(temp);
	return false;
}

/**
 * Read character(s) from port
 */
int Serial::read(char *pBuff, int nSize)
{
	int nRet;
	
	memset(pBuff, 0, nSize);
	if (m_hPort == INVALID_HANDLE_VALUE)
		return -1;
	
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
		nRet = ::read(m_hPort, pBuff, nSize);
	else
		nRet = -1;  // Timeout is an error
	
#endif // _WIN32
	
	return nRet;
}

/**
 * Read character(s) from port
 */
int Serial::readAll(char *pBuff, int nSize)
{
	int nRet;
	
	memset(pBuff, 0, nSize);
	if (m_hPort == INVALID_HANDLE_VALUE)
		return -1;
	
#ifdef _WIN32
	DWORD dwBytes;

	if (ReadFile(m_hPort, pBuff, nSize, &dwBytes, NULL))
	{
		nRet = (int)dwBytes;
	}
	else
	{
		nRet = -1;
	}
	
#else // UNIX
	fd_set rdfs;
	struct timeval tv;
	int got, offset;

	offset = 0;

	while (offset < nSize)
	{
		FD_ZERO(&rdfs);
		FD_SET(m_hPort, &rdfs);
		tv.tv_sec = m_nTimeout / 10;
		tv.tv_usec = 0;
		nRet = select(m_hPort + 1, &rdfs, NULL, NULL, &tv);
		if (nRet > 0)
		{
			got = ::read(m_hPort, pBuff + offset, nSize - offset);
			if (got >= 0)
			{
				offset += got;
				nRet = offset;
			}
			else
			{
				nRet = -1;
				break;
			}
		}
		else
		{
			if (offset == 0) // got no data
			{
				nRet = -1;
			}
			break;
		}
	}
	
#endif // _WIN32
	
	return nRet;
}

/**
 * Write character(s) to port
 */
bool Serial::write(const char *pBuff, int nSize)
{
	bool bRet = false;
	
	if (m_hPort == INVALID_HANDLE_VALUE)
		return false;
	
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
		restart();
	}
	
#else // UNIX
	usleep(100);
	
	if (::write(m_hPort, (char *)pBuff, nSize) == nSize)
	{
		bRet = true;
	}
	else
	{
		restart();
	}
	
#endif // _WIN32
	
	return bRet;
}

/**
 * Flush buffer
 */
void Serial::flush()
{
#ifdef _WIN32
	
	FlushFileBuffers(m_hPort);
	
#else // UNIX
	
#ifndef _NETWARE
	tcflush(m_hPort, TCIOFLUSH);
#endif
	
#endif // _WIN32
}
