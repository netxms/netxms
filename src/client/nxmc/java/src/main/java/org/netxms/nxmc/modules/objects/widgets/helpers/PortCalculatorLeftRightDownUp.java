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
 * left-to-right, then down-up:
 *  5 6 7 8
 *  1 2 3 4
 */
public class PortCalculatorLeftRightDownUp extends PortCalculator
{
   private int x;
   private int y;
   private int portCount;
   private int col = 0;
   private int baseX;
   private int rowCount;

   /**
    * @param baseX
    * @param portCount
    * @param rowCount
    */
   public PortCalculatorLeftRightDownUp(int baseX, int portCount, int rowCount)
   {
      this.x = this.baseX = HORIZONTAL_MARGIN + HORIZONTAL_SPACING + baseX;
      this.y = VERTICAL_MARGIN + (rowCount - 1) * (VERTICAL_SPACING + PORT_HEIGHT);
      this.portCount = portCount;
      this.rowCount = rowCount;
   }
   
   /**
    * @see org.netxms.nxmc.modules.objects.widgets.helpers.PortCalculator#calculateNextPos()
    */
   @Override
   public Point calculateNextPos()
   {      
      if (col == 0)
      {
         col++;
         return new Point(x, y);
      }
      
      int colCount = ((portCount%rowCount) != 0) ? (portCount/rowCount)+1 : (portCount/rowCount); // To maintain correct row count
      
      if (col == colCount && y > VERTICAL_MARGIN)
      {
         col = 0;
         y -= VERTICAL_SPACING + PORT_HEIGHT;
         x = baseX;
      }
      else
      {
         x += HORIZONTAL_SPACING + PORT_WIDTH;
      }

      col++;
      return new Point(x, y);
   }
}
