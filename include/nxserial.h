/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: nxserial.h
**
**/

#ifndef _nxserial_h_
#define _nxserial_h_

#include <nms_util.h>

/*** Serial communications ***/
#ifdef _WIN32

#define FLOW_NONE       0
#define FLOW_SOFTWARE   1
#define FLOW_HARDWARE   2

#else    /* _WIN32 */

#include <termios.h>

enum
{
	NOPARITY,
	ODDPARITY,
	EVENPARITY,
	ONESTOPBIT,
	TWOSTOPBITS
};

enum
{
	FLOW_NONE,
	FLOW_HARDWARE,
	FLOW_SOFTWARE
};

#endif   /* _WIN32 */

/**
 * Class for serial communications
 */
class LIBNETXMS_EXPORTABLE Serial
{
private:
	TCHAR *m_device;
	uint32_t m_timeout;
	int m_speed;
	int m_dataBits;
	int m_stopBits;
	int m_parity;
	int m_flowControl;
   uint32_t m_writeDelay;
	size_t m_writeBlockSize;

#ifndef _WIN32
	int m_handle;
	struct termios m_originalSettings;
#else
	HANDLE m_handle;
#endif

   bool writeBlock(const void *data, size_t size);

public:
	Serial();
	~Serial();

	bool open(const TCHAR *device);
	void close();

	void setTimeout(uint32_t timeout);
   bool set(int speed, int dataBits = 8, int parity = NOPARITY, int stopBits = ONESTOPBIT, int flowControl = FLOW_NONE);
   void setWriteBlockSize(size_t bs) { m_writeBlockSize = bs; }
   void setWriteDelay(uint32_t delay) { m_writeDelay = delay; }

	ssize_t read(void *buffer, size_t size); /* waits up to timeout and do single read */
	ssize_t readAll(void *buffer, size_t size); /* read until timeout or out of space */
	ssize_t readToMark(char *buffer, size_t size, const char **marks, char **occurence);
	bool write(const void *buffer, size_t size);
	void flush();
	bool restart();
};

#endif   /* _nxserial_h_ */
