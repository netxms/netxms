/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
import org.netxms.base.Logger;

/**
 * Data aggregation function
 */
public enum AggregationFunction
{
   LAST(0),
   MIN(1),
   MAX(2),
   AVERAGE(3),
   SUM(4);

   private int value;
   private static Map<Integer, AggregationFunction> lookupTable = new HashMap<Integer, AggregationFunction>();

   static
   {
      for(AggregationFunction element : AggregationFunction.values())
      {
         lookupTable.put(element.value, element);
      }
   }

   /**
    * @param value
    */
   private AggregationFunction(int value)
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
   public static AggregationFunction getByValue(int value)
   {
      final AggregationFunction element = lookupTable.get(value);
      if (element == null)
      {
         Logger.warning(AggregationFunction.class.getName(), "Unknown element " + value);
         return LAST; // fallback
      }
      return element;
   }
}
