/*
** NetXMS - Network Management System
** Copyright (C) 2018-2019 RadenSolutions
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
** File: server_console.h
**
**/

#ifndef _server_console_h_
#define _server_console_h_

#include <nms_common.h>
#include <nxcpapi.h>
#include <nxsrvapi.h>

/**
 * Abstract console
 */
class LIBNXSRV_EXPORTABLE ServerConsole
{
protected:
   bool m_remote;

   virtual void write(const TCHAR *text) = 0;

public:
   ServerConsole();
   virtual ~ServerConsole();

   bool isRemote() { return m_remote; }

   void print(const TCHAR *text) { write(text); }
   void printf(const TCHAR *format, ...);
   void vprintf(const TCHAR *format, va_list args);
};

/**
 * Local terminal console
 */
class LIBNXSRV_EXPORTABLE LocalTerminalConsole : public ServerConsole
{
protected:
   virtual void write(const TCHAR *text) override;

public:
   LocalTerminalConsole();
   virtual ~LocalTerminalConsole();
};

/**
 * String buffer console
 */
class LIBNXSRV_EXPORTABLE StringBufferConsole : public ServerConsole
{
private:
   String m_buffer;
   MUTEX m_mutex;

protected:
   virtual void write(const TCHAR *text) override;

public:
   StringBufferConsole();
   virtual ~StringBufferConsole();

   const String& getOutput() const { return m_buffer; }
};

/**
 * Socket console
 */
class LIBNXSRV_EXPORTABLE SocketConsole : public ServerConsole
{
private:
   SOCKET m_socket;
   MUTEX m_mutex;
   UINT16 m_messageCode;

protected:
   virtual void write(const TCHAR *text) override;

public:
   SocketConsole(SOCKET s, UINT16 msgCode = CMD_ADM_MESSAGE);
   virtual ~SocketConsole();

   MUTEX getMutex() { return m_mutex; }
};

typedef ServerConsole* CONSOLE_CTX;

/**
 * Wrapper function for backward compatibility
 */
inline void ConsolePrintf(CONSOLE_CTX console, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   console->vprintf(format, args);
   va_end(args);
}

/**
 * Wrapper function for backward compatibility
 */
inline void ConsoleWrite(CONSOLE_CTX console, const TCHAR *text)
{
   console->print(text);
}

#endif   /* _nxcore_console_h_ */
