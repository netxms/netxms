/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.topology.RadioInterface;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.objects.views.RadioInterfacesController;
import org.netxms.nxmc.tools.ViewerElementUpdater;

/**
 * Label provider for radio interface list (controller version)
 */
public class RadioInterfaceControllerLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private TableViewer viewer;
   private NXCSession session = Registry.getSession();

   public RadioInterfaceControllerLabelProvider(TableViewer viewer)
   {
      this.viewer = viewer;
   }

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
		RadioInterface rif = (RadioInterface)element;
		switch(columnIndex)
		{
			case RadioInterfacesController.COLUMN_AP_MAC_ADDR:
            return ((AccessPoint)rif.getOwner()).getMacAddress().toString();
			case RadioInterfacesController.COLUMN_AP_MODEL:
            return ((AccessPoint)rif.getOwner()).getModel();
			case RadioInterfacesController.COLUMN_AP_NAME:
            return ((AccessPoint)rif.getOwner()).getObjectName();
			case RadioInterfacesController.COLUMN_AP_SERIAL:
            return ((AccessPoint)rif.getOwner()).getSerialNumber();
			case RadioInterfacesController.COLUMN_AP_VENDOR:
            return ((AccessPoint)rif.getOwner()).getVendor();
         case RadioInterfacesController.COLUMN_BAND:
            return rif.getBand().toString();
         case RadioInterfacesController.COLUMN_BSSID:
            return rif.getBSSID().toString();
			case RadioInterfacesController.COLUMN_CHANNEL:
            return (rif.getChannel() > 0) ? Integer.toString(rif.getChannel()) : "";
         case RadioInterfacesController.COLUMN_FREQUENCY:
            return (rif.getFrequency() > 0) ? (Integer.toString(rif.getFrequency()) + " MHz") : "";
			case RadioInterfacesController.COLUMN_INDEX:
				return Integer.toString(rif.getIndex());
         case RadioInterfacesController.COLUMN_NIC_VENDOR:
            return session.getVendorByMac(rif.getBSSID(), new ViewerElementUpdater(viewer, element));
			case RadioInterfacesController.COLUMN_NAME:
				return rif.getName();
         case RadioInterfacesController.COLUMN_SSID:
            return rif.getSSID();
			case RadioInterfacesController.COLUMN_TX_POWER_DBM:
				return Integer.toString(rif.getPowerDBm());
			case RadioInterfacesController.COLUMN_TX_POWER_MW:
				return Integer.toString(rif.getPowerMW());
		}
		return null;
	}
}
