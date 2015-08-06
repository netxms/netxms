/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectview.objecttabs.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.objects.Interface;
import org.netxms.ui.eclipse.objectview.objecttabs.InterfacesTab;
import org.netxms.ui.eclipse.tools.ComparatorHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for interface list
 */
public class InterfaceListComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		final Interface iface1 = (Interface)e1;
		final Interface iface2 = (Interface)e2;
		final int column = (Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$
		
		int result;
		switch(column)
		{
			case InterfacesTab.COLUMN_8021X_BACKEND_STATE:
				result = iface1.getDot1xBackendState() - iface2.getDot1xBackendState();
				break;
			case InterfacesTab.COLUMN_8021X_PAE_STATE:
				result = iface1.getDot1xPaeState() - iface2.getDot1xPaeState();
				break;
			case InterfacesTab.COLUMN_ADMIN_STATE:
				result = iface1.getAdminState() - iface2.getAdminState();
				break;
         case InterfacesTab.COLUMN_ALIAS:
            result = iface1.getAlias().compareToIgnoreCase(iface2.getAlias());
            break;
			case InterfacesTab.COLUMN_DESCRIPTION:
				result = iface1.getDescription().compareToIgnoreCase(iface2.getDescription());
				break;
			case InterfacesTab.COLUMN_EXPECTED_STATE:
				result = iface1.getExpectedState() - iface2.getExpectedState();
				break;
			case InterfacesTab.COLUMN_ID:
				result = Long.signum(iface1.getObjectId() - iface2.getObjectId());
				break;
			case InterfacesTab.COLUMN_INDEX:
				result = iface1.getIfIndex() - iface2.getIfIndex();
				break;
         case InterfacesTab.COLUMN_MTU:
            result = iface1.getMtu() - iface2.getMtu();
            break;
			case InterfacesTab.COLUMN_NAME:
				result = iface1.getObjectName().compareToIgnoreCase(iface2.getObjectName());
				break;
			case InterfacesTab.COLUMN_OPER_STATE:
				result = iface1.getOperState() - iface2.getOperState();
				break;
			case InterfacesTab.COLUMN_PORT:
				result = iface1.getPort() - iface2.getPort();
				break;
			case InterfacesTab.COLUMN_SLOT:
				result = iface1.getSlot() - iface2.getSlot();
				break;
         case InterfacesTab.COLUMN_SPEED:
            result = Long.signum(iface1.getSpeed() - iface2.getSpeed());
            break;
			case InterfacesTab.COLUMN_STATUS:
				result = iface1.getStatus().compareTo(iface2.getStatus());
				break;
			case InterfacesTab.COLUMN_TYPE:
				result = iface1.getIfType() - iface2.getIfType();
				break;
			case InterfacesTab.COLUMN_MAC_ADDRESS:
				result = iface1.getMacAddress().compareTo(iface2.getMacAddress());
				break;
			case InterfacesTab.COLUMN_IP_ADDRESS:
				result = ComparatorHelper.compareInetAddresses(iface1.getFirstUnicastAddress(), iface2.getFirstUnicastAddress());
				break;
			default:
				result = 0;
				break;
		}
		
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
