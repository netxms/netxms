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
package org.netxms.nxmc.services;

import org.eclipse.jface.viewers.ISelectionProvider;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.modules.objects.actions.ObjectAction;

/**
 * Descriptor for object action (will be shown as menu item in object context menu and optionally on object toolbar)
 */
public interface ObjectActionDescriptor
{
   /**
    * Create action for given selection provider.
    *
    * @param viewPlacement view placement information
    * @param selectionProvider selection provider
    * @return action
    */
   public ObjectAction<?> createAction(ViewPlacement viewPlacement, ISelectionProvider selectionProvider);

   /**
    * Get ID of server component that is required for this view.
    *
    * @return ID of required component or null if there are no restrictions
    */
   public String getRequiredComponentId();

   /**
    * Check if action should be shown in object toolbar.
    *
    * @return true if action should be shown in object toolbar
    */
   public boolean showInToolbar();
}
