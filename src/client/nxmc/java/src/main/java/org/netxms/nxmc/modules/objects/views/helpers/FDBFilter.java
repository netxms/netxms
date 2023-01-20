/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2021 RadenSolutions
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
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.NXCSession;
import org.netxms.client.topology.FdbEntry;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Filter for switch forwarding database  
 */
public class FDBFilter extends ViewerFilter implements AbstractViewerFilter
{
   private static final I18n i18n = LocalizationHelper.getI18n(FDBFilter.class);

   private static final String TYPE_MATCH_DYNAMIC = i18n.tr("Dynamic").toLowerCase();
   private static final String TYPE_MATCH_STATIC = i18n.tr("Static").toLowerCase();
   private static final String TYPE_MATCH_UNKNOWN = i18n.tr("Unknown").toLowerCase();

   private NXCSession session = Registry.getSession();
   private String filterString = null;
   
   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if ((filterString == null) || (filterString.isEmpty()))
         return true;
      
      final FdbEntry en = (FdbEntry)element;
      if (containsMac(en))
         return true;
      if (containsPort(en))
         return true;
      if (containsInterface(en))
         return true;
      if (containsVlan(en))
         return true;
      if (containsNode(en))
         return true;
      if (containsType(en))
         return true;
      
      String vendor = session.getVendorByMac(en.getAddress(), null);
      return (vendor != null) && vendor.toLowerCase().contains(filterString);
   }

   /**
    * Checks if filterString contains FDB type
    */
   private boolean containsType(FdbEntry en)
   {
      switch(en.getType())
      {
         case 3:
            return TYPE_MATCH_DYNAMIC.contains(filterString);
         case 5:
            return TYPE_MATCH_STATIC.contains(filterString);
         default:
            return TYPE_MATCH_UNKNOWN.contains(filterString);
      }
   }

   /**
    * Checks if filterString contains node name
    */
   private boolean containsNode(FdbEntry en)
   {
      if (session.getObjectName(en.getNodeId()).toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   /**
    * Checks if filterString contains vlan
    */
   private boolean containsVlan(FdbEntry en)
   {
      if (Integer.toString(en.getVlanId()).toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * Checks if filterString contains interface name
    */
   private boolean containsInterface(FdbEntry en)
   {
      if (en.getInterfaceName().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * Checks if filterString contains port
    */
   private boolean containsPort(FdbEntry en)
   {
      if (Integer.toString(en.getPort()).toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * Checks if filterString contains MAC
    */
   private boolean containsMac(FdbEntry en)
   {
      if (en.getAddress().toString().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * @return the filterString
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
}
