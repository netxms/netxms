/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.nxmc.base.views;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.ISelectionProvider;

/**
 * Base class for all navigation views. Those views intended for placement in navigation area within perspective.
 */
public abstract class NavigationView extends View
{
   /**
    * Create navigation view with random ID.
    *
    * @param name view name
    * @param image view image
    */
   public NavigationView(String name, ImageDescriptor image)
   {
      super(name, image);
   }

   /**
    * Create navigation view with specific ID.
    *
    * @param name view name
    * @param image view image
    * @param id view ID
    */
   public NavigationView(String name, ImageDescriptor image, String id)
   {
      super(name, image, id);
   }

   /**
    * Get selection provider for navigation selection.
    *
    * @return selection provider
    */
   public abstract ISelectionProvider getSelectionProvider();
}
