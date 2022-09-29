/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.PhysicalLink;
import org.netxms.nxmc.base.widgets.SortableTableViewer;

/**
 * Physical link comparator
 */
public class PhysicalLinkComparator extends ViewerComparator
{
   private PhysicalLinkLabelProvider labelProvider;

   /**
    * Create physical link comparator.
    *
    * @param labelProvider label provider for physical links
    */
   public PhysicalLinkComparator(PhysicalLinkLabelProvider labelProvider)
   {
      this.labelProvider = labelProvider;
   }

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      PhysicalLink link1 = (PhysicalLink)e1;
      PhysicalLink link2 = (PhysicalLink)e2;
      final int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$

      int result;
      switch(column)
      {
         case PhysicalLinkManager.COL_PHYSICAL_LINK_ID:
            result = Long.compare(link1.getId(), link2.getId());
            break;
         case PhysicalLinkManager.COL_DESCRIPTOIN:
            result = link1.getDescription().compareToIgnoreCase(link2.getDescription());
            break;
         case PhysicalLinkManager.COL_LEFT_OBJECT:
         case PhysicalLinkManager.COL_RIGHT_OBJECT:
            result = labelProvider.getObjectText(link1, column == PhysicalLinkManager.COL_LEFT_OBJECT).compareToIgnoreCase(labelProvider.getObjectText(link2, column == PhysicalLinkManager.COL_LEFT_OBJECT));
            break;
         case PhysicalLinkManager.COL_LEFT_PORT:
         case PhysicalLinkManager.COL_RIGHT_PORT:
            result = labelProvider.getPortText(link1, column == PhysicalLinkManager.COL_LEFT_OBJECT).compareToIgnoreCase(labelProvider.getPortText(link2, column == PhysicalLinkManager.COL_LEFT_OBJECT));
            break;
         default:
            result = 0;
            break;
      }

      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
