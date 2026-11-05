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

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.nxmc.modules.ai.views.AiSkillsManager;
import org.netxms.nxmc.modules.ai.views.AiSkillsManager.AiItemEntry;

/**
 * Comparator for AI skills and tools list
 */
public class AiItemComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      TableColumn sortColumn = ((TableViewer)viewer).getTable().getSortColumn();
      if (sortColumn == null)
         return 0;

      AiItemEntry item1 = (AiItemEntry)e1;
      AiItemEntry item2 = (AiItemEntry)e2;

      int rc;
      switch((Integer)sortColumn.getData("ID"))
      {
         case AiSkillsManager.COLUMN_TYPE:
            rc = item1.getTypeName().compareToIgnoreCase(item2.getTypeName());
            break;
         case AiSkillsManager.COLUMN_NAME:
            rc = item1.getName().compareToIgnoreCase(item2.getName());
            break;
         case AiSkillsManager.COLUMN_DESCRIPTION:
            rc = item1.getDescription().compareToIgnoreCase(item2.getDescription());
            break;
         case AiSkillsManager.COLUMN_STATUS:
            rc = Boolean.compare(item1.isDisabled(), item2.isDisabled());
            break;
         default:
            rc = 0;
            break;
      }

      int dir = ((TableViewer)viewer).getTable().getSortDirection();
      return (dir == SWT.UP) ? rc : -rc;
   }
}
