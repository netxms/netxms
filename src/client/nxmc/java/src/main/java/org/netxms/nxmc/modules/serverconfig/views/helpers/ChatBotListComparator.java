/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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
package org.netxms.nxmc.modules.serverconfig.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.ChatBot;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.serverconfig.views.ChatBots;

/**
 * Comparator for chat bot list
 */
public class ChatBotListComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      int result;

      ChatBot b1 = (ChatBot)e1;
      ChatBot b2 = (ChatBot)e2;
      switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"))
      {
         case ChatBots.COLUMN_NAME:
            result = b1.getName().compareToIgnoreCase(b2.getName());
            break;
         case ChatBots.COLUMN_DESCRIPTION:
            result = b1.getDescription().compareToIgnoreCase(b2.getDescription());
            break;
         case ChatBots.COLUMN_DRIVER:
            result = b1.getDriverName().compareToIgnoreCase(b2.getDriverName());
            break;
         case ChatBots.COLUMN_HEALTH:
            result = Boolean.compare(b1.isHealthy(), b2.isHealthy());
            break;
         case ChatBots.COLUMN_SESSIONS:
            result = Integer.compare(b1.getActiveSessionCount(), b2.getActiveSessionCount());
            break;
         case ChatBots.COLUMN_LAST_MESSAGE:
            long t1 = (b1.getLastInboundMessageTime() != null) ? b1.getLastInboundMessageTime().getTime() : 0;
            long t2 = (b2.getLastInboundMessageTime() != null) ? b2.getLastInboundMessageTime().getTime() : 0;
            result = Long.compare(t1, t2);
            break;
         case ChatBots.COLUMN_ERROR_MESSAGE:
            result = b1.getErrorMessage().compareToIgnoreCase(b2.getErrorMessage());
            break;
         default:
            result = 0;
            break;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
