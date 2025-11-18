/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.objects.widgets.helpers;

import org.eclipse.swt.graphics.Point;

/**
 * Calculates port tab port layout
 * down-up, then left-to-right:
 *  2 4 6 8
 *  1 3 5 7
 */
public class PortCalculatorDownUpLeftRight extends PortCalculator
{
   private int x;
   private int y;
   private int rowCount;
   private int row = 0;

   /**
    * @param baseX
    * @param rowCount
    */
   public PortCalculatorDownUpLeftRight(int baseX, int rowCount)
   {
      x = HORIZONTAL_MARGIN + HORIZONTAL_SPACING + baseX;
      y = VERTICAL_MARGIN + (rowCount-1) * (VERTICAL_SPACING + PORT_HEIGHT);
      this.rowCount = rowCount;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.widgets.helpers.PortCalculator#calculateNextPos()
    */
   @Override
   public Point calculateNextPos()
   {      
      if (row == 0)
      {  
         row++;
         return new Point(x, y);
      }
      
      if (row == rowCount)
      {
         row = 0;
         y = VERTICAL_MARGIN + (rowCount-1) * (VERTICAL_SPACING + PORT_HEIGHT);
         x += HORIZONTAL_SPACING + PORT_WIDTH;
      }
      else
      {
         y -= VERTICAL_SPACING + PORT_HEIGHT;
      }

      row++;
      return new Point(x, y);
   }
}
