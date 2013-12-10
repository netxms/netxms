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
import org.netxms.base.*;
import org.netxms.client.NXCSession;

/**
 * Subnet object
 */
public class Subnet extends GenericObject
{
	private InetAddress subnetMask;
	private long zoneId;

	/**
	 * @param msg
	 */
	public Subnet(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		subnetMask = msg.getVariableAsInetAddress(NXCPCodes.VID_IP_NETMASK);
		zoneId = msg.getVariableAsInt64(NXCPCodes.VID_ZONE_ID);
	}
	
	/**
	 * Get number of bits in subnet mask
	 * 
	 * @return
	 */
	public int getMaskBits()
	{
	   byte[] addr = subnetMask.getAddress();
	   int bits = 0;
	   for(int i = 0; i < addr.length; i++)
	   {
	      if (addr[i] == (byte)0xFF)
	      {
	         bits += 8;
	      }
	      else
	      {
	         for(int j = 0x80; j > 0; j >>= 1)
	         {
	            if ((addr[i] & j) == 0)
	               break;
	            bits++;
	         }
	         break;
	      }
	   }
	   return bits;
	}
	
	/**
	 * @return Subnet mask
	 */
	public InetAddress getSubnetMask()
	{
		return subnetMask;
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

	/**
	 * @return the zoneId
	 */
	public long getZoneId()
	{
		return zoneId;
	}
}
