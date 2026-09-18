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
import org.netxms.nxmc.base.helpers.TokenizedFilter;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Template targets filter
 */
public class TemplateTargetsFilter extends ViewerFilter  implements AbstractViewerFilter
{
   private TokenizedFilter filter = new TokenizedFilter(null);
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
      if (filter.isEmpty())
         return true;

      AbstractObject object = (AbstractObject)element;
      return filter.matches(Long.toString(object.getObjectId()), labelProvider.getName(element), TemplateTargetsLabelProvider.getZone(element),
            TemplateTargetsLabelProvider.getPrimaryHostName(element), TemplateTargetsLabelProvider.getDescription(element));
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
