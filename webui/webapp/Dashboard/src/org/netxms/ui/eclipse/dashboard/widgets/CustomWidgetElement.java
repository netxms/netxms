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

import org.eclipse.core.runtime.IAdapterManager;
import org.eclipse.core.runtime.Platform;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.ui.IViewPart;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.dashboard.api.DashboardElementCreationData;
import org.netxms.ui.eclipse.dashboard.widgets.internal.CustomWidgetConfig;

/**
 * Dashboard element containing custom widget
 */
public class CustomWidgetElement extends ElementWidget
{
	/**
	 * @param parent
	 * @param element
	 */
	public CustomWidgetElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, SWT.NONE, element, viewPart);
		
		CustomWidgetConfig config;
		try
		{
			config = CustomWidgetConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new CustomWidgetConfig();
		}
		
		FillLayout layout = new FillLayout();
		setLayout(layout);

		IAdapterManager adapterManager = Platform.getAdapterManager();
		adapterManager.loadAdapter(new DashboardElementCreationData(this, element.getData()), config.getClassName());
	}
}
