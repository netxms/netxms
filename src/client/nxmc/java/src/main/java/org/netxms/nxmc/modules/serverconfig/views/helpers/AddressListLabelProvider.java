/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.InetAddressListElement;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.views.NetworkDiscoveryConfigurator;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for address lists
 */
public class AddressListLabelProvider extends LabelProvider  implements ITableLabelProvider
{
   private I18n i18n = LocalizationHelper.getI18n(AddressListLabelProvider.class);
   
   private NXCSession session = Registry.getSession();
   private boolean isDiscoveryTarget;
   
   public AddressListLabelProvider(boolean isDiscoveryTarget)
   {
      this.isDiscoveryTarget = isDiscoveryTarget; 
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
      InetAddressListElement e = (InetAddressListElement)element;    
      switch(columnIndex)
      {
         case NetworkDiscoveryConfigurator.RANGE:
            return (e.getType() == InetAddressListElement.SUBNET) ? e.getBaseAddress().getHostAddress() + "/" + e.getMaskBits() : e.getBaseAddress().getHostAddress() + " - " + e.getEndAddress().getHostAddress();
         case NetworkDiscoveryConfigurator.ZONE:
            return isDiscoveryTarget ? session.getZoneName(e.getZoneUIN()) : e.getComment();
         case NetworkDiscoveryConfigurator.PROXY:
            return (e.getProxyId() != 0) ? session.getObjectName(e.getProxyId()) : i18n.tr("Zone proxy");
         case NetworkDiscoveryConfigurator.COMMENTS:
            return e.getComment();
      }
      return null;
   }
}
