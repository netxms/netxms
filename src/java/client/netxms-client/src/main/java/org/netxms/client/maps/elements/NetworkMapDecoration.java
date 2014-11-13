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
 * Decoration element of network map
 *
 */
public class NetworkMapDecoration extends NetworkMapElement
{
	public static final int GROUP_BOX = 0;
	public static final int IMAGE = 1;
	
	private int decorationType;
	private int color;
	private String title;
	private int width;
	private int height;
	
	/**
	 * @param msg
	 * @param baseId
	 */
	protected NetworkMapDecoration(NXCPMessage msg, long baseId)
	{
		super(msg, baseId);
		decorationType = msg.getVariableAsInteger(baseId + 10);
		color = msg.getVariableAsInteger(baseId + 11);
		title = msg.getVariableAsString(baseId + 12);
		width = msg.getVariableAsInteger(baseId + 13);
		height = msg.getVariableAsInteger(baseId + 14);
	}
	
	/**
	 * Create new decoration object
	 * 
	 * @param id
	 * @param decorationType
	 */
	public NetworkMapDecoration(long id, int decorationType)
	{
		super(id);
		type = MAP_ELEMENT_DECORATION;
		this.decorationType = decorationType;
		title = "";
		width = 50;
		height = 20;
		color = 0;
	}

	/**
	 * @return the decorationType
	 */
	public int getDecorationType()
	{
		return decorationType;
	}

	/**
	 * @return the color
	 */
	public int getColor()
	{
		return color;
	}

	/**
	 * @return the title
	 */
	public String getTitle()
	{
		return title;
	}

	/**
	 * @param color the color to set
	 */
	public void setColor(int color)
	{
		this.color = color;
	}

	/**
	 * @param title the title to set
	 */
	public void setTitle(String title)
	{
		this.title = title;
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.maps.elements.NetworkMapElement#fillMessage(org.netxms.base.NXCPMessage, long)
	 */
	@Override
	public void fillMessage(NXCPMessage msg, long baseId)
	{
		super.fillMessage(msg, baseId);
		msg.setVariableInt32(baseId + 10, decorationType);
		msg.setVariableInt32(baseId + 11, color);
		msg.setVariable(baseId + 12, title);
		msg.setVariableInt32(baseId + 13, width);
		msg.setVariableInt32(baseId + 14, height);
	}

	/**
	 * @return the width
	 */
	public int getWidth()
	{
		return width;
	}

	/**
	 * @return the height
	 */
	public int getHeight()
	{
		return height;
	}
	
	/**
	 * Set decoration size
	 * 
	 * @param w width
	 * @param h height
	 */
	public void setSize(int w, int h)
	{
		width = w;
		height = h;
	}
}
