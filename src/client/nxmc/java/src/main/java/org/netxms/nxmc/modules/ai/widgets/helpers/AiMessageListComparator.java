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
package org.netxms.nxmc.modules.ai.widgets.helpers;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.ai.AiMessage;
import org.netxms.nxmc.modules.ai.widgets.AiMessageList;

/**
 * Comparator for AI message list
 */
public class AiMessageListComparator extends ViewerComparator
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

      AiMessage m1 = (AiMessage)e1;
      AiMessage m2 = (AiMessage)e2;

      int rc;
      switch((Integer)sortColumn.getData("ID"))
      {
         case AiMessageList.COLUMN_ID:
            rc = Long.signum(m1.getId() - m2.getId());
            break;
         case AiMessageList.COLUMN_TYPE:
            rc = Integer.signum(m1.getMessageType().getValue() - m2.getMessageType().getValue());
            break;
         case AiMessageList.COLUMN_STATUS:
            rc = Integer.signum(m1.getStatus().getValue() - m2.getStatus().getValue());
            break;
         case AiMessageList.COLUMN_TITLE:
            rc = m1.getTitle().compareToIgnoreCase(m2.getTitle());
            break;
         case AiMessageList.COLUMN_RELATED_OBJECT:
            rc = Long.signum(m1.getRelatedObjectId() - m2.getRelatedObjectId());
            break;
         case AiMessageList.COLUMN_CREATED:
            rc = m1.getCreationTime().compareTo(m2.getCreationTime());
            break;
         case AiMessageList.COLUMN_EXPIRES:
            rc = m1.getExpirationTime().compareTo(m2.getExpirationTime());
            break;
         default:
            rc = 0;
            break;
      }
      int dir = ((TableViewer)viewer).getTable().getSortDirection();
      return (dir == SWT.UP) ? rc : -rc;
   }
}
