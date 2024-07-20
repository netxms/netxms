/* 
** NetXMS - Network Management System
** Copyright (C) 2005-2023 Raden Solutions
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

/**
 * Constructor
 */
Serial::Serial()
{
	m_timeout = 5000;
	m_handle = INVALID_HANDLE_VALUE;
	m_device = nullptr;
	m_speed = 9600;
	m_dataBits = 8;
	m_parity = NOPARITY;
	m_stopBits = ONESTOPBIT;
	m_flowControl = FLOW_NONE;
   m_writeDelay = 100;
   m_writeBlockSize = -1;
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
	MemFree(m_device);
}

/**
 * Open serial device
 */
bool Serial::open(const TCHAR *device)
{
	bool bRet = false;

	close();    // In case if port already open
	MemFree(m_device);
	m_device = MemCopyString(device);

#ifdef _WIN32
	m_handle = CreateFile(m_device, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (m_handle != INVALID_HANDLE_VALUE)
	{
#else // UNIX
	m_handle = ::_topen(m_device, O_RDWR | O_NOCTTY | O_NDELAY);
	if (m_handle != -1)
	{
		tcgetattr(m_handle, &m_originalSettings);
#endif
		set(38400, 8, NOPARITY, ONESTOPBIT, FLOW_NONE);
		bRet = true;
	}
	return bRet;
}

/**
 * Set communication parameters
 */
bool Serial::set(int speed, int dataBits, int parity, int stopBits, int flowControl)
{
	bool success = false;
		
	m_dataBits = dataBits;
	m_speed = speed;
	m_parity = parity;
	m_stopBits = stopBits;
	m_flowControl = flowControl;

#ifdef _WIN32
	DCB dcb;

	dcb.DCBlength = sizeof(DCB);
	if (GetCommState(m_handle, &dcb))
	{
		dcb.BaudRate = speed;
		dcb.ByteSize = dataBits;
		dcb.Parity = parity;
		dcb.StopBits = stopBits;
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

		if (SetCommState(m_handle, &dcb))
		{
			COMMTIMEOUTS ct;
			memset(&ct, 0, sizeof(COMMTIMEOUTS));
			ct.ReadTotalTimeoutConstant = m_timeout;
			ct.WriteTotalTimeoutConstant = m_timeout;
			SetCommTimeouts(m_handle, &ct);
			success = true;
		}
	}

#else /* UNIX */
	struct termios newTio;

	tcgetattr(m_handle, &newTio);

	newTio.c_cc[VMIN] = 1;
	newTio.c_cc[VTIME] = m_timeout / 100; // convert to deciseconds (VTIME in 1/10sec)

	newTio.c_cflag |= CLOCAL | CREAD;

	int baud;
	switch(speed)
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
	cfsetispeed(&newTio, baud);
	cfsetospeed(&newTio, baud);

	newTio.c_cflag &= ~(CSIZE);
	switch(dataBits)
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
	switch(parity)
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
	if (stopBits != ONESTOPBIT)
	{
		newTio.c_cflag |= CSTOPB;
	}

	//newTio.c_cflag &= ~(CRTSCTS | IXON | IXOFF);
	newTio.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHOKE | ECHOCTL | ISIG | IEXTEN);
	newTio.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
	newTio.c_iflag |= IGNBRK;
	newTio.c_oflag &= ~(OPOST | ONLCR);

	switch(flowControl)
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

	success = (tcsetattr(m_handle, TCSANOW, &newTio) == 0);

#endif // _WIN32
	return success;
}

/**
 * Close serial port
 */
void Serial::close()
{
	if (m_handle != INVALID_HANDLE_VALUE)
	{
#ifdef _WIN32
		//PurgeComm(m_handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
		CloseHandle(m_handle);
#else // UNIX
		tcsetattr(m_handle, TCSANOW, &m_originalSettings);
		::close(m_handle);
#endif // _WIN32s
		m_handle = INVALID_HANDLE_VALUE;
	}
}

/**
 * Set receive timeout (in milliseconds)
 */
void Serial::setTimeout(uint32_t timeout)
{
#ifdef _WIN32
	COMMTIMEOUTS ct;
	
	m_timeout = timeout;
	memset(&ct, 0, sizeof(COMMTIMEOUTS));
	ct.ReadTotalTimeoutConstant = m_timeout;
	ct.WriteTotalTimeoutConstant = m_timeout;
	
	SetCommTimeouts(m_handle, &ct);
#else
	struct termios tio;

	tcgetattr(m_handle, &tio);

	m_timeout = timeout;
	tio.c_cc[VTIME] = timeout / 100; // convert to deciseconds (VTIME in 1/10sec)

   tcsetattr(m_handle, TCSANOW, &tio);
#endif // WIN32
}

/**
 * Restart port
 */
bool Serial::restart()
{
	if (m_device == nullptr)
		return false;

	close();
	ThreadSleepMs(500);

	TCHAR *device = m_device;
	m_device = nullptr;	// to prevent destruction by open()
   int speed = m_speed;
   int dataBits = m_dataBits;
   int parity = m_parity;
   int stopBits = m_stopBits;
   int flowControl = m_flowControl;
	if (open(device))
   {
		if (set(speed, dataBits, parity, stopBits, flowControl))
		{
         setTimeout(m_timeout);
			MemFree(device);
			return true;
		}
   }
	MemFree(device);
	return false;
}

/**
 * Read character(s) from port
 */
ssize_t Serial::read(void *buffer, size_t size)
{
	memset(buffer, 0, size);
	if (m_handle == INVALID_HANDLE_VALUE)
		return -1;
	
   ssize_t nRet;

#ifdef _WIN32
   // Read one byte
	DWORD nDone;
	if (ReadFile(m_handle, buffer, 1, &nDone, nullptr))
	{
		nRet = static_cast<ssize_t>(nDone);

      COMSTAT stat;
      if (ClearCommError(m_handle, nullptr, &stat))
      {
         if (stat.cbInQue > 0)
         {
            // Read rest of buffered data
            if (ReadFile(m_handle, static_cast<char*>(buffer) + 1, std::min(stat.cbInQue, static_cast<DWORD>(size) - 1), &nDone, nullptr))
            {
               nRet += static_cast<ssize_t>(nDone);
            }
         }
      }
	}
	else
	{
		nRet = -1;
	}

#else // UNIX
	SocketPoller sp;
	sp.add(m_handle);
	if (sp.poll(m_timeout) > 0)
	{
	   do
	   {
	      nRet = ::read(m_handle, buffer, size);
	   }
	   while((nRet == -1) && (errno == EAGAIN));
	}
	else
	{
		nRet = -1;  // Timeout is an error
	}

#endif // _WIN32
	
	return nRet;
}

/**
 * Read character(s) from port
 */
ssize_t Serial::readAll(void *buffer, size_t size)
{
	memset(buffer, 0, size);
	if (m_handle == INVALID_HANDLE_VALUE)
		return -1;
	
	ssize_t nRet = -1;

#ifdef _WIN32
	DWORD bytes;
	if (ReadFile(m_handle, buffer, static_cast<DWORD>(size), &bytes, nullptr))
	{
		nRet = static_cast<ssize_t>(bytes);
	}
#else // UNIX
	SocketPoller sp;
	size_t offset = 0;
	while(offset < size)
	{
	   sp.reset();
		sp.add(m_handle);
		nRet = sp.poll(m_timeout);
		if (nRet > 0)
		{
			int got = ::read(m_handle, static_cast<BYTE*>(buffer) + offset, size - offset);
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
 * Read data up to one of given marks
 */
ssize_t Serial::readToMark(char *buffer, size_t size, const char **marks, char **occurence)
{
   if (size == 0)
      return 0;

   char *curr = buffer;
   size_t sizeLeft = size - 1;
   ssize_t totalBytesRead = 0;
   *occurence = nullptr;

   while(sizeLeft > 0)
   {
      ssize_t bytesRead = read(curr, sizeLeft);
      if (bytesRead <= 0)
         return bytesRead;

      totalBytesRead += bytesRead;
      curr += bytesRead;
      sizeLeft -= bytesRead;
      *curr = 0;
      for(int i = 0; marks[i] != nullptr; i++)
      {
         char *mark = strstr(buffer, marks[i]);
         if (mark != nullptr)
         {
            *occurence = mark;
            return totalBytesRead;
         }
      }
   }

   return totalBytesRead;
}

/**
 * Write character(s) to port
 */
bool Serial::writeBlock(const void *data, size_t size)
{
	if (m_handle == INVALID_HANDLE_VALUE)
		return false;
	
   ThreadSleepMs(m_writeDelay);

   bool success = false;
#ifdef _WIN32
	
	DWORD nDone;
	if (WriteFile(m_handle, data, static_cast<DWORD>(size), &nDone, nullptr) != 0)
	{
		if (nDone == static_cast<DWORD>(size))
		{
		   success = true;
		}
	}
	else
	{
		restart();
	}
	
#else // UNIX

	if (::write(m_handle, data, size) == static_cast<ssize_t>(size))
	{
	   success = true;
	}
	else
	{
		restart();
	}
	
#endif // _WIN32
	
	return success;
}

/**
 * Write character(s) to port
 */
bool Serial::write(const void *data, size_t size)
{
   if (m_writeBlockSize > 0)
   {
      size_t pos = 0;
      while(pos < size)
      {
         size_t bs = std::min(m_writeBlockSize, size - pos);
         if (!writeBlock(static_cast<const BYTE*>(data) + pos, bs))
            return false;
         pos += bs;
      }
      return true;
   }
   else
   {
      return writeBlock(data, size);
   }
}

/**
 * Flush buffer
 */
void Serial::flush()
{
#ifdef _WIN32
   FlushFileBuffers(m_handle);
#else
   tcflush(m_handle, TCIOFLUSH);
#endif
}
