/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2026 Raden Solutions
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
import java.util.Date;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.client.constants.ColumnFilterSetOperation;
import org.netxms.client.constants.ColumnFilterType;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogFilter;
import org.netxms.client.log.OrderingColumn;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.AiObservationMonitorConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;
import com.google.gson.Gson;

/**
 * "AI observation monitor" dashboard element - shows recent observations recorded by AI operator instances
 */
public class AiObservationMonitorElement extends ElementWidget
{
   private static final Logger logger = LoggerFactory.getLogger(AiObservationMonitorElement.class);

   private static final int COLUMN_TIMESTAMP = 0;
   private static final int COLUMN_SEVERITY = 1;
   private static final int COLUMN_TITLE = 2;
   private static final int COLUMN_OBJECT = 3;
   private static final int COLUMN_STATE = 4;

   private final I18n i18n = LocalizationHelper.getI18n(AiObservationMonitorElement.class);
   private final String[] stateTexts = { i18n.tr("New"), i18n.tr("Acknowledged"), i18n.tr("Dismissed") };

   private SortableTableViewer viewer;
   private AiObservationMonitorConfig config;
   private final NXCSession session;
   private ViewRefreshController refreshController;

   /**
    * Create AI observation monitor element.
    *
    * @param parent parent composite
    * @param element dashboard element
    * @param view owning view part
    */
   protected AiObservationMonitorElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
   {
      super(parent, element, view);

      try
      {
         config = new Gson().fromJson(element.getData(), AiObservationMonitorConfig.class);
         if (config == null)
            config = new AiObservationMonitorConfig();
      }
      catch(Exception e)
      {
         logger.error("Cannot parse dashboard element configuration", e);
         config = new AiObservationMonitorConfig();
      }

      processCommonSettings(config);

      session = Registry.getSession();

      final String[] columnNames = { i18n.tr("Time"), i18n.tr("Severity"), i18n.tr("Title"), i18n.tr("Object"), i18n.tr("State") };
      final int[] columnWidths = { 140, 100, 300, 180, 100 };
      viewer = new SortableTableViewer(getContentArea(), columnNames, columnWidths, COLUMN_TIMESTAMP, SWT.DOWN, SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ObservationLabelProvider());
      viewer.disableSorting();

      refreshController = new ViewRefreshController(view, config.getRefreshInterval(), () -> {
         if (!isDisposed())
            refreshData();
      });
      refreshData();

      addDisposeListener((e) -> refreshController.dispose());
   }

   /**
    * Refresh observation list from server
    */
   private void refreshData()
   {
      Job job = new Job(i18n.tr("Loading AI operator observations"), view, this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<ObservationEntry> observations = loadObservations();
            runInUIThread(() -> {
               if (!viewer.getControl().isDisposed())
                  viewer.setInput(observations.toArray());
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load AI operator observations");
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Load observations from server log.
    *
    * @return list of observations (newest first)
    * @throws Exception on error
    */
   private List<ObservationEntry> loadObservations() throws Exception
   {
      List<ObservationEntry> observations = new ArrayList<>();
      Log log = null;
      try
      {
         log = session.openServerLog("AIOperatorObservations");

         LogFilter filter = new LogFilter();
         if (config.getInstanceId() != 0)
            filter.setColumnFilter("instance_id", new ColumnFilter(ColumnFilterType.EQUALS, config.getInstanceId()));
         if (config.isNewOnly())
            filter.setColumnFilter("state", new ColumnFilter("0"));

         long effectiveObjectId = getEffectiveObjectId(config.getObjectId());
         if (effectiveObjectId != 0)
         {
            ColumnFilter cf = new ColumnFilter();
            cf.setOperation(ColumnFilterSetOperation.OR);
            cf.addSubFilter(new ColumnFilter(ColumnFilterType.EQUALS, effectiveObjectId));
            cf.addSubFilter(new ColumnFilter(ColumnFilterType.CHILDOF, effectiveObjectId));
            filter.setColumnFilter("object_id", cf);
         }

         List<OrderingColumn> ordering = new ArrayList<>(1);
         ordering.add(new OrderingColumn("id", "ID", true));
         filter.setOrderingColumns(ordering);

         log.query(filter);
         Table data = log.retrieveData(0, Math.min(config.getMaxRecords(), 500));

         int idxTimestamp = log.getColumnIndex("observation_timestamp");
         int idxSeverity = log.getColumnIndex("severity");
         int idxTitle = log.getColumnIndex("title");
         int idxObjectId = log.getColumnIndex("object_id");
         int idxState = log.getColumnIndex("state");

         for(TableRow row : data.getAllRows())
         {
            ObservationEntry e = new ObservationEntry();
            e.timestamp = new Date(row.getValueAsLong(idxTimestamp) * 1000);
            e.severity = (int)row.getValueAsLong(idxSeverity);
            e.title = row.get(idxTitle).getValue();
            e.objectId = row.getValueAsLong(idxObjectId);
            e.state = (int)row.getValueAsLong(idxState);
            observations.add(e);
         }
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
      return observations;
   }

   /**
    * Single observation record
    */
   private static class ObservationEntry
   {
      Date timestamp;
      int severity;
      String title;
      long objectId;
      int state;
   }

   /**
    * Label provider for observation list
    */
   private class ObservationLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return (columnIndex == COLUMN_SEVERITY) ? StatusDisplayInfo.getStatusImage(((ObservationEntry)element).severity) : null;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         ObservationEntry observation = (ObservationEntry)element;
         switch(columnIndex)
         {
            case COLUMN_TIMESTAMP:
               return DateFormatFactory.getDateTimeFormat().format(observation.timestamp);
            case COLUMN_SEVERITY:
               return StatusDisplayInfo.getStatusText(observation.severity);
            case COLUMN_TITLE:
               return observation.title;
            case COLUMN_OBJECT:
               return (observation.objectId != 0) ? session.getObjectName(observation.objectId) : "";
            case COLUMN_STATE:
               return ((observation.state >= 0) && (observation.state <= 2)) ? stateTexts[observation.state] : Integer.toString(observation.state);
         }
         return null;
      }
   }
}
