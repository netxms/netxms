/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2025 RadenSolutions
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
package org.netxms.nxmc.modules.businessservice.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.businessservices.BusinessServiceCheck;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Filter for switch forwarding database  
 */
public class BusinessServiceCheckFilter extends ViewerFilter implements AbstractViewerFilter
{
   private String filterString = null;
   private BusinessServiceCheckLabelProvider labelProvider;
   
   public BusinessServiceCheckFilter(BusinessServiceCheckLabelProvider labelProvider)
   {
      this.labelProvider = labelProvider;
   }
   
   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if ((filterString == null) || (filterString.isEmpty()))
         return true;

      BusinessServiceCheck check = (BusinessServiceCheck)element;
      return check.getDescription().toLowerCase().contains(filterString) ||
             labelProvider.getTypeName(check).toLowerCase().contains(filterString) ||
             labelProvider.getCheckStateText(check).toLowerCase().contains(filterString) ||
             labelProvider.getObjectName(check).toLowerCase().contains(filterString) ||
             labelProvider.getDciName(check).toLowerCase().contains(filterString) ||
             check.getFailureReason().toLowerCase().contains(filterString);
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
