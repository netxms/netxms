/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Raden Solutions
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
package org.netxms.ui.eclipse.topology.widgets.helpers;

/* 
 * Calculates port tab port layout
 * up-down, then left-to-right:
 *  1 3 5 7
 *  2 4 6 8
 */
public class PortCalculatorUpDownLeftRight implements PortCalculator
{   
   private int x;
   private int y;
   private int rowCount;
   private int row = 0;
   
   /**
    * @param nameSize
    * @param rowCount
    */
   public PortCalculatorUpDownLeftRight(int nameSize, int rowCount)
   {
      x = HORIZONTAL_MARGIN + HORIZONTAL_SPACING + nameSize;
      y = VERTICAL_MARGIN;
      this.rowCount = rowCount;
   }
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.topology.widgets.helpers.PortCalculator#calculateNextPos()
    */
   @Override
   public void calculateNextPos()
   {
      row++;
      if (row == rowCount)
      {
         row = 0;
         y = VERTICAL_MARGIN;
         x += HORIZONTAL_SPACING + PORT_WIDTH;
      }
      else
         y += VERTICAL_SPACING + PORT_HEIGHT;
   }
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.topology.widgets.helpers.PortCalculator#getXPos()
    */
   @Override
   public int getXPos()
   {
      return x;
   }
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.topology.widgets.helpers.PortCalculator#getYPos()
    */
   @Override
   public int getYPos()
   {
      return y;
   }
}
