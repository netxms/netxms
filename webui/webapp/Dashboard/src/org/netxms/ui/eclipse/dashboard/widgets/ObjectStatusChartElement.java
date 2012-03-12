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

import java.util.Arrays;

import org.eclipse.swt.SWT;
import org.eclipse.ui.IViewPart;
import org.netxms.client.constants.Severity;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.ChartFactory;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardDciInfo;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ObjectStatusChartConfig;

/**
 * Line chart element
 */
public class ObjectStatusChartElement extends ComparisonChartElement
{
	private static final long serialVersionUID = 1L;

	private ObjectStatusChartConfig config;
	
	/**
	 * @param parent
	 * @param data
	 */
	public ObjectStatusChartElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);
		
		try
		{
			config = ObjectStatusChartConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new ObjectStatusChartConfig();
		}

		refreshInterval = config.getRefreshRate() * 1000;
		
		chart = ChartFactory.createBarChart(this, SWT.NONE);
		chart.setTitleVisible(true);
		chart.setChartTitle(config.getTitle());
		chart.setLegendPosition(config.getLegendPosition());
		chart.setLegendVisible(config.isShowLegend());
		chart.set3DModeEnabled(config.isShowIn3D());
		chart.setTransposed(config.isTransposed());
		chart.setTranslucent(config.isTranslucent());

		for(int i = 0; i <= Severity.UNKNOWN; i++)
		{
			chart.addParameter(new GraphItem(0, 0, 0, 0, StatusDisplayInfo.getStatusText(i), StatusDisplayInfo.getStatusText(i)), 0.0);
			chart.setPaletteEntry(i, new ChartColor(StatusDisplayInfo.getStatusColor(i).getRGB()));
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
		return null;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.dashboard.widgets.ComparisonChartElement#refreshData(org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardDciInfo[])
	 */
	@Override
	protected void refreshData(DashboardDciInfo[] dciList)
	{
		int[] objectCount = new int[6];
		Arrays.fill(objectCount, 0);

		GenericObject root = session.findObjectById(config.getRootObject());
		if (root != null)
			collectData(objectCount, root);

		for(int i = 0; i < objectCount.length; i++)
			chart.updateParameter(i, objectCount[i], false);
		chart.refresh();
	}

	/**
	 * @param objectCount
	 * @param root
	 */
	private void collectData(int[] objectCount, GenericObject root)
	{
		for(GenericObject o : root.getChildsAsArray())
		{
			if ((o.getStatus() <= Severity.UNKNOWN) && filterObject(o))
				objectCount[o.getStatus()]++;
			collectData(objectCount, o);
		}
	}

	/**
	 * @param o
	 * @return
	 */
	private boolean filterObject(GenericObject o)
	{
		int[] cf = config.getClassFilter();
		for(int i = 0; i < cf.length; i++)
			if (o.getObjectClass() == cf[i])
				return true;
		return false;
	}
}
