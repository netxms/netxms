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
package org.netxms.ui.eclipse.dashboard;

import org.eclipse.core.runtime.IAdapterFactory;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.dashboard.widgets.internal.AlarmViewerConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.AvailabilityChartConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.BarChartConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.EmbeddedDashboardConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.GeoMapConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.LabelConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.LineChartConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.NetworkMapConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.PieChartConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.StatusIndicatorConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.TubeChartConfig;

/**
 * Adapter factory for dashboard elements
 */
public class DashboardElementAdapterFactory implements IAdapterFactory
{
	@SuppressWarnings("rawtypes")
	private static final Class[] supportedClasses = 
	{
		DashboardElementConfig.class
	};

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.IAdapterFactory#getAdapterList()
	 */
	@SuppressWarnings("rawtypes")
	@Override
	public Class[] getAdapterList()
	{
		return supportedClasses;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.IAdapterFactory#getAdapter(java.lang.Object, java.lang.Class)
	 */
	@SuppressWarnings("rawtypes")
	@Override
	public Object getAdapter(Object adaptableObject, Class adapterType)
	{
		if (!(adaptableObject instanceof DashboardElement))
			return null;
		
		if (adapterType == DashboardElementConfig.class)
		{
			DashboardElement element = (DashboardElement)adaptableObject;
			try
			{
				switch(element.getType())
				{
					case DashboardElement.ALARM_VIEWER:
						return AlarmViewerConfig.createFromXml(element.getData());
					case DashboardElement.AVAILABLITY_CHART:
						return AvailabilityChartConfig.createFromXml(element.getData());
					case DashboardElement.BAR_CHART:
						return BarChartConfig.createFromXml(element.getData());
					case DashboardElement.DASHBOARD:
						return EmbeddedDashboardConfig.createFromXml(element.getData());
					case DashboardElement.GEO_MAP:
						return GeoMapConfig.createFromXml(element.getData());
					case DashboardElement.LABEL:
						return LabelConfig.createFromXml(element.getData());
					case DashboardElement.LINE_CHART:
						return LineChartConfig.createFromXml(element.getData());
					case DashboardElement.NETWORK_MAP:
						return NetworkMapConfig.createFromXml(element.getData());
					case DashboardElement.PIE_CHART:
						return PieChartConfig.createFromXml(element.getData());
					case DashboardElement.STATUS_INDICATOR:
						return StatusIndicatorConfig.createFromXml(element.getData());
					case DashboardElement.TUBE_CHART:
						return TubeChartConfig.createFromXml(element.getData());
					default:
						return null;
				}
			}
			catch(Exception e)
			{
				e.printStackTrace();
				return null;
			}
		}
		return null;
	}
}
