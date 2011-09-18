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
import org.netxms.client.datacollection.GraphSettings;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementArray;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Configuration for pie chart
 */
@Root(name="element")
public class PieChartConfig extends DashboardElementConfig
{
	@ElementArray(required=true)
	private DashboardDciInfo[] dciList = new DashboardDciInfo[0];

	@Element(required=false)
	private String title = "";

	@Element(required=false)
	private int legendPosition = GraphSettings.POSITION_RIGHT;
	
	@Element(required=false)
	private boolean showLegend = true;
	
	@Element(required=false)
	private boolean showIn3D = false;
	
	@Element(required=false)
	private boolean translucent = false;

	/**
	 * Create line chart settings object from XML document
	 * 
	 * @param xml XML document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
	public static PieChartConfig createFromXml(final String xml) throws Exception
	{
		Serializer serializer = new Persister();
		return serializer.read(PieChartConfig.class, xml);
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#createXml()
	 */
	@Override
	public String createXml() throws Exception
	{
		Serializer serializer = new Persister();
		Writer writer = new StringWriter();
		serializer.write(this, writer);
		return writer.toString();
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

	/**
	 * @return the dciList
	 */
	public DashboardDciInfo[] getDciList()
	{
		return dciList;
	}

	/**
	 * @param dciList the dciList to set
	 */
	public void setDciList(DashboardDciInfo[] dciList)
	{
		this.dciList = dciList;
	}

	/**
	 * @return the showIn3D
	 */
	public boolean isShowIn3D()
	{
		return showIn3D;
	}

	/**
	 * @param showIn3D the showIn3D to set
	 */
	public void setShowIn3D(boolean showIn3D)
	{
		this.showIn3D = showIn3D;
	}

	/**
	 * @return the translucent
	 */
	public boolean isTranslucent()
	{
		return translucent;
	}

	/**
	 * @param translucent the translucent to set
	 */
	public void setTranslucent(boolean translucent)
	{
		this.translucent = translucent;
	}

	/**
	 * @return the legendPosition
	 */
	public int getLegendPosition()
	{
		return legendPosition;
	}

	/**
	 * @param legendPosition the legendPosition to set
	 */
	public void setLegendPosition(int legendPosition)
	{
		this.legendPosition = legendPosition;
	}

	/**
	 * @return the showLegend
	 */
	public boolean isShowLegend()
	{
		return showLegend;
	}

	/**
	 * @param showLegend the showLegend to set
	 */
	public void setShowLegend(boolean showLegend)
	{
		this.showLegend = showLegend;
	}
}
