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

import org.netxms.client.objects.Interface;

/**
 * Objects of this class represents physical switch ports
 */
public class Port
{
	private long objectId;
	private long ifIndex;
   private int chassis;
	private int module;
   private int pic;
	private int port;
   private int lane;
   private boolean locationValid;

	/**
	 * Create port reference.
	 *
	 * @param objectId object id
	 * @param ifIndex interface index
	 * @param chassis chassis
	 * @param module module
	 * @param pic physical interface card
	 * @param port port
    * @param lane lane number on split (breakout) port or -1 if port is not split
    * @param locationValid true if physical location is known
	 */
   public Port(long objectId, long ifIndex, int chassis, int module, int pic, int port, int lane, boolean locationValid)
	{
		this.objectId = objectId;
		this.ifIndex = ifIndex;
		this.chassis = chassis;
		this.module = module;
		this.pic = pic;
		this.port = port;
      this.lane = lane;
      this.locationValid = locationValid;
	}

   /**
    * Create port reference from interface object.
    *
    * @param iface interface object
    */
   public Port(Interface iface)
   {
      this(iface.getObjectId(), iface.getIfIndex(), iface.getChassis(), iface.getModule(), iface.getPIC(), iface.getPort(), iface.getLane(),
            iface.isPhysicalPort() && iface.isPhysicalLocationValid());
   }
	
	/**
    * @return the chassis
    */
   public int getChassis()
   {
      return chassis;
   }

   /**
	 * @return module number
	 */
	public int getModule()
	{
		return module;
	}
	
	/**
    * @return the pic
    */
   public int getPIC()
   {
      return pic;
   }

   /**
	 * @return the port
	 */
	public int getPort()
	{
		return port;
	}
	
   /**
    * @return lane number on split (breakout) port or -1 if port is not split
    */
   public int getLane()
   {
      return lane;
   }

   /**
    * @return true if physical location is known
    */
   public boolean isLocationValid()
   {
      return locationValid;
   }

   /**
    * Check if this port has same physical location as given port.
    *
    * @param p other port
    * @return true if both ports have valid and identical physical location
    */
   public boolean isSameLocation(Port p)
   {
      return locationValid && p.locationValid && (chassis == p.chassis) && (module == p.module) && (pic == p.pic) && (port == p.port) &&
            (lane == p.lane);
   }

	/**
	 * @return the objectId
	 */
	public long getObjectId()
	{
		return objectId;
	}
	
	/**
	 * @return the ifIndex
	 */
	public long getIfIndex()
	{
		return ifIndex;
	}

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "Port [objectId=" + objectId + ", ifIndex=" + ifIndex + ", chassis=" + chassis + ", module=" + module + ", pic=" + pic
            + ", port=" + port + ", lane=" + lane + ", locationValid=" + locationValid + "]";
   }
}
