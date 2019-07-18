/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
import org.netxms.client.objects.Interface;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.InterfacesView;
import org.netxms.nxmc.tools.ComparatorHelper;

/**
 * Comparator for interface list
 */
public class InterfaceListComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		final Interface iface1 = (Interface)e1;
		final Interface iface2 = (Interface)e2;
      final int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID");
		
		int result;
		switch(column)
		{
         case InterfacesView.COLUMN_8021X_BACKEND_STATE:
				result = iface1.getDot1xBackendState() - iface2.getDot1xBackendState();
				break;
         case InterfacesView.COLUMN_8021X_PAE_STATE:
				result = iface1.getDot1xPaeState() - iface2.getDot1xPaeState();
				break;
         case InterfacesView.COLUMN_ADMIN_STATE:
				result = iface1.getAdminState() - iface2.getAdminState();
				break;
         case InterfacesView.COLUMN_ALIAS:
            result = ComparatorHelper.compareStringsNatural(iface1.getAlias(), iface2.getAlias());
            break;
         case InterfacesView.COLUMN_DESCRIPTION:
            result = ComparatorHelper.compareStringsNatural(iface1.getDescription(), iface2.getDescription());
				break;
         case InterfacesView.COLUMN_EXPECTED_STATE:
				result = iface1.getExpectedState() - iface2.getExpectedState();
				break;
         case InterfacesView.COLUMN_ID:
				result = Long.signum(iface1.getObjectId() - iface2.getObjectId());
				break;
         case InterfacesView.COLUMN_INDEX:
				result = iface1.getIfIndex() - iface2.getIfIndex();
				break;
         case InterfacesView.COLUMN_MTU:
            result = iface1.getMtu() - iface2.getMtu();
            break;
         case InterfacesView.COLUMN_NAME:
            result = ComparatorHelper.compareStringsNatural(iface1.getObjectName(), iface2.getObjectName());
				break;
         case InterfacesView.COLUMN_OPER_STATE:
				result = iface1.getOperState() - iface2.getOperState();
				break;
         case InterfacesView.COLUMN_PHYSICAL_LOCATION:
			   if (iface1.isPhysicalPort() && iface2.isPhysicalPort())
			   {
	            result = iface1.getChassis() - iface2.getChassis();
	            if (result == 0)
	               result = iface1.getModule() - iface2.getModule();
	            if (result == 0)
	               result = iface1.getPIC() - iface2.getPIC();
	            if (result == 0)
	               result = iface1.getPort() - iface2.getPort();
			   }
			   else if (iface1.isPhysicalPort())
			   {
			      result = 1;
			   }
            else if (iface2.isPhysicalPort())
            {
               result = -1;
            }
            else
            {
               result = 0;
            }
				break;
         case InterfacesView.COLUMN_SPEED:
            result = Long.signum(iface1.getSpeed() - iface2.getSpeed());
            break;
         case InterfacesView.COLUMN_STATUS:
				result = iface1.getStatus().compareTo(iface2.getStatus());
				break;
         case InterfacesView.COLUMN_TYPE:
				result = iface1.getIfType() - iface2.getIfType();
				break;
         case InterfacesView.COLUMN_MAC_ADDRESS:
				result = iface1.getMacAddress().compareTo(iface2.getMacAddress());
				break;
         case InterfacesView.COLUMN_IP_ADDRESS:
				result = ComparatorHelper.compareInetAddresses(iface1.getFirstUnicastAddress(), iface2.getFirstUnicastAddress());
				break;
			default:
				result = 0;
				break;
		}
		
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
