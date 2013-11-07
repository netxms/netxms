/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.topology.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.topology.RadioInterface;
import org.netxms.ui.eclipse.topology.views.RadioInterfaces;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for radio interface list
 */
public class RadioInterfaceComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		RadioInterface rif1 = (RadioInterface)e1;
		RadioInterface rif2 = (RadioInterface)e2;
		
		int result;
		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case RadioInterfaces.COLUMN_AP_MAC_ADDR:
				result = rif1.getAccessPoint().getMacAddress().compareTo(rif2.getAccessPoint().getMacAddress());
				break;
			case RadioInterfaces.COLUMN_AP_MODEL:
				result = rif1.getAccessPoint().getModel().compareToIgnoreCase(rif2.getAccessPoint().getModel());
				break;
			case RadioInterfaces.COLUMN_AP_NAME:
				result = rif1.getAccessPoint().getObjectName().compareToIgnoreCase(rif2.getAccessPoint().getObjectName());
				break;
			case RadioInterfaces.COLUMN_AP_SERIAL:
				result = rif1.getAccessPoint().getSerialNumber().compareToIgnoreCase(rif2.getAccessPoint().getSerialNumber());
				break;
			case RadioInterfaces.COLUMN_AP_VENDOR:
				result = rif1.getAccessPoint().getVendor().compareToIgnoreCase(rif2.getAccessPoint().getVendor());
				break;
			case RadioInterfaces.COLUMN_CHANNEL:
				result = rif1.getChannel() - rif2.getChannel();
				break;
			case RadioInterfaces.COLUMN_INDEX:
				result = rif1.getIndex() - rif2.getIndex();
				break;
			case RadioInterfaces.COLUMN_MAC_ADDR:
				result = rif1.getMacAddress().compareTo(rif2.getMacAddress());
				break;
			case RadioInterfaces.COLUMN_NAME:
				result = rif1.getName().compareToIgnoreCase(rif2.getName());
				break;
			case RadioInterfaces.COLUMN_TX_POWER_DBM:
				result = rif1.getPowerDBm() - rif2.getPowerDBm();
				break;
			case RadioInterfaces.COLUMN_TX_POWER_MW:
				result = rif1.getPowerMW() - rif2.getPowerMW();
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.DOWN) ? result : -result;
	}
}
