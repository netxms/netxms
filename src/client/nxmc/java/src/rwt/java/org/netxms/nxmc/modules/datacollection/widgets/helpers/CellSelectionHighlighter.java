/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Victor Kirhenshtein
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

import org.eclipse.jface.viewers.ViewerCell;
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
   }

   /**
    * @param newCell
    * @param oldCell
    */
   protected void focusCellChanged(ViewerCell newCell, ViewerCell oldCell)
   {
   }

   /**
    * @param cell
    */
   protected void markCell(ViewerCell cell)
   {
      if (cell == null)
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
