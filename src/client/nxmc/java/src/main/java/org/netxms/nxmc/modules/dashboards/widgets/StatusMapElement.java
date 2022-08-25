/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.widgets;

import org.eclipse.swt.SWT;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.nxmc.modules.dashboards.config.StatusMapConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.modules.objects.widgets.AbstractObjectStatusMap;
import org.netxms.nxmc.modules.objects.widgets.FlatObjectStatusMap;
import org.netxms.nxmc.modules.objects.widgets.RadialObjectStatusMap;

/**
 * Status map element for dashboard
 */
public class StatusMapElement extends ElementWidget
{
	private AbstractObjectStatusMap map;
	private StatusMapConfig config;

	/**
    * @param parent
    * @param element
    * @param view
    */
   public StatusMapElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
	{
      super(parent, element, view, true);

		try
		{
			config = StatusMapConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new StatusMapConfig();
		}

		processCommonSettings(config);

      if (config.isShowRadial())
         map = new RadialObjectStatusMap(view, getContentArea(), SWT.NONE);
		else
         map = new FlatObjectStatusMap(view, getContentArea(), SWT.NONE);
      map.setFitToScreen(config.isFitToScreen());
      if (!config.isShowRadial())
		   ((FlatObjectStatusMap)map).setGroupObjects(config.isGroupObjects());
      map.setHideObjectsInMaintenance(config.isHideObjectsInMaintenance());
		map.setSeverityFilter(config.getSeverityFilter());
      enableFilter(config.isShowTextFilter());
		map.setRootObject(config.getObjectId());

		map.addRefreshListener(new Runnable() {
         @Override
         public void run()
         {
            requestDashboardLayout();
         }
      });
	}

   /**
    * @see org.netxms.nxmc.modules.dashboards.widgets.ElementWidget#onFilterModify()
    */
   @Override
   protected void onFilterModify()
   {
      map.setTextFilter(getFilterText());
   }
}
