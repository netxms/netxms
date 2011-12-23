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
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.dashboard.widgets.internal.EmbeddedDashboardConfig;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Embedded dashboard element
 *
 */
public class EmbeddedDashboardElement extends ElementWidget
{
	private Dashboard object;
	private EmbeddedDashboardConfig config;
	
	/**
	 * @param parent
	 * @param data
	 */
	public EmbeddedDashboardElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, SWT.NONE, element, viewPart);

		try
		{
			config = EmbeddedDashboardConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new EmbeddedDashboardConfig();
		}
		
		NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		object = (Dashboard)session.findObjectById(config.getObjectId(), Dashboard.class);
		
		FillLayout layout = new FillLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		setLayout(layout);
		
		if (object != null)
		{
			new DashboardControl(this, SWT.NONE, object, viewPart, true);	/* TODO: set embedded=false if border=true */
		}
	}
}
