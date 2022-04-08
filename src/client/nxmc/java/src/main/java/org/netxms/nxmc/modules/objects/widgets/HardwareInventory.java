/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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
package org.netxms.nxmc.modules.objects.widgets;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.HardwareComponent;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.widgets.helpers.HardwareComponentComparator;
import org.netxms.nxmc.modules.objects.widgets.helpers.HardwareComponentLabelProvider;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Hardware inventory widget
 */
public class HardwareInventory extends Composite
{
   public static final int COLUMN_CATEGORY = 0;
   public static final int COLUMN_INDEX = 1;
   public static final int COLUMN_TYPE = 2;
   public static final int COLUMN_VENDOR = 3;
   public static final int COLUMN_MODEL = 4;
   public static final int COLUMN_CAPACITY = 5;
   public static final int COLUMN_PART_NUMBER = 6;
   public static final int COLUMN_SERIAL_NUMBER = 7;
   public static final int COLUMN_LOCATION = 8;
   public static final int COLUMN_DESCRIPTION = 9;
   
   private static final String[] names = { "Category", "Index", "Type", "Vendor", "Model", "Capacity", "Part number", "Serial number", "Location", "Description" };
   private static final int[] widths = { 100, 70, 100, 200, 200, 90, 130, 130, 200, 300 };
   
   private View viewPart;
   private long rootObjectId;
   private ColumnViewer viewer;
   private String configPrefix;
   private MenuManager menuManager = null;
   
   /**
    * @param parent
    * @param style
    */
   public HardwareInventory(Composite parent, int style, View viewPart, String configPrefix)
   {
      super(parent, style);
      this.viewPart = viewPart;
      this.configPrefix = configPrefix;
      
      setLayout(new FillLayout());
      createTableViewer();
   }
   
   /**
    * Create table viewer
    */
   private void createTableViewer()
   {
      viewer = new SortableTableViewer(this, names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
      WidgetHelper.restoreColumnViewerSettings(viewer, configPrefix);
      viewer.getControl().addDisposeListener(new DisposeListener() {       
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnViewerSettings(viewer, configPrefix);
         }
      });
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new HardwareComponentLabelProvider());
      viewer.setComparator(new HardwareComponentComparator());
      
      if (menuManager != null)
      {
         Menu menu = menuManager.createContextMenu(viewer.getControl());
         viewer.getControl().setMenu(menu);
      }
   }
   
   /**
    * Refresh list
    */
   public void refresh()
   {
      final NXCSession session = Registry.getSession();
      new Job("Loading hardware inventory", viewPart) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<HardwareComponent> components = session.getNodeHardwareComponents(rootObjectId);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(components.toArray());
               }
            });
      }
         
         @Override
         protected String getErrorMessage()
         {
            return "Unable to get node hardware inventory";
         }
      }.start();
   }

   /**
    * @return the viewer
    */
   public ColumnViewer getViewer()
   {
      return viewer;
   }

   /**
    * @return the rootObjectId
    */
   public long getRootObjectId()
   {
      return rootObjectId;
   }

   /**
    * @param rootObjectId the rootObjectId to set
    */
   public void setRootObjectId(long rootObjectId)
   {
      this.rootObjectId = rootObjectId;
   }
   
   /**
    * @param manager
    */
   public void setViewerMenu(MenuManager manager)
   {
      menuManager = manager;
      if (viewer != null)
      {
         Menu menu = menuManager.createContextMenu(viewer.getControl());
         viewer.getControl().setMenu(menu);
      }
   }

   /**
    * Clear data form view
    */
   public void clear()
   {
      viewer.setInput(new Object[0]);
   }
}
