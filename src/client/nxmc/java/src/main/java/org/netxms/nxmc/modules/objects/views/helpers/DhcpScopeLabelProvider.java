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

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.dhcp.DhcpScope;
import org.netxms.nxmc.modules.objects.views.DhcpView;

/**
 * Label provider for DHCP scope list
 */
public class DhcpScopeLabelProvider extends LabelProvider implements ITableLabelProvider
{
   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      DhcpScope scope = (DhcpScope)element;
      switch(columnIndex)
      {
         case DhcpView.SCOPE_NETWORK:
            return ((scope.getNetwork() != null) && scope.getNetwork().isValidAddress()) ? scope.getNetwork().toString() : scope.getId();
         case DhcpView.SCOPE_NAME:
            return scope.getName();
         case DhcpView.SCOPE_RANGES:
            return scope.getRanges();
         case DhcpView.SCOPE_TOTAL:
            return Long.toUnsignedString(scope.getTotalAddresses());
         case DhcpView.SCOPE_USED:
            return Long.toUnsignedString(scope.getUsedAddresses());
         case DhcpView.SCOPE_DECLINED:
            return scope.isDeclinedAddressesKnown() ? Long.toUnsignedString(scope.getDeclinedAddresses()) : "";
         case DhcpView.SCOPE_FREE:
            return Long.toUnsignedString(scope.getFreeAddresses());
         case DhcpView.SCOPE_UTILIZATION:
            return String.format("%.1f%%", scope.getUtilization());
      }
      return null;
   }
}
