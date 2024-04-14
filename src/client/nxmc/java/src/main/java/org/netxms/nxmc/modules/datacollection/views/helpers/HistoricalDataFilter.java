/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.localization.DateFormatFactory;

/**
 * Filter for graph tree
 */
public class HistoricalDataFilter extends ViewerFilter implements AbstractViewerFilter
{
   private String filterString = null;

   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if ((filterString == null) || filterString.isEmpty())
         return true;

      return DateFormatFactory.getDateTimeFormat().format(((DciDataRow)element).getTimestamp()).contains(filterString) || ((DciDataRow)element).getValueAsString().contains(filterString) ||
            ((DciDataRow)element).getRawValue().contains(filterString);
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
}
