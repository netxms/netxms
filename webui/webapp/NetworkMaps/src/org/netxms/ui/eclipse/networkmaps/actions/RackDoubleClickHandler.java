/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.networkmaps.actions;

import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Rack;
import org.netxms.ui.eclipse.networkmaps.api.ObjectDoubleClickHandler;
import org.netxms.ui.eclipse.objectview.views.RackView;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Double click handler for rack objects
 */
public class RackDoubleClickHandler implements ObjectDoubleClickHandler
{
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.networkmaps.api.ObjectDoubleClickHandler#onDoubleClick(org.netxms.client.objects.AbstractObject, org.eclipse.ui.IViewPart)
    */
   @Override
   public boolean onDoubleClick(AbstractObject object, IViewPart viewPart)
   {
      if (!(object instanceof Rack))         
         return false;
      
      IWorkbenchPage page = (viewPart != null) ?
            viewPart.getSite().getPage() :
            PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage();
      try
      {
         page.showView(RackView.ID, Long.toString(object.getObjectId()), IWorkbenchPage.VIEW_ACTIVATE);
      }
      catch(PartInitException e)
      {
         MessageDialogHelper.openError(page.getActivePart().getSite().getShell(), "Error", String.format("Could not open rack view: %s", e.getLocalizedMessage()));
      }
      return true;
   }
}
