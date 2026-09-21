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
package org.netxms.client.objects.configs;

/**
 * Point in room-local coordinates (millimetres)
 */
public class RoomPoint
{
   public int x;
   public int y;

   /**
    * Create new point.
    *
    * @param x X coordinate in millimetres
    * @param y Y coordinate in millimetres
    */
   public RoomPoint(int x, int y)
   {
      this.x = x;
      this.y = y;
   }

   /**
    * Copy constructor.
    *
    * @param src source point
    */
   public RoomPoint(RoomPoint src)
   {
      this.x = src.x;
      this.y = src.y;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "(" + x + ", " + y + ")";
   }
}
