/**
 * NetXMS - open source network management system
 * Copyright (C) 2022 Raden Solutions
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
import org.netxms.client.NXCSession;
import org.netxms.client.topology.OSPFNeighbor;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.OSPFView;
import org.netxms.nxmc.tools.ComparatorHelper;

/**
 * Comparator for OSPF neighbors
 */
public class OSPFNeighborComparator extends ViewerComparator
{
   final NXCSession session = Registry.getSession();

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      OSPFNeighbor n1 = (OSPFNeighbor)e1;
      OSPFNeighbor n2 = (OSPFNeighbor)e2;
      final int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID");

      int result;
      switch(column)
      {
         case OSPFView.COLUMN_NEIGHBOR_AREA_ID:
            result = ComparatorHelper.compareInetAddresses(n1.getAreaId(), n2.getAreaId());
            break;
         case OSPFView.COLUMN_NEIGHBOR_IF_INDEX:
            result = Integer.compareUnsigned(n1.getInterfaceIndex(), n2.getInterfaceIndex());
            break;
         case OSPFView.COLUMN_NEIGHBOR_INTERFACE:
            result = ComparatorHelper.compareStringsNatural(n1.isVirtual() ? "" : session.getObjectName(n1.getInterfaceId()), n2.isVirtual() ? "" : session.getObjectName(n2.getInterfaceId()));
            break;
         case OSPFView.COLUMN_NEIGHBOR_IP_ADDRESS:
            result = ComparatorHelper.compareInetAddresses(n1.getIpAddress(), n2.getIpAddress());
            break;
         case OSPFView.COLUMN_NEIGHBOR_NODE:
            result = ComparatorHelper.compareStringsNatural((n1.getNodeId() != 0) ? session.getObjectName(n1.getNodeId()) : "", (n2.getNodeId() != 0) ? session.getObjectName(n2.getNodeId()) : "");
            break;
         case OSPFView.COLUMN_NEIGHBOR_ROUTER_ID:
            result = ComparatorHelper.compareInetAddresses(n1.getRouterId(), n2.getRouterId());
            break;
         case OSPFView.COLUMN_NEIGHBOR_STATE:
            result = n1.getState().getValue() - n2.getState().getValue();
            break;
         case OSPFView.COLUMN_NEIGHBOR_VIRTUAL:
            result = Boolean.compare(n1.isVirtual(), n2.isVirtual());
            break;
         default:
            result = 0;
            break;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
