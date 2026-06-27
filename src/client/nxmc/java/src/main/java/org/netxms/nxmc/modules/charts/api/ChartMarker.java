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
package org.netxms.nxmc.modules.charts.api;

import org.eclipse.swt.graphics.RGB;

/**
 * User-defined vertical marker (oscilloscope-style cursor) on a line chart. Markers are session-local
 * annotations placed at an absolute point in time; they are not persisted.
 */
public class ChartMarker
{
   private long timestamp;
   private String label;
   private RGB color;

   /**
    * Create new marker.
    *
    * @param timestamp marker position (milliseconds since epoch)
    * @param label marker label
    * @param color marker color
    */
   public ChartMarker(long timestamp, String label, RGB color)
   {
      this.timestamp = timestamp;
      this.label = label;
      this.color = color;
   }

   /**
    * @return marker position (milliseconds since epoch)
    */
   public long getTimestamp()
   {
      return timestamp;
   }

   /**
    * @param timestamp marker position (milliseconds since epoch)
    */
   public void setTimestamp(long timestamp)
   {
      this.timestamp = timestamp;
   }

   /**
    * @return marker label
    */
   public String getLabel()
   {
      return label;
   }

   /**
    * @param label marker label
    */
   public void setLabel(String label)
   {
      this.label = label;
   }

   /**
    * @return marker color
    */
   public RGB getColor()
   {
      return color;
   }

   /**
    * @param color marker color
    */
   public void setColor(RGB color)
   {
      this.color = color;
   }
}
