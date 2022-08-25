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
package org.netxms.nxmc.modules.dashboards.views;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.modules.dashboards.widgets.DashboardControl;
import org.netxms.nxmc.modules.objects.views.ObjectView;

/**
 * Base class for different dashboard views
 */
public abstract class AbstractDashboardView extends ObjectView
{
   protected Composite viewArea;
   protected DashboardControl dbc;

   /**
    * Create view.
    *
    * @param name view name
    * @param image view image
    * @param id view ID
    */
   public AbstractDashboardView(String name, ImageDescriptor image, String id)
   {
      super(name, image, id, false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      viewArea = parent;
   }
}
