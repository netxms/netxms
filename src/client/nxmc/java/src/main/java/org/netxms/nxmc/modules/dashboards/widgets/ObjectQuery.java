/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.queries.ObjectQueryResult;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.CopyTableRowsAction;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.ObjectDetailsConfig;
import org.netxms.nxmc.modules.dashboards.config.ObjectDetailsConfig.ObjectProperty;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.modules.dashboards.widgets.helpers.ObjectDetailsLabelProvider;
import org.netxms.nxmc.modules.dashboards.widgets.helpers.ObjectSelectionProvider;
import org.netxms.nxmc.modules.networkmaps.ObjectDoubleClickHandlerRegistry;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;
import com.google.gson.Gson;

/**
 * "Object query" dashboard element
 */
public class ObjectQuery extends ElementWidget
{
   private static final Logger logger = LoggerFactory.getLogger(ObjectQuery.class);
   private final I18n i18n = LocalizationHelper.getI18n(ObjectQuery.class);

   private ObjectDetailsConfig config;
   private ViewRefreshController refreshController;
   private NXCSession session;
   private boolean updateInProgress = false;
   private SortableTableViewer viewer;
   private ObjectSelectionProvider objectSelectionProvider;
   private ObjectDoubleClickHandlerRegistry doubleClickHandlers;
   private Action actionCopyToClipboard;
   private Action actionCopyAllToClipboard;
   private Action actionExportToCSV;
   private Action actionExportAllToCSV;

   /**
    * @param parent
    * @param element
    * @param viewPart
    */
   public ObjectQuery(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
   {
      super(parent, element, view);
      session = Registry.getSession();
      
      try
      {
         config = new Gson().fromJson(element.getData(), ObjectDetailsConfig.class);
      }
      catch(Exception e)
      {
         logger.error("Cannot parse dashboard element configuration", e);
         config = new ObjectDetailsConfig();
      }

      processCommonSettings(config);

      // Do not set sort column to leave element sorting provided form server
      viewer = new SortableTableViewer(getContentArea(), SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ObjectDetailsLabelProvider(config.getProperties()));
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            if (((SortableTableViewer)viewer).getTable().getSortColumn() == null)
               return 0;

            ObjectProperty p = (ObjectProperty)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ObjectProperty");
            String v1 = ((ObjectQueryResult)e1).getPropertyValue(p.name);
            String v2 = ((ObjectQueryResult)e2).getPropertyValue(p.name);
            int result = 0;
            switch(p.type)
            {
               case ObjectProperty.INTEGER:
                  result = Long.signum(safeParseLong(v1) - safeParseLong(v2));
                  break;
               case ObjectProperty.FLOAT:
                  double d1 = safeParseDouble(v1);
                  double d2 = safeParseDouble(v2);
                  result = (d1 < d2) ? -1 : ((d1 > d2) ? 1 : 0);
                  break;
               default:
                  result = v1.compareToIgnoreCase(v2);
                  break;
            }
            return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
         }
      });

      for(ObjectProperty p : config.getProperties())
      {
         TableColumn c = viewer.addColumn(p.displayName == null || p.displayName.isEmpty() ? p.name : p.displayName, 150);
         c.setData("ObjectProperty", p);
      }

      actionCopyToClipboard = new CopyTableRowsAction(viewer, true);
      actionCopyToClipboard.setImageDescriptor(SharedIcons.COPY_TO_CLIPBOARD);

      actionCopyAllToClipboard = new CopyTableRowsAction(viewer, false);
      actionExportToCSV = new ExportToCsvAction(view, viewer, true);
      actionExportAllToCSV = new ExportToCsvAction(view, viewer, false);

      objectSelectionProvider = new ObjectSelectionProvider(viewer);
      createContextMenu();

      doubleClickHandlers = new ObjectDoubleClickHandlerRegistry(view);
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            Object o = ((IStructuredSelection)viewer.getSelection()).getFirstElement();
            if ((o != null) && (o instanceof ObjectQueryResult))
               doubleClickHandlers.handleDoubleClick(((ObjectQueryResult)o).getObject());
         }
      });
      
      refreshController = new ViewRefreshController(view, config.getRefreshRate(), new Runnable() {
         @Override
         public void run()
         {
            if (ObjectQuery.this.isDisposed())
               return;
            
            refreshData();
         }
      });
      refreshData();

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            refreshController.dispose();
         }
      });
   }
   
   /**
    * Parse long int without exception
    * 
    * @param s
    * @return
    */
   static long safeParseLong(String s)
   {
      try
      {
         return Long.parseLong(s);
      }
      catch(NumberFormatException e)
      {
         return 0;
      }
   }
   
   /**
    * Parse floating point number without exception
    * 
    * @param s
    * @return
    */
   static double safeParseDouble(String s)
   {
      try
      {
         return Double.parseDouble(s);
      }
      catch(NumberFormatException e)
      {
         return 0;
      }
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      // Create menu manager for underlying node object
      final MenuManager menuManager = new ObjectContextMenuManager(view, objectSelectionProvider, null) {
         @Override
         protected void fillContextMenu()
         {
            super.fillContextMenu();
            add(new Separator());
            add(actionCopyToClipboard);
            add(actionCopyAllToClipboard);
            add(actionExportToCSV);
            add(actionExportAllToCSV);
         }
      };

      Menu menu = menuManager.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Refresh graph's data
    */
   private void refreshData()
   {
      if (updateInProgress)
         return;

      updateInProgress = true;

      final long contextObjectId = getContextObjectId();
      final long rootObjectId = getEffectiveObjectId(config.getRootObjectId());
      Job job = new Job(i18n.tr("Running object query"), view, this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<ObjectProperty> properties = config.getProperties();
            List<String> names = new ArrayList<String>(properties.size());
            for(ObjectProperty p : properties)
               names.add(p.name);
            final List<ObjectQueryResult> objects = 
                  session.queryObjectDetails(config.getQuery(), rootObjectId, names, config.getOrderingProperties(), null, contextObjectId, false, config.getRecordLimit(), null, null);
            runInUIThread(() -> {
               if (viewer.getControl().isDisposed())
                  return;
               viewer.setInput(objects);
               viewer.packColumns();
               updateInProgress = false;
            });
         }

         @Override
         protected void jobFailureHandler(Exception e)
         {
            updateInProgress = false;
            super.jobFailureHandler(e);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot execute object query");
         }
      };
      job.setUser(false);
      job.start();
   }
}
