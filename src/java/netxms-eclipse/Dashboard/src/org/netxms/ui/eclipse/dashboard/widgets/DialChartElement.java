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

import org.eclipse.swt.SWT;
import org.eclipse.ui.IViewPart;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.ChartFactory;
import org.netxms.ui.eclipse.charts.api.DialChart;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardDciInfo;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DialChartConfig;

/**
 * Dial chart element
 */
public class DialChartElement extends ComparisonChartElement
{
	private DialChartConfig config;
	
	/**
	 * @param parent
	 * @param data
	 */
	public DialChartElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);
		
		try
		{
			config = DialChartConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new DialChartConfig();
		}

		chart = ChartFactory.createDialChart(this, SWT.NONE);
		chart.setChartTitle(config.getTitle());
		chart.setTitleVisible(config.isShowTitle());
		chart.setLegendVisible(config.isShowLegend());
		((DialChart)chart).setLegendInside(config.isLegendInside());
		((DialChart)chart).setMinValue(config.getMinValue());
		((DialChart)chart).setMaxValue(config.getMaxValue());
		((DialChart)chart).setLeftYellowZone(config.getLeftYellowZone());
		((DialChart)chart).setLeftRedZone(config.getLeftRedZone());
		((DialChart)chart).setRightYellowZone(config.getRightYellowZone());
		((DialChart)chart).setRightRedZone(config.getRightRedZone());
		
		int index = 0;
		for(DashboardDciInfo dci : config.getDciList())
		{
			chart.addParameter(new GraphItem(dci.nodeId, dci.dciId, 0, 0, Long.toString(dci.dciId), dci.getName()), 0.0);
			int color = dci.getColorAsInt();
			if (color != -1)
				chart.setPaletteEntry(index++, new ChartColor(color));
		}
		chart.initializationComplete();

		startRefreshTimer();
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.dashboard.widgets.ComparisonChartElement#getDciList()
	 */
	@Override
	protected DashboardDciInfo[] getDciList()
	{
		return config.getDciList();
	}
}
