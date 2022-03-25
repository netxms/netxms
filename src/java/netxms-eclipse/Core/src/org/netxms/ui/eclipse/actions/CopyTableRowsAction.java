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
package org.netxms.ui.eclipse.actions;

import java.util.List;
import org.eclipse.jface.viewers.ColumnViewer;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Action for copying selected table viewer rows to clipboard
 */
public class CopyTableRowsAction extends TableRowAction
{
   /**
    * Create "Copy" action for copying table rows
    * 
    * @param viewer viewer to copy rows from
    * @param viewerProvider viewer provider
    * @param selectionOnly true to copy only selected rows
    */
   public CopyTableRowsAction(ColumnViewer viewer, ViewerProvider viewerProvider, boolean selectionOnly)
   {
      super(viewer, viewerProvider, selectionOnly, selectionOnly ? "&Copy to clipboard" : "&Copy all to clipboard");
      setId(selectionOnly ? "org.netxms.ui.eclipse.popupActions.CopyTableRows" : "org.netxms.ui.eclipse.actions.CopyAllTableRows"); //$NON-NLS-1$ //$NON-NLS-2$
   }

   /**
    * Create "Copy" action for copying table rows
    * 
    * @param viewer viewer to copy rows from
    * @param selectionOnly true to copy only selected rows
    */
   public CopyTableRowsAction(ColumnViewer viewer, boolean selectionOnly)
   {
      this(viewer, null, selectionOnly);
   }

   /**
    * Create "Copy" action for copying table rows
    * 
    * @param viewerProvider viewer provider
    * @param selectionOnly true to copy only selected rows
    */
   public CopyTableRowsAction(ViewerProvider viewerProvider, boolean selectionOnly)
   {
      this(null, viewerProvider, selectionOnly);
   }

   /**
    * @see org.eclipse.jface.action.Action#run()
    */
   @Override
   public void run()
   {
      final List<String[]> data = getRowsFromViewer(false);
      String nl = System.lineSeparator();
      StringBuilder sb = new StringBuilder();
      for(String[] row : data)
      {
         sb.append(row[0]);
         for(int i = 1; i < row.length; i++)
         {
            sb.append('\t');
            sb.append(row[i]);
         }
         sb.append(nl);
      }
      WidgetHelper.copyToClipboard(sb.toString());
   }
}
