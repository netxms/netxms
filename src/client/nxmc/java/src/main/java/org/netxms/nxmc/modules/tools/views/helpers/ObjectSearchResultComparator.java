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
package org.netxms.nxmc.modules.tools.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.base.InetAddressEx;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.queries.ObjectQueryResult;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.tools.views.ObjectFinder;
import org.netxms.nxmc.tools.ComparatorHelper;

/**
 * Comparator for object search results
 */
public class ObjectSearchResultComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      TableColumn sortColumn = ((SortableTableViewer)viewer).getTable().getSortColumn();
      if (sortColumn == null)
         return 0;

      final int column = (Integer)sortColumn.getData("ID"); //$NON-NLS-1$

      final AbstractObject object1 = (e1 instanceof ObjectQueryResult) ? ((ObjectQueryResult)e1).getObject() : (AbstractObject)e1;
      final AbstractObject object2 = (e2 instanceof ObjectQueryResult) ? ((ObjectQueryResult)e2).getObject() : (AbstractObject)e2;

      int result;
      switch(column)
      {
         case ObjectFinder.COL_ID:
            result = Long.signum(object1.getObjectId() - object2.getObjectId());
            break;
         case ObjectFinder.COL_IP_ADDRESS:
            InetAddressEx a1 = getIpAddress(object1);
            InetAddressEx a2 = getIpAddress(object2);
            result = ComparatorHelper.compareInetAddresses(a1.getAddress(), a2.getAddress());
            break;
         case ObjectFinder.COL_NAME:
            result = object1.getObjectName().compareToIgnoreCase(object2.getObjectName());
            break;
         default:
            String t1 = ((ITableLabelProvider)((SortableTableViewer)viewer).getLabelProvider()).getColumnText(object1, column);
            String t2 = ((ITableLabelProvider)((SortableTableViewer)viewer).getLabelProvider()).getColumnText(object2, column);
            result = t1.compareToIgnoreCase(t2);
            break;
      }

      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
   
   /**
    * @param object
    * @return
    */
   private static InetAddressEx getIpAddress(AbstractObject object)
   {
      InetAddressEx addr = null;
      if (object instanceof AbstractNode)
         addr = ((AbstractNode)object).getPrimaryIP();
      else if (object instanceof AccessPoint)
         addr = ((AccessPoint)object).getIpAddress();
      else if (object instanceof Interface)
         addr = ((Interface)object).getFirstUnicastAddressEx();
      return (addr != null) ? addr : new InetAddressEx();
   }
}
