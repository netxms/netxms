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
import org.netxms.client.NXCSession;
import org.netxms.client.topology.WirelessStation;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ComparatorHelper;
import org.netxms.ui.eclipse.topology.views.WirelessStations;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for wireless station list
 */
public class WirelessStationComparator extends ViewerComparator
{
	private static final long serialVersionUID = 1L;

	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		WirelessStation ws1 = (WirelessStation)e1;
		WirelessStation ws2 = (WirelessStation)e2;
		
		int result;
		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case WirelessStations.COLUMN_MAC_ADDRESS:
				result = ws1.getMacAddress().compareTo(ws2.getMacAddress());
				break;
			case WirelessStations.COLUMN_IP_ADDRESS:
				result = ComparatorHelper.compareInetAddresses(ws1.getIpAddress(), ws2.getIpAddress());
				break;
			case WirelessStations.COLUMN_NODE_NAME:
				result = session.getObjectName(ws1.getNodeObjectId()).compareToIgnoreCase(session.getObjectName(ws2.getNodeObjectId()));
				break;
			case WirelessStations.COLUMN_ACCESS_POINT:
				result = session.getObjectName(ws1.getAccessPointId()).compareToIgnoreCase(session.getObjectName(ws2.getAccessPointId()));
				break;
			case WirelessStations.COLUMN_SSID:
				result = ws1.getSsid().compareToIgnoreCase(ws2.getSsid());
				break;
			case WirelessStations.COLUMN_RADIO:
				result = ws1.getRadioInterface().compareToIgnoreCase(ws2.getRadioInterface());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.DOWN) ? result : -result;
	}
}
