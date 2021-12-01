/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.client.reporting;

import org.netxms.base.NXCPMessage;

/**
 * Report parameter definition
 */
public class ReportParameter
{
	private int index;
	private String name;
	private String dependsOn;
	private String description;
	private String type;
	private String defaultValue;
   private String multiselectValues;
	private int span;

   /**
    * Create parameter definition from NXCP message
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public ReportParameter(NXCPMessage msg, long baseId)
   {
      index = msg.getFieldAsInt32(baseId);
      name = msg.getFieldAsString(baseId + 1);
      description = msg.getFieldAsString(baseId + 2);
      type = msg.getFieldAsString(baseId + 3);
      defaultValue = msg.getFieldAsString(baseId + 4);
      dependsOn = msg.getFieldAsString(baseId + 5);
      span = msg.getFieldAsInt32(baseId + 6);
      multiselectValues = msg.getFieldAsString(baseId + 7);
   }

	/**
	 * Get parameter index
	 * 
	 * @return parameter index
	 */
	public int getIndex()
	{
		return index;
	}

	/**
	 * Get parameter name
	 * 
	 * @return parameter name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * Get dependencies
	 * 
	 * @return dependencies
	 */
	public String getDependsOn()
	{
		return dependsOn;
	}

	/**
	 * Get description
	 * 
	 * @return description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * Get parameter type
	 * 
	 * @return parameter type
	 */
	public String getType()
	{
		return type;
	}

	/**
	 * Get default value
	 * 
	 * @return default value
	 */
	public String getDefaultValue()
	{
		return defaultValue;
	}

	/**
    * Get possible values for multiselect field.
    *
    * @return possible values for multiselect field
    */
   public String getMultiselectValues()
   {
      return multiselectValues;
   }

   /**
    * Get column span for this parameter
    * 
    * @return column span for this parameter
    */
	public int getSpan()
	{
		return span;
	}

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "ReportParameter [index=" + index + ", name=" + name + ", dependsOn=" + dependsOn + ", description=" + description + ", type=" + type + ", defaultValue=" + defaultValue +
            ", multiselectValues=" + multiselectValues + ", span=" + span + "]";
   }
}
