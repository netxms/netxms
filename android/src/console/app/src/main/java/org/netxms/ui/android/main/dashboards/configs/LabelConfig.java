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
package org.netxms.ui.android.main.dashboards.configs;

import org.simpleframework.xml.Element;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Configuration for label
 */
public class LabelConfig extends DashboardElementConfig
{
	@Element(required=true)
	private String title = "";

	@Element(required=false)
	private String foreground = "0x000000";

	@Element(required=false)
	private String background = "0xFFFFFF";

	/**
	 * Create line chart settings object from XML document
	 * 
	 * @param xml XML document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
	public static LabelConfig createFromXml(final String xml) throws Exception
	{
		Serializer serializer = new Persister();
		return serializer.read(LabelConfig.class, xml);
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
	 * @return the foreground
	 */
	public String getForeground()
	{
		return foreground;
	}

	/**
	 * @return
	 */
	public int getForegroundColorAsInt()
	{
		if (foreground.startsWith("0x"))
			return Integer.parseInt(foreground.substring(2), 16);
		return Integer.parseInt(foreground, 10);
	}

	/**
	 * @param foreground the foreground to set
	 */
	public void setForeground(String foreground)
	{
		this.foreground = foreground;
	}

	/**
	 * @return the background
	 */
	public String getBackground()
	{
		return background;
	}
	
	/**
	 * @return
	 */
	public int getBackgroundColorAsInt()
	{
		if (background.startsWith("0x"))
			return Integer.parseInt(background.substring(2), 16);
		return Integer.parseInt(background, 10);
	}

	/**
	 * @param background the background to set
	 */
	public void setBackground(String background)
	{
		this.background = background;
	}
}
