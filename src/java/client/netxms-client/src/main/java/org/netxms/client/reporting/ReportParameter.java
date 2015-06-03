/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Raden Solutions
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
	private int span;

   /**
    * Create parameter definition from NXCP message
    * 
    * @param msg
    * @param baseId
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
   }

	/**
	 * @return
	 */
	public int getIndex()
	{
		return index;
	}

	/**
	 * @return
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return
	 */
	public String getDependsOn()
	{
		return dependsOn;
	}

	/**
	 * @return
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return
	 */
	public String getType()
	{
		return type;
	}

	/**
	 * @return
	 */
	public String getDefaultValue()
	{
		return defaultValue;
	}

	/**
	 * @return
	 */
	public int getSpan()
	{
		return span;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString()
	{
		return "ReportParameter [index=" + index + ", name=" + name + ", dependsOn=" + dependsOn + ", description=" + description
				+ ", type=" + type + ", defaultValue=" + defaultValue + ", span=" + span + "]";
	}
}
