/**
 * NetXMS - open source network management system
 * Copyright (C) 2019-2024 RadenSolutions
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
import org.netxms.client.objects.AccessPoint;
import org.netxms.nxmc.resources.StatusDisplayInfo;

/**
 * Filter for access points view
 */
public class AccessPointFilter extends NodeSubObjectFilter
{
   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if ((filterString == null) || (filterString.isEmpty()))
         return true;

      final AccessPoint ap = (AccessPoint)element;
      return 
         Long.toString(ap.getObjectId()).contains(filterString) ||
         ap.getObjectName().toLowerCase().contains(filterString) ||
         ap.getVendor().toLowerCase().contains(filterString) ||
         ap.getModel().toLowerCase().contains(filterString) ||
         ap.getSerialNumber().toLowerCase().contains(filterString) ||
         ap.getMacAddress().toString().toLowerCase().contains(filterString) ||
         StatusDisplayInfo.getStatusText(ap.getStatus()).contains(filterString);
   }
}
