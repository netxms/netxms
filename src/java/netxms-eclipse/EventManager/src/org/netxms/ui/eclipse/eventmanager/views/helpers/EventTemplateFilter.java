/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.eventmanager.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.events.EventTemplate;

/**
 * Event template filter
 */
public class EventTemplateFilter extends ViewerFilter
{
   private String filterText = null;
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if (filterText == null)
         return true;
      
      return ((EventTemplate)element).getName().toLowerCase().contains(filterText) ||
             ((EventTemplate)element).getMessage().toLowerCase().contains(filterText) ||
             ((EventTemplate)element).getDescription().toLowerCase().contains(filterText);
   }

   /**
    * @param filterText
    */
   public void setFilterText(String filterText)
   {
      if ((filterText == null) || filterText.trim().isEmpty())
         this.filterText = null;
      else
         this.filterText = filterText.trim().toLowerCase();
   }
}
