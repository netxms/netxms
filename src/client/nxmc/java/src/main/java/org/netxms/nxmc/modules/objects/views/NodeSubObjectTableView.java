/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.nxmc.modules.objects.views;

import org.apache.commons.lang3.SystemUtils;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.views.helpers.NodeSubObjectFilter;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Base class for object views that display node sub-objects using table viewer.
 */
public abstract class NodeSubObjectTableView extends NodeSubObjectView
{
   protected SortableTableViewer viewer;
   protected NodeSubObjectFilter filter;
   protected Action actionCopyToClipboard;
   protected Action actionExportToCsv;

   /**
    * @param name
    * @param image
    * @param id
    */
   public NodeSubObjectTableView(String name, ImageDescriptor image, String id, boolean hasFilters)
   {
      super(name, image, id, hasFilters);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.NodeSubObjectView#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      super.createContent(parent);

      createViewer();
      createActions();
      createPopupMenu();
   }

   /**
    * Returns created viewer
    * 
    * @return viewer
    */
   protected abstract void createViewer();

   /**
    * Create actions
    */
   protected void createActions()
   {
      actionCopyToClipboard = new Action("Copy to clipboard", SharedIcons.COPY) {
         @Override
         public void run()
         {
            copyToClipboard(-1);
         }
      };

      actionExportToCsv = new ExportToCsvAction(this, viewer, true);
   }

   /**
    * Create pop-up menu
    */
   protected void createPopupMenu()
   {
      ObjectContextMenuManager menuMgr = new ObjectContextMenuManager(this, viewer, null) {
         @Override
         protected void fillContextMenu()
         {
            super.fillContextMenu();
            add(new Separator());
            NodeSubObjectTableView.this.fillContextMenu(this);
         }
      };
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected abstract void fillContextMenu(IMenuManager manager);

   /**
    * Copy content to clipboard
    * 
    * @param column column number or -1 to copy all columns
    */
   protected void copyToClipboard(int column)
   {
      final TableItem[] selection = viewer.getTable().getSelection();
      if (selection.length > 0)
      {
         final String newLine = SystemUtils.IS_OS_WINDOWS ? "\n" : "\n";
         final StringBuilder sb = new StringBuilder();
         for(int i = 0; i < selection.length; i++)
         {
            if (i > 0)
               sb.append(newLine);
            if (column == -1)
            {
               for(int j = 0; j < viewer.getTable().getColumnCount(); j++)
               {
                  if (j > 0)
                     sb.append('\t');
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
}
