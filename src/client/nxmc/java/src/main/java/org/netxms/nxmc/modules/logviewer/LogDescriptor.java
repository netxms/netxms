/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.logviewer;

import org.netxms.client.NXCSession;
import org.netxms.client.constants.ColumnFilterSetOperation;
import org.netxms.client.constants.ColumnFilterType;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.log.LogFilter;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objecttools.ObjectTool;

/**
 * Server log descriptor
 */
public class LogDescriptor
{
   protected String viewName;
   protected String logName;
   protected String filterColumn;

   /**
    * Create new log descriptor.
    * 
    * @param viewName view name
    * @param logName log name
    * @param filterColumn filter column
    */
   public LogDescriptor(String viewName, String logName, String filterColumn)
   {
      this.viewName = viewName;
      this.logName = logName;
      this.filterColumn = filterColumn;
   }

   /**
    * Check if this descriptor is valid for given user session. Default implementation always returns true.
    *
    * @param session user session to check
    * @return true if this descriptor is valid
    */
   public boolean isValidForSession(NXCSession session)
   {
      return true;
   }

   /**
    * Create object filter for log view 
    * 
    * @param object object to filter by
    * 
    * @return log filter object
    */
   public LogFilter createFilter(AbstractObject object)
   {      
      ColumnFilter cf = new ColumnFilter();
      cf.setOperation(ColumnFilterSetOperation.OR);
      cf.addSubFilter(new ColumnFilter((object instanceof AbstractNode) ? ColumnFilterType.EQUALS : ColumnFilterType.CHILDOF, object.getObjectId()));

      LogFilter filter = new LogFilter();
      filter.setColumnFilter(filterColumn, cf);
      return filter;
   }

   /**
    * If is applicable for selected object
    * 
    * @param object object to check 
    * @return true if log is applicable for object
    */
   public boolean isApplicableForObject(AbstractObject object)
   {
      return (object instanceof DataCollectionTarget) || ObjectTool.isContainerObject(object);
   }

   /**
    * @return the logName
    */
   public String getLogName()
   {
      return logName;
   }

   /**
    * @return the viewName
    */
   public String getViewName()
   {
      return viewName;
   }   
}
