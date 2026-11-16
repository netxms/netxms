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

/**
 * Direction of deviations reported as anomalies
 */
public enum AnomalyDirection
{
   BOTH(0),
   UP(1),
   DOWN(2);

   private int value;

   /**
    * Internal constructor
    *
    * @param value integer value
    */
   private AnomalyDirection(int value)
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
    * @return enum element corresponding to given integer value or BOTH for invalid value
    */
   public static AnomalyDirection getByValue(int value)
   {
      for(AnomalyDirection element : values())
         if (element.value == value)
            return element;
      return BOTH;
   }

   /**
    * Get enum element by name used in anomaly detection settings document
    *
    * @param name element name (case insensitive)
    * @return enum element corresponding to given name or BOTH for unknown or missing name
    */
   public static AnomalyDirection getByName(String name)
   {
      for(AnomalyDirection element : values())
         if (element.name().equalsIgnoreCase(name))
            return element;
      return BOTH;
   }

   /**
    * Get name used in anomaly detection settings document
    *
    * @return element name
    */
   public String getName()
   {
      return name().toLowerCase();
   }
}
