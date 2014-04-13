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
package org.netxms.api.client.reporting;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/**
 * Report definition
 */
public class ReportDefinition
{
	private UUID id;
	private String name;
	private int numberOfColumns = 1;
	private List<ReportParameter> parameters = new ArrayList<ReportParameter>(0);

	public UUID getId()
	{
		return id;
	}

	public void setId(UUID id)
	{
		this.id = id;
	}

	public String getName()
	{
		return name;
	}

	public void setName(String name)
	{
		this.name = name;
	}

	public List<ReportParameter> getParameters()
	{
		return parameters;
	}

	public void addParameter(ReportParameter parameter)
	{
		parameters.add(parameter);
	}

	public int getNumberOfColumns()
	{
		return numberOfColumns;
	}

	public void setNumberOfColumns(int numberOfColumns)
	{
		if (numberOfColumns > 0)
		{
			this.numberOfColumns = numberOfColumns;
		}
		else
		{
			this.numberOfColumns = 1;
		}
	}

	@Override
	public String toString()
	{
		return "ReportDefinition [id=" + id + ", name=" + name + ", numberOfColumns=" + numberOfColumns + ", parameters="
				+ parameters + "]";
	}
}
