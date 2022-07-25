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
package org.netxms.nxmc.modules.dashboards.widgets;

import org.eclipse.swt.SWT;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.nxmc.modules.dashboards.config.GeoMapConfig;
import org.netxms.nxmc.modules.dashboards.views.DashboardView;
import org.netxms.nxmc.modules.worldmap.widgets.ObjectGeoLocationViewer;

/**
 * Geo map element for dashboard
 */
public class GeoMapElement extends ElementWidget
{
	private ObjectGeoLocationViewer mapWidget;
	private GeoMapConfig config;
	
	/**
	 * @param parent
	 * @param data
	 */
   public GeoMapElement(DashboardControl parent, DashboardElement element, DashboardView view)
	{
      super(parent, element, view);

		try
		{
			config = GeoMapConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new GeoMapConfig();
		}

      processCommonSettings(config);

      mapWidget = new ObjectGeoLocationViewer(getContentArea(), SWT.NONE, view);
		mapWidget.setRootObjectId(config.getRootObjectId());
		mapWidget.showMap(config.getLatitude(), config.getLongitude(), config.getZoom());
	}
}
