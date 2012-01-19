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
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.ui.IViewPart;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.objects.ServiceContainer;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.ChartFactory;
import org.netxms.ui.eclipse.charts.api.DataComparisonChart;
import org.netxms.ui.eclipse.dashboard.widgets.internal.AvailabilityChartConfig;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Business service availability chart element
 */
public class AvailabilityChartElement extends ElementWidget implements DisposeListener, SessionListener
{
	private AvailabilityChartConfig config;
	private DataComparisonChart chart;

	/**
	 * @param parent
	 * @param data
	 */
	public AvailabilityChartElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);
		
		try
		{
			config = AvailabilityChartConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new AvailabilityChartConfig();
		}

		setLayout(new FillLayout());
		
		chart = ChartFactory.createPieChart(this, SWT.NONE);
		chart.setTitleVisible(config.isShowTitle());
		chart.setChartTitle(config.getTitle());
		chart.setLegendPosition(config.getLegendPosition());
		chart.setLegendVisible(config.isShowLegend());
		chart.set3DModeEnabled(config.isShowIn3D());
		chart.setTranslucent(config.isTranslucent());
		chart.setRotation(225.0);
		
		chart.addParameter(new GraphItem(0, 0, 0, 0, "Up", "Uptime"), 100);
		chart.addParameter(new GraphItem(0, 0, 0, 0, "Down", "Downtime"), 0);
		chart.setPaletteEntry(0, new ChartColor(127, 154, 72));
		chart.setPaletteEntry(1, new ChartColor(158, 65, 62));
		chart.initializationComplete();
		
		addDisposeListener(this);
		ConsoleSharedData.getSession().addListener(this);

		refreshChart();
	}
	
	/**
	 * Refresh chart
	 */
	private void refreshChart()
	{
		NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		ServiceContainer service = (ServiceContainer)session.findObjectById(config.getObjectId(), ServiceContainer.class);
		if (service != null)
		{
			chart.updateParameter(0, service.getUptimeForDay(), false);
			chart.updateParameter(1, 100.0 - service.getUptimeForDay(), true);
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.DisposeListener#widgetDisposed(org.eclipse.swt.events.DisposeEvent)
	 */
	@Override
	public void widgetDisposed(DisposeEvent e)
	{
		ConsoleSharedData.getSession().removeListener(this);
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
	 */
	@Override
	public void notificationHandler(SessionNotification n)
	{
		if ((n.getCode() == NXCNotification.OBJECT_CHANGED) &&
		    (n.getSubCode() == config.getObjectId()) &&
		    !isDisposed())
		{
			getDisplay().asyncExec(new Runnable() {
				@Override
				public void run()
				{
					refreshChart();
				}
			});
		}
	}
}
