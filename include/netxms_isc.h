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

#define ISC_SERVICE_EVENT_FORWARDER    ((UINT32)1)
#define ISC_SERVICE_OBJECT_SYNC        ((UINT32)2)
#define ISC_SERVICE_LICENSE_SERVER     ((UINT32)3)

#define ISC_SERVICE_CUSTOM_1           ((UINT32)100000)


//
// ISC error codes
//


#define ISC_ERR_SUCCESS                ((UINT32)0)
#define ISC_ERR_UNKNOWN_SERVICE        ((UINT32)1)
#define ISC_ERR_REQUEST_OUT_OF_STATE   ((UINT32)2)
#define ISC_ERR_SERVICE_DISABLED       ((UINT32)3)
#define ISC_ERR_ENCRYPTION_REQUIRED    ((UINT32)4)
#define ISC_ERR_CONNECTION_BROKEN      ((UINT32)5)
#define ISC_ERR_ALREADY_CONNECTED      ((UINT32)6)
#define ISC_ERR_SOCKET_ERROR           ((UINT32)7)
#define ISC_ERR_CONNECT_FAILED         ((UINT32)8)
#define ISC_ERR_INVALID_NXCP_VERSION   ((UINT32)9)
#define ISC_ERR_REQUEST_TIMEOUT        ((UINT32)10)
#define ISC_ERR_NOT_IMPLEMENTED        ((UINT32)11)
#define ISC_ERR_NO_CIPHERS             ((UINT32)12)
#define ISC_ERR_INVALID_PUBLIC_KEY     ((UINT32)13)
#define ISC_ERR_INVALID_SESSION_KEY    ((UINT32)14)
#define ISC_ERR_INTERNAL_ERROR         ((UINT32)15)
#define ISC_ERR_SESSION_SETUP_FAILED   ((UINT32)16)
#define ISC_ERR_OBJECT_NOT_FOUND       ((UINT32)17)
#define ISC_ERR_POST_EVENT_FAILED      ((UINT32)18)


//
// ISC session
//

class ISCSession
{
private:
	SOCKET m_socket;
	UINT32 m_peerAddress;	// Peer address in host byte order
	void *m_userData;

public:
	ISCSession(SOCKET sock, struct sockaddr_in *addr)
	{
		m_socket = sock;
		m_peerAddress = ntohl(addr->sin_addr.s_addr);
		m_userData = NULL;
	}

	SOCKET GetSocket() { return m_socket; }
	UINT32 GetPeerAddress() { return m_peerAddress; }

	void SetUserData(void *data) { m_userData = data; }
	void *GetUserData() { return m_userData; }
};


//
// ISC service definition
//

typedef struct
{
	UINT32 id;								// Service ID
	const TCHAR *name;					// Name
	const TCHAR *enableParameter;		// Server parameter to be set to enable service
	BOOL (*setupSession)(ISCSession *, CSCPMessage *);  // Session setup handler
	void (*closeSession)(ISCSession *);          // Session close handler
	BOOL (*processMsg)(ISCSession *, CSCPMessage *, CSCPMessage *);
} ISC_SERVICE;


#endif
