package org.netxms.api.client.reporting;

import org.netxms.base.NXCPMessage;

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
