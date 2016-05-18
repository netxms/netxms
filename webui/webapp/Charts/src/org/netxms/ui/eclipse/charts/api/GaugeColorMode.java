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
package org.netxms.ui.eclipse.charts.api;

import java.util.HashMap;
import java.util.Map;
import org.netxms.base.Logger;

/**
 * Gauge color mode
 */
public enum GaugeColorMode
{
   ZONE(0),
   CUSTOM(1),
   THRESHOLD(2);
   
   private int value;
   private static Map<Integer, GaugeColorMode> lookupTable = new HashMap<Integer, GaugeColorMode>();

   static
   {
      for(GaugeColorMode element : GaugeColorMode.values())
      {
         lookupTable.put(element.value, element);
      }
   }

   /**
    * @param value
    */
   private GaugeColorMode(int value)
   {
      this.value = value;
   }

   /**
    * @return
    */
   public int getValue()
   {
      return value;
   }

   /**
    * @param value
    * @return
    */
   public static GaugeColorMode getByValue(int value)
   {
      final GaugeColorMode element = lookupTable.get(value);
      if (element == null)
      {
         Logger.warning(GaugeColorMode.class.getName(), "Unknown element " + value); //$NON-NLS-1$
         return ZONE; // fallback
      }
      return element;
   }
}
