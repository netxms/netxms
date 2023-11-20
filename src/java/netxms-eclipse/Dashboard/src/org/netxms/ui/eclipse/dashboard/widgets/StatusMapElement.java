/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
import org.eclipse.ui.IViewPart;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.xml.XMLTools;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.dashboard.widgets.internal.StatusMapConfig;
import org.netxms.ui.eclipse.objectview.widgets.AbstractObjectStatusMap;
import org.netxms.ui.eclipse.objectview.widgets.FlatObjectStatusMap;
import org.netxms.ui.eclipse.objectview.widgets.RadialObjectStatusMap;

/**
 * Status map element for dashboard
 */
public class StatusMapElement extends ElementWidget
{
	private AbstractObjectStatusMap map;
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
         config = XMLTools.createFromXml(StatusMapConfig.class, element.getData());
		}
		catch(Exception e)
		{
         Activator.logError("Cannot parse dashboard element configuration", e);
			config = new StatusMapConfig();
		}

		processCommonSettings(config);

      if (config.isShowRadial())
         map = new RadialObjectStatusMap(viewPart, getContentArea(), SWT.NONE, false);			   
		else
		   map = new FlatObjectStatusMap(viewPart, getContentArea(), SWT.NONE, false);
      map.setFitToScreen(config.isFitToScreen());
      if (!config.isShowRadial())
		   ((FlatObjectStatusMap)map).setGroupObjects(config.isGroupObjects());
      map.setHideObjectsInMaintenance(config.isHideObjectsInMaintenance());
      map.setSeverityFilter(config.getSeverityFilter());
      map.enableFilter(config.isShowTextFilter());
      map.setRootObject(getEffectiveObjectId(config.getObjectId()));

		map.addRefreshListener(new Runnable() {
         @Override
         public void run()
         {
            requestDashboardLayout();
         }
      });
	}
}
