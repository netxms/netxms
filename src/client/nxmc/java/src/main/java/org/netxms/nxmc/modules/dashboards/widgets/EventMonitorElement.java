/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2025 Raden Solutions
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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.client.constants.ColumnFilterSetOperation;
import org.netxms.client.constants.ColumnFilterType;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.events.Event;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogFilter;
import org.netxms.client.log.OrderingColumn;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.EventMonitorConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.modules.events.widgets.EventTraceWidget;
import org.netxms.nxmc.modules.events.widgets.helpers.HistoricalEvent;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;
import com.google.gson.Gson;

/**
 * Event monitor element widget
 */
public class EventMonitorElement extends ElementWidget
{
   private static final Logger logger = LoggerFactory.getLogger(EventMonitorElement.class);

   private final I18n i18n = LocalizationHelper.getI18n(EventMonitorElement.class);

   private EventTraceWidget viewer;
   private EventMonitorConfig config;
   private final NXCSession session;
   private long lastLoadedEventId = 0;
   private boolean historicalLoadComplete = false;
   private List<Event> pendingRealTimeEvents = new ArrayList<>();

   /**
    * Create event monitor dashboard element.
    *
    * @param parent parent widget
    * @param element dashboard element definition
    * @param view owning view part
    */
   protected EventMonitorElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
   {
      super(parent, element, view);

      try
      {
         config = new Gson().fromJson(element.getData(), EventMonitorConfig.class);
      }
      catch(Exception e)
      {
         logger.error("Cannot parse dashboard element configuration", e);
         config = new EventMonitorConfig();
      }

      processCommonSettings(config);

      session = Registry.getSession();

      viewer = new EventTraceWidget(getContentArea(), SWT.NONE, view);
      viewer.setRootObject(getEffectiveObjectId(config.getObjectId()));
      viewer.setFilterText(config.getFilter());
      viewer.setEventCodeFilter(config.getEventCodes());

      // Set up event interceptor to buffer events during historical load
      if (config.getMaxEvents() > 0)
      {
         viewer.setEventInterceptor((event) -> {
            synchronized(pendingRealTimeEvents)
            {
               if (!historicalLoadComplete)
               {
                  if (event instanceof Event)
                     pendingRealTimeEvents.add((Event)event);
                  return true;
               }
            }
            return false;
         });
      }

      // Subscribe and load historical events
      Job job = new Job(i18n.tr("Loading events"), view, this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.subscribe(NXCSession.CHANNEL_EVENTS);

            if (config.getMaxEvents() > 0)
            {
               List<HistoricalEvent> historicalEvents = loadHistoricalEvents();
               logger.debug("Loaded {} historical events", historicalEvents.size());

               runInUIThread(() -> {
                  if (!isDisposed())
                  {
                     if (!historicalEvents.isEmpty())
                     {
                        viewer.addElements(historicalEvents);
                        lastLoadedEventId = historicalEvents.get(0).getId();
                     }

                     synchronized(pendingRealTimeEvents)
                     {
                        historicalLoadComplete = true;
                        // Add buffered real-time events that are newer than historical ones
                        for(Event event : pendingRealTimeEvents)
                        {
                           if (event.getId() > lastLoadedEventId)
                           {
                              viewer.addEvent(event);
                           }
                        }
                        pendingRealTimeEvents.clear();
                     }

                     viewer.setEventInterceptor(null);
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load events");
         }
      };
      job.setUser(false);
      job.start();

      addDisposeListener((e) -> unsubscribe());
   }

   /**
    * Load historical events from EventLog.
    *
    * @return list of historical events (newest first)
    * @throws Exception on error
    */
   private List<HistoricalEvent> loadHistoricalEvents() throws Exception
   {
      List<HistoricalEvent> events = new ArrayList<>();
      Log log = null;

      try
      {
         log = session.openServerLog("EventLog");
         LogFilter filter = new LogFilter();

         // Add time range filter if configured
         if (config.getTimeRangeMinutes() > 0)
         {
            long now = System.currentTimeMillis() / 1000;
            long rangeStart = now - (config.getTimeRangeMinutes() * 60L);
            filter.setColumnFilter("event_timestamp", new ColumnFilter(ColumnFilterType.GREATER, rangeStart));
         }

         // Add object filter if configured
         long effectiveObjectId = getEffectiveObjectId(config.getObjectId());
         if (effectiveObjectId != 0)
         {
            AbstractObject object = session.findObjectById(effectiveObjectId);
            if (object != null)
            {
               filter.setColumnFilter("event_source", new ColumnFilter(isEventSource(object) ? ColumnFilterType.EQUALS : ColumnFilterType.CHILDOF, effectiveObjectId));
            }
         }

         // Add event code filter if configured
         int[] eventCodes = config.getEventCodes();
         if (eventCodes.length > 0)
         {
            if (eventCodes.length == 1)
            {
               filter.setColumnFilter("event_code", new ColumnFilter(ColumnFilterType.EQUALS, eventCodes[0]));
            }
            else
            {
               ColumnFilter setFilter = new ColumnFilter();
               setFilter.setOperation(ColumnFilterSetOperation.OR);
               for(int code : eventCodes)
               {
                  setFilter.addSubFilter(new ColumnFilter(ColumnFilterType.EQUALS, code));
               }
               filter.setColumnFilter("event_code", setFilter);
            }
         }

         // Order by timestamp descending to get most recent first
         List<OrderingColumn> ordering = new ArrayList<>(1);
         ordering.add(new OrderingColumn("event_timestamp", "Time", true));
         filter.setOrderingColumns(ordering);

         // Execute query
         log.query(filter);

         // Retrieve limited number of events
         int maxToLoad = Math.min(config.getMaxEvents(), 500);
         Table data = log.retrieveData(0, maxToLoad);

         // Get column indices
         int idxId = log.getColumnIndex("event_id");
         int idxTimestamp = log.getColumnIndex("event_timestamp");
         int idxSource = log.getColumnIndex("event_source");
         int idxCode = log.getColumnIndex("event_code");
         int idxSeverity = log.getColumnIndex("event_severity");
         int idxMessage = log.getColumnIndex("event_message");
         int idxDciId = log.getColumnIndex("dci_id");

         // Convert table rows to HistoricalEvent objects
         for(TableRow row : data.getAllRows())
         {
            events.add(new HistoricalEvent(row, idxId, idxTimestamp, idxSource, idxCode, idxSeverity, idxMessage, idxDciId));
         }
      }
      catch(Exception e)
      {
         logger.error("Error loading historical events", e);
      }
      finally
      {
         if (log != null)
         {
            try
            {
               log.close();
            }
            catch(Exception e)
            {
               logger.debug("Error closing log handle", e);
            }
         }
      }

      return events;
   }

   /**
    * Check if given object can be used as event source
    *
    * @param object object to check
    * @return true if object can be used as event source
    */
   private static boolean isEventSource(AbstractObject object)
   {
      int objectClass = object.getObjectClass();
      return (objectClass == AbstractObject.OBJECT_NODE) ||
             (objectClass == AbstractObject.OBJECT_CLUSTER) ||
             (objectClass == AbstractObject.OBJECT_MOBILEDEVICE) ||
             (objectClass == AbstractObject.OBJECT_SENSOR) ||
             (objectClass == AbstractObject.OBJECT_ACCESSPOINT);
   }

   /**
    * Unsubscribe from notifications
    */
   private void unsubscribe()
   {
      Job job = new Job(String.format(i18n.tr("Unsubscribing from channel %s"), NXCSession.CHANNEL_EVENTS), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.unsubscribe(NXCSession.CHANNEL_EVENTS);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot unsubscribe from channel %s"), NXCSession.CHANNEL_EVENTS);
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
      super.dispose();
   }
}
