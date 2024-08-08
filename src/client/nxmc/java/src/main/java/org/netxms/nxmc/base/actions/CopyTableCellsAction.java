/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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

import java.util.List;
import org.eclipse.jface.viewers.ColumnViewer;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Action for copying predefined columns in selected table viewer rows to clipboard
 */
public class CopyTableCellsAction extends TableRowAction
{
   private int column;

   /**
    * Create "Copy" action for copying table rows
    * 
    * @param viewer viewer to copy rows from
    * @param viewerProvider viewer provider
    * @param column column number
    * @param selectionOnly true to copy only selected rows
    * @param name action name
    */
   public CopyTableCellsAction(ColumnViewer viewer, ViewerProvider viewerProvider, int column, boolean selectionOnly, String name)
   {
      super(viewer, viewerProvider, selectionOnly, name);
      this.column = column;
   }

   /**
    * Create "Copy" action for copying table rows
    * 
    * @param viewer viewer to copy rows from
    * @param column column number
    * @param selectionOnly true to copy only selected rows
    * @param name action name
    */
   public CopyTableCellsAction(ColumnViewer viewer, int column, boolean selectionOnly, String name)
   {
      this(viewer, null, column, selectionOnly, name);
   }

   /**
    * Create "Copy" action for copying table rows
    * 
    * @param viewerProvider viewer provider
    * @param column column number
    * @param selectionOnly true to copy only selected rows
    * @param name action name
    */
   public CopyTableCellsAction(ViewerProvider viewerProvider, int column, boolean selectionOnly, String name)
   {
      this(null, viewerProvider, column, selectionOnly, name);
   }

   /**
    * @see org.eclipse.jface.action.Action#run()
    */
   @Override
   public void run()
   {
      final List<String[]> data = getRowsFromViewer(false);
      if (data.size() == 0)
      {
         WidgetHelper.copyToClipboard("");
      }
      else if (data.size() == 1)
      {
         String[] row = data.get(0);
         WidgetHelper.copyToClipboard(row.length > column ? row[column] : "");
      }
      else
      {
         String nl = System.lineSeparator();
         StringBuilder sb = new StringBuilder();
         for(String[] row : data)
         {
            sb.append(row.length > column ? row[column] : "");
            sb.append(nl);
         }
         WidgetHelper.copyToClipboard(sb.toString());
      }
   }
}
