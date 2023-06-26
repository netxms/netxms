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
package org.netxms.nxmc.modules.objects.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCSession;
import org.netxms.client.topology.ArpCacheEntry;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.ArpCacheView;
import org.netxms.nxmc.tools.ComparatorHelper;

/**
 * Comparator for ARP cache records
 */
public class ArpCacheComparator extends ViewerComparator
{
   private NXCSession session = Registry.getSession();

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      ArpCacheEntry arp1 = (ArpCacheEntry)e1;
      ArpCacheEntry arp2 = (ArpCacheEntry)e2;

      int result;
      switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"))
      {
         case ArpCacheView.COLUMN_INTERFACE:
            result = arp1.getInterfaceName().compareToIgnoreCase(arp2.getInterfaceName());
            break;
         case ArpCacheView.COLUMN_IP_ADDRESS:
            result = ComparatorHelper.compareInetAddresses(arp1.getIpAddress(), arp2.getIpAddress());
            break;
         case ArpCacheView.COLUMN_MAC_ADDRESS:
            result = arp1.getMacAddress().compareTo(arp2.getMacAddress());
            break;
         case ArpCacheView.COLUMN_NODE:
            String n1 = (arp1.getNodeId() != 0) ? session.getObjectName(arp1.getNodeId()) : "";
            String n2 = (arp2.getNodeId() != 0) ? session.getObjectName(arp2.getNodeId()) : "";
            result = n1.compareToIgnoreCase(n2);
            break;
         case ArpCacheView.COLUMN_VENDOR:
            result = getVendor(arp1).compareTo(getVendor(arp2));
            break;
         default:
            result = 0;
            break;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }

   /**
    * Get NIC vendor
    *
    * @param ws FDB entry
    * @return NIC vendor
    */
   private String getVendor(ArpCacheEntry entry)
   {
      String vendor = session.getVendorByMac(entry.getMacAddress(), null);
      return (vendor != null) ? vendor : "";
   }
}
