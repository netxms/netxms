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
 * Type of anomaly detected in DCI value
 */
public enum AnomalyType
{
   NONE(0),
   RANGE(1),
   STEP(2),
   DRIFT(3);

   private int value;

   /**
    * Internal constructor
    *
    * @param value integer value
    */
   private AnomalyType(int value)
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
    * @return enum element corresponding to given integer value or NONE for invalid value
    */
   public static AnomalyType getByValue(int value)
   {
      for(AnomalyType element : values())
         if (element.value == value)
            return element;
      return NONE;
   }
}
