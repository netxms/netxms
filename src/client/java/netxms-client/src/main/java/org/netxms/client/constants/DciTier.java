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
 * DCI history query tier — selects which storage layer the server reads from.
 */
public enum DciTier
{
   AUTO(0),
   RAW(1),
   HOURLY(2),
   DAILY(3);

   private static Logger logger = LoggerFactory.getLogger(DciTier.class);
   private static Map<Integer, DciTier> lookupTable = new HashMap<Integer, DciTier>();
   static
   {
      for(DciTier element : DciTier.values())
      {
         lookupTable.put(element.value, element);
      }
   }

   private int value;

   private DciTier(int value)
   {
      this.value = value;
   }

   public int getValue()
   {
      return value;
   }

   public static DciTier getByValue(int value)
   {
      final DciTier element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return AUTO;
      }
      return element;
   }
}
