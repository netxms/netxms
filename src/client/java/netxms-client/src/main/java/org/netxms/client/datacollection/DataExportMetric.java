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
package org.netxms.client.datacollection;

import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.DataType;
import org.netxms.client.constants.DciTier;

/**
 * Metric selected by DCI history export request (preview of export content)
 */
public class DataExportMetric
{
   private final long ownerId;
   private final long dciId;
   private final String tag;
   private final String name;
   private final String description;
   private final String unit;
   private final DataOrigin origin;
   private final DataType dataType;
   private final DciTier tier;

   /**
    * Create from NXCP message
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public DataExportMetric(NXCPMessage msg, long baseId)
   {
      ownerId = msg.getFieldAsInt64(baseId);
      dciId = msg.getFieldAsInt64(baseId + 1);
      tag = msg.getFieldAsString(baseId + 2);
      name = msg.getFieldAsString(baseId + 3);
      description = msg.getFieldAsString(baseId + 4);
      unit = msg.getFieldAsString(baseId + 5);
      origin = DataOrigin.getByValue(msg.getFieldAsInt32(baseId + 6));
      dataType = DataType.getByValue(msg.getFieldAsInt32(baseId + 7));
      tier = DciTier.getByValue(msg.getFieldAsInt32(baseId + 8));
   }

   /**
    * @return owner object ID
    */
   public long getOwnerId()
   {
      return ownerId;
   }

   /**
    * @return DCI ID
    */
   public long getDciId()
   {
      return dciId;
   }

   /**
    * @return system (classifier) tag
    */
   public String getTag()
   {
      return tag;
   }

   /**
    * @return DCI name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return DCI description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * @return unit name
    */
   public String getUnit()
   {
      return unit;
   }

   /**
    * @return data origin
    */
   public DataOrigin getOrigin()
   {
      return origin;
   }

   /**
    * @return transformed data type
    */
   public DataType getDataType()
   {
      return dataType;
   }

   /**
    * @return history tier that export will read for this metric
    */
   public DciTier getTier()
   {
      return tier;
   }
}
