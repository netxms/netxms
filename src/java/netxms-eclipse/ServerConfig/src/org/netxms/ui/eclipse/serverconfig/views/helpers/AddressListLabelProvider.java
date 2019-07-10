/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.serverconfig.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.InetAddressListElement;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.serverconfig.views.NetworkDiscoveryConfigurator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for address lists
 */
public class AddressListLabelProvider extends LabelProvider  implements ITableLabelProvider
{
   private NXCSession session = ConsoleSharedData.getSession();
   private boolean isDiscoveryTarget;
   
   public AddressListLabelProvider(boolean isDiscoveryTarget)
   {
      this.isDiscoveryTarget = isDiscoveryTarget; 
   }

   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   @Override
   public String getColumnText(Object element, int columnIndex)
   {      
      InetAddressListElement e = (InetAddressListElement)element;    
      switch(columnIndex)
      {
         case NetworkDiscoveryConfigurator.RANGE:
            return (e.getType() == InetAddressListElement.SUBNET) ? e.getBaseAddress().getHostAddress() + "/" + e.getMaskBits() : e.getBaseAddress().getHostAddress() + " - " + e.getEndAddress().getHostAddress();
         case NetworkDiscoveryConfigurator.PROXY:
            if(isDiscoveryTarget)
               return (e.getProxyId() != 0) ? session.getObjectName(e.getProxyId()) : session.getZoneName(e.getZoneUIN());
            else
               return e.getComment();
         case NetworkDiscoveryConfigurator.COMMENT:
            return e.getComment();
      }
      return null;
   }
}
