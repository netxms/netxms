/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
import org.netxms.ui.eclipse.charts.api.ChartFactory;
import org.netxms.ui.eclipse.dashboard.widgets.internal.TableBarChartConfig;

/**
 * Bar chart for tables
 */
public class TableBarChartElement extends TableComparisonChartElement
{
	/**
	 * @param parent
	 * @param data
	 */
	public TableBarChartElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);
		
		try
		{
			config = TableBarChartConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new TableBarChartConfig();
		}

		refreshInterval = config.getRefreshRate() * 1000;
		chart = ChartFactory.createBarChart(this, SWT.NONE);
		chart.setTitleVisible(config.isShowTitle());
		chart.setChartTitle(config.getTitle());
		chart.setLegendPosition(config.getLegendPosition());
		chart.setLegendVisible(config.isShowLegend());
		chart.set3DModeEnabled(config.isShowIn3D());
		chart.setTransposed(((TableBarChartConfig)config).isTransposed());
		chart.setTranslucent(config.isTranslucent());
		
		startRefreshTimer();
	}
}
