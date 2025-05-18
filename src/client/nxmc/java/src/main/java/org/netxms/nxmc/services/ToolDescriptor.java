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
package org.netxms.nxmc.services;

import org.eclipse.jface.resource.ImageDescriptor;
import org.netxms.nxmc.base.views.View;

/**
 * Element for tools perspective
 */
public interface ToolDescriptor
{
   /**
    * Get element name.
    *
    * @return element name
    */
   public String getName();

   /**
    * Get image for this element.
    *
    * @return image for this element
    */
   public ImageDescriptor getImage();

   /**
    * Create view for this element
    *
    * @return view
    */
   public View createView();

   /**
    * Get ID of server component that is required for this view.
    *
    * @return ID of required component or null if there are no restrictions
    */
   public String getRequiredComponentId();
}
