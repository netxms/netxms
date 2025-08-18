/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.dashboards.config.NetworkMapConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.modules.networkmaps.widgets.NetworkMapWidget;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.Gson;

/**
 * Network map element for dashboard
 *
 */
public class NetworkMapElement extends ElementWidget
{
   private static final Logger logger = LoggerFactory.getLogger(NetworkMapElement.class);

	private NetworkMap mapObject;
	private NetworkMapWidget mapWidget;
	private NetworkMapConfig config;

	/**
	 * @param parent
	 * @param data
	 */
   public NetworkMapElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
	{
      super(parent, element, view);

      try
      {
         config = new Gson().fromJson(element.getData(), NetworkMapConfig.class);
      }
      catch(Exception e)
      {
         logger.error("Cannot parse dashboard element configuration", e);
         config = new NetworkMapConfig();
      }

      processCommonSettings(config);

      NXCSession session = Registry.getSession();
      mapObject = session.findObjectById(config.getObjectId(), NetworkMap.class);

		if (mapObject != null)
		{
         mapWidget = new NetworkMapWidget(getContentArea(), SWT.NONE, view);
			mapWidget.setContent(mapObject);
	      mapWidget.zoomTo((double)config.getZoomLevel() / 100.0);
		}

		if (config.isObjectDoubleClickEnabled())
		{
		   mapWidget.enableObjectDoubleClick();
		}

		if (config.isHideLinkLabelsEnabled())
		{
		   mapWidget.hideLinkLabels(true);
		}
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
