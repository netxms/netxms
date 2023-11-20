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

import java.util.Calendar;
import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.TimePeriod;
import org.netxms.client.constants.TimeFrameType;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.xml.XMLTools;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.ChartType;
import org.netxms.ui.eclipse.charts.widgets.Chart;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.widgets.internal.AvailabilityChartConfig;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.DateBuilder;
import org.netxms.ui.eclipse.tools.ViewRefreshController;

/**
 * Availability chart element
 */
public class AvailabilityChartElement extends ElementWidget
{
   private AvailabilityChartConfig elementConfig;
   private Chart chart;
   private NXCSession session;
   private ViewRefreshController refreshController;

	/**
	 * @param parent
	 * @param data
	 */
	public AvailabilityChartElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);
      session = ConsoleSharedData.getSession();

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if (refreshController != null)
               refreshController.dispose();
         }
      });

		try
		{
         elementConfig = XMLTools.createFromXml(AvailabilityChartConfig.class, element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
         elementConfig = new AvailabilityChartConfig();
		}

      processCommonSettings(elementConfig);

      ChartConfiguration chartConfig = new ChartConfiguration();
      chartConfig.setTitleVisible(false);
      chartConfig.setLegendPosition(elementConfig.getLegendPosition());
      chartConfig.setLegendVisible(elementConfig.isShowLegend());
      chartConfig.setTranslucent(elementConfig.isTranslucent());
      chart = new Chart(getContentArea(), SWT.NONE, ChartType.PIE, chartConfig);
      chart.addParameter(new GraphItem("Uptime", "Uptime", null));
      chart.addParameter(new GraphItem("Downtime", "Downtime", null));
      chart.setPaletteEntry(0, new ChartColor(127, 154, 72));
      chart.setPaletteEntry(1, new ChartColor(158, 65, 62));
      chart.rebuild();

      if ((elementConfig.getPeriod() & 0x01) == 0)
      {
         // "This xxx" type - refresh every minute because current day is included
         refreshController = new ViewRefreshController(viewPart, 60, new Runnable() {
            @Override
            public void run()
            {
               if (AvailabilityChartElement.this.isDisposed())
                  return;
               refreshData();
            }
         });
      }
      refreshData();
	}

   /**
    * Refresh chart data
    */
   private void refreshData()
	{
      final TimePeriod timePeriod = buildTimePeriod(elementConfig.getPeriod(), elementConfig.getNumberOfDays());
      ConsoleJob availabilityJob = new ConsoleJob("Get business service availability", viewPart, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final double availability = session.getBusinessServiceAvailablity(elementConfig.getObjectId(), timePeriod);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  chart.updateParameter(0, availability, false);
                  chart.updateParameter(1, 100.0 - availability, true);
                  chart.clearErrors();

                  if ((elementConfig.getPeriod() & 0x01) != 0)
                  {
                     // "Last XXX" type of period - refresh at next midnight plus one minute
                     Date refreshTime = new DateBuilder().add(Calendar.DAY_OF_MONTH, 1).setMidnight().add(Calendar.MINUTE, 1).create();
                     getDisplay().timerExec((int)(refreshTime.getTime() - System.currentTimeMillis()), new Runnable() {
                        @Override
                        public void run()
                        {
                           if (!AvailabilityChartElement.this.isDisposed())
                              refreshData();
                        }
                     });
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot get availability for business service %s", session.getObjectName(elementConfig.getObjectId()));
         }

         @Override
         protected IStatus createFailureStatus(final Exception e)
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  chart.addError(getErrorMessage() + " (" + e.getLocalizedMessage() + ")"); //$NON-NLS-1$ //$NON-NLS-2$
               }
            });
            return Status.OK_STATUS;
         }
      };
      availabilityJob.setUser(false);
      availabilityJob.start();
	}

   /**
    * Build time period from
    * 
    * @param periodType
    * @param numberOfDays
    * @return
    */
   private static TimePeriod buildTimePeriod(int periodType, int numberOfDays)
   {
      switch(periodType)
      {
         case AvailabilityChartConfig.CUSTOM:
            return new TimePeriod(TimeFrameType.FIXED, 0, null, new DateBuilder().add(Calendar.DAY_OF_MONTH, 1 - numberOfDays).setMidnight().create(), new Date());
         case AvailabilityChartConfig.LAST_MONTH:
            return new TimePeriod(TimeFrameType.FIXED, 0, null, new DateBuilder().add(Calendar.MONTH, -1).set(Calendar.DAY_OF_MONTH, 1).setMidnight().create(),
                  new DateBuilder().add(Calendar.MONTH, -1).setLastDayOfMonth().setMidnight().add(Calendar.SECOND, 86399).create());
         case AvailabilityChartConfig.LAST_WEEK:
            return new TimePeriod(TimeFrameType.FIXED, 0, null, new DateBuilder().add(Calendar.WEEK_OF_YEAR, -1).set(Calendar.DAY_OF_WEEK, Calendar.MONDAY).setMidnight().create(),
                  new DateBuilder().set(Calendar.DAY_OF_WEEK, Calendar.MONDAY).setMidnight().add(Calendar.SECOND, -1).create());
         case AvailabilityChartConfig.LAST_YEAR:
            return new TimePeriod(TimeFrameType.FIXED, 0, null, new DateBuilder().add(Calendar.YEAR, -1).set(Calendar.DAY_OF_YEAR, 1).setMidnight().create(),
                  new DateBuilder().set(Calendar.DAY_OF_YEAR, 1).setMidnight().add(Calendar.SECOND, -1).create());
         case AvailabilityChartConfig.THIS_MONTH:
            return new TimePeriod(TimeFrameType.FIXED, 0, null, new DateBuilder().set(Calendar.DAY_OF_MONTH, 1).setMidnight().create(), new Date());
         case AvailabilityChartConfig.THIS_WEEK:
            return new TimePeriod(TimeFrameType.FIXED, 0, null, new DateBuilder().set(Calendar.DAY_OF_WEEK, Calendar.MONDAY).setMidnight().create(), new Date());
         case AvailabilityChartConfig.THIS_YEAR:
            return new TimePeriod(TimeFrameType.FIXED, 0, null, new DateBuilder().set(Calendar.DAY_OF_YEAR, 1).setMidnight().create(), new Date());
         case AvailabilityChartConfig.TODAY:
            return new TimePeriod(TimeFrameType.FIXED, 0, null, new DateBuilder().setMidnight().create(), new Date());
         case AvailabilityChartConfig.YESTERDAY:
            return new TimePeriod(TimeFrameType.FIXED, 0, null, new DateBuilder().setMidnight().add(Calendar.DATE, -1).create(), new DateBuilder().setMidnight().add(Calendar.SECOND, -1).create());
      }
      return null;
   }
}
