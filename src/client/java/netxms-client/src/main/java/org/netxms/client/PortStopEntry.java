/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client;

import org.netxms.base.NXCPMessage;

/**
 * Entry in port stop list. Defines a port that should be blocked from access.
 */
public class PortStopEntry
{
   public static final char PROTOCOL_TCP = 'T';
   public static final char PROTOCOL_UDP = 'U';
   public static final char PROTOCOL_BOTH = 'B';

   private int port;
   private char protocol;

   /**
    * Create new port stop entry.
    *
    * @param port port number (1-65535)
    * @param protocol protocol ('T' = TCP, 'U' = UDP, 'B' = Both)
    */
   public PortStopEntry(int port, char protocol)
   {
      this.port = port;
      this.protocol = protocol;
   }

   /**
    * Create entry from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base variable ID
    */
   public PortStopEntry(NXCPMessage msg, long baseId)
   {
      port = msg.getFieldAsInt32(baseId);
      protocol = (char)msg.getFieldAsInt16(baseId + 1);
   }

   /**
    * Fill NXCP message with entry data.
    *
    * @param msg NXCP message
    * @param baseId base variable ID
    */
   public void fillMessage(NXCPMessage msg, long baseId)
   {
      msg.setFieldInt32(baseId, port);
      msg.setFieldInt16(baseId + 1, protocol);
   }

   /**
    * Get port number.
    *
    * @return port number
    */
   public int getPort()
   {
      return port;
   }

   /**
    * Set port number.
    *
    * @param port port number (1-65535)
    */
   public void setPort(int port)
   {
      this.port = port;
   }

   /**
    * Get protocol.
    *
    * @return protocol character ('T', 'U', or 'B')
    */
   public char getProtocol()
   {
      return protocol;
   }

   /**
    * Set protocol.
    *
    * @param protocol protocol character ('T' = TCP, 'U' = UDP, 'B' = Both)
    */
   public void setProtocol(char protocol)
   {
      this.protocol = protocol;
   }

   /**
    * Check if TCP is blocked.
    *
    * @return true if TCP is blocked
    */
   public boolean isTcpBlocked()
   {
      return protocol == PROTOCOL_TCP || protocol == PROTOCOL_BOTH;
   }

   /**
    * Check if UDP is blocked.
    *
    * @return true if UDP is blocked
    */
   public boolean isUdpBlocked()
   {
      return protocol == PROTOCOL_UDP || protocol == PROTOCOL_BOTH;
   }

   /**
    * Get protocol as human-readable string.
    *
    * @return protocol string ("TCP", "UDP", or "TCP/UDP")
    */
   public String getProtocolString()
   {
      switch(protocol)
      {
         case PROTOCOL_TCP:
            return "TCP";
         case PROTOCOL_UDP:
            return "UDP";
         case PROTOCOL_BOTH:
            return "TCP/UDP";
         default:
            return "Unknown";
      }
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "PortStopEntry [port=" + port + ", protocol=" + getProtocolString() + "]";
   }

   /**
    * @see java.lang.Object#hashCode()
    */
   @Override
   public int hashCode()
   {
      final int prime = 31;
      int result = 1;
      result = prime * result + port;
      result = prime * result + protocol;
      return result;
   }

   /**
    * @see java.lang.Object#equals(java.lang.Object)
    */
   @Override
   public boolean equals(Object obj)
   {
      if (this == obj)
         return true;
      if (obj == null)
         return false;
      if (getClass() != obj.getClass())
         return false;
      PortStopEntry other = (PortStopEntry)obj;
      if (port != other.port)
         return false;
      if (protocol != other.protocol)
         return false;
      return true;
   }
}
