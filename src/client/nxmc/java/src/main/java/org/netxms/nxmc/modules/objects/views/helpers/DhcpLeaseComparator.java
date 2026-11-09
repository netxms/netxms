/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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

import java.util.Date;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCSession;
import org.netxms.client.dhcp.DhcpLease;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.DhcpView;
import org.netxms.nxmc.tools.ComparatorHelper;

/**
 * Comparator for DHCP lease list
 */
public class DhcpLeaseComparator extends ViewerComparator
{
   private NXCSession session = Registry.getSession();

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      DhcpLease l1 = (DhcpLease)e1;
      DhcpLease l2 = (DhcpLease)e2;

      int result;
      switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"))
      {
         case DhcpView.LEASE_IP_ADDRESS:
            result = ComparatorHelper.compareInetAddresses((l1.getIpAddress() != null) ? l1.getIpAddress().getAddress() : null, (l2.getIpAddress() != null) ? l2.getIpAddress().getAddress() : null);
            break;
         case DhcpView.LEASE_MAC_ADDRESS:
            result = getMacText(l1).compareToIgnoreCase(getMacText(l2));
            break;
         case DhcpView.LEASE_VENDOR:
            result = getVendor(l1).compareToIgnoreCase(getVendor(l2));
            break;
         case DhcpView.LEASE_HOSTNAME:
            result = l1.getHostname().compareToIgnoreCase(l2.getHostname());
            break;
         case DhcpView.LEASE_CLIENT_ID:
            result = l1.getClientId().compareToIgnoreCase(l2.getClientId());
            break;
         case DhcpView.LEASE_SCOPE:
            result = l1.getScopeId().compareToIgnoreCase(l2.getScopeId());
            break;
         case DhcpView.LEASE_TYPE:
            result = Integer.compare(l1.getType().getValue(), l2.getType().getValue());
            break;
         case DhcpView.LEASE_STATE:
            result = Integer.compare(l1.getState().getValue(), l2.getState().getValue());
            break;
         case DhcpView.LEASE_LAST_TRANSACTION:
            result = compareDates(l1.getLastTransaction(), l2.getLastTransaction());
            break;
         case DhcpView.LEASE_EXPIRATION:
            result = compareDates(l1.getExpiration(), l2.getExpiration());
            break;
         default:
            result = 0;
            break;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }

   /**
    * Compare optional dates (null is less than any date).
    */
   private static int compareDates(Date d1, Date d2)
   {
      if (d1 == null)
         return (d2 == null) ? 0 : -1;
      if (d2 == null)
         return 1;
      return d1.compareTo(d2);
   }

   /**
    * Get MAC address as text.
    */
   private static String getMacText(DhcpLease lease)
   {
      return DhcpLeaseLabelProvider.hasMacAddress(lease) ? lease.getMacAddress().toString() : "";
   }

   /**
    * Get NIC vendor.
    */
   private String getVendor(DhcpLease lease)
   {
      if (!DhcpLeaseLabelProvider.hasMacAddress(lease))
         return "";
      String vendor = session.getVendorByMac(lease.getMacAddress(), null);
      return (vendor != null) ? vendor : "";
   }
}
