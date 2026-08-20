/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.widgets.helpers;

import java.util.Set;
import org.eclipse.jface.viewers.ViewerCell;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.internal.theme.CssColor;
import org.eclipse.rap.rwt.internal.theme.SimpleSelector;
import org.eclipse.rap.rwt.internal.theme.ThemeUtil;
import org.eclipse.swt.graphics.Color;
import org.netxms.nxmc.base.widgets.SortableTableViewer;

/**
 * Cell selection highlighter
 */
public class CellSelectionHighlighter
{
   /**
    * @param viewer
    */
   public CellSelectionHighlighter(SortableTableViewer viewer, CellSelectionManager manager)
   {
      // Theme variant "cellselect" disables highlighting of selected rows, so that only selected cells are highlighted
      viewer.getTable().setData(RWT.CUSTOM_VARIANT, "cellselect");
   }

   /**
    * @param newCell
    * @param oldCell
    */
   protected void focusCellChanged(ViewerCell newCell, ViewerCell oldCell)
   {
   }

   /**
    * Called by selection manager when set of selected cells is changed.
    *
    * @param added cells added to selection
    * @param removed cells removed from selection
    */
   protected void selectionChanged(Set<ViewerCell> added, Set<ViewerCell> removed)
   {
      for(ViewerCell cell : removed)
         unmarkCell(cell);
      for(ViewerCell cell : added)
         markCell(cell);
   }

   /**
    * @param cell
    */
   protected void markCell(ViewerCell cell)
   {
      if ((cell == null) || cell.getItem().isDisposed())
         return;

      cell.setBackground(getCellColor(true));
      cell.setForeground(getCellColor(false));
   }

   /**
    * @param cell
    */
   protected void unmarkCell(ViewerCell cell)
   {
      if ((cell == null) || cell.getItem().isDisposed())
         return;

      cell.setBackground(null);
      cell.setForeground(null);
   }

   /**
    * Get selection colors directly with theme util istead of using Display.getSystemColor to avoid spurious "transparent system
    * color" exceptions.
    *
    * @param background true to retrieve background color
    * @return color
    */
   static Color getCellColor(boolean background)
   {
      CssColor css = (CssColor)ThemeUtil.getCssValue("List-Item", background ? "background-color" : "color", SimpleSelector.SELECTED);
      return (css != null) ? CssColor.createColor(css) : null;
   }
}
