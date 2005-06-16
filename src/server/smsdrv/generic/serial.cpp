/* $Id: serial.cpp,v 1.1 2005-06-16 13:19:38 alk Exp $ */

#include "main.h"

using namespace std;

Serial::Serial(void)
{
	m_nTimeout = 0;
	m_hPort = INVALID_HANDLE_VALUE;
}

Serial::~Serial(void)
{
}

bool Serial::Open(std::string sPort)
{
	bool bRet = false;

	m_sPort = sPort;

#ifdef _WIN32
	m_hPort = CreateFile(sPort.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, 0, NULL);
	if (m_hPort != INVALID_HANDLE_VALUE)
	{
		Set(CBR_38400, 8, NOPARITY, ONESTOPBIT);

#ifdef _WIN32
		COMMTIMEOUTS ct;

		memset(&ct, 0, sizeof(COMMTIMEOUTS));
		ct.ReadTotalTimeoutConstant = m_nTimeout;
		ct.WriteTotalTimeoutConstant = m_nTimeout;

		SetCommTimeouts(m_hPort, &ct);
#endif

		bRet = true;
	}
#else // UNIX
	struct termios newTio;

	m_hPort = open(sPort.c_str(), O_RDWR | O_NOCTTY); 
	if (m_hPort < 0)
	{
		return false;
	}

	memset(&newTio, 0, sizeof(newTio));

	// control modes
	newTio.c_cflag = B38400 | CLOCAL | CREAD;
	newTio.c_cflag |= CS8;

	// input modes
	newTio.c_iflag = IGNPAR | IGNBRK;

	// output modes
	newTio.c_oflag = 0;

	// local modes
	newTio.c_lflag = 0;

	// control chars
	newTio.c_cc[VTIME] = m_nTimeout;
	newTio.c_cc[VMIN] = 0;

	// JIC
	newTio.c_lflag &= ~ICANON;

	tcflush(m_hPort, TCIFLUSH);
	if (tcsetattr(m_hPort, TCSANOW, &newTio) == -1)
	{
		close(m_hPort);
		m_hPort = -1;
		return false;
	}

	bRet = true;
#endif // _WIN32

	return bRet;
}

bool Serial::Set(int nSpeed, int nDataBits, int nParity, int nStopBits)
{
	bool bRet = false;

	m_nDataBits = nDataBits;
	m_nSpeed = nSpeed;
	m_nParity = nParity;
	m_nStopBits = nStopBits;

#ifdef _WIN32
	DCB dcb;

	if (GetCommState(m_hPort, &dcb))
	{
		dcb.BaudRate = nSpeed;
		dcb.ByteSize = nDataBits;
		dcb.Parity = nParity;
		dcb.StopBits = nStopBits;
		dcb.fDtrControl = DTR_CONTROL_DISABLE;
		dcb.fParity = FALSE;
		dcb.fOutxCtsFlow = FALSE;
		dcb.fOutxDsrFlow = FALSE;
		dcb.fDsrSensitivity = FALSE;
		dcb.fOutX = FALSE;
		dcb.fInX = FALSE;
		dcb.fRtsControl = RTS_CONTROL_DISABLE;

		if (SetCommState(m_hPort, &dcb))
		{
			bRet = true;
		}
	}

#else // UNIX
	struct termios newTio;

	tcgetattr(m_hPort, &newTio);

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
		case 57600:  newTio.c_cflag |= B57600;  break;
		case 115200: newTio.c_cflag |= B115200; break;

		default:     newTio.c_cflag |= B38400;  break;
	}

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

	if (nStopBits != ONESTOPBIT)
	{
		newTio.c_cflag |= CSTOPB;
	}

	/*switch (nFlowControl)
	{
		case CTSRTS: // hardware, set on *CONTROL*
			newTio.c_cflag |= CRTSCTS;
			break;
		case XONXOFF: // software, set on *INPUT*
			newTio.c_iflag |= IXON | IXOFF;
			break;
		default: // none
			break;
	} */

	tcsetattr(m_hPort, TCSANOW, &newTio);

#endif // _WIN32

	return bRet;
}

void Serial::Close(void)
{
#ifdef _WIN32
	CloseHandle(m_hPort);

#else // UNIX
	close(m_hPort);
#endif // _WIN32s
}

void Serial::SetTimeout(int nTimeout) // milliseconds
{
#ifdef _WIN32
	m_nTimeout = nTimeout * 1000; // usec to sec
#else
	m_nTimeout = nTimeout * 10; // sec (VTIME in 1/10sec)
#endif // WIN32
}

bool Serial::Restart(void)
{
	Close();
	Open(m_sPort);
	Set(m_nSpeed, m_nDataBits, m_nParity, m_nStopBits);

	return true;
}

bool Serial::Read(char *pBuff, int nSize)
{
	bool bRet = false;

	memset(pBuff, 0, nSize);

#ifdef _WIN32

	Sleep(100);

	DWORD nDone;
	if (ReadFile(m_hPort, pBuff, nSize, &nDone, NULL) != 0)
	{
		if (nDone == nSize)
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
	int nDone = 0;
	int nGot = 0;

	do
	{
		if ((nGot = read(m_hPort, pBuff + nDone, nSize - nDone)) <= 0)
		{
			break;
		}

		nDone += nGot;
	} while(nDone != nSize);

	if (nDone == nSize)
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

bool Serial::Write(char *pBuff, int nSize)
{
	bool bRet = false;

#ifdef _WIN32

	Sleep(100);

	DWORD nDone;
	if (WriteFile(m_hPort, pBuff, nSize, &nDone, NULL) != 0)
	{
		if (nDone == nSize)
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

void Serial::Flush(void)
{
#ifdef _WIN32

	FlushFileBuffers(m_hPort);

#else // UNIX

	tcflush(m_hPort, TCIOFLUSH);

#endif // _WIN32
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $

*/
