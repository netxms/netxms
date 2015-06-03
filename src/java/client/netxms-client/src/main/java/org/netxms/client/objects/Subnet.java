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
package org.netxms.client.objects;

import java.net.InetAddress;
import org.netxms.base.InetAddressEx;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Subnet object
 */
public class Subnet extends GenericObject
{
	private long zoneId;
	private InetAddressEx networkAddress;

	/**
	 * @param msg
	 */
	public Subnet(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		zoneId = msg.getFieldAsInt64(NXCPCodes.VID_ZONE_ID);
      networkAddress = msg.getFieldAsInetAddressEx(NXCPCodes.VID_IP_ADDRESS);
	}
	
	/**
    * @return the networkAddress
    */
   public InetAddressEx getNetworkAddress()
   {
      return networkAddress;
   }

   /**
    * @return the networkAddress
    */
   public InetAddress getSubnetAddress()
   {
      return networkAddress.address;
   }

   /**
	 * Get number of bits in subnet mask
	 * 
	 * @return
	 */
	public int getSubnetMask()
	{
	   return networkAddress.mask;
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.client.NXCObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Subnet";
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.AbstractObject#isAllowedOnMap()
	 */
	@Override
	public boolean isAllowedOnMap()
	{
		return true;
	}

   /* (non-Javadoc)
    * @see org.netxms.client.objects.AbstractObject#isAlarmsVisible()
    */
   @Override
   public boolean isAlarmsVisible()
   {
      return true;
   }

	/**
	 * @return the zoneId
	 */
	public long getZoneId()
	{
		return zoneId;
	}
}
