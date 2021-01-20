/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.reporting.model;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

/**
 * Report definition
 */
public class ReportDefinition
{
   private String name;
   private int numberOfColumns = 1;
   private Map<String, ReportParameter> parameters = new HashMap<String, ReportParameter>(0);

   public String getName()
   {
      return name;
   }

   public void setName(String name)
   {
      this.name = name;
   }

   public Map<String, ReportParameter> getParameters()
   {
      return parameters;
   }

   public void putParameter(ReportParameter parameter)
   {
      this.parameters.put(parameter.getName(), parameter);
   }

   public int getNumberOfColumns()
   {
      return numberOfColumns;
   }

   public void setNumberOfColumns(int numberOfColumns)
   {
      if (numberOfColumns > 0)
      {
         this.numberOfColumns = numberOfColumns;
      }
      else
      {
         this.numberOfColumns = 1;
      }
   }

   public void fillMessage(NXCPMessage message)
   {
      message.setField(NXCPCodes.VID_NAME, name);
      final int size = parameters.size();
      message.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, size);
      message.setFieldInt32(NXCPCodes.VID_NUM_COLUMNS, numberOfColumns);
      final Set<Map.Entry<String, ReportParameter>> entries = parameters.entrySet();
      long index = NXCPCodes.VID_ROW_DATA_BASE;
      for(Map.Entry<String, ReportParameter> entry : entries)
      {
         final ReportParameter parameter = entry.getValue();

         // 0
         message.setFieldInt32(index++, parameter.getIndex());
         // 1
         message.setField(index++, parameter.getName());
         // 2
         message.setField(index++, parameter.getDescription());
         // 3
         message.setField(index++, parameter.getType());
         // 4
         message.setField(index++, parameter.getDefaultValue());
         // 5
         message.setField(index++, parameter.getDependsOn());
         // 6
         message.setFieldInt32(index++, parameter.getSpan());
         // 7-9
         index += 3;
      }
   }

   @Override
   public String toString()
   {
      return "ReportDefinition{" + "name='" + name + '\'' + ", numberOfColumns=" + numberOfColumns + ", parameters=" + parameters
            + '}';
   }
}
