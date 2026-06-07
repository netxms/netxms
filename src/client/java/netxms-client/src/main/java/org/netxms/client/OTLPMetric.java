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

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPMessage;

/**
 * OTLP metric observed by the server for a node, as offered by the metric selector.
 */
public class OTLPMetric
{
   public static final int GAUGE = 0;
   public static final int SUM = 1;
   public static final int HISTOGRAM = 2;

   private String name;
   private int type;
   private List<String> attributeKeys;

   /**
    * Create OTLP metric from NXCP message. Record layout: baseId = name,
    * baseId+1 = type, baseId+2 = attribute key count, baseId+3.. = key names.
    *
    * @param msg NXCP message
    * @param baseId base field ID of this record
    */
   protected OTLPMetric(final NXCPMessage msg, final long baseId)
   {
      name = msg.getFieldAsString(baseId);
      type = msg.getFieldAsInt32(baseId + 1);
      int keyCount = msg.getFieldAsInt32(baseId + 2);
      attributeKeys = new ArrayList<String>(keyCount);
      long fieldId = baseId + 3;
      for(int i = 0; i < keyCount; i++)
         attributeKeys.add(msg.getFieldAsString(fieldId++));
   }

   /**
    * Number of NXCP fields occupied by this record (name, type, key count, and the keys).
    * Used by the reader to advance to the next record.
    *
    * @return number of NXCP fields in this record
    */
   public int getFieldCount()
   {
      return 3 + attributeKeys.size();
   }

   /**
    * @return metric name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return metric type (GAUGE, SUM, or HISTOGRAM)
    */
   public int getType()
   {
      return type;
   }

   /**
    * @return human-readable metric type name
    */
   public String getTypeName()
   {
      switch(type)
      {
         case GAUGE:
            return "Gauge";
         case SUM:
            return "Sum";
         case HISTOGRAM:
            return "Histogram";
         default:
            return "Unknown";
      }
   }

   /**
    * @return list of attribute key names observed for this metric
    */
   public List<String> getAttributeKeys()
   {
      return attributeKeys;
   }
}
