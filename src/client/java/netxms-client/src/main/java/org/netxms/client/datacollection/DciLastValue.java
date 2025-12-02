/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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

import java.util.Date;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.Table;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.DataType;

/**
 * Generic last value for single valued or table DCI.
 */
public class DciLastValue
{
   private int dciType;
   private DataOrigin dataOrigin;
   private Date timestamp;
   private DataType dataType;
   private String value;
   private String rawValue;
   private Table tableValue;

   /**
    * Create from NXCP message
    * 
    * @param msg NXCP message
    */
   public DciLastValue(NXCPMessage msg)
   {
      dciType = msg.getFieldAsInt32(NXCPCodes.VID_DCOBJECT_TYPE);
      dataOrigin = DataOrigin.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_DCI_SOURCE_TYPE));
      timestamp = msg.getFieldAsTimestamp(NXCPCodes.VID_TIMESTAMP_MS);
      if (dciType == DataCollectionObject.DCO_TYPE_TABLE)
      {
         tableValue = new Table(msg);
      }
      else if (dciType == DataCollectionObject.DCO_TYPE_ITEM)
      {
         dataType = DataType.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_DCI_DATA_TYPE));
         value = msg.getFieldAsString(NXCPCodes.VID_VALUE);
         rawValue = msg.getFieldAsString(NXCPCodes.VID_RAW_VALUE);
      }
   }

   /**
    * Get DCI type (table or single valued). Will return either <code>DataCollectionObject.DCO_TYPE_ITEM</code> or
    * <code>DataCollectionObject.DCO_TYPE_TABLE</code>.
    *
    * @return DCI type
    */
   public int getDciType()
   {
      return dciType;
   }

   /**
    * Get data origin
    *
    * @return data origin
    */
   public DataOrigin getDataOrigin()
   {
      return dataOrigin;
   }

   /**
    * Get timestamp for this value
    *
    * @return timestamp for this value
    */
   public Date getTimestamp()
   {
      return timestamp;
   }

   /**
    * Get data type for single value DCI
    *
    * @return data type for single value DCI or null for table DCI
    */
   public DataType getDataType()
   {
      return dataType;
   }

   /**
    * Get value of single value DCI
    *
    * @return value of single value DCI or null for table DCI
    */
   public String getValue()
   {
      return value;
   }

   /**
    * Get raw value of single value DCI
    *
    * @return raw value of single value DCI or null for table DCI
    */
   public String getRawValue()
   {
      return rawValue;
   }

   /**
    * Get value of table DCI
    *
    * @return table DCI value or null for single value DCI
    */
   public Table getTableValue()
   {
      return tableValue;
   }
}
