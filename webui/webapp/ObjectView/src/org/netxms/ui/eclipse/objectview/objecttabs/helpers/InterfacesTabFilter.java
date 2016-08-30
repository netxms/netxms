/**
 * NetXMS - open source network management system
 * Copyright (C) 2016 RadenSolutions
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

   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if ((filterString == null) || (filterString.isEmpty()))
         return true;

      final Interface interf = (Interface)element;
      if (containsOId(interf))
      {
         return true;
      }
      else if (containsName(interf))
      {
         return true;
      }
      else if (containsAlias(interf))
      {
         return true;
      }
      else if (containsIfType(interf))
      {
         return true;
      }
      else if (containsIfIndex(interf))
      {
         return true;
      }
      else if (containsIfTypeName(interf))
      {
         return true;
      }      
      else if (containsSlot(interf))
      {
         return true;
      }
      else if (containsPort(interf))
      {
         return true;
      }
      else if (containsMtu(interf))
      {
         return true;
      }
      else if (containsSpeed(interf))
      {
         return true;
      }
      else if (containsDescription(interf))
      {
         return true;
      }
      else if (containsMac(interf))
      {
         return true;
      }
      else if (containsIp(interf))
      {
         return true;
      }
      else if (containsPeerNode(interf))
      {
         return true;
      }
      /*else if (containsPeerMac(interf))
      {
         return true;
      }
      else if (containsPeerIp(interf))
      {
         return true;
      }*/
      else if (containsPeerDiscoveryProtocol(interf))
      {
         return true;
      }
      else if (containsAdminState(interf))
      {
         return true;
      }
      else if (containsOperState(interf))
      {
         return true;
      }
      else if (containsExpState(interf))
      {
         return true;
      }
      else if (containsStatus(interf))
      {
         return true;
      }
      else if (containsDot1xPaeState(interf))
      {
         return true;
      }
      else if (containsDot1xBackendState(interf))
      {
         return true;
      }
      return false;
   }
   
   public boolean containsOId(Interface interf)
   {
      if (Long.toString(interf.getObjectId()).contains(filterString))
            return true;
      return false;
   }
   
   public boolean containsName(Interface interf)
   {
      if (interf.getObjectName().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsAlias(Interface interf)
   {
      if (interf.getAlias().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsIfType(Interface interf)
   {
      if (Integer.toString(interf.getIfType()).toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsIfTypeName(Interface interf)
   {
      if (interf.getIfTypeName().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsIfIndex(Interface interf)
   {
      if (Integer.toString(interf.getIfIndex()).toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsSlot(Interface interf)
   {
      if (Integer.toString(interf.getSlot()).toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsPort(Interface interf)
   {
      if (Integer.toString(interf.getPort()).toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsMtu(Interface interf)
   {
      if (Integer.toString(interf.getMtu()).toLowerCase().contains(filterString))
         return true;
      return false;      
   }
   
   public boolean containsSpeed(Interface interf)
   {
      if (Long.toString(interf.getSpeed()).toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsDescription(Interface interf)
   {
      if (interf.getDescription().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsMac(Interface interf)
   {
      if (interf.getMacAddress().toString().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsIp(Interface interf)
   {
      if (interf.getIpAddressListAsString().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsPeerNode(Interface interf)
   {
      if (Long.toString(interf.getPeerNodeId()).toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsPeerMac(Interface interf)
   {
      Interface peer = (Interface)session.findObjectById(interf.getPeerInterfaceId(), Interface.class);
      if (peer == null)
         return false;
      if (peer.getMacAddress().toString().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsPeerIp(Interface interf)
   {
      Interface peer = (Interface)session.findObjectById(interf.getPeerInterfaceId(), Interface.class);
      if (peer == null)
         return false;
      if (peer.getIpAddressListAsString().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   public boolean containsPeerDiscoveryProtocol(Interface interf)
   {
      if (interf.getPeerDiscoveryProtocol().toString().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsAdminState(Interface interf)
   {
      if (interf.getAdminStateAsText().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsOperState(Interface interf)
   {
      if (interf.getOperStateAsText().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsExpState(Interface interf)
   {
      if (Integer.toString(interf.getExpectedState()).toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsStatus(Interface interf)
   {
      if (interf.getStatus().toString().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsDot1xPaeState(Interface interf)
   {
      if (interf.getDot1xPaeStateAsText().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   public boolean containsDot1xBackendState(Interface interf)
   {
      if (interf.getDot1xBackendStateAsText().toLowerCase().contains(filterString))
         return true;
      return false;
   }
  
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
}
