/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.client.maps;

import java.util.HashMap;
import java.util.Map;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Map type
 */
public enum LinkDataLocation
{
   CENTER(0),
   OBJECT1(1),
   OBJECT2(2);

   private static Logger logger = LoggerFactory.getLogger(LinkDataLocation.class);
	private static Map<Integer, LinkDataLocation> lookupTable = new HashMap<Integer, LinkDataLocation>();
	static
	{
		for(LinkDataLocation element : LinkDataLocation.values())
		{
			lookupTable.put(element.value, element);
		}
	}

   private int value;

	private LinkDataLocation(int value)
	{
		this.value = value;
	}

	public int getValue()
	{
		return value;
	}

	public static LinkDataLocation getByValue(int value)
	{
		final LinkDataLocation layout = lookupTable.get(value);
		if (layout == null)
		{
         logger.warn("Unknown link data location: " + value);
         return CENTER; // fallback
		}
		return layout;
	}
}
