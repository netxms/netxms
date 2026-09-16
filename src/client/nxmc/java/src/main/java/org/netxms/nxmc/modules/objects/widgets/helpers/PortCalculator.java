/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
 * Base class for port display coordinates calculator in port view widget. Port box size and spacing are scaled by percentage
 * given at construction time; margins are not scaled.
 */
public abstract class PortCalculator
{
   public static final int HORIZONTAL_MARGIN = 20;
   public static final int VERTICAL_MARGIN = 10;
   public static final int DEFAULT_HORIZONTAL_SPACING = 10;
   public static final int DEFAULT_VERTICAL_SPACING = 10;
   public static final int DEFAULT_PORT_WIDTH = 44;
   public static final int DEFAULT_PORT_HEIGHT = 30;

   protected final int horizontalSpacing;
   protected final int verticalSpacing;
   protected final int portWidth;
   protected final int portHeight;

   /**
    * Create calculator with port metrics scaled by given percentage.
    *
    * @param scale scale in percents (100 = default size)
    */
   protected PortCalculator(int scale)
   {
      horizontalSpacing = scaled(DEFAULT_HORIZONTAL_SPACING, scale);
      verticalSpacing = scaled(DEFAULT_VERTICAL_SPACING, scale);
      portWidth = scaled(DEFAULT_PORT_WIDTH, scale);
      portHeight = scaled(DEFAULT_PORT_HEIGHT, scale);
   }

   /**
    * Scale metric value by given percentage.
    *
    * @param value value at 100%
    * @param scale scale in percents
    * @return scaled value, never less than 1
    */
   public static int scaled(int value, int scale)
   {
      return Math.max(1, value * scale / 100);
   }

   /**
    * Calculates next position of port
    */
   public abstract Point calculateNextPos();
}
