/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Template targets filter
 */
public class TemplateTargetsFilter extends ViewerFilter  implements AbstractViewerFilter
{
   private String filterString;
   private TemplateTargetsLabelProvider labelProvider;
   
   /**
    * Constructor
    * 
    * @param lablelProvider lable provider
    */
   public TemplateTargetsFilter(TemplateTargetsLabelProvider labelProvider)
   {
      this.labelProvider = labelProvider;
   }   

   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {      
      AbstractObject object = (AbstractObject)element;
      if ((filterString != null) && !filterString.isEmpty())
      {         
         return Long.toString(object.getObjectId()).contains(filterString) || 
               labelProvider.getName(element).toLowerCase().contains(filterString) ||
               TemplateTargetsLabelProvider.getZone(element).toLowerCase().contains(filterString) ||
               TemplateTargetsLabelProvider.getPrimaryHostName(element).toLowerCase().contains(filterString) ||
               TemplateTargetsLabelProvider.getDescription(element).toLowerCase().contains(filterString);
      }
      return true;
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
