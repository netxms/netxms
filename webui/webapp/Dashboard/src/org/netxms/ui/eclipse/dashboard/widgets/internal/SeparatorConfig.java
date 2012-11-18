/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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

import org.simpleframework.xml.Element;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Configuration for separator
 */
public class SeparatorConfig extends DashboardElementConfig
{
	@Element(required=false)
	private int lineWidth = 2;

	@Element(required=false)
	private int topMargin = 0;

	@Element(required=false)
	private int bottomMargin = 0;

	@Element(required=false)
	private int leftMargin = 0;

	@Element(required=false)
	private int rightMargin = 0;

	@Element(required=false)
	private String foreground = "0x000000"; //$NON-NLS-1$

	@Element(required=false)
	private String background = "0xFFFFFF"; //$NON-NLS-1$

	/**
	 * Create line chart settings object from XML document
	 * 
	 * @param xml XML document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
	public static SeparatorConfig createFromXml(final String xml) throws Exception
	{
		Serializer serializer = new Persister();
		return serializer.read(SeparatorConfig.class, xml);
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
		if (foreground.startsWith("0x")) //$NON-NLS-1$
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
		if (background.startsWith("0x")) //$NON-NLS-1$
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

	/**
	 * @return the lineWidth
	 */
	public final int getLineWidth()
	{
		return lineWidth;
	}

	/**
	 * @param lineWidth the lineWidth to set
	 */
	public final void setLineWidth(int lineWidth)
	{
		this.lineWidth = lineWidth;
	}

	/**
	 * @return the topMargin
	 */
	public final int getTopMargin()
	{
		return topMargin;
	}

	/**
	 * @param topMargin the topMargin to set
	 */
	public final void setTopMargin(int topMargin)
	{
		this.topMargin = topMargin;
	}

	/**
	 * @return the bottomMargin
	 */
	public final int getBottomMargin()
	{
		return bottomMargin;
	}

	/**
	 * @param bottomMargin the bottomMargin to set
	 */
	public final void setBottomMargin(int bottomMargin)
	{
		this.bottomMargin = bottomMargin;
	}

	/**
	 * @return the leftMargin
	 */
	public final int getLeftMargin()
	{
		return leftMargin;
	}

	/**
	 * @param leftMargin the leftMargin to set
	 */
	public final void setLeftMargin(int leftMargin)
	{
		this.leftMargin = leftMargin;
	}

	/**
	 * @return the rightMargin
	 */
	public final int getRightMargin()
	{
		return rightMargin;
	}

	/**
	 * @param rightMargin the rightMargin to set
	 */
	public final void setRightMargin(int rightMargin)
	{
		this.rightMargin = rightMargin;
	}
}
