/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2022 RadenSolutions
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
package org.netxms.nxmc.modules.dashboards.config;

import java.util.Map;
import java.util.Set;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.ObjectIdMatchingData;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * Configuration for file monitor
 */
@Root(name = "element", strict = false)
public class FileMonitorConfig extends DashboardElementConfig
{
   @Element(required = true)
   private long objectId = 0;

   @Element(required = false)
   private String fileName = "";

   @Element(required = false)
   private int historyLimit = 1000;

   @Element(required = false)
   private String filter = null;

   @Element(required = false)
   private String syntaxHighlighter = null;

   /**
    * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#getObjects()
    */
   @Override
   public Set<Long> getObjects()
   {
      Set<Long> objects = super.getObjects();
      objects.add(objectId);
      return objects;
   }

   /**
    * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#remapObjects(java.util.Map)
    */
   @Override
   public void remapObjects(Map<Long, ObjectIdMatchingData> remapData)
   {
      super.remapObjects(remapData);
      ObjectIdMatchingData md = remapData.get(objectId);
      if (md != null)
         objectId = md.dstId;
   }

   /**
    * @return the objectId
    */
   public long getObjectId()
   {
      return objectId;
   }

   /**
    * @param objectId the objectId to set
    */
   public void setObjectId(long objectId)
   {
      this.objectId = objectId;
   }

   /**
    * @return the fileName
    */
   public String getFileName()
   {
      return fileName;
   }

   /**
    * @param fileName the fileName to set
    */
   public void setFileName(String fileName)
   {
      this.fileName = fileName;
   }

   /**
    * @return the historyLimit
    */
   public int getHistoryLimit()
   {
      return historyLimit;
   }

   /**
    * @param historyLimit the historyLimit to set
    */
   public void setHistoryLimit(int historyLimit)
   {
      this.historyLimit = historyLimit;
   }

   /**
    * @return the filter
    */
   public String getFilter()
   {
      return (filter != null) ? filter : "";
   }

   /**
    * @param filter the filter to set
    */
   public void setFilter(String filter)
   {
      this.filter = ((filter == null) || filter.isBlank()) ? null : filter;
   }

   /**
    * @return the syntaxHighlighter
    */
   public String getSyntaxHighlighter()
   {
      return (syntaxHighlighter != null) ? syntaxHighlighter : "";
   }

   /**
    * @param syntaxHighlighter the syntaxHighlighter to set
    */
   public void setSyntaxHighlighter(String syntaxHighlighter)
   {
      this.syntaxHighlighter = ((syntaxHighlighter == null) || syntaxHighlighter.isBlank()) ? null : syntaxHighlighter;
   }
}
