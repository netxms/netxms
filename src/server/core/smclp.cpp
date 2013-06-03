/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: smclp.cpp
**
**/

#include "nxcore.h"


/**
 * Constructor
 */
SMCLP_Connection::SMCLP_Connection(DWORD ip, WORD port)
{
	m_ip = ip;
	m_port = htons(port);
	m_timeout = 5000;
	m_conn = NULL;
}

/**
 * Destructor
 */
SMCLP_Connection::~SMCLP_Connection()
{
	disconnect();
}

/**
 * Connect to target
 */
bool SMCLP_Connection::connect(const TCHAR *login, const TCHAR *password)
{
	bool success = false;

	if (m_conn != NULL)
   {
		delete m_conn;
   }

   TelnetConnection *m_conn = new TelnetConnection();
   if (m_conn->connect(htonl(m_ip), m_port, m_timeout))
   {
#ifdef UNICODE
      char *_login = UTF8StringFromWideString(login);
      char *_password = UTF8StringFromWideString(password);
#else
      char *_login = login;
      char *_password = password;
#endif

      if (m_conn->waitForText(":", m_timeout))
      {
         m_conn->writeLine(_login);
         if (m_conn->waitForText(":", m_timeout)) {
            m_conn->writeLine(_password);
            if (m_conn->waitForText("iLO->", m_timeout)) {
               success = true;
            }
         }
      }

#ifdef UNICODE
      safe_free(_login);
      safe_free(_password);
#endif
   }


   return success;
}


/**
 * Disconnect from target
 */
void SMCLP_Connection::disconnect()
{
	if (m_conn != NULL)
	{
      m_conn->writeLine("quit");
		m_conn->disconnect();
      delete_and_null(m_conn);
	}
}


/**
 * Get parameter from target
 */
TCHAR *SMCLP_Connection::get(const TCHAR *path, const TCHAR *parameter)
{
   TCHAR *ret = NULL;
#ifdef UNICODE
   char *_path = UTF8StringFromWideString(path);
#else
   const char *_path = path;
#endif
   char buffer[1024];
   snprintf(buffer, 1024, "show -o format=text %s", path);
   m_conn->writeLine(buffer);
#ifdef UNICODE
   free(_path);
#endif

   while (m_conn->readLine(buffer, 1024, m_timeout / 10) > 0)
   {
      if (strstr(buffer, "iLO->") != NULL)
      {
         break;
      }

#ifdef UNICODE
      TCHAR _buffer;
      char *_buffer = WideStringFromUTF8String(buffer);
#else
      char *_buffer = buffer;
#endif

      StrStripA(_buffer);
      int numStrings = 0;
      TCHAR **splitted = SplitString(_buffer, _T('='), &numStrings);
      if (numStrings == 2 && !_tcsicmp(splitted[0], parameter))
      {
         ret = _tcsdup(splitted[1]);
         break;
      }

#ifdef UNICODE
      free(_buffer);
#endif
   }

   return ret;
}
