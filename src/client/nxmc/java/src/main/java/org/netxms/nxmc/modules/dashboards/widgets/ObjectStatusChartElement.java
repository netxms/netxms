/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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

import java.util.Arrays;
import org.eclipse.swt.SWT;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Sensor;
import org.netxms.nxmc.modules.charts.api.ChartColor;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.modules.dashboards.config.ObjectStatusChartConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.Gson;

/**
 * Status chart element
 */
public class ObjectStatusChartElement extends ComparisonChartElement
{
   private static final Logger logger = LoggerFactory.getLogger(ObjectStatusChartElement.class);

	private ObjectStatusChartConfig elementConfig;

	/**
	 * @param parent
	 * @param data
	 */
   public ObjectStatusChartElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
	{
      super(parent, element, view);
      
      try
      {
         elementConfig = new Gson().fromJson(element.getData(), ObjectStatusChartConfig.class);
      }
      catch(Exception e)
      {
         logger.error("Cannot parse dashboard element configuration", e);
         elementConfig = new ObjectStatusChartConfig();
      }

      processCommonSettings(elementConfig);

		refreshInterval = elementConfig.getRefreshRate();

      ChartConfiguration chartConfig = new ChartConfiguration();
      chartConfig.setTitleVisible(false);
      chartConfig.setLegendPosition(ChartConfiguration.POSITION_RIGHT);
      chartConfig.setLegendVisible(elementConfig.isShowLegend());
      chartConfig.setTransposed(elementConfig.isTransposed());
      chartConfig.setTranslucent(elementConfig.isTranslucent());
      chart = new Chart(getContentArea(), SWT.NONE, ChartType.BAR, chartConfig, view);

		for(int i = 0; i <= ObjectStatus.UNKNOWN.getValue(); i++)
		{
         ChartDciConfig item = new ChartDciConfig();
         item.name = StatusDisplayInfo.getStatusText(i);
         item.dciDescription = item.name;
         chart.addParameter(item);
			chart.setPaletteEntry(i, new ChartColor(StatusDisplayInfo.getStatusColor(i).getRGB()));
		}
      chart.rebuild();

		startRefreshTimer();
	}

   /**
    * @see org.netxms.ui.eclipse.dashboard.widgets.ComparisonChartElement#getDciList()
    */
	@Override
	protected ChartDciConfig[] getDciList()
	{
		return null;
	}

   /**
    * @see org.netxms.nxmc.modules.dashboards.widgets.ComparisonChartElement#refreshData()
    */
	@Override
   protected void refreshData()
	{
		int[] objectCount = new int[6];
		Arrays.fill(objectCount, 0);

      AbstractObject root = session.findObjectById(getEffectiveObjectId(elementConfig.getRootObject()));
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
	private void collectData(int[] objectCount, AbstractObject root)
	{
		for(AbstractObject o : root.getAllChildren(-1))
		{
         if ((o.getStatus().compareTo(ObjectStatus.UNKNOWN) <= 0) && filterObject(o))
				objectCount[o.getStatus().getValue()]++;
		}
	}

	/**
	 * @param o
	 * @return
	 */
	private boolean filterObject(AbstractObject o)
	{
	   return (o instanceof Node) || (o instanceof MobileDevice) || (o instanceof Sensor) || (o instanceof AccessPoint);
	}
}
