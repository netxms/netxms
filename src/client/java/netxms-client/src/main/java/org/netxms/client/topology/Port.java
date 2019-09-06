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

	/**
	 *
	 * @param objectId object id
	 * @param ifIndex interface index
	 * @param chassis chassis
	 * @param module module
	 * @param pic physical interface card
	 * @param port port
	 */
	public Port(long objectId, long ifIndex, int chassis, int module, int pic, int port)
	{
		this.objectId = objectId;
		this.ifIndex = ifIndex;
		this.chassis = chassis;
		this.module = module;
		this.pic = pic;
		this.port = port;
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
            + ", port=" + port + "]";
   }
}
