/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.swt.events.FocusListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.ObjectQueryResult;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.widgets.helpers.ObjectDetailsLabelProvider;
import org.netxms.ui.eclipse.dashboard.widgets.helpers.ObjectSelectionProvider;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ObjectDetailsConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ObjectDetailsConfig.ObjectProperty;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.networkmaps.api.ObjectDoubleClickHandlerRegistry;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ViewRefreshController;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * "Object details" dashboard element
 */
public class ObjectDetails extends ElementWidget
{
   private ObjectDetailsConfig config;
   private ViewRefreshController refreshController;
   private NXCSession session;
   private boolean updateInProgress = false;
   private SortableTableViewer viewer;
   private ObjectSelectionProvider objectSelectionProvider;
   private ObjectDoubleClickHandlerRegistry doubleClickHandlers;
   
   /**
    * @param parent
    * @param element
    * @param viewPart
    */
   public ObjectDetails(DashboardControl parent, DashboardElement element, IViewPart viewPart)
   {
      super(parent, element, viewPart);
      session = ConsoleSharedData.getSession();

      try
      {
         config = ObjectDetailsConfig.createFromXml(element.getData());
      }
      catch(Exception e)
      {
         e.printStackTrace();
         config = new ObjectDetailsConfig();
      }
      
      setLayout(new FillLayout());
      
      viewer = new SortableTableViewer(this, SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ObjectDetailsLabelProvider(config.getProperties()));
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
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
            }
            return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
         }
      });
      
      for(ObjectProperty p : config.getProperties())
      {
         TableColumn c = viewer.addColumn(p.displayName, 150);
         c.setData("ObjectProperty", p);
      }

      viewer.getTable().setSortDirection(SWT.UP);
      viewer.getTable().setSortColumn(viewer.getTable().getColumn(0));
      
      objectSelectionProvider = new ObjectSelectionProvider(viewer);
      createPopupMenu();
      
      viewer.getControl().addFocusListener(new FocusListener() {
         @Override
         public void focusLost(FocusEvent e)
         {
         }
         
         @Override
         public void focusGained(FocusEvent e)
         {
            setSelectionProviderDelegate(objectSelectionProvider);
         }
      });
      
      doubleClickHandlers = new ObjectDoubleClickHandlerRegistry(viewPart);
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            Object o = ((IStructuredSelection)viewer.getSelection()).getFirstElement();
            if ((o != null) && (o instanceof ObjectQueryResult))
               doubleClickHandlers.handleDoubleClick(((ObjectQueryResult)o).getObject());
         }
      });
      
      refreshController = new ViewRefreshController(viewPart, config.getRefreshRate(), new Runnable() {
         @Override
         public void run()
         {
            if (ObjectDetails.this.isDisposed())
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
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      // Create menu manager for underlying node object
      final MenuManager menuManager = new MenuManager(null, viewPart.getViewSite().getId() + "." + this.hashCode());
      menuManager.setRemoveAllWhenShown(true);
      menuManager.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            ObjectContextMenu.fill(mgr, viewPart.getSite(), objectSelectionProvider);
         }
      });
      
      // Create menu.
      Menu menu = menuManager.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
      
      // Register menu for extension.
      viewPart.getSite().registerContextMenu(menuManager.getId(), menuManager, objectSelectionProvider); //$NON-NLS-1$
   }
   
   /**
    * Refresh graph's data
    */
   private void refreshData()
   {
      if (updateInProgress)
         return;
      
      updateInProgress = true;
      
      ConsoleJob job = new ConsoleJob("Update object details", viewPart, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<ObjectProperty> properties = config.getProperties();
            List<String> names = new ArrayList<String>(properties.size());
            for(ObjectProperty p : properties)
               names.add(p.name);
            final List<ObjectQueryResult> objects = session.queryObjectDetails(config.getQuery(), names);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (viewer.getControl().isDisposed())
                     return;
                  viewer.setInput(objects);
                  viewer.packColumns();
                  updateInProgress = false;
               }
            });
         }

         @Override
         protected void jobFailureHandler()
         {
            updateInProgress = false;
            super.jobFailureHandler();
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot execute object query";
         }
      };
      job.setUser(false);
      job.start();
   }
}
