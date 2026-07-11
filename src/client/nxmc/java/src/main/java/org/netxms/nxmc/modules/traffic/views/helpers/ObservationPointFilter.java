/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.traffic.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.objects.ObservationPoint;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Filter for observation point list
 */
public class ObservationPointFilter extends ViewerFilter implements AbstractViewerFilter
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

      ObservationPoint point = (ObservationPoint)element;
      return point.getObjectName().toLowerCase().contains(filterString) ||
            point.getExternalId().toLowerCase().contains(filterString) ||
            point.getPointType().toLowerCase().contains(filterString);
   }

   /**
    * @see org.netxms.nxmc.base.views.AbstractViewerFilter#setFilterString(java.lang.String)
    */
   @Override
   public void setFilterString(String string)
   {
      filterString = (string != null) ? string.toLowerCase() : null;
   }
}
