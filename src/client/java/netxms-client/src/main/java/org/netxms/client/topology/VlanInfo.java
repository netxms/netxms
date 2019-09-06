/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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

/**
 * Contains information about VLAN
 */
public class VlanInfo
{
	private int vlanId;
	private String name;
	private Port[] ports;
	private long nextFieldId;
	
	/**
	 * Create VLAN information object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable id
	 */
	public VlanInfo(NXCPMessage msg, long baseId)
	{
		vlanId = msg.getFieldAsInt32(baseId);
		name = msg.getFieldAsString(baseId + 1);
		
		int count = msg.getFieldAsInt32(baseId + 2);
		ports = new Port[count];
		
		long fieldId = baseId + 3;
      for(int i = 0; i < count; i++)
		{
         int ifIndex = msg.getFieldAsInt32(fieldId++);
         long objectId = msg.getFieldAsInt64(fieldId++);
         int chassis = msg.getFieldAsInt32(fieldId++);
         int module = msg.getFieldAsInt32(fieldId++);
         int pic = msg.getFieldAsInt32(fieldId++);
         int port = msg.getFieldAsInt32(fieldId++);
			ports[i] = new Port(objectId, ifIndex, chassis, module, pic, port);
		}
      nextFieldId = fieldId;
	}

	/**
    * @return the nextFieldId
    */
   public long getNextFieldId()
   {
      return nextFieldId;
   }

   /**
	 * @return the vlanId
	 */
	public int getVlanId()
	{
		return vlanId;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the ports
	 */
	public Port[] getPorts()
	{
		return ports;
	}

   /**
    * Check if given port is within VLAN
    * @param chassis chassis
    * @param module module
    * @param pic pic
    * @param port port
    * @return if given port is within VLAN
    */
   public boolean containsPort(int chassis, int module, int pic, int port)
   {
      for(Port p : ports)
      {
         if ((p.getChassis() == chassis) && (p.getModule() == module) && (p.getPIC() == pic) && (p.getPort() == port))
            return true;
      }
      return false;
   }
}
