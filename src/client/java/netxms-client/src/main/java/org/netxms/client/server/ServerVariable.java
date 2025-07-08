/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.ServerVariableDataType;

/**
 * Server's configuration variable.
 */
public final class ServerVariable
{
	private String name;
	private String value;
   private String defaultValue;
	private String description;
	private ServerVariableDataType dataType;
	private HashMap<String, String> values = new HashMap<String, String>();
	private boolean isServerRestartNeeded;
	private String unit;

	/**
	 * Default constructor for NXCServerVariable.
	 * 
	 * @param name Variable's name
	 * @param value Variable's value
	 * @param isServerRestartNeeded Server restart flag (server has to be restarted after variable change if this flag is set)
	 * @param dataType Data type for server configuration variable
	 * @param description Description
	 * @param unit Unit
	 * 
	 * @see ServerVariableDataType
	 */
	public ServerVariable(String name, String value, boolean isServerRestartNeeded, ServerVariableDataType dataType, String description, String unit)
	{
		this.name = name;
		this.value = value;
		this.defaultValue = value;
		this.description = description;
		this.dataType = dataType;
		this.isServerRestartNeeded = isServerRestartNeeded;
		this.unit = unit;
	}
	
	/**
	 * Create variable from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base object id
	 */
	public ServerVariable(NXCPMessage msg, long baseId)
	{
      name = msg.getFieldAsString(baseId);
      value = msg.getFieldAsString(baseId + 1);
      isServerRestartNeeded = msg.getFieldAsBoolean(baseId + 2);
      String code = msg.getFieldAsString(baseId + 3);
      dataType = ServerVariableDataType.getByCode(((code != null) && !code.isEmpty()) ? code.charAt(0) : 'S');
      description = msg.getFieldAsString(baseId + 4);
      defaultValue = msg.getFieldAsString(baseId + 5);
      unit = msg.getFieldAsString(baseId + 6);
	}

   /** 
    * Add possible value to variable from NXCP message
    * 
    * @param msg NXCP Message
    * @param baseId value base id
    */
   public void addPossibleValue(NXCPMessage msg, long baseId)
   {
      values.put(msg.getFieldAsString(baseId), msg.getFieldAsString(baseId + 1));
   }

	/**
	 * @return Varaible's name
	 */
	public String getName()
	{
		return name;
	}

   /**
    * Get variable's value for display. Will replace numeric value with textual description when possible.
    * 
    * @param value to convert to display form
    * @return Variable's value prepared for display
    */
   protected String getValueForDisplay(String value)
   {
      if (value == null)
         return "";

      if (dataType == ServerVariableDataType.PASSWORD)
         return "********";

      if (dataType == ServerVariableDataType.BOOLEAN)
      {
         try
         {
            return (Integer.parseInt(value) != 0) ? "true" : "false";
         }
         catch(NumberFormatException e)
         {
            return "false";
         }
      }

      if (dataType == ServerVariableDataType.CHOICE)
      {
         String s = values.get(value);
         return (s != null) ? s : value;
      }

      if ((dataType == ServerVariableDataType.INTEGER) && (unit != null) && !unit.isEmpty())
      {
         return value + " " + unit;
      }

      return value;
   }

	/**
	 * Get variable's value for display. Will replace numeric value with textual description when possible.
	 * 
    * @return Variable's value prepared for display
    */
   public String getValueForDisplay()
   {
      return getValueForDisplay(value);
   }

	/**
	 * @return Variable's value
	 */
	public String getValue()
	{
		return (value != null) ? value : "";
	}
	
	/**
	 * Get variable value as boolean
	 * 
	 * @return value as boolean
	 */
	public boolean getValueAsBoolean()
	{
	   try
	   {
	      return Integer.parseInt(value) != 0;
	   }
	   catch(NumberFormatException e)
	   {
	      return false;
	   }
	}
	
   /**
    * Get variable value as integer
    * 
    * @return value as integer
    */
   public int getValueAsInt()
   {
      try
      {
         return Integer.parseInt(value);
      }
      catch(NumberFormatException e)
      {
         return 0;
      }
   }
   
	/**
	 * @return Variable`s data type
	 */
	public ServerVariableDataType getDataType()
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

	/**
	 * @return A list of possible variable values
	 */
	public HashMap<String, String> getPossibleValues()
	{
	   return values;
	}

	/**
	 * @return Variable`s default value
	 */
	public String getDefaultValue()
	{
	   return defaultValue;
	}
	
   /**
    * Get default value prepared for display.
    * 
    * @return Variable`s default value prepared for display
    */
   public String getDefaultValueForDisplay()
   {
      return getValueForDisplay(defaultValue);
   }

	/**
	 * @return true if variable`s value is the default value
	 */
	public boolean isDefault()
	{
	   return value.equals(defaultValue);
	}

	/**
	 * @return get variable unit
	 */
	public String getUnit()
	{
	   return unit;
	}
}
