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
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Configuration for embedded dashboard element
 */
public class EmbeddedDashboardConfig extends DashboardElementConfig
{
	@Element(required=false)
	private long objectId = 0;
	
	@Element(required=false)
	private long[] dashboardObjects = new long[0];
	
	@Element(required=false)
	private int displayInterval = 60;

	@Element(required=false)
	private String title = "";

	/**
	 * Create line chart settings object from XML document
	 * 
	 * @param xml XML document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
	public static EmbeddedDashboardConfig createFromXml(final String xml) throws Exception
	{
		Serializer serializer = new Persister();
		EmbeddedDashboardConfig config = serializer.read(EmbeddedDashboardConfig.class, xml);
		
		// fix configuration if it was saved in old format
		if ((config.objectId != 0) && (config.dashboardObjects.length == 0))
		{
			config.dashboardObjects = new long[1];
			config.dashboardObjects[0] = config.objectId;
			config.objectId = 0;
		}
		return config;
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
	 * @return the dashboardObjects
	 */
	public long[] getDashboardObjects()
	{
		return dashboardObjects;
	}

	/**
	 * @param dashboardObjects the dashboardObjects to set
	 */
	public void setDashboardObjects(long[] dashboardObjects)
	{
		this.dashboardObjects = dashboardObjects;
	}

	/**
	 * @return the displayInterval
	 */
	public int getDisplayInterval()
	{
		return displayInterval;
	}

	/**
	 * @param displayInterval the displayInterval to set
	 */
	public void setDisplayInterval(int displayInterval)
	{
		this.displayInterval = displayInterval;
	}
}
