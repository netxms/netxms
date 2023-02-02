/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.widgets.helpers;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.netxms.client.NXCSession;
import org.netxms.client.TableRow;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.queries.ObjectQueryResult;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.helpers.TransformationSelectionProvider;

/**
 * Selection provider that transforms summary table row selection into object selection
 */
public class ObjectSelectionProvider extends TransformationSelectionProvider
{
   private NXCSession session = Registry.getSession();

   /**
    * @param parent
    */
   public ObjectSelectionProvider(ISelectionProvider parent)
   {
      super(parent);
   }

   /**
    * @see org.netxms.ui.eclipse.dashboard.widgets.helpers.ProxySelectionProvider#transformSelection(org.eclipse.jface.viewers.ISelection)
    */
   @Override
   protected ISelection transformSelection(ISelection selection)
   {
      if ((selection == null) || selection.isEmpty() || !(selection instanceof IStructuredSelection))
         return selection;

      List<AbstractObject> objects = new ArrayList<AbstractObject>(((IStructuredSelection)selection).size());
      for(Object o : ((IStructuredSelection)selection).toList())
      {
         if (o instanceof ObjectQueryResult)
         {
            objects.add(((ObjectQueryResult)o).getObject());
         }
         else if (o instanceof TableRow)
         {
            AbstractObject object = session.findObjectById(((TableRow)o).getObjectId());
            if (object != null)
               objects.add(object);
         }
      }

      return new StructuredSelection(objects);
   }
}
