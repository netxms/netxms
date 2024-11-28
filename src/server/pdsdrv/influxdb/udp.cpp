/**
 ** NetXMS - Network Management System
 ** Performance Data Storage Driver for InfluxDB
 ** Copyright (C) 2019 Sebastian YEPES FERNANDEZ & Julien DERIVIERE
 ** Copyright (C) 2021-2024 Raden Solutions
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
 **              ,.-----__
 **          ,:::://///,:::-.
 **         /:''/////// ``:::`;/|/
 **        /'   ||||||     :://'`\
 **      .' ,   ||||||     `/(  e \
 **-===~__-'\__X_`````\_____/~`-._ `.
 **            ~~        ~~       `~-'
 ** Armadillo, The Metric Eater - https://www.nationalgeographic.com/animals/mammals/group/armadillos/
 **
 ** File: udp.cpp
 **/

#include "influxdb.h"

/**
 * Constructor for UDP sender
 */
UDPSender::UDPSender(const Config& config) : InfluxDBSender(config)
{
   if (m_queueSizeLimit > 64000)
      m_queueSizeLimit = 64000;  // Cannot send more than 64K over UDP
   if (m_port == 0)
      m_port = 8189;
   createSocket();
}

/**
 * Destructor for UDP sender
 */
UDPSender::~UDPSender()
{
   if (m_socket != INVALID_SOCKET)
      closesocket(m_socket);
}

/**
 * Create socket
 */
void UDPSender::createSocket()
{
   InetAddress addr = InetAddress::resolveHostName(m_hostname);
   if (addr.isValidUnicast() || addr.isLoopback())
   {
      m_socket = ConnectToHostUDP(addr, m_port);
      if (m_socket == INVALID_SOCKET)
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot create UDP socket"));
      }
   }
   else
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Invalid host name %s"), m_hostname.cstr());
      m_socket = INVALID_SOCKET;
   }
}

/**
 * Send data block over UDP
 */
bool UDPSender::send(const char *data, size_t size)
{
   if (m_socket == INVALID_SOCKET)
   {
      time_t now = time(nullptr);
      if (m_lastConnect + 60 > now) // Attempt to re-create socket once per minute
         return false;
      createSocket();
      m_lastConnect = now;
      if (m_socket == INVALID_SOCKET)
         return false;
   }

   if (SendEx(m_socket, data, size, 0, nullptr) <= 0)
   {
      closesocket(m_socket);
      m_socket = INVALID_SOCKET;
      return false;
   }
   return true;
}
