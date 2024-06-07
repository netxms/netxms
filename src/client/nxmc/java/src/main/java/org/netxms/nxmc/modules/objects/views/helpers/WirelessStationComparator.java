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
import org.netxms.client.topology.WirelessStation;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.WirelessStations;
import org.netxms.nxmc.tools.ComparatorHelper;

/**
 * Comparator for wireless station list
 */
public class WirelessStationComparator extends ViewerComparator
{
   private NXCSession session = Registry.getSession();

   /**
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
         case WirelessStations.COLUMN_VENDOR:
            result = getVendor(ws1).compareToIgnoreCase(getVendor(ws2));
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
			case WirelessStations.COLUMN_RADIO:
				result = ws1.getRadioInterface().compareToIgnoreCase(ws2.getRadioInterface());
				break;
         case WirelessStations.COLUMN_RSSI:
            result = Integer.compare(ws1.getRSSI(), ws2.getRSSI());
            break;
         case WirelessStations.COLUMN_SSID:
            result = ws1.getSsid().compareToIgnoreCase(ws2.getSsid());
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
    * @param ws wireless station
    * @return NIC vendor
    */
   private String getVendor(WirelessStation ws)
   {
      String vendor = session.getVendorByMac(ws.getMacAddress(), null);
      return (vendor != null) ? vendor : "";
   }
}
