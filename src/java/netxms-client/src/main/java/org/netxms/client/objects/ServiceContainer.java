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

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Abstract base class for different SLM objects
 */
public abstract class ServiceContainer extends GenericObject
{
	private double uptimeForDay;
	private double uptimeForWeek;
	private double uptimeForMonth;
	
	public ServiceContainer(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		uptimeForDay = msg.getVariableAsReal(NXCPCodes.VID_UPTIME_DAY);
		uptimeForWeek = msg.getVariableAsReal(NXCPCodes.VID_UPTIME_WEEK);
		uptimeForMonth = msg.getVariableAsReal(NXCPCodes.VID_UPTIME_MONTH);
	}

	/**
	 * @return the uptimeForDay
	 */
	public double getUptimeForDay()
	{
		return uptimeForDay;
	}

	/**
	 * @return the uptimeForWeek
	 */
	public double getUptimeForWeek()
	{
		return uptimeForWeek;
	}

	/**
	 * @return the uptimeForMonth
	 */
	public double getUptimeForMonth()
	{
		return uptimeForMonth;
	}

   /* (non-Javadoc)
    * @see org.netxms.client.objects.AbstractObject#isAllowedOnMap()
    */
   @Override
   public boolean isAllowedOnMap()
   {
      return true;
   }
}
