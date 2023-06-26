/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.client.topology;

import java.net.InetAddress;
import org.netxms.base.MacAddress;
import org.netxms.base.NXCPMessage;

/**
 * ARP cache entry
 */
public class ArpCacheEntry
{
   private InetAddress ipAddress;
   private MacAddress macAddress;
   private int interfaceIndex;
   private String interfaceName;
   private long nodeId;
   private String nodeName;

   /**
    * Create ARP cache entry from NXCP message.
    * 
    * @param msg NXCP message
    * @param baseId base field id
    */
   public ArpCacheEntry(NXCPMessage msg, long baseId)
   {
      ipAddress = msg.getFieldAsInetAddress(baseId);
      macAddress = msg.getFieldAsMacAddress(baseId + 1);
      interfaceIndex = msg.getFieldAsInt32(baseId + 2);
      interfaceName = msg.getFieldAsString(baseId + 3);
      nodeId = msg.getFieldAsInt64(baseId + 4);
      nodeName = msg.getFieldAsString(baseId + 5);
   }

   /**
    * Get IP address.
    *
    * @return IP address
    */
   public InetAddress getIpAddress()
   {
      return ipAddress;
   }

   /**
    * Get MAC address.
    *
    * @return MAC address
    */
   public MacAddress getMacAddress()
   {
      return macAddress;
   }

   /**
    * @return the interfaceIndex
    */
   public int getInterfaceIndex()
   {
      return interfaceIndex;
   }

   /**
    * @return the interfaceName
    */
   public String getInterfaceName()
   {
      return interfaceName.isEmpty() ? "[" + Integer.toString(interfaceIndex) + "]" : interfaceName;
   }

   /**
    * @return the nodeId
    */
   public long getNodeId()
   {
      return nodeId;
   }

   /**
    * @return the nodeName
    */
   public String getNodeName()
   {
      return nodeName;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "ArpCacheEntry [ipAddress=" + ipAddress + ", macAddress=" + macAddress + ", interfaceIndex=" + interfaceIndex + ", interfaceName=" + interfaceName + ", nodeId=" + nodeId + ", nodeName=" +
            nodeName + "]";
   }
}
