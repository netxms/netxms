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
package org.netxms.client.datacollection;

/**
 * Column definition for DCI summary table
 */
public class DciSummaryTableColumn
{
	private String name;
	private String dciName;

	/**
	 * @param name
	 * @param dciName
	 */
	public DciSummaryTableColumn(String name, String dciName)
	{
		super();
		this.name = name;
		this.dciName = dciName;
	}
	
	/**
	 * Copy constructor.
	 * 
	 * @param src
	 */
	public DciSummaryTableColumn(DciSummaryTableColumn src)
	{
		name = src.name;
		dciName = src.dciName;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the dciName
	 */
	public String getDciName()
	{
		return dciName;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @param dciName the dciName to set
	 */
	public void setDciName(String dciName)
	{
		this.dciName = dciName;
	}
}
