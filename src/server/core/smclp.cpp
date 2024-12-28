/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
SMCLP_Connection::SMCLP_Connection(UINT32 ip, WORD port)
{
	m_ip = ip;
	m_port = port;
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

   m_conn = TelnetConnection::createConnection(m_ip, m_port, m_timeout);
   if (m_conn != NULL)
   {
      char *_login = UTF8StringFromWideString(login);
      char *_password = UTF8StringFromWideString(password);

      if (m_conn->waitForText(":", m_timeout))
      {
         m_conn->writeLine(_login);
         if (m_conn->waitForText(":", m_timeout)) 
         {
            m_conn->writeLine(_password);
            if (m_conn->waitForText("->", m_timeout)) 
            {
               success = true;
            }
         }
      }

      MemFree(_login);
      MemFree(_password);
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
   char *_path = UTF8StringFromWideString(path);
   char buffer[1024];
   snprintf(buffer, 1024, "show -o format=text %s", _path);
   m_conn->writeLine(buffer);
   free(_path);

   while (m_conn->readLine(buffer, 1024, m_timeout / 10) > 0)
   {
      if ((strstr(buffer, "->") != NULL) && (strstr(buffer, "show -o format=text") == NULL))
      {
         break;
      }

      TCHAR *_buffer = WideStringFromUTF8String(buffer);

      TCHAR *ptr = _tcschr(_buffer, _T('='));
      if (ptr != NULL)
      {
         *ptr = 0;
         ptr++;
         Trim(_buffer);
         Trim(ptr);
         if (!_tcsicmp(_buffer, parameter))
         {
            ret = _tcsdup(ptr);
            break;
         }
      }

      free(_buffer);
   }

   return ret;
}

/**
 * Check connection
 */
bool SMCLP_Connection::checkConnection()
{
   if (!m_conn->writeLine("cd /"))
      return false;
   return m_conn->waitForText("->", m_timeout);
}
