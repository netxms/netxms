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
package org.netxms.ui.eclipse.objectbrowser.views;

import java.util.Date;
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
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.objects.queries.ObjectQueryResult;
import org.netxms.ui.eclipse.actions.CopyTableRowsAction;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.objectbrowser.views.helpers.ObjectQueryResultLabelProvider;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Result view for object query
 */
public class ObjectQueryResultView extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.objectbrowser.views.ObjectQueryResultView";

   private SortableTableViewer viewer;
   private Action actionExportToCSV;
   private Action actionCopyToClipboard;
   private Action actionCopySelectionToClipboard;

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      viewer = new SortableTableViewer(parent, SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ObjectQueryResultLabelProvider(viewer.getTable()));

      createActions();
      contributeToActionBars();
      createContextMenu();
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
    * Fill action bars
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Fill local pull-down menu
    * 
    * @param manager
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionCopyToClipboard);
      manager.add(actionExportToCSV);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager
    */
   private void fillLocalToolBar(IToolBarManager manager)
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
    * Set view content
    *
    * @param resultSet object query result set
    * @param queryName query name
    */
   public void setContent(List<ObjectQueryResult> resultSet, String queryName)
   {
      setPartName(queryName + " - " + RegionalSettings.getDateTimeFormat().format(new Date()));
      updateResultTable(resultSet);
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
}
