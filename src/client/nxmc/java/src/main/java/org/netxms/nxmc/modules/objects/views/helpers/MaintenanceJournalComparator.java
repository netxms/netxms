/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.MaintenanceJournalEntry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.MaintenanceJournalView;

/**
 * Physical link comparator
 */
public class MaintenanceJournalComparator extends ViewerComparator
{
   private MaintenanceJournalLabelProvider labelProvider;

   /**
    * Create physical link comparator.
    *
    * @param labelProvider label provider for physical links
    */
   public MaintenanceJournalComparator(MaintenanceJournalLabelProvider labelProvider)
   {
      this.labelProvider = labelProvider;
   }

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      MaintenanceJournalEntry entry1 = (MaintenanceJournalEntry)e1;
      MaintenanceJournalEntry entry2 = (MaintenanceJournalEntry)e2;
      final int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID");

      int result;
      switch(column)
      {
         case MaintenanceJournalView.COL_ID:
            result = Long.compare(entry1.getId(), entry2.getId());
            break;
         case MaintenanceJournalView.COL_OBJECT:
         case MaintenanceJournalView.COL_AUHTHOR:
         case MaintenanceJournalView.COL_EDITOR:
         case MaintenanceJournalView.COL_DESCRIPTION:
            result = labelProvider.getColumnText(entry1, column).compareToIgnoreCase(labelProvider.getColumnText(entry2, column));
            break;
         case MaintenanceJournalView.COL_CREATE_TIME:
            result = entry1.getCreationTime().compareTo(entry2.getCreationTime());
            break;
         case MaintenanceJournalView.COL_MODIFY_TIME:
            result = entry1.getModificationTime().compareTo(entry2.getModificationTime());
            break;
         default:
            result = 0;
            break;
      }

      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
