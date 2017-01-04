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
 * Interface to calculate
 * port coordinates in
 * port objecttab for different
 * layouts
 */
public interface PortCalculator
{
   public static final int HORIZONTAL_MARGIN = 20;
   public static final int VERTICAL_MARGIN = 10;
   public static final int HORIZONTAL_SPACING = 10;
   public static final int VERTICAL_SPACING = 10;
   public static final int PORT_WIDTH = 44;
   public static final int PORT_HEIGHT = 30;
   
   /**
    * Calculates next position of port
    */
   public void calculateNextPos();
   
   /**
    * @return X coordinate
    */
   public int getXPos();
   
   /**
    * @return Y coordinate
    */
   public int getYPos();
}
