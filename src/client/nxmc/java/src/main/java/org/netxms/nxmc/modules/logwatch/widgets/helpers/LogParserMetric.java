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
package org.netxms.nxmc.modules.logwatch.widgets.helpers;

import org.netxms.nxmc.modules.logwatch.widgets.LogParserMetricEditor;
import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Text;

/**
 * Log parser push metric
 */
@Root(name = "metric", strict = false)
public class LogParserMetric
{
   @Text(required = false)
   private String metric = "";

   @Attribute(required = true)
   private Integer group = 1;

   @Attribute(required = true)
   private boolean push = false;

   private LogParserMetricEditor editor;

   /**
    * @return the group
    */
   public Integer getGroup()
   {
      return group;
   }

   /**
    * @param group the group to set
    */
   public void setGroup(Integer group)
   {
      this.group = group;
   }

   /**
    * @return the push
    */
   public boolean isPush()
   {
      return push;
   }

   /**
    * @param push the push to set
    */
   public void setPush(boolean push)
   {
      this.push = push;
   }

   /**
    * @return the metric
    */
   public String getMetric()
   {
      return metric;
   }

   /**
    * @param metric the metric to set
    */
   public void setMetric(String metric)
   {
      this.metric = metric;
   }

   /**
    * @return the editor
    */
   public LogParserMetricEditor getEditor()
   {
      return editor;
   }

   /**
    * @param editor the editor to set
    */
   public void setEditor(LogParserMetricEditor editor)
   {
      this.editor = editor;
   }
}
