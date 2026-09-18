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
import org.netxms.nxmc.base.helpers.TokenizedFilter;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Filter for switch forwarding database  
 */
public class BusinessServiceCheckFilter extends ViewerFilter implements AbstractViewerFilter
{
   private TokenizedFilter filter = new TokenizedFilter(null);
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
      if (filter.isEmpty())
         return true;

      BusinessServiceCheck check = (BusinessServiceCheck)element;
      return filter.matches(check.getDescription(), labelProvider.getTypeName(check), labelProvider.getCheckStateText(check),
            labelProvider.getObjectName(check), labelProvider.getDciName(check), check.getFailureReason());
   }

   /**
    * @return the filterString
    */
   public String getFilterString()
   {
      return filter.getFilterString();
   }

   /**
    * @param filterString the filterString to set
    */
   public void setFilterString(String filterString)
   {
      this.filter = new TokenizedFilter(filterString);
   }   
}
