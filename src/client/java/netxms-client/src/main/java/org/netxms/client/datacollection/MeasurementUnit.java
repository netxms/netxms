/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.client.datacollection;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import org.netxms.base.NXCPMessage;

/**
 * Represents measurement unit information.
 */
public class MeasurementUnit
{
   public static final MeasurementUnit BYTES_IEC = new MeasurementUnit("B (IEC)");
   public static final MeasurementUnit BYTES_METRIC = new MeasurementUnit("B");
   public static final MeasurementUnit BPS_IEC = new MeasurementUnit("bps (IEC)");
   public static final MeasurementUnit BPS_METRIC = new MeasurementUnit("bps");
   public static final MeasurementUnit HZ = new MeasurementUnit("Hz");

   public static final Set<String> UNITS_WITHOUT_MULTIPLIERS = new HashSet<>(Arrays.asList("%", "°C", "°F", "dBm", "rpm"));

   private String name;
   private boolean binary;

   /**
    * Protected constructor for deserialization
    */
   protected MeasurementUnit()
   {
      name = "";
      binary = false;
   }

   /**
    * Create measurement unit from scratch
    * 
    * @param name unit name
    */
   public MeasurementUnit(String name)
   {
      this.name = name.replace(" (IEC)", "").replace(" (Metric)", "");
      this.binary = name.contains(" (IEC)");
   }
   
   /**
    * Create measurement unit object from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public MeasurementUnit(NXCPMessage msg, long baseId)
   {
      this(msg.getFieldAsString(baseId));
   }   

   /**
    * Get unit name.
    *
    * @return unit name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Check if unit uses binary (IEC) multipliers
    *
    * @return true if unit uses binary multipliers
    */
   public boolean isBinary()
   {
      return binary;
   }
   
   /**
    * If small multipliers should be used for current unit
    * 
    * @return true if small multipliers can be used for current unit
    */
   public boolean useMultiplierForUnit()
   {
      return !UNITS_WITHOUT_MULTIPLIERS.contains(name);
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "MeasurementUnit [name=" + name + ", binary=" + binary + "]";
   }
}
