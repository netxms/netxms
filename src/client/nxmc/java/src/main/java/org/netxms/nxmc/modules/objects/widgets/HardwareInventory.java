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
package org.netxms.nxmc.modules.objects.widgets;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.HardwareComponent;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.objects.widgets.helpers.HardwareComponentComparator;
import org.netxms.nxmc.modules.objects.widgets.helpers.HardwareComponentLabelProvider;
import org.xnap.commons.i18n.I18n;

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

   private final I18n i18n = LocalizationHelper.getI18n(HardwareInventory.class);

   private ObjectView view;
   private SortableTableViewer viewer;
   private MenuManager menuManager = null;

   /**
    * Create new hardware inventory widget.
    *
    * @param parent parent composite
    * @param style widget style
    * @param view owning view
    */
   public HardwareInventory(Composite parent, int style, ObjectView view)
   {
      super(parent, style);
      this.view = view;

      setLayout(new FillLayout());
      createTableViewer();
   }

   /**
    * Create table viewer
    */
   private void createTableViewer()
   {
      final String[] names = { i18n.tr("Category"), i18n.tr("Index"), i18n.tr("Type"), i18n.tr("Vendor"), i18n.tr("Model"),
            i18n.tr("Capacity"), i18n.tr("Part number"), i18n.tr("Serial number"), i18n.tr("Location"), i18n.tr("Description") };
      final int[] widths = { 100, 70, 100, 200, 200, 90, 130, 130, 200, 300 };
      viewer = new SortableTableViewer(this, names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
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
      new Job(i18n.tr("Loading hardware inventory"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<HardwareComponent> components = session.getNodeHardwareComponents(view.getObjectId());
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
