/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
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
 * Event reference type enum for EventReference
 */
public enum EventReferenceType
{
   ERROR(-1),
   AGENT_POLICY(0),
   DCI(1),
   EP_RULE(2),
   SNMP_TRAP(3),
   CONDITION(4),
   SYSLOG(5),
   WINDOWS_EVENT_LOG(6);

   private static Logger logger = LoggerFactory.getLogger(EventReferenceType.class);
   private static Map<Integer, EventReferenceType> lookupTable = new HashMap<Integer, EventReferenceType>();
   static
   {
      for(EventReferenceType element : EventReferenceType.values())
      {
         lookupTable.put(element.value, element);
      }
   }

   private int value;

   /**
    * Internal constructor
    * 
    * @param value integer value
    */
   private EventReferenceType(int value)
   {
      this.value = value;
   }

   /**
    * Get integer value
    * 
    * @return integer value
    */
   public int getValue()
   {
      return value;
   }

   /**
    * Get enum element by integer value
    * 
    * @param value integer value
    * @return enum element corresponding to given integer value or fall-back element for invalid value
    */
   public static EventReferenceType getByValue(int value)
   {
      final EventReferenceType element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return ERROR; // fallback
      }
      return element;
   }
};
