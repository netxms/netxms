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
package org.netxms.client.maps;

import java.util.HashMap;
import java.util.Map;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Network map canvas type
 */
public enum MapCanvasType
{
   GRAPH(0),
   GEOGRAPHICAL(1);

   private static Logger logger = LoggerFactory.getLogger(MapCanvasType.class);
   private static Map<Integer, MapCanvasType> lookupTable = new HashMap<Integer, MapCanvasType>();
   static
   {
      for(MapCanvasType element : MapCanvasType.values())
      {
         lookupTable.put(element.value, element);
      }
   }

   private int value;

   private MapCanvasType(int value)
   {
      this.value = value;
   }

   public int getValue()
   {
      return value;
   }

   public static MapCanvasType getByValue(int value)
   {
      final MapCanvasType ct = lookupTable.get(value);
      if (ct == null)
      {
         logger.warn("Unknown map canvas type: " + value);
         return GRAPH;
      }
      return ct;
   }
}
