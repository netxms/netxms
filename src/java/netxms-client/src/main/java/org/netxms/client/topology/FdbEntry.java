/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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

import org.netxms.base.NXCPMessage;
import org.netxms.client.MacAddress;

/**
 * Switch forwarding database entry
 */
public class FdbEntry
{
   private MacAddress address;
   private int interfaceIndex;
   private String interfaceName;
   private int port;
   private long nodeId;
   private int vlanId;
   private int type;
   
   /**
    * Create FDB entry from NXCP message.
    * 
    * @param msg
    * @param baseId
    */
   public FdbEntry(NXCPMessage msg, long baseId)
   {
      address = new MacAddress(msg.getVariableAsBinary(baseId));
      interfaceIndex = msg.getVariableAsInteger(baseId + 1);
      port = msg.getVariableAsInteger(baseId + 2);
      nodeId = msg.getVariableAsInt64(baseId + 3);
      vlanId = msg.getVariableAsInteger(baseId + 4);
      type = msg.getVariableAsInteger(baseId + 5);
      interfaceName = msg.getVariableAsString(baseId + 6);
   }

   /**
    * @return the address
    */
   public MacAddress getAddress()
   {
      return address;
   }

   /**
    * @return the interfaceIndex
    */
   public int getInterfaceIndex()
   {
      return interfaceIndex;
   }

   /**
    * @return the port
    */
   public int getPort()
   {
      return port;
   }

   /**
    * @return the nodeId
    */
   public long getNodeId()
   {
      return nodeId;
   }

   /**
    * @return the vlanId
    */
   public int getVlanId()
   {
      return vlanId;
   }

   /**
    * @return the type
    */
   public int getType()
   {
      return type;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "FdbEntry [address=" + address + ", interfaceIndex=" + interfaceIndex + ", port=" + port + ", nodeId=" + nodeId
            + ", vlanId=" + vlanId + ", type=" + type + "]";
   }

   /**
    * @return the interfaceName
    */
   public String getInterfaceName()
   {
      return interfaceName;
   }
}
