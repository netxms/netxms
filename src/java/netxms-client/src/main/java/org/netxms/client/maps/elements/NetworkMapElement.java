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
	public static final int MAP_ELEMENT_DCI_CONTAINER = 3;
	
	protected long id;
	protected int type;
	protected int x;
	protected int y;
	private int flags;
	
	/**
	 * Factory method for creating map element from NXCP message.
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 * @return map element object
	 */
	public static NetworkMapElement createMapElement(NXCPMessage msg, long baseId)
	{
		int type = msg.getVariableAsInteger(baseId + 1);
		switch(type)
		{
			case MAP_ELEMENT_OBJECT:
				return new NetworkMapObject(msg, baseId);
			case MAP_ELEMENT_DECORATION:
				return new NetworkMapDecoration(msg, baseId);
			case MAP_ELEMENT_DCI_CONTAINER:
            return new NetworkMapDCIContainer(msg, baseId);
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
		id = msg.getVariableAsInt64(baseId);
		type = msg.getVariableAsInteger(baseId + 1);
		x = msg.getVariableAsInteger(baseId + 2);
		y = msg.getVariableAsInteger(baseId + 3);
		flags = msg.getVariableAsInteger(baseId + 4);
	}
	
	/**
	 * Create new generic element
	 */
	public NetworkMapElement(long id)
	{
		this.id = id;
		type = MAP_ELEMENT_GENERIC;
		x = 0;
		y = 0;
		flags = 0;
	}
	
	/**
	 * Fill NXCP message with element data
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	public void fillMessage(NXCPMessage msg, long baseId)
	{
		msg.setVariableInt32(baseId, (int)id);
		msg.setVariableInt16(baseId + 1, type);
		msg.setVariableInt32(baseId + 2, x);
		msg.setVariableInt32(baseId + 3, y);
		msg.setVariableInt32(baseId + 4, flags);
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

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}
	
	/**
	 * Set elements's location
	 * 
	 * @param x
	 * @param y
	 */
	public void setLocation(int x, int y)
	{
		this.x = x;
		this.y = y;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#equals(java.lang.Object)
	 */
	@Override
	public boolean equals(Object obj)
	{
		if (obj instanceof NetworkMapElement)
			return ((NetworkMapElement)obj).id == id;
		return super.equals(obj);
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode()
	{
		return (int)id;
	}

   /**
    * @return the flags
    */
   public int getFlags()
   {
      return flags;
   }

   /**
    * @param flags the flags to set
    */
   public void setFlags(int flags)
   {
      this.flags = flags;
   }
   
   /**
    * @param flag the flag to be added to current flags
    */
   public void addFlag(int flags)
   {
      this.flags |= flags;
   }
   
   /**
    * @param flag the flag 
    */
   public void removeFlag(int flags)
   {
      this.flags &= ~flags;
   }
}
