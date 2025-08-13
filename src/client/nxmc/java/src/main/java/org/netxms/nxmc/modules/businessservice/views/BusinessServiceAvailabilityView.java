/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.businessservice.views;

import java.util.Calendar;
import java.util.Date;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
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
import org.netxms.client.TimePeriod;
import org.netxms.client.businessservices.BusinessServiceTicket;
import org.netxms.client.constants.TimeFrameType;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.BusinessServicePrototype;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.DateTimeSelector;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.businessservice.views.helpers.BusinessServiceTicketComparator;
import org.netxms.nxmc.modules.businessservice.views.helpers.BusinessServiceTicketLabelProvider;
import org.netxms.nxmc.modules.charts.api.ChartColor;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.DateBuilder;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Business service "Availability" view
 */
public class BusinessServiceAvailabilityView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(BusinessServiceAvailabilityView.class);
   private static final String ID = "BusinessServiceAvailability";   

   public static final int COLUMN_ID = 0;
   public static final int COLUMN_SERVICE_ID = 1;
   public static final int COLUMN_CHECK_ID = 2;
   public static final int COLUMN_CHECK_DESCRIPTION = 3;
   public static final int COLUMN_CREATION_TIME = 4;
   public static final int COLUMN_TERMINATION_TIME = 5;
   public static final int COLUMN_REASON = 6;

   private DateTimeSelector startDateSelector;
   private DateTimeSelector endDateSelector;
   private Button buttonQuery;
   private Chart chart;
   private SortableTableViewer ticketViewer;

   /**
    * Create business service availability view
    */
   public BusinessServiceAvailabilityView()
   {
      super(LocalizationHelper.getI18n(BusinessServiceAvailabilityView.class).tr("Availability"), ResourceManager.getImageDescriptor("icons/object-views/availability_chart.png"), ID, false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
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

      createQuickSetButton(buttonGroup, i18n.tr("Today"), new DateBuilder().setMidnight().create(), null);
      createQuickSetButton(buttonGroup, i18n.tr("Yesterday"), new DateBuilder().setMidnight().add(Calendar.DATE, -1).create(),
            new DateBuilder().setMidnight().add(Calendar.SECOND, -1).create());
      createQuickSetButton(buttonGroup, i18n.tr("This week"), new DateBuilder().set(Calendar.DAY_OF_WEEK, Calendar.MONDAY).setMidnight().create(), null);
      createQuickSetButton(buttonGroup, i18n.tr("Last week"), new DateBuilder().add(Calendar.WEEK_OF_YEAR, -1).set(Calendar.DAY_OF_WEEK, Calendar.MONDAY).setMidnight().create(),
            new DateBuilder().set(Calendar.DAY_OF_WEEK, Calendar.MONDAY).setMidnight().add(Calendar.SECOND, -1).create());
      createQuickSetButton(buttonGroup, i18n.tr("This month"), new DateBuilder().set(Calendar.DAY_OF_MONTH, 1).setMidnight().create(), null);
      createQuickSetButton(buttonGroup, i18n.tr("Last month"), new DateBuilder().add(Calendar.MONTH, -1).set(Calendar.DAY_OF_MONTH, 1).setMidnight().create(),
            new DateBuilder().add(Calendar.MONTH, -1).setLastDayOfMonth().setMidnight().add(Calendar.SECOND, 86399).create());
      createQuickSetButton(buttonGroup, i18n.tr("This year"), new DateBuilder().set(Calendar.DAY_OF_YEAR, 1).setMidnight().create(), null);
      createQuickSetButton(buttonGroup, i18n.tr("Last year"), new DateBuilder().add(Calendar.YEAR, -1).set(Calendar.DAY_OF_YEAR, 1).setMidnight().create(),
            new DateBuilder().set(Calendar.DAY_OF_YEAR, 1).setMidnight().add(Calendar.SECOND, -1).create());

      buttonQuery = new Button(queryGroup, SWT.PUSH);
      buttonQuery.setText(i18n.tr("&Query"));
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

      chart = new Chart(parent, SWT.NONE, ChartType.PIE, chartConfiguration, this);
      chart.setBackground(getDisplay().getSystemColor(SWT.COLOR_WIDGET_BACKGROUND));
      chart.addParameter(new ChartDciConfig(i18n.tr("Uptime")));
      chart.addParameter(new ChartDciConfig(i18n.tr("Downtime")));
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

      final String[] names = { i18n.tr("ID"), i18n.tr("Service"), i18n.tr("Check ID"), i18n.tr("Description"), i18n.tr("Created"), i18n.tr("Closed"), i18n.tr("Reason") };
      final int[] widths = {70, 200, 70, 300, 150, 150, 300};
      ticketViewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SortableTableViewer.DEFAULT_STYLE);
      ticketViewer.setContentProvider(new ArrayContentProvider());
      ticketViewer.setLabelProvider(new BusinessServiceTicketLabelProvider());
      ticketViewer.setComparator(new BusinessServiceTicketComparator());

      WidgetHelper.restoreColumnSettings(ticketViewer.getTable(), ID);
      ticketViewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(ticketViewer.getTable(), ID);
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
      gd.minimumWidth = WidgetHelper.BUTTON_WIDTH_HINT;
      button.setLayoutData(gd);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      ticketViewer.setInput(new Object[0]);
      chart.clearParameters();
      chart.refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      TimePeriod timePeriod = new TimePeriod(TimeFrameType.FIXED, 0, null, startDateSelector.getValue(), endDateSelector.getValue());
      Job availabilityJob = new Job(i18n.tr("Getting business service availability"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final double availability = session.getBusinessServiceAvailablity(getObject().getObjectId(), timePeriod);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  chart.updateParameter(0, availability, false);
                  chart.updateParameter(1, 100.0 - availability, true);
                  chart.refresh();
                  clearMessages();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get availability for business service {0}", getObject().getObjectName());
         }
      };
      availabilityJob.setUser(false);
      availabilityJob.start();

      Job ticketJob = new Job(i18n.tr("Reading business service tickets"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
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
            return i18n.tr("Cannot get tickets for business service {0}", getObject().getObjectName());
         }
      };
      ticketJob.setUser(false);
      ticketJob.start();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof BusinessService) && !(context instanceof BusinessServicePrototype);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 70;
   }
}
