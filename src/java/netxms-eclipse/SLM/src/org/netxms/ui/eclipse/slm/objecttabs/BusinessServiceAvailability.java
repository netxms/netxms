/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.ui.eclipse.slm.objecttabs;

import java.util.Calendar;
import java.util.Date;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.client.TimePeriod;
import org.netxms.client.businessservices.BusinessServiceTicket;
import org.netxms.client.constants.TimeFrameType;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.BusinessServicePrototype;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.ChartType;
import org.netxms.ui.eclipse.charts.widgets.Chart;
import org.netxms.ui.eclipse.compatibility.GraphItem;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.slm.Activator;
import org.netxms.ui.eclipse.slm.objecttabs.helpers.BusinessServiceTicketComparator;
import org.netxms.ui.eclipse.slm.objecttabs.helpers.BusinessServiceTicketLabelProvider;
import org.netxms.ui.eclipse.tools.DateBuilder;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.DateTimeSelector;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Availability view for business services
 */
public class BusinessServiceAvailability extends ObjectTab
{
   public static final int COLUMN_ID = 0;
   public static final int COLUMN_SERVICE_ID = 1;
   public static final int COLUMN_CHECK_ID = 2;
   public static final int COLUMN_CHECK_DESCRIPTION = 3;
   public static final int COLUMN_CREATION_TIME = 4;
   public static final int COLUMN_TERMINATION_TIME = 5;
   public static final int COLUMN_REASON = 6;

   private static final String CONFIG_PREFIX = "BusinessServiceAvailability";

   private NXCSession session;
   private DateTimeSelector startDateSelector;
   private DateTimeSelector endDateSelector;
   private Button buttonQuery;
   private Chart chart;
   private SortableTableViewer ticketViewer;

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createTabContent(Composite parent)
   { 
      session = ConsoleSharedData.getSession();

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      parent.setLayout(layout);

      Composite queryGroup = new Composite(parent, SWT.NONE);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      queryGroup.setLayoutData(gd);
      layout = new GridLayout();
      layout.numColumns = 2;
      queryGroup.setLayout(layout);

      Composite timeSelectorGroup = new Composite(queryGroup, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = 3;
      layout.makeColumnsEqualWidth = false;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.horizontalSpacing = 30;
      timeSelectorGroup.setLayout(layout);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      timeSelectorGroup.setLayoutData(gd);

      startDateSelector = new DateTimeSelector(timeSelectorGroup, SWT.NONE);

      Label label = new Label(timeSelectorGroup, SWT.CENTER);
      label.setText("\u2015");
      gd = new GridData();
      gd.horizontalAlignment = SWT.CENTER;
      gd.verticalAlignment = SWT.CENTER;
      label.setLayoutData(gd);

      endDateSelector = new DateTimeSelector(timeSelectorGroup, SWT.NONE);

      Composite buttonGroup = new Composite(queryGroup, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = 8;
      layout.makeColumnsEqualWidth = true;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      buttonGroup.setLayout(layout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      buttonGroup.setLayoutData(gd);

      createQuickSetButton(buttonGroup, "Today", new DateBuilder().setMidnight().create(), null);
      createQuickSetButton(buttonGroup, "Yesterday", new DateBuilder().setMidnight().add(Calendar.DATE, -1).create(),
            new DateBuilder().setMidnight().add(Calendar.SECOND, -1).create());
      createQuickSetButton(buttonGroup, "This week", new DateBuilder().set(Calendar.DAY_OF_WEEK, Calendar.MONDAY).setMidnight().create(), null);
      createQuickSetButton(buttonGroup, "Last week", new DateBuilder().add(Calendar.WEEK_OF_YEAR, -1).set(Calendar.DAY_OF_WEEK, Calendar.MONDAY).setMidnight().create(),
            new DateBuilder().set(Calendar.DAY_OF_WEEK, Calendar.MONDAY).setMidnight().add(Calendar.SECOND, -1).create());
      createQuickSetButton(buttonGroup, "This month", new DateBuilder().set(Calendar.DAY_OF_MONTH, 1).setMidnight().create(), null);
      createQuickSetButton(buttonGroup, "Last month", new DateBuilder().add(Calendar.MONTH, -1).set(Calendar.DAY_OF_MONTH, 1).setMidnight().create(),
            new DateBuilder().add(Calendar.MONTH, -1).setLastDayOfMonth().setMidnight().add(Calendar.SECOND, 86399).create());
      createQuickSetButton(buttonGroup, "This year", new DateBuilder().set(Calendar.DAY_OF_YEAR, 1).setMidnight().create(), null);
      createQuickSetButton(buttonGroup, "Last year", new DateBuilder().add(Calendar.YEAR, -1).set(Calendar.DAY_OF_YEAR, 1).setMidnight().create(),
            new DateBuilder().set(Calendar.DAY_OF_YEAR, 1).setMidnight().add(Calendar.SECOND, -1).create());

      buttonQuery = new Button(queryGroup, SWT.PUSH);
      buttonQuery.setText("&Query");
      buttonQuery.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            refresh();
         }
      });
      gd = new GridData();
      gd.widthHint = 200;
      gd.horizontalAlignment = SWT.RIGHT;
      buttonQuery.setLayoutData(gd);

      Label separator = new Label(parent, SWT.SEPARATOR | SWT.HORIZONTAL);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      separator.setLayoutData(gd);

      ChartConfiguration chartConfiguration = new ChartConfiguration();
      chartConfiguration.setLegendVisible(true);
      chartConfiguration.setLegendPosition(ChartConfiguration.POSITION_RIGHT);
      chartConfiguration.setTranslucent(false);

      chart = new Chart(parent, SWT.NONE, ChartType.PIE, chartConfiguration);
      chart.setBackground(chart.getDisplay().getSystemColor(SWT.COLOR_WIDGET_BACKGROUND));
      chart.addParameter(new GraphItem("Uptime", "Uptime", null));
      chart.addParameter(new GraphItem("Downtime", "Downtime", null));
      chart.setPaletteEntry(0, new ChartColor(127, 154, 72));
      chart.setPaletteEntry(1, new ChartColor(158, 65, 62));
      chart.rebuild();

      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      chart.setLayoutData(gd);

      separator = new Label(parent, SWT.SEPARATOR | SWT.HORIZONTAL);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      separator.setLayoutData(gd);

      final String[] names = { "ID", "Service", "Check ID", "Description", "Created", "Closed", "Reason" };
      final int[] widths = {70, 200, 70, 300, 150, 150, 300};
      ticketViewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SortableTableViewer.DEFAULT_STYLE);
      ticketViewer.setContentProvider(new ArrayContentProvider());
      ticketViewer.setLabelProvider(new BusinessServiceTicketLabelProvider());
      ticketViewer.setComparator(new BusinessServiceTicketComparator());

