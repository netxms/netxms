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
 * left-to-right, then down-up:
 *  5 6 7 8
 *  1 2 3 4
 */
public class PortCalculatorLeftRightDownUp implements PortCalculator
{
   private int x;
   private int y;
   private int portCount;
   private int col = 0;
   private int nameSize;
   private int rowCount;
   
   /**
    * @param nameSize
    * @param portCount
    * @param rowCount
    */
   public PortCalculatorLeftRightDownUp(int nameSize, int portCount, int rowCount)
   {
      x = HORIZONTAL_MARGIN + HORIZONTAL_SPACING + nameSize;
      y = VERTICAL_MARGIN + (rowCount-1) * (VERTICAL_SPACING + PORT_HEIGHT);
      
      this.portCount = portCount;
      this.nameSize = nameSize;
      this.rowCount = rowCount;
   }
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.topology.widgets.helpers.PortCalculator#calculateNextPos()
    */
   @Override
   public void calculateNextPos()
   {
      int colCount = ((portCount%rowCount) != 0) ? (portCount/rowCount)+1 : (portCount/rowCount); // To maintain correct row count
      col++;
      if (col == colCount && y > VERTICAL_MARGIN)
      {
         col = 0;
         y -= VERTICAL_SPACING + PORT_HEIGHT;
         x = HORIZONTAL_MARGIN + HORIZONTAL_SPACING + nameSize;
      }
      else
         x += HORIZONTAL_SPACING + PORT_WIDTH;
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
