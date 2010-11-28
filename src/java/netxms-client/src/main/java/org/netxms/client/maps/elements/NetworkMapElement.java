/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.client.maps.elements;

import org.netxms.base.NXCPMessage;

/**
 * Network map element.
 *
 */
public class NetworkMapElement
{
	public static final int MAP_ELEMENT_GENERIC = 0;
	public static final int MAP_ELEMENT_OBJECT = 1;
	public static final int MAP_ELEMENT_DECORATION = 2;
	
	protected int type;
	protected int x;
	protected int y;
	
	/**
	 * Factory method for creating map element from NXCP message.
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 * @return map element object
	 */
	public static NetworkMapElement createMapElement(NXCPMessage msg, long baseId)
	{
		int type = msg.getVariableAsInteger(baseId);
		switch(type)
		{
			case MAP_ELEMENT_OBJECT:
				return new NetworkMapObject(msg, baseId);
			case MAP_ELEMENT_DECORATION:
				return new NetworkMapDecoration(msg, baseId);
			default:
				return new NetworkMapElement(msg, baseId);
		}
	}
	
	/**
	 * Create element from NXCP message.
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	protected NetworkMapElement(NXCPMessage msg, long baseId)
	{
		type = msg.getVariableAsInteger(baseId);
		x = msg.getVariableAsInteger(baseId + 1);
		y = msg.getVariableAsInteger(baseId + 2);
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * @return the x
	 */
	public int getX()
	{
		return x;
	}

	/**
	 * @return the y
	 */
	public int getY()
	{
		return y;
	}
}
