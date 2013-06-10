/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: netxms_isc.h
**
**/

#ifndef _netxms_isc_h_
#define _netxms_isc_h_


//
// Default port number
//

#define NETXMS_ISC_PORT        4702


//
// Well-known ISC services
//

#define ISC_SERVICE_EVENT_FORWARDER    ((DWORD)1)
#define ISC_SERVICE_OBJECT_SYNC        ((DWORD)2)
#define ISC_SERVICE_LICENSE_SERVER     ((DWORD)3)

#define ISC_SERVICE_CUSTOM_1           ((DWORD)100000)


//
// ISC error codes
//


#define ISC_ERR_SUCCESS                ((DWORD)0)
#define ISC_ERR_UNKNOWN_SERVICE        ((DWORD)1)
#define ISC_ERR_REQUEST_OUT_OF_STATE   ((DWORD)2)
#define ISC_ERR_SERVICE_DISABLED       ((DWORD)3)
#define ISC_ERR_ENCRYPTION_REQUIRED    ((DWORD)4)
#define ISC_ERR_CONNECTION_BROKEN      ((DWORD)5)
#define ISC_ERR_ALREADY_CONNECTED      ((DWORD)6)
#define ISC_ERR_SOCKET_ERROR           ((DWORD)7)
#define ISC_ERR_CONNECT_FAILED         ((DWORD)8)
#define ISC_ERR_INVALID_NXCP_VERSION   ((DWORD)9)
#define ISC_ERR_REQUEST_TIMEOUT        ((DWORD)10)
#define ISC_ERR_NOT_IMPLEMENTED        ((DWORD)11)
#define ISC_ERR_NO_CIPHERS             ((DWORD)12)
#define ISC_ERR_INVALID_PUBLIC_KEY     ((DWORD)13)
#define ISC_ERR_INVALID_SESSION_KEY    ((DWORD)14)
#define ISC_ERR_INTERNAL_ERROR         ((DWORD)15)
#define ISC_ERR_SESSION_SETUP_FAILED   ((DWORD)16)
#define ISC_ERR_OBJECT_NOT_FOUND       ((DWORD)17)
#define ISC_ERR_POST_EVENT_FAILED      ((DWORD)18)


//
// ISC session
//

class ISCSession
{
private:
	SOCKET m_socket;
	DWORD m_peerAddress;	// Peer address in host byte order
	void *m_userData;

public:
	ISCSession(SOCKET sock, struct sockaddr_in *addr)
	{
		m_socket = sock;
		m_peerAddress = ntohl(addr->sin_addr.s_addr);
		m_userData = NULL;
	}

	SOCKET GetSocket() { return m_socket; }
	DWORD GetPeerAddress() { return m_peerAddress; }

	void SetUserData(void *data) { m_userData = data; }
	void *GetUserData() { return m_userData; }
};


//
// ISC service definition
//

typedef struct
{
	DWORD id;								// Service ID
	const TCHAR *name;					// Name
	const TCHAR *enableParameter;		// Server parameter to be set to enable service
	BOOL (*setupSession)(ISCSession *, CSCPMessage *);  // Session setup handler
	void (*closeSession)(ISCSession *);          // Session close handler
	BOOL (*processMsg)(ISCSession *, CSCPMessage *, CSCPMessage *);
} ISC_SERVICE;


#endif
