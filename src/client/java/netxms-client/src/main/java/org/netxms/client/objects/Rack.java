/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
import java.util.Map;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.configs.PassiveRackElement;
import org.netxms.client.objects.interfaces.Assets;
import org.netxms.client.objects.interfaces.HardwareEntity;

/**
 * Rack object
 */
public class Rack extends GenericObject implements Assets
{
	private int height;
	private boolean topBottomNumbering;
   private List<PassiveRackElement> passiveElements;
   private Map<String, String> assetList;

	/**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
	 */
	public Rack(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		height = msg.getFieldAsInt32(NXCPCodes.VID_HEIGHT);
		topBottomNumbering = msg.getFieldAsBoolean(NXCPCodes.VID_TOP_BOTTOM);
		passiveElements = new ArrayList<PassiveRackElement>();
		int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
		long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
		for (int i = 0; i < count; i++)
		{
		   passiveElements.add(new PassiveRackElement(msg, fieldId));
         fieldId += 10;
		}
		
		assetList = msg.getStringMapFromFields(NXCPCodes.VID_AM_DATA_BASE, NXCPCodes.VID_AM_COUNT);
	}

	/**
	 * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Rack";
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
    * Get rack attribute config entry list
    * 
    * @return entryList
    */
   public List<PassiveRackElement> getPassiveElements()
   {
      return passiveElements;
   }

   /**
	 * Get rack units, ordered by unit numbers
	 * 
	 * @return rack units, ordered by unit numbers
	 */
	public List<HardwareEntity> getUnits()
	{
	   List<HardwareEntity> units = new ArrayList<HardwareEntity>();
	   for(AbstractObject o : getChildrenAsArray())
	   {
	      if (o instanceof HardwareEntity)
	         units.add((HardwareEntity)o);
	   }
	   Collections.sort(units, new Comparator<HardwareEntity>() {
         @Override
         public int compare(HardwareEntity e1, HardwareEntity e2)
         {
            return e1.getRackPosition() - e2.getRackPosition();
         }
      });
	   return units;
	}

	/**
    * Get passive element by ID.
    * 
    * @param id element ID
    * @return passive element with given ID or null if such element does not exist
    */
	public PassiveRackElement getPassiveElement(long id)
	{
	   for(PassiveRackElement e : passiveElements)
	      if (e.getId() == id)
            return e;
      return null;
	}

   /**
    * @see org.netxms.client.objects.interfaces.Assets#getAssets()
    */
   @Override
   public Map<String, String> getAssets()
   {
      return assetList;
   }
}
