/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.client.log;

/**
 * Ordering column information.
 *
 */
public class OrderingColumn
{
	private String name;
	private boolean descending;

	/**
	 * Create ordering column object.
	 * 
	 * @param name Column name
	 * @param descending Set to true if sorting must be in descending order
	 */
	public OrderingColumn(String name, boolean descending)
	{
		this.name = name;
		this.descending = descending;
	}

	/**
	 * Create ordering column object. Will use ascending sorting order.
	 * 
	 * @param name Column name
	 */
	public OrderingColumn(String name)
	{
		this.name = name;
		this.descending = false;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the descending
	 */
	public boolean isDescending()
	{
		return descending;
	}

	/**
	 * Set column to use descending or ascending order.
	 * 
	 * @param descending Set to true if sorting must be in descending order
	 */
	public void setDescending(boolean descending)
	{
		this.descending = descending;
	}
}
