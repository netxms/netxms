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

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.base.MacAddress;
import org.netxms.client.NXCSession;
import org.netxms.client.topology.RadioInterface;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.RadioInterfacesAP;

/**
 * Comparator for radio interface list (access point version)
 */
public class RadioInterfaceAPComparator extends ViewerComparator
{
   private NXCSession session = Registry.getSession();

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		RadioInterface rif1 = (RadioInterface)e1;
		RadioInterface rif2 = (RadioInterface)e2;

		int result;
      switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"))
		{
         case RadioInterfacesAP.COLUMN_BAND:
            result = rif1.getBand().compareTo(rif2.getBand());
            break;
         case RadioInterfacesAP.COLUMN_BSSID:
            result = rif1.getBSSID().compareTo(rif2.getBSSID());
            break;
         case RadioInterfacesAP.COLUMN_CHANNEL:
				result = rif1.getChannel() - rif2.getChannel();
				break;
         case RadioInterfacesAP.COLUMN_FREQUENCY:
            result = rif1.getFrequency() - rif2.getFrequency();
            break;
         case RadioInterfacesAP.COLUMN_INDEX:
				result = rif1.getIndex() - rif2.getIndex();
				break;
         case RadioInterfacesAP.COLUMN_NIC_VENDOR:
            result = getVendorByMAC(rif1.getBSSID()).compareToIgnoreCase(getVendorByMAC(rif2.getBSSID()));
            break;
         case RadioInterfacesAP.COLUMN_NAME:
				result = rif1.getName().compareToIgnoreCase(rif2.getName());
				break;
         case RadioInterfacesAP.COLUMN_SSID:
            result = rif1.getSSID().compareToIgnoreCase(rif2.getSSID());
            break;
         case RadioInterfacesAP.COLUMN_TX_POWER_DBM:
				result = rif1.getPowerDBm() - rif2.getPowerDBm();
				break;
         case RadioInterfacesAP.COLUMN_TX_POWER_MW:
				result = rif1.getPowerMW() - rif2.getPowerMW();
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}

   /**
    * Get vendor by MAC address
    *
    * @param macAddr MAC address
    * @return vendor name or empty string
    */
   private String getVendorByMAC(MacAddress macAddr)
   {
      String vendor = session.getVendorByMac(macAddr, null);
      return vendor != null ? vendor : "";
   }
}
