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
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.IViewPart;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.dashboard.widgets.internal.StatusMapConfig;
import org.netxms.ui.eclipse.objectview.widgets.ObjectStatusMap;

/**
 * Status map element for dashboard
 */
public class StatusMapElement extends ElementWidget
{
	private ObjectStatusMap map;
	private StatusMapConfig config;
	
	/**
	 * @param parent
	 * @param data
	 */
	public StatusMapElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);

		try
		{
			config = StatusMapConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new StatusMapConfig();
		}
		
		if (config.getTitle().trim().isEmpty())
		{
			setLayout(new FillLayout());
			map = new ObjectStatusMap(viewPart, this, SWT.NONE);
		}
		else
		{
			GridLayout layout = new GridLayout();
			layout.marginWidth = 0;
			layout.marginHeight = 0;
			setLayout(layout);

			Label title = new Label(this, SWT.CENTER);
			title.setText(config.getTitle().trim());
			title.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
			title.setFont(JFaceResources.getBannerFont());
			title.setBackground(getBackground());
			
			map = new ObjectStatusMap(viewPart, this, SWT.NONE);
			map.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		}
		map.setGroupObjects(config.isGroupObjects());
		map.setSeverityFilter(config.getSeverityFilter());
		map.setRootObject(config.getObjectId());
	}
}
