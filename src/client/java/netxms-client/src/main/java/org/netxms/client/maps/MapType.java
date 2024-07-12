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
public enum MapType
{
   CUSTOM(0),
   LAYER2_TOPOLOGY(1),
   IP_TOPOLOGY(2),
   INTERNAL_TOPOLOGY(3),
   OSPF_TOPOLOGY(4),
   HYBRID_TOPOLOGY(5);

   private static Logger logger = LoggerFactory.getLogger(MapType.class);
	private static Map<Integer, MapType> lookupTable = new HashMap<Integer, MapType>();
	static
	{
		for(MapType element : MapType.values())
		{
			lookupTable.put(element.value, element);
		}
	}

   private int value;

	private MapType(int value)
	{
		this.value = value;
	}

	public int getValue()
	{
		return value;
	}

	public static MapType getByValue(int value)
	{
		final MapType layout = lookupTable.get(value);
		if (layout == null)
		{
         logger.warn("Unknown layout alghoritm: " + value);
         return CUSTOM; // fallback
		}
		return layout;
	}
}
