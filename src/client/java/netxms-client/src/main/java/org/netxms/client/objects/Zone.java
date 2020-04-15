/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Victor Kirhenshtein
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

import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Zone object
 */
public class Zone extends GenericObject
{
	private int uin;
	private long[] proxyNodes;
	
	/**
	 * Create zone object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param session owning session
	 */
	public Zone(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		uin = msg.getFieldAsInt32(NXCPCodes.VID_ZONE_UIN);
		proxyNodes = msg.getFieldAsUInt32Array(NXCPCodes.VID_ZONE_PROXY_LIST);
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
	 * Get zone UIN (unique identification number)
	 * 
	 * @return zone UIN
	 */
	public int getUIN()
	{
		return uin;
	}
	
   /**
    * Get list of proxy nodes
    * 
    * @return list of proxy nodes
    */
   public List<AbstractObject> getProxyNodes()
   {
      return session.findMultipleObjects(proxyNodes, true);
   }

   /* (non-Javadoc)
	 * @see org.netxms.client.objects.GenericObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Zone";
	}
}
