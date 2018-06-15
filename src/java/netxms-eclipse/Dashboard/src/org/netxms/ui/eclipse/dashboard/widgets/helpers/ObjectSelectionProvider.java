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
package org.netxms.ui.eclipse.dashboard.widgets.helpers;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.netxms.client.ObjectQueryResult;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.tools.TransformationSelectionProvider;

/**
 * Selection provider that transforms summary table row selection into object selection
 */
public class ObjectSelectionProvider extends TransformationSelectionProvider
{
   /**
    * @param parent
    */
   public ObjectSelectionProvider(ISelectionProvider parent)
   {
      super(parent);
   }

   /* (non-Javadoc)
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
            objects.add(((ObjectQueryResult)o).getObject());
      }
      
      return new StructuredSelection(objects);
   }
}
