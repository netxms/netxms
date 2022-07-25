/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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

import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Point;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.Threshold;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.modules.dashboards.views.DashboardView;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.xnap.commons.i18n.I18n;

/**
 * Base class for data comparison charts - like bar chart, pie chart, etc.
 */
public abstract class ComparisonChartElement extends ElementWidget
{
   private final I18n i18n = LocalizationHelper.getI18n(ComparisonChartElement.class);

   protected Chart chart;
	protected NXCSession session;
	protected int refreshInterval = 30;
	protected boolean updateThresholds = false;
	
	private ViewRefreshController refreshController;
	private boolean updateInProgress = false;

	/**
	 * @param parent
	 * @param data
	 */
   public ComparisonChartElement(DashboardControl parent, DashboardElement element, DashboardView view)
	{
      super(parent, element, view);
      session = Registry.getSession();

		addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if (refreshController != null)
               refreshController.dispose();
         }
      });
	}

	/**
	 * Start refresh timer
	 */
	protected void startRefreshTimer()
	{
      refreshController = new ViewRefreshController(view, refreshInterval, new Runnable() {
			@Override
			public void run()
			{
				if (ComparisonChartElement.this.isDisposed())
					return;
				
				refreshData(getDciList());
			}
		});
		refreshData(getDciList());
	}

	/**
	 * Refresh graph's data
	 */
	protected void refreshData(final ChartDciConfig[] dciList)
	{
		if (updateInProgress)
			return;

		updateInProgress = true;

      Job job = new Job(i18n.tr("Reading DCI data for chart"), view, this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				final DciData[] data = new DciData[dciList.length];
				for(int i = 0; i < dciList.length; i++)
				{
					if (dciList[i].type == ChartDciConfig.ITEM)
						data[i] = session.getCollectedData(dciList[i].nodeId, dciList[i].dciId, null, null, 1, HistoricalDataType.PROCESSED);
					else
						data[i] = session.getCollectedTableData(dciList[i].nodeId, dciList[i].dciId, dciList[i].instance, dciList[i].column, null, null, 1);
				}

            final Threshold[][] thresholds;
            if (updateThresholds)
            {
               thresholds = new Threshold[dciList.length][];
               for(int i = 0; i < dciList.length; i++)
               {
                  if (dciList[i].type == ChartDciConfig.ITEM)
                     thresholds[i] = session.getThresholds(dciList[i].nodeId, dciList[i].dciId);
                  else
                     thresholds[i] = new Threshold[0];
               }
            }
            else
            {
               thresholds = null;
            }
				
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
                  updateInProgress = false;
                  if (chart.isDisposed())
                     return;

                  for(int i = 0; i < data.length; i++)
						{
                     DciDataRow lastValue = data[i].getLastValue();
                     chart.updateParameter(i, (lastValue != null) ? lastValue : new DciDataRow(new Date(), 0.0), data[i].getDataType(), false);
                     if (updateThresholds)
                        chart.updateParameterThresholds(i, thresholds[i]);
						}
                  chart.refresh();
                  clearMessages();
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot read DCI data");
			}

         @Override
         protected void jobFailureHandler(Exception e)
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  updateInProgress = false;
               }
            });
         }
		};
		job.setUser(false);
		job.start();
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

	protected abstract ChartDciConfig[] getDciList();
}
