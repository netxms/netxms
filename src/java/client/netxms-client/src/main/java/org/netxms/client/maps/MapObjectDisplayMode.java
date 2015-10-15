/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
import org.netxms.base.Logger;

/**
 * Map object display mode
 */
public enum MapObjectDisplayMode
{
	ICON(0), 
	SMALL_LABEL(1), 
	LARGE_LABEL(2), 
	STATUS(3);

	private int value;
	private static Map<Integer, MapObjectDisplayMode> lookupTable = new HashMap<Integer, MapObjectDisplayMode>();

	static
	{
		for(MapObjectDisplayMode element : MapObjectDisplayMode.values())
		{
			lookupTable.put(element.value, element);
		}
	}

	private MapObjectDisplayMode(int value)
	{
		this.value = value;
	}

	public int getValue()
	{
		return value;
	}

	public static MapObjectDisplayMode getByValue(int value)
	{
		final MapObjectDisplayMode element = lookupTable.get(value);
		if (element == null)
		{
         Logger.warning(MapObjectDisplayMode.class.getName(), "Unknown element " + value);
			return ICON; // fallback
		}
		return element;
	}
}
