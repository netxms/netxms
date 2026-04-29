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
package org.netxms.client.constants;

import java.util.HashMap;
import java.util.Map;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * DCI history aggregation function — selects which aggregate column(s) the server returns.
 *
 * MINMAX returns both min and max in a single response (single round trip), suitable for
 * band-graph rendering.
 */
public enum DciAggregationFunction
{
   AVG(0),
   MIN(1),
   MAX(2),
   MINMAX(3);

   private static Logger logger = LoggerFactory.getLogger(DciAggregationFunction.class);
   private static Map<Integer, DciAggregationFunction> lookupTable = new HashMap<Integer, DciAggregationFunction>();
   static
   {
      for(DciAggregationFunction element : DciAggregationFunction.values())
      {
         lookupTable.put(element.value, element);
      }
   }

   private int value;

   private DciAggregationFunction(int value)
   {
      this.value = value;
   }

   public int getValue()
   {
      return value;
   }

   public static DciAggregationFunction getByValue(int value)
   {
      final DciAggregationFunction element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return AVG;
      }
      return element;
   }
}
