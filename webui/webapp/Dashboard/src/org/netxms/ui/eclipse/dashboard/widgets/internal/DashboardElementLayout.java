/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import org.netxms.client.dashboards.DashboardElement;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Layout configuration for dashboard element
 */
@Root(name="layout", strict=false)
public class DashboardElementLayout
{
	@Element(required=false)
	public int horizontalSpan = 1;
	
	@Element(required=false)
	public int verticalSpan = 1;
	
	@Element(required=false)
	public int horizontalAlignment = DashboardElement.FILL;

	@Element(required=false)
	public int vertcalAlignment = DashboardElement.FILL;
	
	@Element(required=false)
	public boolean grabHorizontalSpace = true;

	@Element(required=false)
	public boolean grabVerticalSpace = true;

	@Element(required=false)
	public int widthHint = -1;
	
	@Element(required=false)
	public int heightHint = -1;
	
	/**
	 * @param xml
	 * @return
	 * @throws Exception
	 */
	public static DashboardElementLayout createFromXml(final String xml) throws Exception
	{
		Serializer serializer = new Persister();
		return serializer.read(DashboardElementLayout.class, xml);
	}
	
	/**
	 * @return
	 * @throws Exception
	 */
	public String createXml() throws Exception
	{
		Serializer serializer = new Persister();
		StringWriter writer = new StringWriter();
		serializer.write(this, writer);
		return writer.toString();
	}
}
