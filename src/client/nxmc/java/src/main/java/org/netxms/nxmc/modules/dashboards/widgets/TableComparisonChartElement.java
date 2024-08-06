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

import java.util.Comparator;
import java.util.HashMap;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Widget;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableCell;
import org.netxms.client.TableRow;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.modules.dashboards.config.TableComparisonChartConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.xnap.commons.i18n.I18n;

/**
 * Base class for data comparison charts based on table DCI - like bar chart, pie chart, etc.
 */
public abstract class TableComparisonChartElement extends ElementWidget
{
   private final I18n i18n = LocalizationHelper.getI18n(TableComparisonChartElement.class);

   protected Chart chart;
	protected NXCSession session;
	protected TableComparisonChartConfig config;
	
	private ViewRefreshController refreshController;
	private boolean updateInProgress = false;
   private Map<String, Integer> instanceMap = new HashMap<String, Integer>(ChartConfiguration.MAX_GRAPH_ITEM_COUNT);
	private boolean chartInitialized = false;

	/**
	 * @param parent
	 * @param data
	 */
   public TableComparisonChartElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
	{
      super(parent, element, view);
      session = Registry.getSession();

      addDisposeListener((e) -> {
         if (refreshController != null)
            refreshController.dispose();
      });
	}

	/**
	 * Start refresh timer
	 */
	protected void startRefreshTimer()
	{
		if ((config == null) || (config.getDataColumn() == null))
			return;	// Invalid configuration

      refreshController = new ViewRefreshController(view, config.getRefreshRate(), () -> {
         if (TableComparisonChartElement.this.isDisposed())
            return;

         refreshData();
		});
		refreshData();
	}

	/**
	 * Refresh graph's data
	 */
	protected void refreshData()
	{
		if (updateInProgress)
			return;
		
		updateInProgress = true;
		
      Job job = new Job(i18n.tr("Reading DCI values for table comparision chart"), view, this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				final Table data = session.getTableLastValues(config.getNodeId(), config.getDciId());
            runInUIThread(() -> {
               if (!((Widget)chart).isDisposed())
                  updateChart(data);
               updateInProgress = false;
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot get DCI values for comparision chart");
			}
	
			@Override
         protected void jobFailureHandler(Exception e)
			{
				updateInProgress = false;
            super.jobFailureHandler(e);
			}
		};
		job.setUser(false);
		job.start();
	}

	/**
	 * Update chart with new data
	 * 
	 * @param data
	 */
	private void updateChart(final Table data)
	{
		String instanceColumn = (config.getInstanceColumn() != null) ? config.getInstanceColumn() : ""; // FIXME //$NON-NLS-1$
		if (instanceColumn == null)
			return;

		final int icIndex = data.getColumnIndex(instanceColumn);
		final int dcIndex = data.getColumnIndex(config.getDataColumn());
		if ((icIndex == -1) || (dcIndex == -1))
			return;	// at least one column is missing

		if (config.isSortOnDataColumn())
		{
		   data.sort(new Comparator<TableRow>() {
            @Override
            public int compare(TableRow row1, TableRow row2)
            {
               TableCell c1 = row1.get(dcIndex);
               TableCell c2 = row2.get(dcIndex);

               String s1 = (c1 != null) ? c1.getValue() : "";
               String s2 = (c2 != null) ? c2.getValue() : "";
               
               int result = 0;
               try
               {
                  double value1 = Double.parseDouble(s1);
                  double value2 = Double.parseDouble(s2);
                  result = Double.compare(value1, value2);
               }
               catch(NumberFormatException e)
               {
                  result = s1.compareToIgnoreCase(s2);
               }
               return config.isSortDescending() ? -result : result;
            }
         });
		   
		   // Sorting may reorder instances, so clear everything
		   instanceMap.clear();
		   chart.removeAllParameters();
		}

		boolean rebuild = false;
		for(int i = 0; i < data.getRowCount(); i++)
		{
			String instance = data.getCellValue(i, icIndex);
			if (instance == null)
				continue;

			double value;
			try
			{
				value = Double.parseDouble(data.getCellValue(i, dcIndex));
			}
			catch(NumberFormatException e)
			{
				value = 0.0;
			}
			
			Integer index = instanceMap.get(instance);
			if (index == null)
			{
            if ((instanceMap.size() >= ChartConfiguration.MAX_GRAPH_ITEM_COUNT) ||
				    ((value == 0) && config.isIgnoreZeroValues()))
					continue;

            ChartDciConfig item = new ChartDciConfig();
            item.nodeId = config.getNodeId();
            item.dciId = config.getDciId();
            item.name = instance;
            item.dciDescription = instance;
            index = chart.addParameter(item);
				instanceMap.put(instance, index);
				rebuild = true;
			}

			chart.updateParameter(index, value, false);
		}

		if (!chartInitialized)
		{
         chart.rebuild();
			chartInitialized = true;
		}
		else
		{
			if (rebuild)
				chart.rebuild();
			else
				chart.refresh();
		}
	}

   /**
    * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
    */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		Point size = super.computeSize(wHint, hHint, changed);
		if ((hHint == SWT.DEFAULT) && (size.y < 250))
			size.y = 250;
		return size;
	}
}
