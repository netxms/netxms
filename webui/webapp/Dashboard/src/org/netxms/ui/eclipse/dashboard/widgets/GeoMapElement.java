/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
import org.netxms.ui.eclipse.dashboard.widgets.internal.GeoMapConfig;
import org.netxms.ui.eclipse.osm.widgets.ObjectGeoLocationViewer;

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
	public GeoMapElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);

		try
		{
         config = XMLTools.createFromXml(GeoMapConfig.class, element.getData());
		}
		catch(Exception e)
		{
         Activator.logError("Cannot parse dashboard element configuration", e);
			config = new GeoMapConfig();
		}

      processCommonSettings(config);

      mapWidget = new ObjectGeoLocationViewer(getContentArea(), SWT.NONE);
		mapWidget.setViewPart(viewPart);
      mapWidget.setRootObjectId(getEffectiveObjectId(config.getRootObjectId()));
		mapWidget.showMap(config.getLatitude(), config.getLongitude(), config.getZoom());
	}

   /**
    * @see org.netxms.nxmc.modules.dashboards.widgets.ElementWidget#getPreferredHeight()
    */
   @Override
   protected int getPreferredHeight()
   {
      return 600;
   }
}
