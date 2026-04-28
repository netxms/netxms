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
 * Initial view mode for geographical network maps
 */
public enum MapInitialViewMode
{
   FIT_TO_SCREEN(0),
   ZOOM_AND_CENTER(1);

   private static Logger logger = LoggerFactory.getLogger(MapInitialViewMode.class);
   private static Map<Integer, MapInitialViewMode> lookupTable = new HashMap<Integer, MapInitialViewMode>();
   static
   {
      for(MapInitialViewMode element : MapInitialViewMode.values())
      {
         lookupTable.put(element.value, element);
      }
   }

   private int value;

   private MapInitialViewMode(int value)
   {
      this.value = value;
   }

   public int getValue()
   {
      return value;
   }

   public static MapInitialViewMode getByValue(int value)
   {
      final MapInitialViewMode m = lookupTable.get(value);
      if (m == null)
      {
         logger.warn("Unknown initial view mode: " + value);
         return FIT_TO_SCREEN;
      }
      return m;
   }
}
