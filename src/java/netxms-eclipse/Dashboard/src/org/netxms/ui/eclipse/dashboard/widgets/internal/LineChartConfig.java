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
package org.netxms.ui.eclipse.dashboard.widgets.internal;

import java.io.StringWriter;
import java.io.Writer;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Configuration for line chart
 */
@Root(name="config")
public class LineChartConfig
{
	@Element(required=true)
	private long nodeId = 0;

	@Element(required=true)
	private long[] dciList = new long[0];

	@Element(required=true)
	private String[] colors = new String[0];
	
	@Element(required=false)
	private String title = "";

	/**
	 * Create line chart settings object from XML document
	 * 
	 * @param xml XML document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
	public static LineChartConfig createFromXml(final String xml) throws Exception
	{
		Serializer serializer = new Persister();
		return serializer.read(LineChartConfig.class, xml);
	}
	
	/**
	 * Create XML document from object
	 * 
	 * @return XML document
	 * @throws Exception if the schema for the object is not valid
	 */
	public String createXml() throws Exception
	{
		Serializer serializer = new Persister();
		Writer writer = new StringWriter();
		serializer.write(this, writer);
		return writer.toString();
	}

	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}

	/**
	 * @param nodeId the nodeId to set
	 */
	public void setNodeId(long nodeId)
	{
		this.nodeId = nodeId;
	}

	/**
	 * @return the dciList
	 */
	public long[] getDciList()
	{
		return dciList;
	}

	/**
	 * @param dciList the dciList to set
	 */
	public void setDciList(long[] dciList)
	{
		this.dciList = dciList;
	}

	/**
	 * @return the colors
	 */
	public String[] getColors()
	{
		return colors;
	}

	/**
	 * @return the color
	 */
	public int getColorAsInt(int index)
	{
		if (colors[index].startsWith("0x"))
			return Integer.parseInt(colors[index].substring(2), 16);
		return Integer.parseInt(colors[index], 10);
	}

	/**
	 * @param colors the color to set
	 */
	public void setColors(String[] colors)
	{
		this.colors = colors;
	}

	/**
	 * @param color the color to set
	 */
	public void setColor(int index, int color)
	{
		colors[index] = Integer.toString(color);
	}

	/**
	 * @return the title
	 */
	public String getTitle()
	{
		return title;
	}

	/**
	 * @param title the title to set
	 */
	public void setTitle(String title)
	{
		this.title = title;
	}
}
