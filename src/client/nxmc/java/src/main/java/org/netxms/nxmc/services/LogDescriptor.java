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
package org.netxms.nxmc.services;

import org.netxms.client.NXCSession;
import org.netxms.client.constants.ColumnFilterSetOperation;
import org.netxms.client.constants.ColumnFilterType;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.log.LogFilter;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.modules.logviewer.views.LogViewer;
import org.netxms.nxmc.modules.logviewer.views.ObjectLogViewer;

/**
 * Server log descriptor
 */
public class LogDescriptor
{
   protected String logName;
   protected String viewTitle;
   protected String menuItemText;
   protected String filterColumn;

   /**
    * Create new log descriptor.
    *
    * @param logName log name
    * @param viewTitle view title
    * @param menuItemText text of context menu item (if null teh view title will be used)
    * @param filterColumn filter column or null if this log should not be shown in object context
    */
   public LogDescriptor(String logName, String viewTitle, String menuItemText, String filterColumn)
   {
      this.logName = logName;
      this.viewTitle = viewTitle;
      this.menuItemText = (menuItemText != null) ? menuItemText : viewTitle;
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
      if (filterColumn == null)
         return null;
      
      ColumnFilter cf = new ColumnFilter();
      cf.setOperation(ColumnFilterSetOperation.OR);
      if ((object instanceof DataCollectionTarget) || (object instanceof BusinessService))
      {
         cf.addSubFilter(new ColumnFilter(ColumnFilterType.EQUALS, object.getObjectId()));
      }
      else
         cf.addSubFilter(new ColumnFilter(ColumnFilterType.CHILDOF, object.getObjectId()));
         
      if (object instanceof Collector)
         cf.addSubFilter(new ColumnFilter(ColumnFilterType.CHILDOF, object.getObjectId()));

      LogFilter filter = new LogFilter();
      filter.setColumnFilter(filterColumn, cf);
      return filter;
   }

   /**
    * Create global view for this element
    *
    * @return global view
    */
   public LogViewer createView()
   {
      return new LogViewer(viewTitle, logName, "");
   }

   /**
    * Create context view for this element
    *
    * @return context view
    */
   public ObjectLogViewer createContextView(AbstractObject object, long contextId)
   {
      return new ObjectLogViewer(this, object, contextId);
   }

   /**
    * If is applicable for selected object
    * 
    * @param object object to check 
    * @return true if log is applicable for object
    */
   public boolean isApplicableForObject(AbstractObject object)
   {
      return (filterColumn != null) && ((object instanceof DataCollectionTarget) || ObjectTool.isContainerObject(object));
   }

   /**
    * Get log name
    *
    * @return log name
    */
   public String getLogName()
   {
      return logName;
   }

   /**
    * Get view title for this log
    *
    * @return view title for this log
    */
   public String getViewTitle()
   {
      return viewTitle;
   }

   /**
    * Get context menu item text for this log.
    *
    * @return the menu item text
    */
   public String getMenuItemText()
   {
      return menuItemText;
   }
}
