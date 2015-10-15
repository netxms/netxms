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
package org.netxms.ui.eclipse.perfview.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.datacollection.GraphSettings;

/**
 * Filter for graph tree
 */
public class GraphTreeFilter extends ViewerFilter
{
   private String filterString = null;
   private GraphSettings lastMatch = null;
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if ((filterString == null) || filterString.isEmpty())
         return true;
      
      if (element instanceof GraphSettings)
      {
         if (((GraphSettings)element).getName().toLowerCase().contains(filterString))
         {
            lastMatch = (GraphSettings)element;
            return true;
         }
         return false;
      }
      
      if (element instanceof GraphFolder)
      {
         for(Object o : ((GraphFolder)element).getChildObjects())
         {
            if (select(viewer, element, o))
               return true;
         }
         return false;
      }
      return true;
   }

   /**
    * Set filter string
    * 
    * @param filterString new filter string
    */
   public void setFilterString(final String filterString)
   {
      this.filterString = (filterString != null) ? filterString.toLowerCase() : null;
   }

   /**
    * @return the lastMatch
    */
   public GraphSettings getLastMatch()
   {
      return lastMatch;
   }
}
