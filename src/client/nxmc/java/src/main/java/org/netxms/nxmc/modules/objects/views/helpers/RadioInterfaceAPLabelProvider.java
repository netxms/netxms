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
import org.netxms.client.topology.RadioInterface;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.objects.views.RadioInterfacesAP;
import org.netxms.nxmc.tools.ViewerElementUpdater;

/**
 * Label provider for radio interface list (access point version)
 */
public class RadioInterfaceAPLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private TableViewer viewer;
   private NXCSession session = Registry.getSession();

   public RadioInterfaceAPLabelProvider(TableViewer viewer)
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
         case RadioInterfacesAP.COLUMN_BAND:
            return rif.getBand().toString();
         case RadioInterfacesAP.COLUMN_BSSID:
            return rif.getBSSID().toString();
         case RadioInterfacesAP.COLUMN_CHANNEL:
            return (rif.getChannel() > 0) ? Integer.toString(rif.getChannel()) : "";
         case RadioInterfacesAP.COLUMN_FREQUENCY:
            return (rif.getFrequency() > 0) ? (Integer.toString(rif.getFrequency()) + " MHz") : "";
         case RadioInterfacesAP.COLUMN_INDEX:
				return Integer.toString(rif.getIndex());
         case RadioInterfacesAP.COLUMN_NIC_VENDOR:
            return session.getVendorByMac(rif.getBSSID(), new ViewerElementUpdater(viewer, element));
         case RadioInterfacesAP.COLUMN_NAME:
				return rif.getName();
         case RadioInterfacesAP.COLUMN_SSID:
            return rif.getSSID();
         case RadioInterfacesAP.COLUMN_TX_POWER_DBM:
				return Integer.toString(rif.getPowerDBm());
         case RadioInterfacesAP.COLUMN_TX_POWER_MW:
				return Integer.toString(rif.getPowerMW());
		}
		return null;
	}
}
