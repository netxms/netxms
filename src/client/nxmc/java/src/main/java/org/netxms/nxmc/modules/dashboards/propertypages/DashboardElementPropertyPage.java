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
package org.netxms.nxmc.modules.dashboards.propertypages;

import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;

/**
 * Base class for all property pages for dashboard elements
 */
public abstract class DashboardElementPropertyPage extends PropertyPage
{
   protected DashboardElementConfig elementConfig;

   /**
    * @param title
    */
   public DashboardElementPropertyPage(String title, DashboardElementConfig elementConfig)
   {
      super(title);
      this.elementConfig = elementConfig;
   }

   /**
    * Check if this page should be visible for current object.
    *
    * @return true if this page should be visible
    */
   public boolean isVisible()
   {
      return true;
   }

   /**
    * Get page priority. Default is 65535.
    *
    * @return page priority
    */
   public int getPriority()
   {
      return 65535;
   }

   /**
    * Get page ID.
    *
    * @return page ID
    */
   public abstract String getId();
}
