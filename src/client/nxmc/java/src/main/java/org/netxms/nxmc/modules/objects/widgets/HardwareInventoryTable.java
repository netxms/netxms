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
import org.apache.commons.lang3.SystemUtils;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.client.HardwareComponent;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.objects.widgets.helpers.HardwareComponentComparator;
import org.netxms.nxmc.modules.objects.widgets.helpers.HardwareComponentLabelProvider;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Hardware inventory table (display hardware inventory retrieved from agent)
 */
public class HardwareInventoryTable extends AbstractHardwareInventoryWidget
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

   private final I18n i18n = LocalizationHelper.getI18n(HardwareInventoryTable.class);

   private SortableTableViewer viewer;

   /**
    * Create new hardware inventory widget.
    *
    * @param parent parent composite
    * @param style widget style
    * @param view owning view
    */
   public HardwareInventoryTable(Composite parent, int style, ObjectView view)
   {
      super(parent, style, view);

      final String[] names = { i18n.tr("Category"), i18n.tr("Index"), i18n.tr("Type"), i18n.tr("Vendor"), i18n.tr("Model"),
            i18n.tr("Capacity"), i18n.tr("Part number"), i18n.tr("Serial number"), i18n.tr("Location"), i18n.tr("Description") };
      final int[] widths = { 100, 70, 100, 200, 200, 90, 130, 130, 200, 300 };
      viewer = new SortableTableViewer(this, names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new HardwareComponentLabelProvider());
      viewer.setComparator(new HardwareComponentComparator());
   }

   /**
    * @see org.netxms.nxmc.modules.objects.widgets.AbstractHardwareInventoryWidget#refresh()
    */
   @Override
   public void refresh()
   {
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
    * @see org.netxms.nxmc.modules.objects.widgets.AbstractHardwareInventoryWidget#getViewer()
    */
   @Override
   public ColumnViewer getViewer()
   {
      return viewer;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.widgets.AbstractHardwareInventoryWidget#copySelectionToClipboard(int)
    */
   @Override
   public void copySelectionToClipboard(int column)
   {
      TableItem[] selection = viewer.getTable().getSelection();
      if (selection.length > 0)
      {
         final String newLine = SystemUtils.IS_OS_WINDOWS ? "\n" : "\n";
         StringBuilder sb = new StringBuilder();
         for(int i = 0; i < selection.length; i++)
         {
            if (i > 0)
               sb.append(newLine);
            if (column == -1)
            {
               sb.append(selection[i].getText(0));
               for(int j = 1; j < viewer.getTable().getColumnCount(); j++)
               {
                  sb.append("\t");
                  sb.append(selection[i].getText(j));
               }
            }
            else
            {
               sb.append(selection[i].getText(column));
            }
         }
         WidgetHelper.copyToClipboard(sb.toString());
      }
   }

   /**
    * @see org.netxms.nxmc.modules.objects.widgets.AbstractHardwareInventoryWidget#getNameColumnIndex()
    */
   @Override
   public int getNameColumnIndex()
   {
      return -1;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.widgets.AbstractHardwareInventoryWidget#getDescriptionColumnIndex()
    */
   @Override
   public int getDescriptionColumnIndex()
   {
      return COLUMN_DESCRIPTION;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.widgets.AbstractHardwareInventoryWidget#getModelColumnIndex()
    */
   @Override
   public int getModelColumnIndex()
   {
      return COLUMN_MODEL;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.widgets.AbstractHardwareInventoryWidget#getSerialColumnIndex()
    */
   @Override
   public int getSerialColumnIndex()
   {
      return COLUMN_SERIAL_NUMBER;
   }
}
