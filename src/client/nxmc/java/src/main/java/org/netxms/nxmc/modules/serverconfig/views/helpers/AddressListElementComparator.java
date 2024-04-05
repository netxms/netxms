/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.serverconfig.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.InetAddressListElement;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.views.NetworkDiscoveryConfigurator;
import org.netxms.nxmc.tools.ComparatorHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Comparator for address list elements
 */
public class AddressListElementComparator extends ViewerComparator
{
   private I18n i18n = LocalizationHelper.getI18n(AddressListElementComparator.class);
   
   private NXCSession session = Registry.getSession();
   private boolean isDiscoveryTarget;
   
   public AddressListElementComparator(boolean isDiscoveryTarget)
   {
      this.isDiscoveryTarget = isDiscoveryTarget; 
   }
   
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		InetAddressListElement a1 = (InetAddressListElement)e1;
		InetAddressListElement a2 = (InetAddressListElement)e2;

      int result;
      switch((Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
      {
         case NetworkDiscoveryConfigurator.RANGE:
            result = ComparatorHelper.compareInetAddresses(a1.getBaseAddress(), a2.getBaseAddress());
            if (result == 0)
            {
               result = a1.getType() - a2.getType();
               if (result == 0)
               {
                  result = (a1.getType() == InetAddressListElement.SUBNET) ? a1.getMaskBits() - a2.getMaskBits() : ComparatorHelper.compareInetAddresses(a1.getEndAddress(), a2.getEndAddress());
               }
            }
            break;
         case NetworkDiscoveryConfigurator.ZONE:
            if(isDiscoveryTarget)
            {
               String name1 = session.getZoneName(a1.getZoneUIN());
               String name2 = session.getZoneName(a2.getZoneUIN());
               result = name1.compareTo(name2);
            }
            else
               result = a1.getComment().compareTo(a2.getComment());
            break;
         case NetworkDiscoveryConfigurator.PROXY:
            String name1 = (a1.getProxyId() != 0) ? session.getObjectName(a1.getProxyId()) : i18n.tr("Zone proxy");
            String name2 = (a2.getProxyId() != 0) ? session.getObjectName(a2.getProxyId()) : i18n.tr("Zone proxy");
            result = name1.compareTo(name2);
            break;
         case NetworkDiscoveryConfigurator.COMMENTS:
            result = a1.getComment().compareTo(a2.getComment());
            break;
         default:
            result = 0;
            break;
      }
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
