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

import org.eclipse.jface.viewers.LabelProvider;
import org.netxms.client.InetAddressListElement;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for address lists
 */
public class AddressListLabelProvider extends LabelProvider
{
   private NXCSession session = ConsoleSharedData.getSession();
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
    */
   @Override
   public String getText(Object element)
   {
      InetAddressListElement e = (InetAddressListElement)element;
      String base = (e.getType() == InetAddressListElement.SUBNET) ? e.getBaseAddress().getHostAddress() + "/" + e.getMaskBits() : e.getBaseAddress().getHostAddress() + " - " + e.getEndAddress().getHostAddress();
      if (e.getProxyId() != 0)
         return base + " via " + session.getObjectName(e.getProxyId());
      if (e.getZoneUIN() != 0)
         return base + " in zone " + session.getZoneName(e.getZoneUIN());
      return base;
   }
}
