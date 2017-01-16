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
package org.netxms.client.server;

import java.util.HashMap;

/**
 * Server's configuration variable.
 *
 */
public final class ServerVariable
{
	private String name;
	private String value;
	private String description;
	private String dataType;
	private HashMap<String, String> values = new HashMap<String, String>();
	private boolean isServerRestartNeeded;

	
	/**
	 * Default constructor for NXCServerVariable.
	 * 
	 * @param name Variable's name
	 * @param value Variable's value
	 * @param isServerRestartNeeded Server restart flag (server has to be restarted after variable change if this flag is set)
	 */
	public ServerVariable(String name, String value, boolean isServerRestartNeeded, String dataType, String description)
	{
		this.name = name;
		this.value = value;
		this.description = description;
		this.dataType = dataType;
		this.isServerRestartNeeded = isServerRestartNeeded;
	}


	/**
	 * @return Varaible's name
	 */
	public String getName()
	{
		return name;
	}

	  /**
    * @return Variable's value description
    */
   public String getValueDescription()
   {
      return values.get(value);
   }

	/**
	 * @return Variable's value
	 */
	public String getValue()
	{
		return value;
	}
	
	/**
	 * @return Variable`s data type
	 */
	public String getDataType()
	{
	   return dataType;
	}
	
	/**
	 * @return Variable`s description
	 */
	public String getDescription()
	{
	   return description;
	}

	/**
	 * @return Server restart flag
	 */
	public boolean isServerRestartNeeded()
	{
		return isServerRestartNeeded;
	}
	
	/** Adds possible values to variable
	 * @param value
	 */
	public void setPossibleValues(String description, String value)
	{
	   values.put(description, value);
	}
	
	/**
	 * @return A list of possible variable values
	 */
	public HashMap<String, String> getPossibleValues()
	{
	   return values;
	}
}
