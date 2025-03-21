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
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.NetworkMap;
import org.netxms.ui.eclipse.dashboard.widgets.internal.NetworkMapConfig;
import org.netxms.ui.eclipse.networkmaps.widgets.NetworkMapWidget;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import com.google.gson.Gson;

/**
 * Network map element for dashboard
 *
 */
public class NetworkMapElement extends ElementWidget
{
	private NetworkMap mapObject;
	private NetworkMapWidget mapWidget;
	private NetworkMapConfig config;

	/**
	 * @param parent
	 * @param data
	 */
	public NetworkMapElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);

		try
		{
         config = new Gson().fromJson(element.getData(), NetworkMapConfig.class);
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new NetworkMapConfig();
		}

      processCommonSettings(config);

      NXCSession session = ConsoleSharedData.getSession();
      mapObject = session.findObjectById(config.getObjectId(), NetworkMap.class);

		if (mapObject != null)
		{
         mapWidget = new NetworkMapWidget(getContentArea(), viewPart, SWT.NONE);
			mapWidget.setContent(mapObject);
	      mapWidget.zoomTo((double)config.getZoomLevel() / 100.0);
		}

		if (config.isObjectDoubleClickEnabled())
		{
		   mapWidget.enableObjectDoubleClick();
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
