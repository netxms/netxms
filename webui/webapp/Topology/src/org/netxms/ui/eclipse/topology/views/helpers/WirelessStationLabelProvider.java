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

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.topology.WirelessStation;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.topology.views.WirelessStations;

/**
 * Label provider for wireless station list
 */
public class WirelessStationLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		WirelessStation ws = (WirelessStation)element;
		switch(columnIndex)
		{
			case WirelessStations.COLUMN_MAC_ADDRESS:
				return ws.getMacAddress().toString();
			case WirelessStations.COLUMN_IP_ADDRESS:
				if ((ws.getIpAddress() == null) || ws.getIpAddress().isAnyLocalAddress())
					return ""; //$NON-NLS-1$
				return ws.getIpAddress().getHostAddress();
			case WirelessStations.COLUMN_NODE_NAME:
				if (ws.getNodeObjectId() == 0)
					return ""; //$NON-NLS-1$
				return session.getObjectName(ws.getNodeObjectId());
			case WirelessStations.COLUMN_ACCESS_POINT:
				return session.getObjectName(ws.getAccessPointId());
			case WirelessStations.COLUMN_RADIO:
				return ws.getRadioInterface();
			case WirelessStations.COLUMN_SSID:
				return ws.getSsid();
		}
		return null;
	}
}
