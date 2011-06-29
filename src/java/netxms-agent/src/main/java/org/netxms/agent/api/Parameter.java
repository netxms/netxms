/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.agent.api;

/**
 * Agent's parameter
 */
public abstract class Parameter
{
	public static final int DT_INT    = 0;
	public static final int DT_UINT   = 1;
	public static final int DT_INT64  = 2;
	public static final int DT_UINT64 = 3;
	public static final int DT_STRING = 4;
	public static final int DT_FLOAT  = 5;
	public static final int DT_NULL   = 6;
	
	public static final String RC_NOT_SUPPORTED = "$not_supported$";
	public static final String RC_ERROR = "$error$";
	
	private String name;
	private String description;
	private int type;
	
	/**
	 * @param name
	 * @param description
	 * @param type
	 */
	public Parameter(String name, String description, int type)
	{
		this.name = name;
		this.description = description;
		this.type = type;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}
	
	/**
	 * Parameter's handler.
	 * 
	 * @param parameter Actual parameter requested by server
	 * @return Parameter's value or one of the error indicators
	 */
	public abstract String handler(String parameter);
}
