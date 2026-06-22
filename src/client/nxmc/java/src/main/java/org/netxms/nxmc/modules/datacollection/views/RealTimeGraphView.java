/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.views;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Widget;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.RefreshAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewWithContext;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Real-time graph view. Polls selected metrics on demand (via on-the-fly metric query) at a fixed
 * interval and plots the values in a scrolling chart. Collected values are kept only in memory for
 * the duration of the configured time window and are never stored on the server.
 */
public class RealTimeGraphView extends ViewWithContext
{
   private final I18n i18n = LocalizationHelper.getI18n(RealTimeGraphView.class);
   private static final Logger logger = LoggerFactory.getLogger(RealTimeGraphView.class);

   private static final int[] presetIntervals = { 1, 2, 5, 10, 30, 60 };
   private static final long timeWindow = 600000L; // visible time window in milliseconds (10 minutes)

   private long objectId;
   private long contextId;
   private String fullName;
   private NXCSession session = Registry.getSession();
   private Chart chart = null;
   private Composite chartParent = null;
   private ViewRefreshController refreshController;
   private ChartDciConfig[] dciList;
   private DataOrigin[] origins;
   private List<List<DciDataRow>> buffers;
   private int pollInterval = 2;
   private boolean paused = false;
   private boolean updateInProgress = false;

   private Action actionPause;
   private Action actionClear;
   private Action actionRefresh;
   private Action[] intervalActions;

   /**
    * Build view ID
    *
    * @param object context object
    * @param items list of DCIs to show
    * @return view ID
    */
   private static String buildId(AbstractObject object, List<ChartDciConfig> items)
   {
      StringBuilder sb = new StringBuilder("realtime-graph");
      if (object != null)
      {
         sb.append('.');
         sb.append(object.getObjectId());
      }
      for(ChartDciConfig dci : items)
      {
         sb.append('.');
         sb.append(dci.dciId);
      }
      return sb.toString();
   }

   /**
    * Create real-time graph view with given context object and DCI list.
    *
    * @param contextObject context object
    * @param items set of DCIs to show
    * @param origins data origins of the DCIs (parallel to items), used for on-demand metric query
    * @param contextId owning context ID
    */
   public RealTimeGraphView(AbstractObject contextObject, List<ChartDciConfig> items, List<DataOrigin> origins, long contextId)
   {
      super(LocalizationHelper.getI18n(RealTimeGraphView.class).tr("Real-Time Graph"),
            ResourceManager.getImageDescriptor("icons/object-views/chart-line.png"), buildId(contextObject, items), false);
      this.objectId = contextObject.getObjectId();
      this.contextId = contextId;
      this.dciList = items.toArray(new ChartDciConfig[items.size()]);
      this.origins = origins.toArray(new DataOrigin[origins.size()]);
      updateFullName(contextObject);
   }

   /**
    * Default constructor for use by cloneView()
    */
   protected RealTimeGraphView()
   {
      super(LocalizationHelper.getI18n(RealTimeGraphView.class).tr("Real-Time Graph"),
            ResourceManager.getImageDescriptor("icons/object-views/chart-line.png"), UUID.randomUUID().toString(), false);
      fullName = LocalizationHelper.getI18n(RealTimeGraphView.class).tr("Real-Time Graph");
      dciList = new ChartDciConfig[0];
      origins = new DataOrigin[0];
   }

