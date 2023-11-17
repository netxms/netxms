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
package org.netxms.client.constants;

import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import org.netxms.client.ClientLocalizationHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Sensor device class
 */
public enum SensorDeviceClass
{
   OTHER(0),
   UPS(1),
   WATER_METER(2),
   ELECTRICITY_METER(3),
   TEMPERATURE_SENSOR(4),
   HUMIDITY_SENSOR(5),
   TEMPERATURE_HUMIDITY_SENSOR(6),
   CO2_SENSOR(7),
   POWER_SUPPLY(8),
   CURRENT_SENSOR(9),
   WATER_LEAK_DETECTOR(10),
   SMOKE_DETECTOR(11);

   private static Logger logger = LoggerFactory.getLogger(SensorDeviceClass.class);
   private static Map<Integer, SensorDeviceClass> lookupTable = new HashMap<Integer, SensorDeviceClass>();
   static
   {
      for(SensorDeviceClass element : SensorDeviceClass.values())
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
   private SensorDeviceClass(int value)
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
    * Get display name for this value in default locale.
    *
    * @return display name for this value in default locale
    */
   public String getDisplayName()
   {
      String name = toString();
      return ClientLocalizationHelper.getText("SensorDeviceClass_" + name, Locale.getDefault(), name);
   }

   /**
    * Get display name for this value in default locale.
    *
    * @param locale locale to use
    * @return display name for this value in given locale
    */
   public String getDisplayName(Locale locale)
   {
      String name = toString();
      return ClientLocalizationHelper.getText("SensorDeviceClass_" + name, locale, name);
   }

   /**
    * Get enum element by integer value
    * 
    * @param value integer value
    * @return enum element corresponding to given integer value or fall-back element for invalid value
    */
   public static SensorDeviceClass getByValue(int value)
   {
      final SensorDeviceClass element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return OTHER; // fall-back
      }
      return element;
   }
}
