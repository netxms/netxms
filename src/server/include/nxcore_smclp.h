/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2009 Victor Kirhenshtein
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
** File: nxcore_smclp.h
**
**/

#ifndef _nxcore_smclp_h_
#define _nxcore_smclp_h_

#define SMCLP_TELNET		0
#define SMCLP_SSH			1

/**
 * SM CLP connection class
 */
class NXCORE_EXPORTABLE SMCLP_Connection
{
private:
	UINT32 m_ip;
	WORD m_port;
	UINT32 m_timeout;
	TelnetConnection *m_conn;

public:
	SMCLP_Connection(UINT32 ip, WORD port);
	~SMCLP_Connection();

	bool connect(const TCHAR *login, const TCHAR *password);
	void disconnect();

   TCHAR *get(const TCHAR *path, const TCHAR *parameter);
   bool checkConnection();
};

#endif
