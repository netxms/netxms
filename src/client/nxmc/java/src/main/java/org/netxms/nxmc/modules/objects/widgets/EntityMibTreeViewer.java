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

import org.apache.commons.lang3.SystemUtils;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.jface.viewers.TreeViewerColumn;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.TreeItem;
import org.netxms.client.NXCException;
import org.netxms.client.PhysicalComponent;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractNode;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.objects.widgets.helpers.ComponentTreeContentProvider;
import org.netxms.nxmc.modules.objects.widgets.helpers.ComponentTreeLabelProvider;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Widget for viewing ENTITY-MIB component tree
 */
public class EntityMibTreeViewer extends AbstractHardwareInventoryWidget
{
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_CLASS = 1;
   public static final int COLUMN_DESCRIPTION = 2;
   public static final int COLUMN_MODEL = 3;
   public static final int COLUMN_FIRMWARE = 4;
   public static final int COLUMN_SERIAL = 5;
   public static final int COLUMN_VENDOR = 6;
   public static final int COLUMN_INTERFACE = 7;

   private final I18n i18n = LocalizationHelper.getI18n(EntityMibTreeViewer.class);

   private SortableTreeViewer viewer;
   private ComponentTreeLabelProvider labelProvider;

   /**
    * Create new hardware inventory widget.
    *
    * @param parent parent composite
    * @param style widget style
    * @param view owning view
    */
   public EntityMibTreeViewer(Composite parent, int style, ObjectView view)
   {
      super(parent, style, view);

      viewer = new SortableTreeViewer(this, SWT.FULL_SELECTION | SWT.MULTI);
      addColumn(i18n.tr("Name"), 200);
      addColumn(i18n.tr("Class"), 100);
      addColumn(i18n.tr("Description"), 250);
      addColumn(i18n.tr("Model"), 150);
      addColumn(i18n.tr("Firmware"), 100);
      addColumn(i18n.tr("Serial Number"), 150);
      addColumn(i18n.tr("Vendor"), 150);
      addColumn(i18n.tr("Interface"), 150);
      labelProvider = new ComponentTreeLabelProvider();
      viewer.setLabelProvider(labelProvider);
      viewer.setContentProvider(new ComponentTreeContentProvider());
      viewer.getTree().setHeaderVisible(true);
      viewer.getTree().setLinesVisible(true);
   }

   /**
    * @param name
    */
   private void addColumn(String name, int width)
   {
      TreeViewerColumn tc = new TreeViewerColumn(viewer, SWT.LEFT);
      tc.getColumn().setText(name);
      tc.getColumn().setWidth(width);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.widgets.AbstractHardwareInventoryWidget#refresh()
    */
   @Override
   public void refresh()
   {
      final long objectId = view.getObjectId();
      new Job(i18n.tr("Loading hardware inventory"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               final PhysicalComponent root = session.getNodePhysicalComponents(objectId);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (viewer.getTree().isDisposed())
                        return;

                     if (view.getObjectId() == objectId)
                     {
                        labelProvider.setNode((AbstractNode)view.getObject());
                        viewer.setInput(new Object[] { root });
                        viewer.expandAll();
                        viewer.packColumns();
                     }
                  }
               });
            }
            catch(NXCException e)
            {
               if (e.getErrorCode() != RCC.NO_COMPONENT_DATA)
                  throw e;
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (viewer.getTree().isDisposed())
                        return;

                     if (view.getObjectId() == objectId)
                     {
                        labelProvider.setNode(null);
                        viewer.setInput(new Object[0]);
                     }
                  }
               });
            }
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
      TreeItem[] selection = viewer.getTree().getSelection();
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
               for(int j = 1; j < viewer.getTree().getColumnCount(); j++)
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
      return COLUMN_NAME;
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
      return COLUMN_SERIAL;
   }
}
