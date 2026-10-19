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

import java.util.List;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.views.DataExportView;
import org.netxms.nxmc.modules.objects.actions.ObjectAction;

/**
 * Open data export view with selected spatial object (facility, room, power domain, cooling zone, rack) as root
 */
public class ExportDataAction extends ObjectAction<AbstractObject>
{
   private final ViewPlacement viewPlacement;

   /**
    * Create action
    *
    * @param viewPlacement view placement information
    * @param selectionProvider associated selection provider
    */
   public ExportDataAction(ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      super(AbstractObject.class, LocalizationHelper.getI18n(ExportDataAction.class).tr("Export &data..."), viewPlacement, selectionProvider);
      this.viewPlacement = viewPlacement;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#isValidForSelection(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   public boolean isValidForSelection(IStructuredSelection selection)
   {
      if (selection.size() != 1)
         return false;
      Object element = selection.getFirstElement();
      if (!(element instanceof AbstractObject))
         return false;
      switch(((AbstractObject)element).getObjectClass())
      {
         case AbstractObject.OBJECT_FACILITY:
         case AbstractObject.OBJECT_POWERDOMAIN:
         case AbstractObject.OBJECT_COOLINGZONE:
         case AbstractObject.OBJECT_ROOM:
         case AbstractObject.OBJECT_RACK:
            return true;
         default:
            return false;
      }
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#run(java.util.List)
    */
   @Override
   protected void run(List<AbstractObject> objects)
   {
      viewPlacement.openView(new DataExportView(objects.get(0).getObjectId()));
   }
}
