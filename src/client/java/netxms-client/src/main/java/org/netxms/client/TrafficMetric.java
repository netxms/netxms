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
package org.netxms.client;

import org.netxms.base.NXCPMessage;

/**
 * Traffic observer metric definition, as offered by the DCI editor metric picker.
 * Served by the connector's metric catalog and filtered by the observer's capabilities.
 */
public class TrafficMetric
{
   private String name;
   private String displayName;
   private String unit;
   private String description;
   private boolean isTable;
   private long requiredCapability;

   /**
    * Create traffic metric definition from NXCP message. Record stride is 10 fields:
    * baseId = name, +1 = display name, +2 = unit, +3 = description, +4 = is-table,
    * +5 = required capability bitmask (remaining fields reserved).
    *
    * @param msg NXCP message
    * @param baseId base field ID of this record
    */
   public TrafficMetric(final NXCPMessage msg, final long baseId)
   {
      name = msg.getFieldAsString(baseId);
      displayName = msg.getFieldAsString(baseId + 1);
      unit = msg.getFieldAsString(baseId + 2);
      description = msg.getFieldAsString(baseId + 3);
      isTable = msg.getFieldAsBoolean(baseId + 4);
      requiredCapability = msg.getFieldAsInt64(baseId + 5);
   }

   /**
    * @return metric name (as used in the DCI metric field)
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return human-readable display name
    */
   public String getDisplayName()
   {
      return displayName;
   }

   /**
    * @return unit string (may be empty)
    */
   public String getUnit()
   {
      return unit;
   }

   /**
    * @return metric description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * @return true if this metric is a table
    */
   public boolean isTable()
   {
      return isTable;
   }

   /**
    * @return required capability bitmask (0 = always available)
    */
   public long getRequiredCapability()
   {
      return requiredCapability;
   }
}
