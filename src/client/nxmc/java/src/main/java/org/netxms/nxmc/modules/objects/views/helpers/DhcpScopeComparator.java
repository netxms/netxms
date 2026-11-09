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

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.dhcp.DhcpScope;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.DhcpView;
import org.netxms.nxmc.tools.ComparatorHelper;

/**
 * Comparator for DHCP scope list
 */
public class DhcpScopeComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      DhcpScope s1 = (DhcpScope)e1;
      DhcpScope s2 = (DhcpScope)e2;

      int result;
      switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"))
      {
         case DhcpView.SCOPE_NETWORK:
            result = ComparatorHelper.compareInetAddresses((s1.getNetwork() != null) ? s1.getNetwork().getAddress() : null, (s2.getNetwork() != null) ? s2.getNetwork().getAddress() : null);
            if (result == 0)
               result = s1.getId().compareToIgnoreCase(s2.getId());
            break;
         case DhcpView.SCOPE_NAME:
            result = s1.getName().compareToIgnoreCase(s2.getName());
            break;
         case DhcpView.SCOPE_RANGES:
            result = s1.getRanges().compareToIgnoreCase(s2.getRanges());
            break;
         case DhcpView.SCOPE_TOTAL:
            result = Long.compareUnsigned(s1.getTotalAddresses(), s2.getTotalAddresses());
            break;
         case DhcpView.SCOPE_USED:
            result = Long.compareUnsigned(s1.getUsedAddresses(), s2.getUsedAddresses());
            break;
         case DhcpView.SCOPE_DECLINED:
            result = Long.compareUnsigned(s1.getDeclinedAddresses(), s2.getDeclinedAddresses());
            break;
         case DhcpView.SCOPE_FREE:
            result = Long.compareUnsigned(s1.getFreeAddresses(), s2.getFreeAddresses());
            break;
         case DhcpView.SCOPE_UTILIZATION:
            result = Double.compare(s1.getUtilization(), s2.getUtilization());
            break;
         default:
            result = 0;
            break;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
