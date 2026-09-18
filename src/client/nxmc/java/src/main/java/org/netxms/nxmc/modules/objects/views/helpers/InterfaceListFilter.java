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
      
      if (filter.isEmpty())
         return true;

      return filter.matchesEachToken(token -> matchOId(iface, token) ||
            matchName(iface, token) ||
            matchAlias(iface, token) ||
            matchIfType(iface, token) ||
            matchIfIndex(iface, token) ||
            matchIfTypeName(iface, token) ||
            matchSlot(iface, token) ||
            matchPort(iface, token) ||
            matchMtu(iface, token) ||
            matchSpeed(iface, token) ||
            matchDescription(iface, token) ||
            matchMac(iface, token) ||
            matchIp(iface, token) ||
            matchPeerInterface(iface, token) ||
            matchPeerNode(iface, token) ||
            matchPeerMac(iface, token) ||
            matchPeerIp(iface, token) ||
            matchPeerDiscoveryProtocol(iface, token) ||
            matchAdminState(iface, token) ||
            matchOperState(iface, token) ||
            matchStpState(iface, token) ||
            matchStatus(iface, token) ||
            matchDot1xPaeState(iface, token) ||
            matchDot1xBackendState(iface, token) ||
            matchVendor(iface, token) ||
            matchPeerVendor(iface, token));
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchOId(Interface iface, String token)
   {
      return Long.toString(iface.getObjectId()).contains(token);
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchName(Interface iface, String token)
   {
      return iface.getObjectName().toLowerCase().contains(token);
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchAlias(Interface interf, String token)
   {
      return interf.getAlias().toLowerCase().contains(token);
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchIfType(Interface interf, String token)
   {
      return Integer.toString(interf.getIfType()).toLowerCase().contains(token);
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchIfTypeName(Interface iface, String token)
   {
      return iface.getIfTypeName().toLowerCase().contains(token);
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchIfIndex(Interface interf, String token)
   {
      return Integer.toString(interf.getIfIndex()).toLowerCase().contains(token);
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchSlot(Interface interf, String token)
   {
      return Integer.toString(interf.getModule()).toLowerCase().contains(token);
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchPort(Interface interf, String token)
   {
      return Integer.toString(interf.getPort()).toLowerCase().contains(token);
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchMtu(Interface interf, String token)
   {
      return Integer.toString(interf.getMtu()).toLowerCase().contains(token);
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchSpeed(Interface interf, String token)
   {
      return Long.toString(interf.getSpeed()).toLowerCase().contains(token);
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchDescription(Interface interf, String token)
   {
      return interf.getDescription().toLowerCase().contains(token);
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchMac(Interface interf, String token)
   {
      return interf.getMacAddress().toString().toLowerCase().contains(token);
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchIp(Interface interf, String token)
   {
      return interf.getIpAddressListAsString().toLowerCase().contains(token);
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchPeerNode(Interface iface, String token)
   {
      return Long.toString(iface.getPeerNodeId()).toLowerCase().contains(token);
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchPeerInterface(Interface iface, String token)
   {
      return Long.toString(iface.getPeerInterfaceId()).toLowerCase().contains(token);
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchPeerMac(Interface iface, String token)
   {
      Interface peer = (Interface)session.findObjectById(iface.getPeerInterfaceId(), Interface.class);
      if (peer == null)
         return false;
      return peer.getMacAddress().toString().toLowerCase().contains(token);
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchPeerIp(Interface iface, String token)
   {
      Interface peer = (Interface)session.findObjectById(iface.getPeerInterfaceId(), Interface.class);
      if (peer == null)
         return false;
      return peer.getIpAddressListAsString().toLowerCase().contains(token);
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchPeerDiscoveryProtocol(Interface iface, String token)
   {
      return iface.getPeerDiscoveryProtocol().toString().toLowerCase().contains(token);
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchAdminState(Interface iface, String token)
   {
      return iface.getAdminStateAsText().toLowerCase().contains(token);
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchOperState(Interface iface, String token)
   {
      return iface.getOperStateAsText().toLowerCase().contains(token);
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchStpState(Interface iface, String token)
   {
      return iface.getStpPortState().getText().toLowerCase().contains(token);
   }

   /**
    * @param iface
    * @return
    */
   private boolean matchStatus(Interface iface, String token)
   {
      return iface.getStatus().toString().toLowerCase().contains(token);
   }

   /**
    * @param interf
    * @return
    */
   private boolean matchDot1xPaeState(Interface interf, String token)
   {
      return interf.getDot1xPaeStateAsText().toLowerCase().contains(token);
   }

   /**
    * @param interf
    * @return true if matched
    */
   private boolean matchDot1xBackendState(Interface interf, String token)
   {
      return interf.getDot1xBackendStateAsText().toLowerCase().contains(token);
   }

   /**
    * @param iface
    * @return true if matched
    */
   private boolean matchVendor(Interface iface, String token)
   {
      String vendor = session.getVendorByMac(iface.getMacAddress(), null);
      return (vendor != null) && vendor.toLowerCase().contains(token);
   }

   /**
    * Match peer interface's vendor
    *
    * @param iface interface
    * @return true if matched
    */
   private boolean matchPeerVendor(Interface iface, String token)
   {
      Interface peer = (Interface)session.findObjectById(iface.getPeerInterfaceId(), Interface.class);
      if (peer == null)
         return false;
      String vendor = session.getVendorByMac(peer.getMacAddress(), null);
      return (vendor != null) && vendor.toLowerCase().contains(token);
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
