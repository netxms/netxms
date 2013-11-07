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

import org.simpleframework.xml.Element;

/**
 * Common base class for comparison chart configs
 */
public abstract class ComparisonChartConfig extends AbstractChartConfig
{
	@Element(required = false)
	private boolean showIn3D = false;
	
	@Element(required = false)
	private boolean translucent = false;

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
}
