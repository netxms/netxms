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

import java.io.IOException;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Widget;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.MessageHandler;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.RefreshAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewWithContext;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Real-time graph view. Opens a server-side real-time read feed for every selected DCI; the server reads each DCI at the
 * configured interval through its full configuration (source node, SNMP parameters, delta calculation, transformation script)
 * and pushes samples to this view, which plots them in a scrolling chart. Collected values are kept only in memory for the
 * duration of the visible time window and are never stored on the server.
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
   private Display display;
   private ChartDciConfig[] dciList;
   private DataSeries[] seriesTemplates;
   private List<List<DciDataRow>> buffers;
   private int pollInterval = 2;
   private boolean paused = false;

   private final Object feedLock = new Object();
   private long[] feedIds; // guarded by feedLock
   private int[] errorCodes; // last reported error code per series, 0 if none (UI thread only)
   private int[] errorMessageIds; // message area entry per series, -1 if none (UI thread only)

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
    * @param series empty data series carrying presentation metadata (data type, units, multiplier) of each DCI, parallel to items
    * @param contextId owning context ID
    */
   public RealTimeGraphView(AbstractObject contextObject, List<ChartDciConfig> items, List<DataSeries> series, long contextId)
   {
      super(LocalizationHelper.getI18n(RealTimeGraphView.class).tr("Real-Time Graph"),
            ResourceManager.getImageDescriptor("icons/object-views/rt-chart-line.png"), buildId(contextObject, items), false);
      this.objectId = contextObject.getObjectId();
      this.contextId = contextId;
      this.dciList = items.toArray(new ChartDciConfig[items.size()]);
      this.seriesTemplates = series.toArray(new DataSeries[series.size()]);
      updateFullName(this.dciList);
   }

   /**
    * Default constructor for use by cloneView()
    */
   protected RealTimeGraphView()
   {
      super(LocalizationHelper.getI18n(RealTimeGraphView.class).tr("Real-Time Graph"),
            ResourceManager.getImageDescriptor("icons/object-views/rt-chart-line.png"), UUID.randomUUID().toString(), false);
      fullName = LocalizationHelper.getI18n(RealTimeGraphView.class).tr("Real-Time Graph");
      dciList = new ChartDciConfig[0];
      seriesTemplates = new DataSeries[0];
   }

   /**
    * Update view full name based on the DCI list.
    *
    * @param chartDciConfigs DCI configurations
    */
   private void updateFullName(ChartDciConfig[] chartDciConfigs)
   {
      fullName = i18n.tr("Real-Time Graph");
      if (chartDciConfigs.length == 0)
         return;

      // Set view title to "host name: dci description" if we have only one DCI
      if (chartDciConfigs.length == 1)
      {
         ChartDciConfig item = chartDciConfigs[0];
         setName(item.name.isEmpty() ? item.dciDescription : item.name);
         AbstractObject object = session.findObjectById(item.nodeId);
         if (object != null)
         {
            fullName = object.getObjectName() + ": " + getName();
         }
         else
         {
            fullName = getName();
         }
      }
      else
      {
         long nodeId = chartDciConfigs[0].nodeId;
         for(ChartDciConfig item : chartDciConfigs)
            if (item.nodeId != nodeId)
            {
               nodeId = -1;
               break;
            }
         if (nodeId != -1)
         {
            // All DCIs from same node, set title to "host name"
            AbstractObject object = session.findObjectById(nodeId);
            if (object != null)
            {
               fullName = String.format(i18n.tr("%s: Real-Time Graph"), object.getObjectName());
            }
         }
      }
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
      view.seriesTemplates = seriesTemplates;
      view.paused = paused;

      // Copy already accumulated data so the cloned view continues from the current state
      if (buffers != null)
      {
         view.buffers = new ArrayList<List<DciDataRow>>(buffers.size());
         for(List<DciDataRow> buffer : buffers)
            view.buffers.add(new ArrayList<DciDataRow>(buffer));
      }

      return view;
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreContext(org.netxms.nxmc.Memento)
    */
   @Override
   protected Object restoreContext(Memento memento)
   {
      long objectId = memento.getAsLong("context", 0);
      return session.findObjectById(objectId);
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
      display = parent.getDisplay();
      createActions();

      ChartConfiguration chartConfiguration = new ChartConfiguration();
      chartConfiguration.setTitle(fullName);
      chartConfiguration.setLegendVisible(dciList.length > 1);
      chartConfiguration.setLegendPosition(ChartConfiguration.POSITION_BOTTOM);
      chartConfiguration.setGridVisible(true);
      chartConfiguration.setAutoScale(true);
      chart = new Chart(chartParent, SWT.NONE, ChartType.LINE, chartConfiguration, this);

      // Buffers may already be populated when this view was created by cloning an existing one
      boolean haveData = (buffers != null);
      if (!haveData)
         buffers = new ArrayList<List<DciDataRow>>(dciList.length);
      for(int i = 0; i < dciList.length; i++)
      {
         chart.addParameter(new ChartDciConfig(dciList[i]));
         if (!haveData)
            buffers.add(new ArrayList<DciDataRow>());
      }
      chart.rebuild();
      chartParent.layout(true, true);
      createPopupMenu();

      if (haveData)
         renderBuffers();

      feedIds = new long[dciList.length];
      errorCodes = new int[dciList.length];
      errorMessageIds = new int[dciList.length];
      for(int i = 0; i < dciList.length; i++)
         errorMessageIds[i] = -1;

      if (!paused)
         restartFeeds();
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
            refresh();
         }
      };

      actionPause = new Action(i18n.tr("&Pause"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            paused = isChecked();
            restartFeeds();
         }
      };
      actionPause.setImageDescriptor(ResourceManager.getImageDescriptor("icons/pause.png"));
      actionPause.setChecked(paused);

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
               if (isChecked() && (interval != pollInterval))
               {
                  pollInterval = interval;
                  restartFeeds();
               }
            }
         };
         intervalActions[i].setChecked(interval == pollInterval);
      }
   }

   /**
    * Stop all running feeds and, unless view is paused, start new ones with current polling interval. Runs in a background job;
    * concurrent invocations are serialized so that stop/start sequences never interleave.
    */
   private void restartFeeds()
   {
      final boolean start = !paused;
      final int interval = pollInterval;
      Job job = new Job(i18n.tr("Starting real-time data feeds"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            synchronized(feedLock)
            {
               stopFeedsInternal();
               if (start)
                  startFeedsInternal(interval);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot start real-time data feeds");
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Stop all running feeds in a background job (used on view disposal).
    */
   private void stopFeeds()
   {
      Job job = new Job(i18n.tr("Stopping real-time data feeds"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            synchronized(feedLock)
            {
               stopFeedsInternal();
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return null;
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
   }

   /**
    * Start feed for every series. Caller must hold feedLock. Series whose feed cannot be started get an error entry in the message
    * area instead of failing the whole view.
    *
    * @param interval polling interval in seconds
    * @throws IOException on communication failure
    */
   private void startFeedsInternal(int interval) throws IOException
   {
      for(int i = 0; i < dciList.length; i++)
      {
         final int index = i;
         try
         {
            feedIds[i] = session.startRealtimeDciRead(dciList[i].nodeId, dciList[i].dciId, interval, new MessageHandler() {
               @Override
               public boolean processMessage(NXCPMessage msg)
               {
                  onFeedMessage(index, msg);
                  return true;
               }
            });
         }
         catch(NXCException e)
         {
            final int rcc = e.getErrorCode();
            runInUIThread(() -> reportError(index, rcc));
         }
      }
   }

   /**
    * Stop all running feeds. Caller must hold feedLock.
    */
   private void stopFeedsInternal()
   {
      for(int i = 0; i < feedIds.length; i++)
      {
         if (feedIds[i] == 0)
            continue;
         try
         {
            session.stopRealtimeDciRead(feedIds[i]);
         }
         catch(Exception e)
         {
            logger.debug("Cannot stop real-time read feed {}", feedIds[i], e);
         }
         feedIds[i] = 0;
      }
   }

   /**
    * Run given code in UI thread if view is still alive.
    *
    * @param runnable code to run
    */
   private void runInUIThread(Runnable runnable)
   {
      if ((display == null) || display.isDisposed())
         return;
      display.asyncExec(() -> {
         if ((chart != null) && !((Widget)chart).isDisposed())
            runnable.run();
      });
   }

   /**
    * Process data message pushed by server for given series. Called on session receiver thread.
    *
    * @param index series index
    * @param msg data message
    */
   private void onFeedMessage(int index, NXCPMessage msg)
   {
      final int rcc = msg.getFieldAsInt32(NXCPCodes.VID_RCC);
      if (rcc != RCC.SUCCESS)
      {
         runInUIThread(() -> reportError(index, rcc));
         return;
      }

      final double value;
      try
      {
         value = Double.parseDouble(msg.getFieldAsString(NXCPCodes.VID_VALUE).trim());
      }
      catch(NumberFormatException | NullPointerException e)
      {
         return; // non-numeric value, nothing to plot
      }
      final Date timestamp = msg.getFieldAsTimestamp(NXCPCodes.VID_TIMESTAMP);
      runInUIThread(() -> appendValue(index, timestamp, value));
   }

   /**
    * Append value to given series, clear its error indication if any, and re-render chart. Must be called in UI thread.
    *
    * @param index series index
    * @param timestamp sample timestamp
    * @param value sample value
    */
   private void appendValue(int index, Date timestamp, double value)
   {
      if (errorMessageIds[index] != -1)
      {
         deleteMessage(errorMessageIds[index]);
         errorMessageIds[index] = -1;
         errorCodes[index] = 0;
      }
      buffers.get(index).add(new DciDataRow(timestamp, Double.valueOf(value)));
      renderBuffers();
   }

   /**
    * Show error reported by server for given series. Only the latest distinct error per series is shown. Must be called in UI
    * thread.
    *
    * @param index series index
    * @param rcc error code
    */
   private void reportError(int index, int rcc)
   {
      if (errorCodes[index] == rcc)
         return;
      if (errorMessageIds[index] != -1)
         deleteMessage(errorMessageIds[index]);
      errorCodes[index] = rcc;
      ChartDciConfig dci = dciList[index];
      String name = dci.name.isEmpty() ? dci.dciDescription : dci.name;
      errorMessageIds[index] = addMessage(MessageArea.WARNING, name + ": " + RCC.getText(rcc, Locale.getDefault().getLanguage(), null), true);
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
         chart.updateParameter(i, new DataSeries(seriesTemplates[i]), false);
      }
      chart.refresh();
   }

   /**
    * Trim values outside the visible time window and re-render all series from the in-memory buffers.
    */
   private void renderBuffers()
   {
      Date now = new Date();
      long cutoff = now.getTime() - timeWindow;
      for(int i = 0; i < buffers.size(); i++)
      {
         List<DciDataRow> buffer = buffers.get(i);
         while(!buffer.isEmpty() && (buffer.get(0).getTimestamp().getTime() < cutoff))
            buffer.remove(0);

         DataSeries series = new DataSeries(seriesTemplates[i]);
         for(DciDataRow row : buffer)
            series.addDataRow(row);
         chart.updateParameter(i, series, false);
      }
      chart.setTimeRange(new Date(cutoff), now);
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
      fillMenu(manager);
   }

   /**
    * Create context menu for the chart.
    */
   private void createPopupMenu()
   {
      MenuManager menuManager = new MenuManager();
      menuManager.setRemoveAllWhenShown(true);
      menuManager.addMenuListener((m) -> fillMenu(m));
      chart.setMenuManager(menuManager);
   }

   /**
    * Fill given menu with pause/clear/polling interval actions, shared by the view menu and the chart
    * context menu.
    *
    * @param manager menu manager to fill
    */
   private void fillMenu(IMenuManager manager)
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
    * Restart feeds, forcing immediate re-read of all metrics.
    *
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      restartFeeds();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      if (feedIds != null)
         stopFeeds();
      super.dispose();
   }
}
