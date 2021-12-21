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
import org.netxms.nxmc.base.views.helpers.NavigationHistory;

/**
 * Base class for all navigation views. Those views intended for placement in navigation area within perspective.
 */
public abstract class NavigationView extends View
{
   private NavigationHistory navigationHistory = new NavigationHistory();

   /**
    * Create navigation view with specific ID.
    *
    * @param name view name
    * @param image view image
    * @param id view ID
    * @param hasFilter true if view should contain filter
    * @param showFilterTooltip true if view filter should have tooltip icon
    * @param showFilterLabel true if view filter should contain label on the left of text input field
    */
   public NavigationView(String name, ImageDescriptor image, String id, boolean hasFilter, boolean showFilterTooltip, boolean showFilterLabel)
   {
      super(name, image, id, hasFilter, showFilterTooltip, showFilterLabel);
   }

   /**
    * Get navigation history for this view.
    *
    * @return navigation history for this view
    */
   public NavigationHistory getNavigationHistory()
   {
      return navigationHistory;
   }

   /**
    * Get selection provider for navigation selection.
    *
    * @return selection provider
    */
   public abstract ISelectionProvider getSelectionProvider();

   /**
    * Set selection in navigation view.
    *
    * @param selection new selection
    */
   public abstract void setSelection(Object selection);
}
