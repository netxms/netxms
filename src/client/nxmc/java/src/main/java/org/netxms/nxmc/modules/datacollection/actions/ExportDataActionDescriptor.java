/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.actions;

import org.eclipse.jface.viewers.ISelectionProvider;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.modules.objects.actions.ObjectAction;
import org.netxms.nxmc.services.ObjectActionDescriptor;

/**
 * Descriptor for "Export data..." object action
 */
public class ExportDataActionDescriptor implements ObjectActionDescriptor
{
   /**
    * @see org.netxms.nxmc.services.ObjectActionDescriptor#createAction(org.netxms.nxmc.base.views.ViewPlacement, org.eclipse.jface.viewers.ISelectionProvider)
    */
   @Override
   public ObjectAction<?> createAction(ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      return new ExportDataAction(viewPlacement, selectionProvider);
   }

   /**
    * @see org.netxms.nxmc.services.ObjectActionDescriptor#getRequiredComponentId()
    */
   @Override
   public String getRequiredComponentId()
   {
      return null;
   }

   /**
    * @see org.netxms.nxmc.services.ObjectActionDescriptor#showInToolbar()
    */
   @Override
   public boolean showInToolbar()
   {
      return false;
   }
}
