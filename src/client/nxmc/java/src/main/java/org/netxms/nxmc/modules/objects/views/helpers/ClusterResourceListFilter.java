/**
 * NetXMS - open source network management system
 * Copyright (C) 2019 RadenSolutions
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
import org.netxms.client.objects.ClusterResource;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Abstract filter class for node sub-object views
 */
public class ClusterResourceListFilter extends ViewerFilter implements AbstractViewerFilter
{
   private I18n i18n = LocalizationHelper.getI18n(ClusterResourceListFilter.class);
   
   private String filterString = null;
   private NXCSession session;
   
   /**
    * Default constructor
    */
   public ClusterResourceListFilter()
   {
      session = (NXCSession)Registry.getSession();      
   }

   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if ((filterString == null) || (filterString.isEmpty()))
         return true;

      ClusterResource resource = (ClusterResource)element;
      if (resource.getVirtualAddress().getHostAddress().contains(filterString))
         return true;
      if (resource.getName().toLowerCase().contains(filterString))
         return true;
      
      String ownerName;
      long ownerId = ((ClusterResource)element).getCurrentOwner();
      if (ownerId > 0)
      {
         Node owner = (Node)session.findObjectById(ownerId, Node.class);
         ownerName = (owner != null) ? owner.getObjectName() : "<" + Long.toString(ownerId) + ">"; //$NON-NLS-1$ //$NON-NLS-2$
      }
      else
      {
         ownerName = i18n.tr("NONE");
      }
      
      if (ownerName.toLowerCase().contains(filterString))
         return true;
      
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.AbstractViewerFilter#setFilterString(java.lang.String)
    */
   @Override
   public void setFilterString(String filterString)
   {
      this.filterString = (filterString != null) ? filterString.toLowerCase() : null;
   }
}