      final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      WidgetHelper.restoreColumnSettings(ticketViewer.getTable(), settings, CONFIG_PREFIX);
      ticketViewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(ticketViewer.getTable(), settings, CONFIG_PREFIX);
         }
      });

      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      ticketViewer.getTable().setLayoutData(gd);
   }

   /**
    * Add quick set button
    *
    * @param parent parent composite
    * @param name name to be displayed on button
    * @param startTime start time or null to use current time
    * @param endTime end time or null to use current time
    */
   private void createQuickSetButton(Composite parent, String name, Date startTime, Date endTime)
   {
      Button button = new Button(parent, SWT.PUSH);
      button.setText(name);
      button.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            startDateSelector.setValue((startTime != null) ? startTime : new Date());
            endDateSelector.setValue((endTime != null) ? endTime : new Date());
            refresh();
         }
      });
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      button.setLayoutData(gd);
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean showForObject(AbstractObject object)
   {
      return (object instanceof BusinessService) && !(object instanceof BusinessServicePrototype);
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public void objectChanged(AbstractObject object)
   {
      ticketViewer.setInput(new Object[0]);
      chart.clearParameters();
      chart.refresh();
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
    */
   @Override
   public void refresh()
   {
      TimePeriod timePeriod = new TimePeriod(TimeFrameType.FIXED, 0, null, startDateSelector.getValue(), endDateSelector.getValue());
      ConsoleJob availabilityJob = new ConsoleJob("Get business service availability", getViewPart(), Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final double availability = session.getBusinessServiceAvailablity(getObject().getObjectId(), timePeriod);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  chart.updateParameter(0, availability, false);
                  chart.updateParameter(1, 100.0 - availability, true);
                  chart.refresh();
                  chart.clearErrors();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot get availability for business service %s", getObject().getObjectName());
         }
      };
      availabilityJob.setUser(false);
      availabilityJob.start();

      ConsoleJob ticketJob = new ConsoleJob("Get business service tickets", getViewPart(), Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<BusinessServiceTicket> tickets = session.getBusinessServiceTickets(getObject().getObjectId(), timePeriod);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  ticketViewer.setInput(tickets);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot get tickets for business service %s", getObject().getObjectName());
         }
      };
      ticketJob.setUser(false);
      ticketJob.start();
   }
}
