/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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
package org.netxms.nxmc.modules.networkmaps.widgets.helpers;

/**
 * Geometry of a small arrow drawn in link labels to show data direction: thick shaft and pointed head with swept back
 * corners
 */
public final class DirectionArrow
{
   /**
    * Shaft line: x1, y1, x2, y2
    */
   public final int[] shaft;

   /**
    * Shaft line width
    */
   public final int shaftWidth;

   /**
    * Head polygon: tip, left corner, notch where shaft enters the head, right corner
    */
   public final int[] head;

   /**
    * Create arrow centered at given point and pointing along given unit vector
    *
    * @param cx center X
    * @param cy center Y
    * @param ux unit vector X
    * @param uy unit vector Y
    * @param length arrow length
    */
   public DirectionArrow(int cx, int cy, double ux, double uy, int length)
   {
      double tipX = cx + ux * length / 2.0;
      double tipY = cy + uy * length / 2.0;
      double headLength = length * 0.62;
      double headHalfWidth = length * 0.36;
      double cornerX = tipX - ux * headLength;
      double cornerY = tipY - uy * headLength;
      double notchX = cornerX + ux * headLength * 0.4;
      double notchY = cornerY + uy * headLength * 0.4;
      shaft = new int[] { (int)Math.round(cx - ux * length / 2.0), (int)Math.round(cy - uy * length / 2.0), (int)Math.round(notchX), (int)Math.round(notchY) };
      shaftWidth = Math.max(2, (int)Math.round(length * 0.15));
      head = new int[] {
         (int)Math.round(tipX), (int)Math.round(tipY),
         (int)Math.round(cornerX - uy * headHalfWidth), (int)Math.round(cornerY + ux * headHalfWidth),
         (int)Math.round(notchX), (int)Math.round(notchY),
         (int)Math.round(cornerX + uy * headHalfWidth), (int)Math.round(cornerY - ux * headHalfWidth)
      };
   }

   /**
    * Get unit vector of the segment between two points
    *
    * @return unit vector (x, y) or null for zero length segment
    */
   public static double[] unitVector(int x1, int y1, int x2, int y2)
   {
      double dx = x2 - x1;
      double dy = y2 - y1;
      double length = Math.sqrt(dx * dx + dy * dy);
      return (length < 0.001) ? null : new double[] { dx / length, dy / length };
   }
}
