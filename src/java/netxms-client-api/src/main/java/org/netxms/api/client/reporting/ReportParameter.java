package org.netxms.api.client.reporting;

import org.netxms.base.NXCPMessage;

public class ReportParameter
{
	private int index;
	private String name;
	private String description;
	private String type;
	private String logicalType;

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

	public String getDescription()
	{
		return description;
	}

	public void setDescription(String description)
	{
		this.description = description;
	}

	public String getLogicalType()
	{
		return logicalType;
	}

	public void setLogicalType(String logicalType)
	{
		this.logicalType = logicalType;
	}

	public String getType()
	{
		return type;
	}

	public void setType(String type)
	{
		this.type = type;
	}

	public static ReportParameter createFromMessage(NXCPMessage response, long base)
	{
		long id = base;
		ReportParameter ret = new ReportParameter();
		ret.setIndex(response.getVariableAsInteger(id++));
		ret.setName(response.getVariableAsString(id++));
		ret.setDescription(response.getVariableAsString(id++));
		ret.setType(response.getVariableAsString(id++));
		ret.setLogicalType(response.getVariableAsString(id++));
		return ret;
	}

	@Override
	public String toString()
	{
		return "ReportParameter [index=" + index + ", name=" + name + ", description=" + description + ", type=" + type
				+ ", logicalType=" + logicalType + "]";
	}
}
