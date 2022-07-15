/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.ui.IViewPart;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.DataType;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DciInfo;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.ChartType;
import org.netxms.ui.eclipse.charts.widgets.Chart;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.widgets.internal.GaugeConfig;
import org.netxms.ui.eclipse.jobs.ConsoleJob;

/**
 * Dial chart element
 */
public class GaugeElement extends ComparisonChartElement
{
	private GaugeConfig elementConfig;
	
	/**
	 * @param parent
	 * @param data
	 */
	public GaugeElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);

		try
		{
			elementConfig = GaugeConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			elementConfig = new GaugeConfig();
		}

      processCommonSettings(elementConfig);

		refreshInterval = elementConfig.getRefreshRate();
		
      ChartConfiguration chartConfig = new ChartConfiguration();
      chartConfig.setTitleVisible(false);
      chartConfig.setLegendVisible(false);
      chartConfig.setLegendPosition(elementConfig.getLegendPosition());
      chartConfig.setLabelsVisible(elementConfig.isShowLegend());
      chartConfig.setLabelsInside(elementConfig.isLegendInside());
      chartConfig.setTransposed(elementConfig.isVertical());
      chartConfig.setElementBordersVisible(elementConfig.isElementBordersVisible());
      chartConfig.setMinYScaleValue(elementConfig.getMinValue());
      chartConfig.setMaxYScaleValue(elementConfig.getMaxValue());
      chartConfig.setLeftYellowZone(elementConfig.getLeftYellowZone());
      chartConfig.setLeftRedZone(elementConfig.getLeftRedZone());
      chartConfig.setRightYellowZone(elementConfig.getRightYellowZone());
      chartConfig.setRightRedZone(elementConfig.getRightRedZone());
      chartConfig.setFontName(elementConfig.getFontName());
      chartConfig.setGaugeColorMode(elementConfig.getColorMode());

		switch(elementConfig.getGaugeType())
		{
         case GaugeConfig.BAR:
            chart = new Chart(getContentArea(), SWT.NONE, ChartType.GAUGE, chartConfig);
            updateThresholds = true;
            break;
			case GaugeConfig.TEXT:
            chart = new Chart(getContentArea(), SWT.NONE, ChartType.TEXT, chartConfig);
		      updateThresholds = true;
				break;
			default:
            chart = new Chart(getContentArea(), SWT.NONE, ChartType.DIAL, chartConfig);
				break;
		}
      chart.setDrillDownObjectId(elementConfig.getDrillDownObjectId());

		for(ChartDciConfig dci : elementConfig.getDciList())
         chart.addParameter(new GraphItem(dci, DataOrigin.INTERNAL, DataType.INT32));

      chart.setPaletteEntry(0, new ChartColor(elementConfig.getCustomColor()));
      chart.rebuild();

      updateDciInfo();
		startRefreshTimer();
	}
   
   /**
    * Get DCI info (unit name and multiplier)
    */
   private void updateDciInfo()
   {
      List<Long> nodeIds = new ArrayList<Long>();
      List<Long> dciIds = new ArrayList<Long>();
      
      for(ChartDciConfig dci : elementConfig.getDciList())
      {
         if (dci.getDisplayFormat() == null || dci.getDisplayFormat().isEmpty())
         {
            nodeIds.add(dci.nodeId);
            dciIds.add(dci.dciId);
         }
      }
      
      ConsoleJob job = new ConsoleJob("Get DCI info", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final Map<Long, DciInfo> result = session.getDciInfo(nodeIds, dciIds);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  int i = 0;
                  for(ChartDciConfig dci : elementConfig.getDciList())
                  {
                     GraphItem item = chart.getItem(i);
                     DciInfo info = result.get(dci.getDciId());
                     if (info != null)
                     {
                        item.setUnitName(info.getUnitName());
                        item.setMultipierPower(info.getMultipierPower());
                     }
                     i++;
                  }
                  chart.rebuild();
                  layout(true, true);          
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return null;
         }
      };
      job.setUser(false);
      job.start();
      
   }

   /**
    * @see org.netxms.ui.eclipse.dashboard.widgets.ComparisonChartElement#getDciList()
    */
	@Override
	protected ChartDciConfig[] getDciList()
	{
		return elementConfig.getDciList();
	}
}
