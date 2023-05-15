/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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

import java.util.HashMap;
import java.util.Map;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Chart types
 */
public enum ChartType
{
   LINE(0),
   PIE(1),
   BAR(2),
   DIAL_GAUGE(3),
   BAR_GAUGE(4),
   TEXT_GAUGE(5),
   CIRCULAR_GAUGE(6);

   private static final Logger logger = LoggerFactory.getLogger(ChartType.class);
   private static final Map<Integer, ChartType> lookupTable = new HashMap<Integer, ChartType>();
   static
   {
      for(ChartType element : ChartType.values())
      {
         lookupTable.put(element.value, element);
      }
   }

   private int value;

   /**
    * @param value
    */
   private ChartType(int value)
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
   public static ChartType getByValue(int value)
   {
      final ChartType element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value); //$NON-NLS-1$
         return LINE; // fallback
      }
      return element;
   }
}
