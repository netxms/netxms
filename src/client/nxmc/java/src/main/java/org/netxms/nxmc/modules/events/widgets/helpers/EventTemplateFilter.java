/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.events.widgets.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.events.EventTemplate;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Event template filter
 */
public class EventTemplateFilter extends ViewerFilter implements AbstractViewerFilter
{
   private String filterText = null;

   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if (filterText == null)
         return true;
      else if (((EventTemplate)element).getName().toLowerCase().contains(filterText.toLowerCase()))
         return true;
      else if (((EventTemplate)element).getMessage().toLowerCase().contains(filterText.toLowerCase()))
         return true;
      else if (((EventTemplate)element).getTagList().toLowerCase().contains(filterText.toLowerCase()))
         return true;
      return false;
   }

   /**
    * Set filter text
    * 
    * @param filterText string to filter
    */
   public void setFilterString(String filterText)
   {
      if ((filterText == null) || filterText.trim().isEmpty())
         this.filterText = null;
      else
         this.filterText = filterText.trim().toLowerCase();
   }
}
