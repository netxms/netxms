/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2023 RadenSolutions
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
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Interface;
import org.netxms.nxmc.Registry;

/**
 * Filter for Interfaces tab
 */
public class InterfaceListFilter extends NodeSubObjectFilter
{
   private NXCSession session = Registry.getSession();
   private boolean hideSubInterfaces = false;

   /**
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
            matchPeerInterface(iface) ||
            matchPeerNode(iface) ||
            matchPeerMac(iface) ||
            matchPeerIp(iface) ||
            matchPeerDiscoveryProtocol(iface) ||
            matchAdminState(iface) ||
            matchOperState(iface) ||
            matchStpState(iface) ||
            matchStatus(iface) ||
            matchDot1xPaeState(iface) ||
            matchDot1xBackendState(iface) ||
            matchVendor(iface) ||
            matchPeerVendor(iface);
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
    * @param iface
    * @return
    */
   private boolean matchIfTypeName(Interface iface)
   {
      if (iface.getIfTypeName().toLowerCase().contains(filterString))
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
      if (Integer.toString(interf.getModule()).toLowerCase().contains(filterString))
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
    * @param iface
    * @return
    */
   private boolean matchPeerNode(Interface iface)
   {
      if (Long.toString(iface.getPeerNodeId()).toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchPeerInterface(Interface iface)
   {
      if (Long.toString(iface.getPeerInterfaceId()).toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchPeerMac(Interface iface)
   {
      Interface peer = (Interface)session.findObjectById(iface.getPeerInterfaceId(), Interface.class);
      if (peer == null)
         return false;
      if (peer.getMacAddress().toString().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchPeerIp(Interface iface)
   {
      Interface peer = (Interface)session.findObjectById(iface.getPeerInterfaceId(), Interface.class);
      if (peer == null)
         return false;
      if (peer.getIpAddressListAsString().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchPeerDiscoveryProtocol(Interface iface)
   {
      return iface.getPeerDiscoveryProtocol().toString().toLowerCase().contains(filterString);
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchAdminState(Interface iface)
   {
      return iface.getAdminStateAsText().toLowerCase().contains(filterString);
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchOperState(Interface iface)
   {
      return iface.getOperStateAsText().toLowerCase().contains(filterString);
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchStpState(Interface iface)
   {
      return iface.getStpPortState().getText().toLowerCase().contains(filterString);
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchStatus(Interface iface)
   {
      return iface.getStatus().toString().toLowerCase().contains(filterString);
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
    * @return true if matched
    */
   private boolean matchDot1xBackendState(Interface interf)
   {
      if (interf.getDot1xBackendStateAsText().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @param iface
    * @return true if matched
    */
   private boolean matchVendor(Interface iface)
   {
      String vendor = session.getVendorByMac(iface.getMacAddress(), null);
      return (vendor != null) && vendor.toLowerCase().contains(filterString);
   }

   /**
    * Match peer interface's vendor
    *
    * @param iface interface
    * @return true if matched
    */
   private boolean matchPeerVendor(Interface iface)
   {
      Interface peer = (Interface)session.findObjectById(iface.getPeerInterfaceId(), Interface.class);
      if (peer == null)
         return false;
      String vendor = session.getVendorByMac(peer.getMacAddress(), null);
      return (vendor != null) && vendor.toLowerCase().contains(filterString);
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
