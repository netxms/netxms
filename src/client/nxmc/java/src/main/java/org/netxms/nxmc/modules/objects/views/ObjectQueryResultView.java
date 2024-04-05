/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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

import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.objects.queries.ObjectQueryResult;
import org.netxms.nxmc.base.actions.CopyTableRowsAction;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.helpers.ObjectQueryResultLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;

/**
 * Result view for object query
 */
public class ObjectQueryResultView extends ConfigurationView
{
   private List<ObjectQueryResult> resultSet;
   private SortableTableViewer viewer;
   private Action actionExportToCSV;
   private Action actionCopyToClipboard;
   private Action actionCopySelectionToClipboard;

   public ObjectQueryResultView(String queryName, List<ObjectQueryResult> resultSet)
   {
      super(queryName, ResourceManager.getImageDescriptor("icons/config-views/object-query-results.png"), "object-query.results." + queryName, false);
      this.resultSet = resultSet;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      viewer = new SortableTableViewer(parent, SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ObjectQueryResultLabelProvider(viewer.getTable()));

      createActions();
      createContextMenu();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      updateResultTable(resultSet);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionExportToCSV = new ExportToCsvAction(this, viewer, false);

      actionCopyToClipboard = new CopyTableRowsAction(viewer, false);
      actionCopyToClipboard.setImageDescriptor(SharedIcons.COPY_TO_CLIPBOARD);

      actionCopySelectionToClipboard = new CopyTableRowsAction(viewer, true);
      actionCopySelectionToClipboard.setImageDescriptor(SharedIcons.COPY_TO_CLIPBOARD);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionCopyToClipboard);
      manager.add(actionExportToCSV);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionCopyToClipboard);
      manager.add(actionExportToCSV);
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      MenuManager manager = new MenuManager();
      manager.setRemoveAllWhenShown(true);
      manager.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            IStructuredSelection selection = viewer.getStructuredSelection();
            if (!selection.isEmpty())
               manager.add(actionCopySelectionToClipboard);
         }
      });

      // Create menu.
      Menu menu = manager.createContextMenu(viewer.getTable());
      viewer.getTable().setMenu(menu);
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getControl().setFocus();
   }

   /**
    * Remove all columns
    */
   private void resetResultTable()
   {
      TableColumn[] columns = viewer.getTable().getColumns();
      for(int i = 0; i < columns.length; i++)
         columns[i].dispose();
   }

   /**
    * Update result table
    *
    * @param objects query result
    */
   private void updateResultTable(List<ObjectQueryResult> objects)
   {
      resetResultTable();

      Set<String> registeredProperties = new HashSet<>();
      for(ObjectQueryResult r : objects)
      {
         for(String n : r.getPropertyNames())
         {
            if (!registeredProperties.contains(n))
            {
               TableColumn tc = viewer.addColumn(n, 200);
               tc.setData("propertyName", n);
               registeredProperties.add(n);
            }
         }
      }

      viewer.setInput(objects);
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
   }
}
