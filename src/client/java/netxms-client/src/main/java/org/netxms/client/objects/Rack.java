/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Rack object
 */
public class Rack extends GenericObject
{
	private int height;
	private boolean topBottomNumbering;
	
	/**
	 * @param msg
	 * @param session
	 */
	public Rack(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		height = msg.getFieldAsInt32(NXCPCodes.VID_HEIGHT);
		topBottomNumbering = msg.getFieldAsBoolean(NXCPCodes.VID_TOP_BOTTOM);
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Rack";
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
	 * @return the height
	 */
	public int getHeight()
	{
		return height;
	}
	
	/**
    * @return the topBottomNumbering
    */
   public boolean isTopBottomNumbering()
   {
      return topBottomNumbering;
   }

   /**
	 * Get rack units, ordered by unit numbers
	 * 
	 * @return rack units, ordered by unit numbers
	 */
	public List<RackElement> getUnits()
	{
	   List<RackElement> units = new ArrayList<RackElement>();
	   for(AbstractObject o : getChildsAsArray())
	   {
	      if (o instanceof RackElement)
	         units.add((RackElement)o);
	   }
	   Collections.sort(units, new Comparator<RackElement>() {
         @Override
         public int compare(RackElement e1, RackElement e2)
         {
            return e1.getRackPosition() - e2.getRackPosition();
         }
      });
	   return units;
	}
}
