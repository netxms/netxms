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
 * Configuration for bar chart
 */
public class TableBarChartConfig extends TableComparisonChartConfig
{
	@Element(required=false)
	private boolean transposed = false;
	
	/**
	 * Create bar chart settings object from XML document
	 * 
	 * @param xml XML document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
	public static TableBarChartConfig createFromXml(final String xml) throws Exception
	{
		Serializer serializer = new Persister();
		return serializer.read(TableBarChartConfig.class, xml);
	}
	
	/**
	 * @return the transposed
	 */
	public boolean isTransposed()
	{
		return transposed;
	}

	/**
	 * @param transposed the transposed to set
	 */
	public void setTransposed(boolean transposed)
	{
		this.transposed = transposed;
	}
}
