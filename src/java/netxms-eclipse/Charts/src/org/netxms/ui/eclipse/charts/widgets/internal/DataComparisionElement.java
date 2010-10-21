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
package org.netxms.ui.eclipse.charts.widgets.internal;

/**
 * Single element used in data comparision charts
 *
 */
public class DataComparisionElement
{
	private String name;
	private double value;
	
	/**
	 * @param name
	 * @param value
	 */
	public DataComparisionElement(String name, double value)
	{
		super();
		this.name = name;
		this.value = value;
	}
	
	/**
	 * @return the value
	 */
	public double getValue()
	{
		return value;
	}
	
	/**
	 * @param value the value to set
	 */
	public void setValue(double value)
	{
		this.value = value;
	}
	
	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}
}
