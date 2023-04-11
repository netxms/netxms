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
package org.netxms.ui.eclipse.objectview.widgets;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IViewPart;
import org.netxms.client.HardwareComponent;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.widgets.helpers.HardwareComponentComparator;
import org.netxms.ui.eclipse.objectview.widgets.helpers.HardwareComponentLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

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

   private IViewPart viewPart;
   private long rootObjectId;
   private SortableTableViewer viewer;
   private MenuManager menuManager = null;

   /**
    * @param parent
    * @param style
    */
   public HardwareInventory(Composite parent, int style, IViewPart viewPart)
   {
      super(parent, style);
      this.viewPart = viewPart;

      setLayout(new FillLayout());
      createTableViewer();
   }

   /**
    * Create table viewer
    */
   private void createTableViewer()
   {
      viewer = new SortableTableViewer(this, names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new HardwareComponentLabelProvider());
      viewer.setComparator(new HardwareComponentComparator());

      if (menuManager != null)
      {
         Menu menu = menuManager.createContextMenu(viewer.getControl());
         viewer.getControl().setMenu(menu);
         viewPart.getSite().setSelectionProvider(viewer);
         viewPart.getSite().registerContextMenu(menuManager, viewer);
      }
   }

   /**
    * Refresh list
    */
   public void refresh()
   {
      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Loading hardware inventory", viewPart, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<HardwareComponent> components = session.getNodeHardwareComponents(rootObjectId);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(components.toArray());
                  viewer.packColumns();
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
         viewPart.getSite().setSelectionProvider(viewer);
         viewPart.getSite().registerContextMenu(menuManager, viewer);
      }
   }
}
