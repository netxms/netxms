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

import java.util.HashSet;

/**
 * @author Victor Kirhenshtein
 *
 */
public class ColumnFilter
{
	public static final int EQUALS = 0;
	public static final int RANGE = 1;
	public static final int SET = 2;
	public static final int LIKE = 3;
	
	private int type;
	private long rangeFrom;
	private long rangeTo;
	private long equalsTo;
	private String like;
	private HashSet<ColumnFilter> set;
	
	/**
	 * Create filter of type EQUALS
	 * 
	 * @param value
	 */
	public ColumnFilter(long value)
	{
		type = EQUALS;
		equalsTo = value;
	}

	/**
	 * Create filter of type RANGE
	 * 
	 * @param rangeFrom
	 * @param rangeTo
	 */
	public ColumnFilter(long rangeFrom, long rangeTo)
	{
		type = RANGE;
		this.rangeFrom = rangeFrom;
		this.rangeTo = rangeTo;
	}

	/**
	 * Create filter of type LIKE
	 * 
	 * @param value
	 */
	public ColumnFilter(String value)
	{
		type = LIKE;
		like = value;
	}

	/**
	 * Create filter of type SET
	 */
	public ColumnFilter()
	{
		type = SET;
		set = new HashSet<ColumnFilter>();
	}
	
	/**
	 * Add new element to SET type filter
	 *  
	 * @param filter
	 */
	public void addSubFilter(ColumnFilter filter)
	{
		if (type == SET)
			set.add(filter);
	}
}
