/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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

import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * Configuration for scripted pie chart
 */
@Root(name = "element", strict = false)
public class ScriptedPieChartConfig extends ScriptedComparisonChartConfig
{
   @Element(required = false)
   private boolean doughnutRendering = true;

   @Element(required = false)
   private boolean showTotal = false;

   /**
    * @return the doughnutRendering
    */
   public boolean isDoughnutRendering()
   {
      return doughnutRendering;
   }

   /**
    * @param doughnutRendering the doughnutRendering to set
    */
   public void setDoughnutRendering(boolean doughnutRendering)
   {
      this.doughnutRendering = doughnutRendering;
   }

   /**
    * @return the showTotal
    */
   public boolean isShowTotal()
   {
      return showTotal;
   }

   /**
    * @param showTotal the showTotal to set
    */
   public void setShowTotal(boolean showTotal)
   {
      this.showTotal = showTotal;
   }
}
