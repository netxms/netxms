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
package org.netxms.nxmc.modules.objects.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.MaintenanceJournalEntry;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.modules.objects.views.MaintenanceJournalView;

/**
 * Viewer filter for physical links
 */
public class MaintenanceJournalFilter extends ViewerFilter implements AbstractViewerFilter
{
   private String filterString = null;
   private MaintenanceJournalLabelProvider labelProvider;

   /**
    * Create physical link viewer filter.
    *
    * @param labelProvider physical link label provider
    */
   public MaintenanceJournalFilter(MaintenanceJournalLabelProvider labelProvider)
   {
      this.labelProvider = labelProvider;
   }

   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      MaintenanceJournalEntry me = (MaintenanceJournalEntry)element;

      if ((filterString == null) || (filterString.isEmpty()))
         return true;

      if (labelProvider.getColumnText(me, MaintenanceJournalView.COL_ID).contains(filterString))
         return true;

      if (labelProvider.getColumnText(me, MaintenanceJournalView.COL_OBJECT).toLowerCase().contains(filterString))
         return true;

      if (labelProvider.getColumnText(me, MaintenanceJournalView.COL_AUHTHOR).toLowerCase().contains(filterString))
         return true;

      if (labelProvider.getColumnText(me, MaintenanceJournalView.COL_EDITOR).toLowerCase().contains(filterString))
         return true;

      if (me.getDescription().toLowerCase().contains(filterString))
         return true;

      return false;
   }

   /**
    * Set filter string.
    *
    * @param filterString new filter string
    */
   public void setFilterString(String filterString)
   {
      this.filterString = (filterString != null) ? filterString.toLowerCase() : null;
   }
}
