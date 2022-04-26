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
package org.netxms.nxmc.modules.objects.views.helpers;

import org.eclipse.jface.viewers.Viewer;

/**
 * Filter for Interfaces tab
 */
public class Dot1xPortFilter extends NodeSubObjectFilter
{
   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      final Dot1xPortSummary p1 = (Dot1xPortSummary)element;
      
      if ((filterString == null) || (filterString.isEmpty()))
         return true;
      
      return p1.getNodeName().contains(filterString) || 
         Integer.toString(p1.getPort()).contains(filterString) ||
         p1.getInterfaceName().contains(filterString) ||
         p1.getPaeStateAsText().contains(filterString) ||
         p1.getBackendStateAsText().contains(filterString);
   }
}
