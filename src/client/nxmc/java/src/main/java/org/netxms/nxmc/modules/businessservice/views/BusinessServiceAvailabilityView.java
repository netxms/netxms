/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Reden Solutions
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

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCSession;
import org.netxms.client.TimePeriod;
import org.netxms.client.businessservices.BusinessServiceTicket;
import org.netxms.client.constants.DataType;
import org.netxms.client.constants.TimeFrameType;
import org.netxms.client.constants.TimeUnit;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.BusinessServicePrototype;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.TimePeriodSelector;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.businessservice.views.helpers.BusinessServiceTicketComparator;
import org.netxms.nxmc.modules.businessservice.views.helpers.BusinessServiceTicketLabelProvider;
import org.netxms.nxmc.modules.charts.api.ChartColor;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Business service "Checks" view
 */
public class BusinessServiceAvailabilityView extends ObjectView
{
   private static final I18n i18n = LocalizationHelper.getI18n(BusinessServiceAvailabilityView.class);
   private static final String ID = "BusinessServiceAvailability";   

   public static final int COLUMN_ID = 0;
   public static final int COLUMN_SERVICE_ID = 1;
   public static final int COLUMN_CHECK_ID = 2;
   public static final int COLUMN_CHECK_DESCRIPTION = 3;
   public static final int COLUMN_CREATION_TIME = 4;
   public static final int COLUMN_TERMINATION_TIME = 5;
   public static final int COLUMN_REASON = 6;

   private Button buttonToday;
   private Button buttonLastMonth;
   private Button buttonLastYear;
   private TimePeriodSelector dateTimeSelector;
   private Button buttonSelect;
   private Chart chart;
   private SortableTableViewer ticketViewer;
   
   private NXCSession session;
   
   /**
    * @param name
    * @param image
    */
   public BusinessServiceAvailabilityView()
   {
      super(i18n.tr("BusinessServiceAvailability"), ResourceManager.getImageDescriptor("icons/object-views/availability_chart.png"), "Availability", false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      session = Registry.getSession();   
      parent.setLayout(new GridLayout());
      
      Composite buttonGroup = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.makeColumnsEqualWidth = true;
      buttonGroup.setLayout(layout);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      gd.widthHint = 300;
      buttonGroup.setLayoutData(gd);
      
      buttonToday = new Button(buttonGroup, SWT.PUSH);
      buttonToday.setText(i18n.tr("Today"));
      buttonToday.addSelectionListener(new SelectionListener() {         
         @Override
         public void widgetSelected(SelectionEvent arg0)
         {
            updateTime(1, TimeUnit.DAY);
         }         
         @Override
         public void widgetDefaultSelected(SelectionEvent arg0)
         {
            widgetSelected(arg0);
         }
      });
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      buttonToday.setLayoutData(gd);      
      
      buttonLastMonth = new Button(buttonGroup, SWT.PUSH);
      buttonLastMonth.setText(i18n.tr("Last month"));
      buttonLastMonth.addSelectionListener(new SelectionListener() {         
         @Override
         public void widgetSelected(SelectionEvent arg0)
         {
            updateTime(30, TimeUnit.DAY);
         }         
         @Override
         public void widgetDefaultSelected(SelectionEvent arg0)
         {
            widgetSelected(arg0);
         }
      });
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      buttonLastMonth.setLayoutData(gd);
      
      buttonLastYear = new Button(buttonGroup, SWT.PUSH);
      buttonLastYear.setText(i18n.tr("Last year"));
      buttonLastYear.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent arg0)
         {
            updateTime(365, TimeUnit.DAY);
         }
         @Override
         public void widgetDefaultSelected(SelectionEvent arg0)
         {
            widgetSelected(arg0);
         }
      });
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      buttonLastYear.setLayoutData(gd);

      Composite timeSelectorGroup = new Composite(parent, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = 3;
      layout.makeColumnsEqualWidth = false;
      timeSelectorGroup.setLayout(layout);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      timeSelectorGroup.setLayoutData(gd);
      
      dateTimeSelector = new TimePeriodSelector(timeSelectorGroup, SWT.NONE, new TimePeriod(TimeFrameType.BACK_FROM_NOW, 30, TimeUnit.DAY, null, null));    
      
      buttonSelect = new Button(timeSelectorGroup, SWT.PUSH);
      buttonSelect.setText(i18n.tr("Query"));
      buttonSelect.addSelectionListener(new SelectionListener() {         
         @Override
         public void widgetSelected(SelectionEvent arg0)
         {
            refresh();
         }         
         @Override
         public void widgetDefaultSelected(SelectionEvent arg0)
         {
            widgetSelected(arg0);
         }
      });
      gd = new GridData();
      gd.widthHint = 200;
      buttonSelect.setLayoutData(gd);
      
      ChartConfiguration chartConfiguration = new ChartConfiguration();
      //chartConfiguration.setLegendPosition(legendPosition);
      chartConfiguration.setLegendVisible(true);
      chartConfiguration.setShowIn3D(true);
      chartConfiguration.setTransposed(true);
      chartConfiguration.setTranslucent(true);
      
      chart = new Chart(parent, SWT.NONE, ChartType.PIE, chartConfiguration);        
      chart.addParameter(new GraphItem(DataType.FLOAT, i18n.tr("Uptime"), i18n.tr("Uptime"), "%s"));
      chart.addParameter(new GraphItem(DataType.FLOAT, i18n.tr("Downtime"), i18n.tr("Downtime"), "%s"));
      chart.setPaletteEntry(0, new ChartColor(127, 154, 72));
      chart.setPaletteEntry(1, new ChartColor(158, 65, 62));
      chart.rebuild();
      
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      ((Control)chart).setLayoutData(gd);
      
      final String[] names = 
         { i18n.tr("ID"),
           i18n.tr("Service"),
           i18n.tr("Check id"),
           i18n.tr("Check description"),
           i18n.tr("Creation time"),
           i18n.tr("Termination time"),
           i18n.tr("Reason")           
         };
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
   
   private void updateTime(int timeRange, TimeUnit unit)
   {
      dateTimeSelector.updateTimeperiod(timeRange, unit);
      refresh();
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

   @Override
   public void refresh()
   {
      TimePeriod timePeriod = dateTimeSelector.getTimePeriod();
      Job availabilityJob = new Job(i18n.tr("Get business ervice availability"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            double availability = session.getBusinessServiceAvailablity(getObject().getObjectId(), timePeriod);
            
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
            return i18n.tr("Cannot get availability for business service {0}", getObject().getObjectName());
         }
      };
      availabilityJob.setUser(false);
      availabilityJob.start();
      Job ticketJob = new Job(i18n.tr("Get business service tickets"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<BusinessServiceTicket> tickets = session.getBusinessServiceTickets(getObject().getObjectId(), timePeriod);
            
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
      return 61;
   }
}
