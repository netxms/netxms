/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.ai.views.helpers;

import java.util.Date;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.ai.AiMemoryEntry;
import org.netxms.nxmc.modules.ai.views.AiMemoryManager;

/**
 * Comparator for AI memory entries
 */
public class AiMemoryComparator extends ViewerComparator
{
   private AiMemoryLabelProvider labelProvider;

   /**
    * Create comparator.
    *
    * @param labelProvider label provider used to resolve display texts
    */
   public AiMemoryComparator(AiMemoryLabelProvider labelProvider)
   {
      this.labelProvider = labelProvider;
   }

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      TableColumn sortColumn = ((TableViewer)viewer).getTable().getSortColumn();
      if (sortColumn == null)
         return 0;

      AiMemoryEntry m1 = (AiMemoryEntry)e1;
      AiMemoryEntry m2 = (AiMemoryEntry)e2;
      int rc;
      switch((Integer)sortColumn.getData("ID"))
      {
         case AiMemoryManager.COLUMN_ID:
            rc = Integer.compare(m1.getId(), m2.getId());
            break;
         case AiMemoryManager.COLUMN_SCOPE:
            rc = m1.getScope().getValue() - m2.getScope().getValue();
            if (rc == 0)
               rc = Integer.compare(m1.getScopeId(), m2.getScopeId());
            break;
         case AiMemoryManager.COLUMN_TARGET:
            rc = labelProvider.getTargetText(m1, e1).compareToIgnoreCase(labelProvider.getTargetText(m2, e2));
            break;
         case AiMemoryManager.COLUMN_TITLE:
            rc = m1.getTitle().compareToIgnoreCase(m2.getTitle());
            break;
         case AiMemoryManager.COLUMN_CONTENT:
            rc = m1.getContent().compareToIgnoreCase(m2.getContent());
            break;
         case AiMemoryManager.COLUMN_CREATED_BY:
            rc = Boolean.compare(m1.isCreatedByModel(), m2.isCreatedByModel());
            break;
         case AiMemoryManager.COLUMN_SOURCE:
            rc = labelProvider.getSourceText(m1, e1).compareToIgnoreCase(labelProvider.getSourceText(m2, e2));
            break;
         case AiMemoryManager.COLUMN_CREATED:
            rc = Long.compare(getTime(m1.getCreationTime()), getTime(m2.getCreationTime()));
            break;
         case AiMemoryManager.COLUMN_UPDATED:
            rc = Long.compare(getTime(m1.getModificationTime()), getTime(m2.getModificationTime()));
            break;
         case AiMemoryManager.COLUMN_LOCKED:
            rc = Boolean.compare(m1.isLocked(), m2.isLocked());
            break;
         default:
            rc = 0;
            break;
      }
      int dir = ((TableViewer)viewer).getTable().getSortDirection();
      return (dir == SWT.UP) ? rc : -rc;
   }

   /**
    * Get time in milliseconds from possibly null Date.
    *
    * @param date date object or null
    * @return time in milliseconds or 0
    */
   private static long getTime(Date date)
   {
      return (date != null) ? date.getTime() : 0;
   }
}