   /**
    * Update full name of the view based on context object and DCI list.
    *
    * @param contextObject context object
    */
   private void updateFullName(AbstractObject contextObject)
   {
      StringBuilder sb = new StringBuilder(i18n.tr("Real-Time Graph"));
      if (contextObject != null)
      {
         sb.append(" - ");
         sb.append(contextObject.getObjectName());
      }
      fullName = sb.toString();
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      RealTimeGraphView view = (RealTimeGraphView)super.cloneView();
      view.objectId = objectId;
      view.contextId = contextId;
      view.fullName = fullName;
      view.pollInterval = pollInterval;
      view.dciList = dciList;
      view.origins = origins;
      return view;
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#getFullName()
    */
   @Override
   public String getFullName()
   {
      return fullName;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#isCloseable()
    */
   @Override
   public boolean isCloseable()
   {
      return true;
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context instanceof AbstractObject)
            && ((((AbstractObject)context).getObjectId() == objectId) || (((AbstractObject)context).getObjectId() == contextId));
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#contextChanged(java.lang.Object, java.lang.Object)
    */
   @Override
   protected void contextChanged(Object oldContext, Object newContext)
   {
      // View is pinned to the metrics it was opened with; nothing to reconfigure on context change.
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      chartParent = parent;
      createActions();

      ChartConfiguration chartConfiguration = new ChartConfiguration();
      chartConfiguration.setTitle(fullName);
      chartConfiguration.setLegendVisible(dciList.length > 1);
      chartConfiguration.setLegendPosition(ChartConfiguration.POSITION_BOTTOM);
      chartConfiguration.setGridVisible(true);
      chartConfiguration.setAutoScale(true);
      chart = new Chart(chartParent, SWT.NONE, ChartType.LINE, chartConfiguration, this);

      buffers = new ArrayList<List<DciDataRow>>(dciList.length);
      for(ChartDciConfig dci : dciList)
      {
         chart.addParameter(new ChartDciConfig(dci));
         buffers.add(new ArrayList<DciDataRow>());
      }
      chart.rebuild();
      chartParent.layout(true, true);

      refreshController = new ViewRefreshController(this, pollInterval, new Runnable() {
         @Override
         public void run()
         {
            if ((chart == null) || ((Widget)chart).isDisposed())
               return;
            updateChart();
         }
      });
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionRefresh = new RefreshAction() {
         @Override
         public void run()
         {
            updateChart();
         }
      };

      actionPause = new Action(i18n.tr("&Pause"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            paused = isChecked();
            refreshController.setInterval(paused ? -1 : pollInterval);
         }
      };
      actionPause.setImageDescriptor(ResourceManager.getImageDescriptor("icons/pause.png"));

      actionClear = new Action(i18n.tr("&Clear")) {
         @Override
         public void run()
         {
            clearData();
         }
      };
      actionClear.setImageDescriptor(SharedIcons.CLEAR_LOG);

      intervalActions = new Action[presetIntervals.length];
      for(int i = 0; i < presetIntervals.length; i++)
      {
         final int interval = presetIntervals[i];
         intervalActions[i] = new Action(String.format(i18n.tr("%d seconds"), interval), Action.AS_RADIO_BUTTON) {
            @Override
            public void run()
            {
               if (isChecked())
                  setPollInterval(interval);
            }
         };
         intervalActions[i].setChecked(interval == pollInterval);
      }
   }

   /**
    * Set polling interval.
    *
    * @param interval polling interval in seconds
    */
   private void setPollInterval(int interval)
   {
      pollInterval = interval;
      if (!paused)
         refreshController.setInterval(pollInterval);
   }

   /**
    * Clear collected data from all series.
    */
   private void clearData()
   {
      if (buffers == null)
         return;
      for(int i = 0; i < buffers.size(); i++)
      {
         buffers.get(i).clear();
         chart.updateParameter(i, new DataSeries(), false);
      }
      chart.refresh();
   }

   /**
    * Query current values for all metrics and append them to the chart.
    */
   private void updateChart()
   {
      if (updateInProgress || paused || (chart == null) || ((Widget)chart).isDisposed())
         return;

      updateInProgress = true;
      final ChartDciConfig[] dciList = this.dciList;
      final DataOrigin[] origins = this.origins;
      Job job = new Job(i18n.tr("Querying real-time metric values"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final Date timestamp = new Date();
            final double[] values = new double[dciList.length];
            for(int i = 0; i < dciList.length; i++)
            {
               try
               {
                  String value = session.queryMetric(dciList[i].nodeId, origins[i], dciList[i].dciName);
                  values[i] = ((value != null) && !value.isBlank()) ? Double.parseDouble(value.trim()) : Double.NaN;
               }
               catch(NumberFormatException e)
               {
                  values[i] = Double.NaN; // non-numeric metric value, skip this sample
               }
               catch(Exception e)
               {
                  logger.debug("Cannot query metric {} on node [{}]", dciList[i].dciName, dciList[i].nodeId, e);
                  values[i] = Double.NaN;
               }
            }

            runInUIThread(() -> {
               if ((chart != null) && !((Widget)chart).isDisposed())
                  appendData(timestamp, values);
               updateInProgress = false;
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return null;
         }

         @Override
         protected void jobFailureHandler(Exception e)
         {
            updateInProgress = false;
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Append newly collected values to the in-memory buffers, trim values outside the visible time
    * window, and re-render the chart.
    *
    * @param timestamp collection timestamp
    * @param values collected values (NaN for metrics that could not be queried)
    */
   private void appendData(Date timestamp, double[] values)
   {
      long cutoff = timestamp.getTime() - timeWindow;
      for(int i = 0; i < buffers.size(); i++)
      {
         List<DciDataRow> buffer = buffers.get(i);
         if (!Double.isNaN(values[i]))
            buffer.add(new DciDataRow(timestamp, Double.valueOf(values[i])));
         while(!buffer.isEmpty() && (buffer.get(0).getTimestamp().getTime() < cutoff))
            buffer.remove(0);

         DataSeries series = new DataSeries();
         for(DciDataRow row : buffer)
            series.addDataRow(row);
         chart.updateParameter(i, series, false);
      }
      chart.setTimeRange(new Date(cutoff), timestamp);
      chart.refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionPause);
      manager.add(actionClear);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      MenuManager intervals = new MenuManager(i18n.tr("Polling &interval"));
      for(Action a : intervalActions)
         intervals.add(a);
      manager.add(intervals);
      manager.add(new Separator());
      manager.add(actionPause);
      manager.add(actionClear);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      updateChart();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      if (refreshController != null)
         refreshController.dispose();
      super.dispose();
   }
}
