/* $Id: serial.h,v 1.1 2005-06-16 13:19:38 alk Exp $ */

#ifndef __SERIAL_H__
#define __SERIAL_H__

#ifndef _WIN32
enum
{
	NOPARITY,
	ODDPARITY,
	EVENPARITY,
	ONESTOPBIT,
	TWOSTOPBITS
};

# define INVALID_HANDLE_VALUE (-1)
#endif

class Serial
{
public:
	Serial(void);
	~Serial(void);

	bool Open(std::string sPort);
	void Close(void);
	void SetTimeout(int nTimeout);
	bool Read(char *pBuff, int nSize);
	bool Write(char *pBuff, int nSize);
	void Flush(void);
	bool Set(int nSpeed, int nDataBits, int nParity, int nStopBits);
	bool Restart(void);

private:
	std::string m_sPort;
	int m_nTimeout;
	int m_nSpeed;
	int m_nDataBits;
	int m_nStopBits;
	int m_nParity;

#ifndef _WIN32
	int m_hPort;
#else
	HANDLE m_hPort;
#endif
};

#endif // __SERIAL_H__

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $

*/
