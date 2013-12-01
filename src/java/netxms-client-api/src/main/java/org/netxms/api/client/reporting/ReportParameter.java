/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 NetXMS Team
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
package org.netxms.api.client.reporting;

import org.netxms.base.NXCPMessage;

/**
 * Report parameter
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

	public int getIndex()
	{
		return index;
	}

	public void setIndex(int index)
	{
		this.index = index;
	}

	public String getName()
	{
		return name;
	}

	public void setName(String name)
	{
		this.name = name;
	}

	public String getDependsOn()
	{
		return dependsOn;
	}

	public void setDependsOn(String dependsOn)
	{
		this.dependsOn = dependsOn;
	}

	public String getDescription()
	{
		return description;
	}

	public void setDescription(String description)
	{
		this.description = description;
	}

	public String getType()
	{
		return type;
	}

	public void setType(String type)
	{
		this.type = type;
	}

	public String getDefaultValue()
	{
		return defaultValue;
	}

	public void setDefaultValue(String defaultValue)
	{
		this.defaultValue = defaultValue;
	}

	public int getSpan()
	{
		return span;
	}

	public void setSpan(int span)
	{
		if (span < 1 || span > 2)
		{
			this.span = 1;
		}
		else
		{
			this.span = span;
		}
	}

	public static ReportParameter createFromMessage(NXCPMessage response, long base)
	{
		long id = base;
		ReportParameter ret = new ReportParameter();
		ret.setIndex(response.getVariableAsInteger(id++));
		ret.setName(response.getVariableAsString(id++));
		ret.setDescription(response.getVariableAsString(id++));
		ret.setType(response.getVariableAsString(id++));
		ret.setDefaultValue(response.getVariableAsString(id++));
		ret.setDependsOn(response.getVariableAsString(id++));
		ret.setSpan(response.getVariableAsInteger(id++));
		return ret;
	}

	@Override
	public String toString()
	{
		return "ReportParameter [index=" + index + ", name=" + name + ", dependsOn=" + dependsOn + ", description=" + description
				+ ", type=" + type + ", defaultValue=" + defaultValue + ", span=" + span + "]";
	}
}
