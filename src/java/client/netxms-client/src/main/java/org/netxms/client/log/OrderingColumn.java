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
	private String description;
	private boolean descending;

	/**
	 * Create ordering column object.
	 * 
	 * @param column Log column
	 * @param descending Set to true if sorting must be in descending order
	 */
	public OrderingColumn(LogColumn column, boolean descending)
	{
		this.name = column.getName();
		this.description = column.getDescription();
		this.descending = descending;
	}

	/**
	 * Create ordering column object. Will use ascending sorting order.
	 * 
	 * @param column Log column
	 */
	public OrderingColumn(LogColumn column)
	{
		this.name = column.getName();
		this.description = column.getDescription();
		this.descending = false;
	}
	
	/**
	 * @param name
	 * @param description
	 * @param descending
	 */
	public OrderingColumn(String name, String description, boolean descending)
   {
      this.name = name;
      this.description = description;
      this.descending = descending;
   }

   /**
	 * Copy constructor
	 * 
	 * @param src Source object
	 */
	public OrderingColumn(OrderingColumn src)
	{
		this.name = src.name;
		this.description = src.description;
		this.descending = src.descending;
	}

	/**
	 * Get column name.
	 * 
	 * @return Column name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * Check if descending order is requested.
	 * 
	 * @return true if descending order is requested
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

	/**
	 * Get column description.
	 * 
	 * @return Column description
	 */
	public String getDescription()
	{
		return description;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#equals(java.lang.Object)
	 */
	@Override
	public boolean equals(Object obj)
	{
		if (obj instanceof OrderingColumn)
			return name.equals(((OrderingColumn)obj).getName());
		return super.equals(obj);
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode()
	{
		return name.hashCode();
	}
}
