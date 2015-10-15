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
	
	/**
	 * @param msg
	 * @param session
	 */
	public Rack(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		height = msg.getFieldAsInt32(NXCPCodes.VID_HEIGHT);
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
	 * Get rack units, ordered by unit numbers
	 * 
	 * @return
	 */
	public List<AbstractNode> getUnits()
	{
	   List<AbstractNode> units = new ArrayList<AbstractNode>();
	   for(AbstractObject o : getChildsAsArray())
	   {
	      if (o instanceof AbstractNode)
	         units.add((AbstractNode)o);
	   }
	   Collections.sort(units, new Comparator<AbstractNode>() {
         @Override
         public int compare(AbstractNode node1, AbstractNode node2)
         {
            return node1.getRackPosition() - node2.getRackPosition();
         }
      });
	   return units;
	}
}
