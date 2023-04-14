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
package org.netxms.client.objects;

import java.net.InetAddress;
import java.util.Set;
import org.netxms.base.InetAddressEx;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.interfaces.ZoneMember;

/**
 * Subnet object
 */
public class Subnet extends GenericObject implements ZoneMember
{
	private int zoneId;
	private InetAddressEx networkAddress;

	/**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
	 */
	public Subnet(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		zoneId = msg.getFieldAsInt32(NXCPCodes.VID_ZONE_UIN);
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
      return networkAddress.getAddress();
   }

   /**
	 * Get number of bits in subnet mask
	 * 
	 * @return number of bits in subnet mask
	 */
	public int getSubnetMask()
	{
	   return networkAddress.getMask();
	}

	/**
	 * Check if this subnet is a point-to-point subnet as defined by RFC 3021.
	 * 
	 * @return true if this subnet is point-to-point subnet
	 */
	public boolean isPointToPoint()
	{
	   return networkAddress.getHostBits() == 1;
	}

   /**
    * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
    */
	@Override
	public String getObjectClassName()
	{
		return "Subnet";
	}

   /**
    * @see org.netxms.client.objects.AbstractObject#isAllowedOnMap()
    */
	@Override
	public boolean isAllowedOnMap()
	{
		return true;
	}

   /**
    * @see org.netxms.client.objects.AbstractObject#isAlarmsVisible()
    */
   @Override
   public boolean isAlarmsVisible()
   {
      return true;
   }

   /**
    * @see org.netxms.client.objects.interfaces.ZoneMember#getZoneId()
    */
   @Override
	public int getZoneId()
	{
		return zoneId;
	}

   /**
    * @see org.netxms.client.objects.interfaces.ZoneMember#getZoneName()
    */
   @Override
   public String getZoneName()
   {
      Zone zone = session.findZone(zoneId);
      return (zone != null) ? zone.getObjectName() : Long.toString(zoneId);
   }

   /**
    * @see org.netxms.client.objects.AbstractObject#getStrings()
    */
   @Override
   public Set<String> getStrings()
   {
      Set<String> strings = super.getStrings();
      addString(strings, networkAddress.getHostAddress());
      return strings;
   }
}
