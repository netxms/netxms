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
package org.netxms.nxmc.modules.dashboards.widgets;

import java.util.Calendar;
import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.netxms.client.NXCSession;
import org.netxms.client.TimePeriod;
import org.netxms.client.constants.TimeFrameType;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.charts.api.ChartColor;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.modules.dashboards.config.AvailabilityChartConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.tools.DateBuilder;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;
import com.google.gson.Gson;

/**
 * Availability chart element
 */
public class AvailabilityChartElement extends ElementWidget
{
   private static final Logger logger = LoggerFactory.getLogger(AvailabilityChartElement.class);

   private final I18n i18n = LocalizationHelper.getI18n(AvailabilityChartElement.class);

   private AvailabilityChartConfig elementConfig;
   private Chart chart;
   private NXCSession session;
   private ViewRefreshController refreshController;

   /**
    * @param parent
    * @param element
    * @param view
    */
   public AvailabilityChartElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
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

      try
      {
         elementConfig = new Gson().fromJson(element.getData(), AvailabilityChartConfig.class);
      }
      catch(Exception e)
      {
         logger.error("Cannot parse dashboard element configuration", e);
         elementConfig = new AvailabilityChartConfig();
      }

      processCommonSettings(elementConfig);

      ChartConfiguration chartConfig = new ChartConfiguration();
      chartConfig.setTitleVisible(false);
      chartConfig.setLegendPosition(elementConfig.getLegendPosition());
      chartConfig.setLegendVisible(elementConfig.isShowLegend());
      chartConfig.setTranslucent(elementConfig.isTranslucent());
      chart = new Chart(getContentArea(), SWT.NONE, ChartType.PIE, chartConfig, view);
      chart.addParameter(new ChartDciConfig(i18n.tr("Uptime")));
      chart.addParameter(new ChartDciConfig(i18n.tr("Downtime")));
      chart.setPaletteEntry(0, new ChartColor(127, 154, 72));
      chart.setPaletteEntry(1, new ChartColor(158, 65, 62));
      chart.rebuild();

      if ((elementConfig.getPeriod() & 0x01) == 0)
      {
         // "This xxx" type - refresh every minute because current day is included
         refreshController = new ViewRefreshController(view, 60, new Runnable() {
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
      Job availabilityJob = new Job(i18n.tr("Getting business service availability"), view, this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final double availability = session.getBusinessServiceAvailablity(elementConfig.getObjectId(), timePeriod);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  chart.updateParameter(0, availability, false);
                  chart.updateParameter(1, 100.0 - availability, true);

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
            return String.format(i18n.tr("Cannot get availability for business service %s"), session.getObjectName(elementConfig.getObjectId()));
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
