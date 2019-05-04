/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: socket_listener.h
**
**/

#ifndef _socket_listener_h_
#define _socket_listener_h_

#include <nms_util.h>

/**
 * Connection processing result
 */
enum ConnectionProcessingResult
{
   CPR_COMPLETED = 0,
   CPR_BACKGROUND = 1
};

/**
 * Maximum length of listener name
 */
#define MAX_LISTENER_NAME_LEN    64

/**
 * Generic socket listener
 */
class LIBNETXMS_EXPORTABLE GenericSocketListener
{
private:
   UINT16 m_port;
   TCHAR *m_listenAddress;
   bool m_allowV4;
   bool m_allowV6;

protected:
   SOCKET m_socketV4;
   SOCKET m_socketV6;
   bool m_stop;
   TCHAR m_name[MAX_LISTENER_NAME_LEN];
   UINT32 m_acceptErrors;
   UINT32 m_acceptedConnections;
   UINT32 m_rejectedConnections;
   int m_type;

   virtual bool isStopConditionReached();

public:
   GenericSocketListener(int type, UINT16 port, bool allowV4, bool allowV6);
   virtual ~GenericSocketListener();

   void enableIPv4(bool enabled) { m_allowV4 = enabled; }
   void enableIPv6(bool enabled) { m_allowV6 = enabled; }
   void setName(const TCHAR *name) { _tcslcpy(m_name, name, MAX_LISTENER_NAME_LEN); }
   void setListenAddress(const TCHAR *addr) { MemFree(m_listenAddress); m_listenAddress = MemCopyString(addr); }

   UINT32 getAcceptErrors() const { return m_acceptErrors; }
   UINT32 getAcceptedConnections() const { return m_acceptedConnections; }
   UINT32 getRejectedConnections() const { return m_rejectedConnections; }

   bool initialize();
   virtual void mainLoop() = 0;
   void shutdown();
};

/**
 * Stream (TCP) socket listener
 */
class LIBNETXMS_EXPORTABLE StreamSocketListener : public GenericSocketListener
{
protected:
   virtual bool isConnectionAllowed(const InetAddress& peer);
   virtual ConnectionProcessingResult processConnection(SOCKET s, const InetAddress& peer) = 0;

public:
   StreamSocketListener(UINT16 port, bool allowV4 = true, bool allowV6 = true) : GenericSocketListener(SOCK_STREAM, port, allowV4, allowV6) { }

   virtual void mainLoop() override;
};

/**
 * Datagram (UDP) socket listener
 */
class LIBNETXMS_EXPORTABLE DatagramSocketListener : public GenericSocketListener
{
protected:
   virtual ConnectionProcessingResult processDatagram(SOCKET s) = 0;

public:
   DatagramSocketListener(UINT16 port, bool allowV4 = true, bool allowV6 = true) : GenericSocketListener(SOCK_DGRAM, port, allowV4, allowV6) { }

   virtual void mainLoop() override;
};

#endif   /* _socket_listener_h_ */
