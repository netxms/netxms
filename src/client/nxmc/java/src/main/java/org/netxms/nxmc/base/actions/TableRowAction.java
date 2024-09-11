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
package org.netxms.nxmc.base.actions;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.swt.widgets.TreeColumn;
import org.eclipse.swt.widgets.TreeItem;

/**
 * Generic action class that operates on table or tree viewer rows. Provides necessary helper methods for accessing text in selected
 * rows.
 */
public abstract class TableRowAction extends Action
{
   protected ColumnViewer viewer;
   protected ViewerProvider viewerProvider;
   protected boolean selectionOnly;

   /**
    * Creates a new action with no text and no image.
    *
    * @param viewer viewer to copy rows from
    * @param viewerProvider viewer provider
    * @param selectionOnly true to copy only selected rows
    */
   public TableRowAction(ColumnViewer viewer, ViewerProvider viewerProvider, boolean selectionOnly)
   {
      super();
      this.viewer = viewer;
      this.viewerProvider = viewerProvider;
      this.selectionOnly = selectionOnly;
   }

   /**
    * Creates a new action with the given text and no image. Calls the zero-arg constructor, then setText.
    *
    * @param viewer viewer to copy rows from
    * @param viewerProvider viewer provider
    * @param selectionOnly true to copy only selected rows
    * @param text the string used as the text for the action, or null if there is no text
    */
   public TableRowAction(ColumnViewer viewer, ViewerProvider viewerProvider, boolean selectionOnly, String text)
   {
      super(text);
      this.viewer = viewer;
      this.viewerProvider = viewerProvider;
      this.selectionOnly = selectionOnly;
   }

   /**
    * Creates a new action with the given text and image. Calls the zero-arg constructor, then setText and setImageDescriptor.
    *
    * @param viewer viewer to copy rows from
    * @param viewerProvider viewer provider
    * @param selectionOnly true to copy only selected rows
    * @param text the string used as the text for the action, or null if there is no text
    * @param image the action's image, or null if there is no image
    */
   public TableRowAction(ColumnViewer viewer, ViewerProvider viewerProvider, boolean selectionOnly, String text, ImageDescriptor image)
   {
      super(text, image);
      this.viewer = viewer;
      this.viewerProvider = viewerProvider;
      this.selectionOnly = selectionOnly;
   }

   /**
    * Creates a new action with the given text and style.
    *
    * @param viewer viewer to copy rows from
    * @param viewerProvider viewer provider
    * @param selectionOnly true to copy only selected rows
    * @param text the string used as the text for the action, or null if there is no text
    * @param style one of AS_PUSH_BUTTON, AS_CHECK_BOX, AS_DROP_DOWN_MENU, AS_RADIO_BUTTON, and AS_UNSPECIFIED.
    */
   public TableRowAction(ColumnViewer viewer, ViewerProvider viewerProvider, boolean selectionOnly, String text, int style)
   {
      super(text, style);
      this.viewer = viewer;
      this.viewerProvider = viewerProvider;
      this.selectionOnly = selectionOnly;
   }

   /**
    * Get rows from associated viewer.
    *
    * @param withHeader set to true to include header line into content
    * @return data rows
    */
   protected List<String[]> getRowsFromViewer(boolean withHeader)
   {
      if (viewerProvider != null)
         viewer = viewerProvider.getViewer();

      final List<String[]> data = new ArrayList<String[]>();
      if (viewer instanceof TableViewer)
      {
         int numColumns = ((TableViewer)viewer).getTable().getColumnCount();
         if (numColumns == 0)
            numColumns = 1;

         if (withHeader)
         {
            TableColumn[] columns = ((TableViewer)viewer).getTable().getColumns();
            String[] headerRow = new String[numColumns];
            for(int i = 0; i < numColumns; i++)
            {
               headerRow[i] = columns[i].getText();
            }
            data.add(headerRow);
         }

         TableItem[] selection = selectionOnly ? ((TableViewer)viewer).getTable().getSelection() : ((TableViewer)viewer).getTable().getItems();
         for(TableItem item : selection)
         {
            String[] row = new String[numColumns];
            for(int i = 0; i < numColumns; i++)
               row[i] = item.getText(i);
            data.add(row);
         }
      }
      else if (viewer instanceof TreeViewer)
      {
         int numColumns = ((TreeViewer)viewer).getTree().getColumnCount();
         if (numColumns == 0)
            numColumns = 1;

         if (withHeader)
         {
            TreeColumn[] columns = ((TreeViewer)viewer).getTree().getColumns();
            String[] headerRow = new String[numColumns];
            for(int i = 0; i < numColumns; i++)
            {
               headerRow[i] = columns[i].getText();
            }
            data.add(headerRow);
         }

         TreeItem[] selection = selectionOnly ? ((TreeViewer)viewer).getTree().getSelection() : ((TreeViewer)viewer).getTree().getItems();
         for(TreeItem item : selection)
         {
            String[] row = new String[numColumns];
            for(int i = 0; i < numColumns; i++)
               row[i] = item.getText(i);
            data.add(row);
            if (!selectionOnly)
            {
               addSubItems(item, data, numColumns);
            }
         }
      }
      return data;
   }

   /**
    * Add sub-items from tree viewer.
    *
    * @param root current root item
    * @param data data array to fill
    * @param numColumns number of columns in tree viewer
    */
   private static void addSubItems(TreeItem root, List<String[]> data, int numColumns)
   {
      for(TreeItem item : root.getItems())
      {
         String[] row = new String[numColumns];
         for(int i = 0; i < numColumns; i++)
            row[i] = item.getText(i);
         data.add(row);
         addSubItems(item, data, numColumns);
      }
   }

   /**
    * Get column viewer associated with this action.
    *
    * @return column viewer associated with this action
    */
   public ColumnViewer getViewer()
   {
      return viewer;
   }

   /**
    * Change viewer association for this action.
    *
    * @param viewer new viewer
    */
   public void setViewer(ColumnViewer viewer)
   {
      this.viewer = viewer;
   }
}
