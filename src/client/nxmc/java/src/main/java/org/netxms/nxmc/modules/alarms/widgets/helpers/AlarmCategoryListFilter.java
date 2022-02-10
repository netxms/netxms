/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2022 RadenSolutions
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
package org.netxms.nxmc.modules.alarms.widgets.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.events.AlarmCategory;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Filter for Alarm Category List
 */
public class AlarmCategoryListFilter extends ViewerFilter implements AbstractViewerFilter
{
   private String filterString = null;

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if ((filterString == null) || (filterString.isEmpty()))
         return true;
      else if (checkId(element))
         return true;
      else if (checkName(element))
         return true;
      else if (checkDescr(element))
         return true;
      return false;
   }

   /**
    * Check if contains id
    */
   public boolean checkId(Object element)
   {
      if (Long.toString(((AlarmCategory)element).getId()).toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * Check if contains name
    */
   public boolean checkName(Object element)
   {
      if (((AlarmCategory)element).getName().toLowerCase().contains(filterString))
         return true;
      return false;
   }

   /**
    * Check if contains description
    */
   public boolean checkDescr(Object element)
   {
      if (((AlarmCategory)element).getDescription().toLowerCase().contains(filterString))
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
