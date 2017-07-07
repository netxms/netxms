/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2017 RadenSolutions
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
package org.netxms.ui.eclipse.objectview.objecttabs.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Interface;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Filter for Interfaces tab
 */
public class InterfacesTabFilter extends ViewerFilter
{
   private NXCSession session = ConsoleSharedData.getSession();
   private String filterString = null;
   private boolean hideSubInterfaces = false;

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      final Interface iface = (Interface)element;
      if (hideSubInterfaces && (iface.getParentInterfaceId() != 0))
         return false;
      
      if ((filterString == null) || (filterString.isEmpty()))
         return true;

      return matchOId(iface) ||
            matchName(iface) ||
            matchAlias(iface) ||
            matchIfType(iface) ||
            matchIfIndex(iface) ||
            matchIfTypeName(iface) ||
            matchSlot(iface) ||
            matchPort(iface) ||
            matchMtu(iface) ||
            matchSpeed(iface) ||
            matchDescription(iface) ||
            matchMac(iface) ||
            matchIp(iface) ||
            matchPeerNode(iface) ||
            matchPeerMac(iface) ||
            matchPeerIp(iface) ||
            matchPeerDiscoveryProtocol(iface) ||
            matchAdminState(iface) ||
            matchOperState(iface) ||
            matchExpState(iface) ||
            matchStatus(iface) ||
            matchDot1xPaeState(iface) ||
            matchDot1xBackendState(iface);
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchOId(Interface iface)
   {
      if (Long.toString(iface.getObjectId()).contains(filterString))
         return true;
      return false;
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchName(Interface iface)
   {
      if (iface.getObjectName().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchAlias(Interface interf)
   {
      if (interf.getAlias().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchIfType(Interface interf)
   {
      if (Integer.toString(interf.getIfType()).toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchIfTypeName(Interface interf)
   {
      if (interf.getIfTypeName().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchIfIndex(Interface interf)
   {
      if (Integer.toString(interf.getIfIndex()).toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchSlot(Interface interf)
   {
      if (Integer.toString(interf.getSlot()).toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchPort(Interface interf)
   {
      if (Integer.toString(interf.getPort()).toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchMtu(Interface interf)
   {
      if (Integer.toString(interf.getMtu()).toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchSpeed(Interface interf)
   {
      if (Long.toString(interf.getSpeed()).toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchDescription(Interface interf)
   {
      if (interf.getDescription().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchMac(Interface interf)
   {
      if (interf.getMacAddress().toString().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchIp(Interface interf)
   {
      if (interf.getIpAddressListAsString().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchPeerNode(Interface interf)
   {
      if (Long.toString(interf.getPeerNodeId()).toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchPeerMac(Interface interf)
   {
      Interface peer = (Interface)session.findObjectById(interf.getPeerInterfaceId(), Interface.class);
      if (peer == null)
         return false;
      if (peer.getMacAddress().toString().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchPeerIp(Interface interf)
   {
      Interface peer = (Interface)session.findObjectById(interf.getPeerInterfaceId(), Interface.class);
      if (peer == null)
         return false;
      if (peer.getIpAddressListAsString().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchPeerDiscoveryProtocol(Interface interf)
   {
      if (interf.getPeerDiscoveryProtocol().toString().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchAdminState(Interface interf)
   {
      if (interf.getAdminStateAsText().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchOperState(Interface interf)
   {
      if (interf.getOperStateAsText().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchExpState(Interface interf)
   {
      if (Integer.toString(interf.getExpectedState()).toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchStatus(Interface interf)
   {
      if (interf.getStatus().toString().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchDot1xPaeState(Interface interf)
   {
      if (interf.getDot1xPaeStateAsText().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchDot1xBackendState(Interface interf)
   {
      if (interf.getDot1xBackendStateAsText().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * Get filter string
    * 
    * @return
    */
   public String getFilterString()
   {
      return filterString;
   }

   /**
    * @param filterString the filterString to set
    */
   public void setFilterString(String filterString)
   {
      this.filterString = filterString.toLowerCase();
   }

   /**
    * @return the hideSubInterfaces
    */
   public boolean isHideSubInterfaces()
   {
      return hideSubInterfaces;
   }

   /**
    * @param hideSubInterfaces the hideSubInterfaces to set
    */
   public void setHideSubInterfaces(boolean hideSubInterfaces)
   {
      this.hideSubInterfaces = hideSubInterfaces;
   }
}
